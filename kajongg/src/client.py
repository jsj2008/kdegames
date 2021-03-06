# -*- coding: utf-8 -*-

"""
Copyright (C) 2009-2012 Wolfgang Rohdewald <wolfgang@rohdewald.de>

kajongg is free software you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
"""

from PyQt4.QtCore import QTimer
from twisted.spread import pb
from twisted.internet.defer import Deferred, succeed, DeferredList
from twisted.python.failure import Failure
from util import logDebug, logException, Duration
from message import Message
from common import InternalParameters, Debug
from rule import Ruleset
from meld import meldsContent
from game import RemoteGame
from query import Transaction, Query
from move import Move
from animation import animate
from intelligence import AIDefault
from statesaver import StateSaver

class ClientTable(object):
    """the table as seen by the client"""
    # pylint: disable=R0902
    # pylint: disable=R0913
    # pylint says too many args, too many instance variables

    rulesets = {} # all rulesets for all tables, key is hash

    def __init__(self, client, tableid, gameid, status, ruleset, playOpen, autoPlay, wantedGame, playerNames,
                 playersOnline, endValues):
        self.client = client
        self.tableid = tableid
        self.gameid = gameid
        self.status = status
        self.running = status == 'Running'
        self.suspended = status.startswith('Suspended')
        self.ruleset = ruleset
        self.playOpen = playOpen
        self.autoPlay = autoPlay
        self.wantedGame = wantedGame
        self.playerNames = playerNames
        self.playersOnline = playersOnline
        self.endValues = endValues
        self.myRuleset = None # if set, points to an identical local ruleset
        allRulesets =  Ruleset.availableRulesets()
        for myRuleset in allRulesets:
            if myRuleset == self.ruleset:
                self.myRuleset = myRuleset
                break
        self.chatWindow = None

    def isOnline(self, player):
        """did he join the tabled?"""
        for idx, name in enumerate(self.playerNames):
            if player == name:
                return self.playersOnline[idx]
        return False

    def __str__(self):
        return 'Table %d %s gameid=%s rules %s players %s online %s' % (self.tableid or 0,
            self.status, self.gameid, self.ruleset.name,
            ', '.join(self.playerNames), ', '.join(str(x) for x in self.playersOnline))

    def gameExistsLocally(self):
        """does the game exist in the data base of the client?"""
        assert self.gameid
        return bool(Query('select 1 from game where id=?', list([self.gameid])).records)

    def humanPlayerNames(self):
        """returns a list excluding robot players"""
        return list(x for x in self.playerNames if not x.startswith('Robot '))

    @staticmethod
    def fromList(client, table):
        """convert the tuples delivered by twisted into a more useful class objects
        if tables share rulesets, the server sends them only once.
        The other tables only get the hash of the ruleset."""
        table = list(table) # we can replace items in lists but in not tuples
        if isinstance(table[3], basestring):
            # server only sent the hash
            table[3] = ClientTable.rulesets[table[3]]
        else:
            # server sent full ruleset definition
            table[3] = Ruleset.fromList(table[3])
            ClientTable.rulesets[table[3].hash] = table[3]
        return ClientTable(client, *table)  # pylint: disable=W0142

class Client(pb.Referenceable):
    """interface to the server. This class only implements the logic,
    so we can also use it on the server for robot clients. Compare
    with HumanClient(Client)"""

    clients = []

    def __init__(self, username=None, intelligence=AIDefault):
        """username is something like Robot 1 or None for the game server"""
        self.username = username
        self.game = None
        self.intelligence = intelligence(self)
        self.perspective = None # always None for a robot client
        self.tables = []
        self.table = None
        self.tableList = None
        self.sayable = {} # recompute for each move, use as cache
        self.clients.append(self)

    @staticmethod
    def shutdownClients(exception=None):
        """close connections to servers except maybe one"""
        clients = Client.clients
        def done():
            """return True if clients is cleaned"""
            return len(clients) == 0 or (exception and clients == [exception])
        def disconnectedClient(dummyResult, client):
            """now the client is really disconnected from the server"""
            if client in clients:
                # HumanClient.serverDisconnects also removes it!
                clients.remove(client)
        if isinstance(exception, Failure):
            logException(exception)
        for client in clients[:]:
            if client.tableList:
                client.tableList.hide()
        if done():
            return succeed(None)
        deferreds = []
        for client in clients[:]:
            if client != exception and client.perspective:
                deferreds.append(client.logout().addCallback(disconnectedClient, client))
        return DeferredList(deferreds)

    @staticmethod
    def quitProgram(result=None):
        """now all connections to servers are cleanly closed"""
        if isinstance(result, Failure):
            logException(result)
        InternalParameters.reactor.stop()
        StateSaver.saveAll()
        field = InternalParameters.field
        if field:
            # if we have the ruleset editor visible, we get:
            # File "/hdd/pub/src/gitgames/kajongg/src/rulesetselector.py", line 194, in headerData
            #  if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            #  AttributeError: 'NoneType' object has no attribute 'DisplayRole'
            # how can Qt get None? Same happens with QEvent, see statesaver.py
            if field.confDialog:
                field.confDialog.hide()
        # we may be in a Deferred callback which would
        # catch sys.exit as an exception
        # and the qt4reactor does not quit the app when being stopped
        # sometimes quitting hangs with singleShot(0. Why and which value
        # is the mininum needed?
        QTimer.singleShot(10, InternalParameters.app.quit)

    def logout(self, dummyResult=None): # pylint: disable=R0201
        """virtual"""
        return succeed(None)

    @apply
    def host():
        """the name of the host we are connected with"""
        def fget(dummySelf):
            return None # Client on the server
        return property(**locals())

    def isRobotClient(self):
        """avoid using isinstance because that imports too much for the server"""
        return bool(self.username)

    @staticmethod
    def isHumanClient():
        """avoid using isinstance because that imports too much for the server"""
        return False

    def isServerClient(self):
        """avoid using isinstance because that imports too much for the server"""
        return bool(not self.username)

    def remote_tablesChanged(self, tables):
        """update table list"""
        self.tables = []
        self.remote_newTables(tables)

    def remote_newTables(self, tables):
        """update table list"""
        self.tables.extend(list(ClientTable.fromList(self, x) for x in tables))

    def remote_replaceTable(self, table):
        """update table list"""
        newClientTable = ClientTable.fromList(self, table)
        for idx, table in enumerate(self.tables):
            if table.tableid == newClientTable.tableid:
                self.tables[idx] = newClientTable
                return
        for idx, table in enumerate(self.tables):
            if table.gameid == newClientTable.gameid:
                # joining a suspended game changes tableid
                self.tables[idx] = newClientTable
                return

    def remote_tableClosed(self, tableid, dummyMsg):
        """update table list"""
        for idx, table in enumerate(self.tables):
            if table.tableid == tableid:
                del self.tables[idx]
                break

    def reserveGameId(self, gameid):
        """the game server proposes a new game id. We check if it is available
        in our local data base - we want to use the same gameid everywhere"""
        with Transaction():
            query = Query('insert into game(id,seed) values(?,?)',
                      list([gameid, self.host]), mayFail=True)
            if query.rowcount() != 1:
                return Message.NO

    def readyForGameStart(self, tableid, gameid, wantedGame, playerNames, shouldSave=True):
        """the game server asks us if we are ready. A robot is always ready."""
        if self.isHumanClient():
            assert not self.table
            assert self.tables
            for tryTable in self.tables:
                if tryTable.tableid == tableid:
                    self.table = tryTable
            if not self.table:
                raise Exception('client.readyForGameStart: tableid %d unknown' % tableid)
        if self.table.suspended:
            self.game = RemoteGame.loadFromDB(gameid, client=self)
            self.game.assignPlayers(playerNames)
            if self.isHumanClient():
                if self.game.handctr != self.table.endValues[0]:
                    self.game.close()
                    raise Exception(
                        'The data bases for game %1 have different numbers for played hands: Server:%2, Client:%3', \
                        self.game.seed, self.table.endValues[0], self.game.handctr)
                for player in self.game.players:
                    if player.balance != self.table.endValues[1][player.wind]:
                        self.game.close()
                        raise Exception(
                            'The data bases for game %1 have different balances for wind %2: Server:%3, Client:%4', \
                            self.game.seed, player.wind, self.table.endValues[1][player.wind], player.balance)
        else:
            self.game = RemoteGame(playerNames, self.table.ruleset,
                shouldSave=shouldSave, gameid=gameid, wantedGame=wantedGame, client=self,
                playOpen=self.table.playOpen, autoPlay=self.table.autoPlay)
        self.game.prepareHand()
        return Message.OK

    def readyForHandStart(self, playerNames, rotateWinds):
        """the game server asks us if we are ready. A robot is always ready..."""
        self.game.assignPlayers(playerNames)
        if rotateWinds:
            self.game.rotateWinds()
        self.game.prepareHand()

    def ask(self, move, answers):
        """this is where the robot AI should go.
        sends answer and one parameter to server"""
        self.computeSayable(move, answers)
        return succeed(self.intelligence.selectAnswer(answers))

    def thatWasMe(self, player):
        """returns True if player == myself"""
        if not self.game:
            return False
        return player == self.game.myself

    def remote_move(self, playerName, command, *dummyArgs, **kwargs):
        """the server sends us info or a question and always wants us to answer"""
        player = None
        if self.game:
            if not self.game.client:
                # we aborted the game, ignore what the server tells us
                return
            player = self.game.playerByName(playerName)
        move = Move(player, command, kwargs)
        if Debug.traffic:
            if self.isHumanClient():
                if self.game:
                    self.game.debug('got Move: %s' % move)
                else:
                    logDebug('got Move: %s' % move)
        if self.game:
            self.game.checkTarget()
            if move.token:
                if move.token != self.game.handId(withAI=False):
                    logException( 'wrong token: %s, we have %s' % (move.token, self.game.handId()))
        with Duration('Move %s:' % move):
            return self.exec_move(move)

    @staticmethod
    def convertMessage(answer, answer2=None):
        """the client is done with executing the move. Animations have ended.
        Now we convert Message objects to their name for the write transfer.
        This callback may be called either on the Deferred representing
        the answer. In that case, parameter "answer" is used.
        Or it may be called as a callback on something else like animate().
        In that case, we use answer2."""
        if answer2 is not None:
            answer = answer2
        if not isinstance(answer, Deferred):
            if isinstance(answer, Message):
                answer = answer.name
            if isinstance(answer, tuple) and isinstance(answer[0], Message):
                answer = tuple(list([answer[0].name] + list(answer[1:])))
        return answer

    def exec_move(self, move):
        """mirror the move of a player as told by the the game server"""
        answer = move.message.clientAction(self, move)
        if not isinstance(answer, Deferred):
            answer = succeed(answer)
        answer.addCallback(self.convertMessage)
        if self.game:
            if move.player and not move.player.scoreMatchesServer(move.score):
                self.game.close()
            self.game.moves.append(move)
        if move.message == Message.Discard:
            # do not block here, we want to get the clientDialog
            # before the tile reaches its end position
            animate()
            return answer
        else:
            return animate().addCallback(self.convertMessage, answer)

    def claimed(self, move):
        """somebody claimed a discarded tile"""
        calledTile = self.game.lastDiscard
        self.game.lastDiscard = None
        calledTileName = calledTile.element
        self.game.discardedTiles[calledTileName.lower()] -= 1
        assert calledTileName in move.source, '%s %s'% (calledTileName, move.source)
        if InternalParameters.field:
            InternalParameters.field.discardBoard.lastDiscarded = None
        move.player.lastTile = calledTileName.lower()
        move.player.lastSource = 'd'
        hadTiles = move.source[:]
        hadTiles.remove(calledTileName)
        if not self.thatWasMe(move.player) and not self.game.playOpen:
            move.player.showConcealedTiles(hadTiles)
        move.exposedMeld = move.player.exposeMeld(hadTiles, calledTile=calledTile)
        if self.thatWasMe(move.player):
            if move.message != Message.Kong:
                # we will get a replacement tile first
                return self.myAction(move)
        elif self.game.prevActivePlayer == self.game.myself and self.perspective:
            # even here we ask: if our discard is claimed we need time
            # to notice - think 3 robots or network timing differences
            return self.ask(move, [Message.OK])

    def myAction(self, move):
        """ask myself what I want to do after picking or claiming a tile"""
        # only when all animations ended, our handboard gets focus. Otherwise
        # we would see a blue focusRect in the handboard even when a tile
        # ist still moving from the discardboard to the handboard.
        animate().addCallback(move.player.getsFocus)
        possibleAnswers = [Message.Discard, Message.Kong, Message.MahJongg]
        if not move.player.discarded:
            possibleAnswers.append(Message.OriginalCall)
        return self.ask(move, possibleAnswers)

    def declared(self, move):
        """somebody declared something.
        By declaring we mean exposing a meld, using only tiles from the hand.
        For now we only support Kong: in Classical Chinese it makes no sense
        to declare a Pung."""
        assert move.message == Message.Kong
        if not self.thatWasMe(move.player) and not self.game.playOpen:
            move.player.showConcealedTiles(move.source)
        move.exposedMeld = move.player.exposeMeld(move.source)
        if not self.thatWasMe(move.player):
            self.ask(move, [Message.OK])

    def __maySayChow(self):
        """returns answer arguments for the server if calling chow is possible.
        returns the meld to be completed"""
        if self.game.myself == self.game.nextPlayer():
            return self.game.myself.possibleChows()

    def __maySayPung(self):
        """returns answer arguments for the server if calling pung is possible.
        returns the meld to be completed"""
        if self.game.lastDiscard:
            element = self.game.lastDiscard.element
            assert element[0].isupper(), str(self.game.lastDiscard)
            if self.game.myself.concealedTileNames.count(element) >= 2:
                return [element] * 3

    def __maySayKong(self):
        """returns answer arguments for the server if calling or declaring kong is possible.
        returns the meld to be completed or to be declared"""
        return self.game.myself.possibleKongs()

    def __maySayMahjongg(self, move):
        """returns answer arguments for the server if calling or declaring Mah Jongg is possible"""
        game = self.game
        myself = game.myself
        robbableTile = withDiscard = None
        if move.message == Message.DeclaredKong:
            withDiscard = move.source[0].capitalize()
            if move.player != myself:
                robbableTile = move.exposedMeld.pairs[1] # we want it capitalized for a hidden Kong
        elif move.message == Message.AskForClaims:
            withDiscard = game.lastDiscard.element
        hand = myself.computeHand(withTile=withDiscard, robbedTile=robbableTile, asWinner=True)
        if hand.won:
            if Debug.robbingKong:
                if move.message == Message.DeclaredKong:
                    game.debug('%s may rob the kong from %s/%s' % \
                       (myself, move.player, move.exposedMeld.joined))
            if Debug.mahJongg:
                game.debug('%s may say MJ:%s, active=%s' % (
                    myself, list(x for x in game.players), game.activePlayer))
            return (meldsContent(hand.hiddenMelds), withDiscard, list(hand.lastMeld.pairs))

    def __maySayOriginalCall(self):
        """returns True if Original Call is possible"""
        myself = self.game.myself
        for tileName in set(myself.concealedTileNames):
            if (myself.hand - tileName).callingHands():
                if Debug.originalCall:
                    self.game.debug('%s may say Original Call' % myself)
                return True

    def computeSayable(self, move, answers):
        """find out what the player can legally say with this hand"""
        self.sayable = {}
        for message in Message.defined.values():
            self.sayable[message] = True
        if Message.Pung in answers:
            self.sayable[Message.Pung] = self.__maySayPung()
        if Message.Chow in answers:
            self.sayable[Message.Chow] = self.__maySayChow()
        if Message.Kong in answers:
            self.sayable[Message.Kong] = self.__maySayKong()
        if Message.MahJongg in answers:
            self.sayable[Message.MahJongg] = self.__maySayMahjongg(move)
        if Message.OriginalCall in answers:
            self.sayable[Message.OriginalCall] = self.__maySayOriginalCall()

    def maybeDangerous(self, msg):
        """could answering with msg lead to dangerous game?
        If so return a list of resulting melds
        where a meld is represented by a list of 2char strings"""
        result = []
        if msg in (Message.Chow, Message.Pung, Message.Kong):
            possibleMelds = self.sayable[msg]
            if isinstance(possibleMelds[0], basestring):
                possibleMelds = [possibleMelds]
            result = [x for x in possibleMelds if self.game.myself.mustPlayDangerous(x)]
        return result
