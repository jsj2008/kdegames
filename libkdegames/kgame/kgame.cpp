/*
    This file is part of the KDE games library
    Copyright (C) 2001 Martin Heni (kde at heni-online.de)
    Copyright (C) 2001 Andreas Beckermann (b_mann@gmx.de)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kgame.h"
#include "kgame.moc"
#include "kgamepropertyhandler.h"
#include "kgameproperty.h"
#include "kplayer.h"
#include "kgameio.h"
#include "kgameerror.h"
#include "kgamesequence.h"

#include "kgamemessage.h"

#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include <QBuffer>
#include <QTimer>
#include <QFile>
#include <QList>
#include <QQueue>

#include <klocale.h>
#include <krandomsequence.h>
#include <kdebug.h>

#define KGAME_LOAD_COOKIE 4210

// try to place as much as possible here
// many things are *not* possible here as KGame has to use some inline function
class KGamePrivate
{
public:
    KGamePrivate()
    {
        mUniquePlayerNumber = 0;
        mPolicy=KGame::PolicyLocal;
        mGameSequence = 0;
    }

    int mUniquePlayerNumber;
    QQueue<KPlayer*> mAddPlayerList;// this is a list of to-be-added players. See addPlayer() docu
    KRandomSequence* mRandom;
    KGame::GamePolicy mPolicy;
    KGameSequence* mGameSequence;


    KGamePropertyHandler* mProperties;

    // player lists
    KGame::KGamePlayerList mPlayerList;
    KGame::KGamePlayerList mInactivePlayerList;

    //KGamePropertys
    KGamePropertyInt mMaxPlayer;
    KGamePropertyUInt mMinPlayer;
    KGamePropertyInt mGameStatus; // Game running?
    QList<int> mInactiveIdList;

};

// ------------------- GAME CLASS --------------------------
KGame::KGame(int cookie,QObject* parent) 
    : KGameNetwork(cookie,parent),
      d( new KGamePrivate )
{
 kDebug(11001) << " - " << this << ", sizeof(KGame)=" << sizeof(KGame);

 d->mProperties = new KGamePropertyHandler(this);

 d->mProperties->registerHandler(KGameMessage::IdGameProperty,
                                this,SLOT(sendProperty(int, QDataStream&, bool* )),
                                     SLOT(emitSignal(KGamePropertyBase *)));
 d->mMaxPlayer.registerData(KGamePropertyBase::IdMaxPlayer, this, i18n("MaxPlayers"));
 d->mMaxPlayer.setLocal(-1);  // Infinite
 d->mMinPlayer.registerData(KGamePropertyBase::IdMinPlayer, this, i18n("MinPlayers"));
 d->mMinPlayer.setLocal(0);   // Always ok
 d->mGameStatus.registerData(KGamePropertyBase::IdGameStatus, this, i18n("GameStatus"));
 d->mGameStatus.setLocal(Init);
 // d->mUniquePlayerNumber = 0;
 d->mRandom = new KRandomSequence;
 d->mRandom->setSeed(0);

 connect(this, SIGNAL(signalClientConnected(quint32)),
                this, SLOT(slotClientConnected(quint32)));
 connect(this, SIGNAL(signalClientDisconnected(quint32,bool)),
                this, SLOT(slotClientDisconnected(quint32,bool)));
 connect(this, SIGNAL(signalConnectionBroken()),
                this, SLOT(slotServerDisconnected()));

 setGameSequence(new KGameSequence());

 // BL: FIXME This signal does no longer exist. When we are merging
 // MH: super....and how do I find out about the lost conenction now?
 // KGame and KGameNetwork, this could be improved!
//  connect(this,SIGNAL(signalConnectionLost(KGameClient *)),
//          this,SLOT(slotConnectionLost(KGameClient *)));
}

KGame::~KGame()
{
 kDebug(11001) ;
// Debug();
 reset();
 delete d->mGameSequence;
 delete d->mRandom;
 delete d;
 kDebug(11001) << "done";
}

bool KGame::reset()
{
 deletePlayers();
 deleteInactivePlayers();
 return true;
}

void KGame::deletePlayers()
{
// kDebug(11001) ;
 KGamePlayerList tmp = d->mPlayerList; // in case of PolicyClean player=d->mPlayerList.first() is infinite
 while (!tmp.isEmpty())
         delete tmp.takeFirst();
// kDebug(11001) << "done";
}

void KGame::deleteInactivePlayers()
{
 while (!d->mInactivePlayerList.isEmpty())
         delete d->mInactivePlayerList.takeFirst();
}

bool KGame::load(const QString& filename,bool reset)
{
  if (filename.isNull())
  {
    return false;
  }
  QFile f(filename);
  if (!f.open(QIODevice::ReadOnly))
  {
    return false;
  }
  QDataStream s( &f );
  load(s,reset);
  f.close();
  return true;
}

bool KGame::load(QDataStream &stream,bool reset)
{ return loadgame(stream, false,reset); }

bool KGame::loadgame(QDataStream &stream, bool network,bool resetgame)
{
 // Load Game Data

 // internal data
 qint32 c;
 stream >> c; // cookie

 if (c!=cookie())
 {
   kWarning(11001) << "Trying to load different game version we="<<cookie() << "saved=" << c;
   bool result=false;
   emit signalLoadError(stream,network,(int)c,result);
   return result;
 }
 if (resetgame) reset();

 uint i;
 stream >> i;
// setPolicy((GamePolicy)i);

 stream >> d->mUniquePlayerNumber;

 if (gameSequence())
 {
   gameSequence()->setCurrentPlayer(0);  // TODO !!!
 }
 int newseed;
 stream >> newseed;
 d->mRandom->setSeed(newseed);

 // Switch off the direct emitting of signals while
 // loading properties. This can cause inconsistencies
 // otherwise if a property emits and this emit accesses
 // a property not yet loaded
 // Note we habe to have this external locking to prevent the games unlocking
 // to access the players
 dataHandler()->lockDirectEmit();

 for ( KGamePlayerList::iterator it = playerList()->begin(); it!=playerList()->end();it++ )
 {
   (*it)->dataHandler()->lockDirectEmit();
   // kDebug(11001) << "Player "<<player->id() << "to indirect emit";
 }

 // Properties
 dataHandler()->load(stream);

 // If there is additional data to be loaded before players are loaded then do
 // this here.
 emit signalLoadPrePlayers(stream);
 
 // Switch back on the direct emitting of signals and emit the
 // queued signals for properties. 
 // Unlocks properties before loading players in order to make game 
 // initializations related to properties before using them in players 
 // initialization
 dataHandler()->unlockDirectEmit(); 

 // Load Playerobjects
 uint playercount;
 stream >> playercount;
 kDebug(11001) << "Loading KGame" << playercount << "KPlayer objects";
 for (i=0;i<playercount;i++)
 {
   KPlayer *newplayer=loadPlayer(stream,network);
   systemAddPlayer(newplayer);
 }

 qint16 cookie;
 stream >> cookie;
 if (cookie==KGAME_LOAD_COOKIE) {
   kDebug(11001) << "   Game loaded propertly";
 } else {
   kError(11001) << "   Game loading error. probably format error";
 }

 // Switch back on the direct emitting of signals and emit the
 // queued signals for players.
 // Note we habe to have this external locking to prevent the games unlocking
 // to access the players
 for ( KGamePlayerList::iterator it = playerList()->begin(); it!=playerList()->end();it++ )
 {
   (*it)->dataHandler()->unlockDirectEmit();
   // kDebug(11001) << "Player "<<player->id() << "to direct emit";
 }

 emit signalLoad(stream);
 return true;
}

bool KGame::save(const QString& filename,bool saveplayers)
{
 if (filename.isNull())
 {
   return false;
 }
 QFile f(filename);
 if (!f.open(QIODevice::WriteOnly))
 {
   return false;
 }
 QDataStream s( &f );
 save(s,saveplayers);
 f.close();
 return true;
}

bool KGame::save(QDataStream &stream,bool saveplayers)
{ return savegame(stream, false,saveplayers); }

bool KGame::savegame(QDataStream &stream,bool /*network*/,bool saveplayers)
{
  // Save Game Data

  // internal variables
  qint32 c=cookie();
  stream << c;

  uint p=(uint)policy();
  stream << p;
  stream << d->mUniquePlayerNumber;
  int newseed=(int)d->mRandom->getLong(65535);
  stream << newseed;
  d->mRandom->setSeed(newseed);

 // Properties
 dataHandler()->save(stream);

 // Save all data that need to be saved *before* the players are saved
 emit signalSavePrePlayers(stream);

 if (saveplayers)
 {
   savePlayers(stream,playerList());
 }
 else
 {
   stream << (uint)0; // no players saved
 }

 stream << (qint16)KGAME_LOAD_COOKIE;

 emit signalSave(stream);
 return true;
}

void KGame::savePlayer(QDataStream &stream,KPlayer* p)
{
// this could be in KGameMessage as well
 stream << (qint32)p->rtti();
 stream << (qint32)p->id();
 stream << (qint32)p->calcIOValue();
 p->save(stream);
}

void KGame::savePlayers(QDataStream &stream, KGamePlayerList *list)
{
 if (!list)
 {
   list=playerList();
 }

 qint32 cnt=list->count();
 kDebug(11001) << "Saving KGame" << cnt << "KPlayer objects";
 stream << cnt;
 
 for ( KGamePlayerList::iterator it = playerList()->begin(); it!=playerList()->end();it++ )
 {
   savePlayer(stream,*it);
 }
}

KPlayer *KGame::createPlayer(int /*rtti*/,int /*io*/,bool /*isvirtual*/)
{
  kWarning(11001) << "   No user defined player created. Creating default KPlayer. This crashes if you have overwritten KPlayer!!!! ";
  return new KPlayer;
}
KPlayer *KGame::loadPlayer(QDataStream& stream,bool isvirtual)
{
  qint32 rtti,id,iovalue;
  stream >> rtti >> id >> iovalue;
  KPlayer *newplayer=findPlayer(id);
  if (!newplayer)
  {
    kDebug(11001) << "Player "<< id << "not found...asking user to create one";
    newplayer=createPlayer(rtti,iovalue,isvirtual);
    //emit signalCreatePlayer(newplayer,rtti,iovalue,isvirtual,this);
  }
  /*
  if (!newplayer)
  {
    kWarning(11001) << "   No user defined player created. Creating default KPlayer. This crashes if you have overwritten KPlayer!!!! ";
    newplayer=new KPlayer;
  }
  else
  {
    kDebug(11001) << "   USER Player" << newplayer << "done player->rtti=" << newplayer->rtti() << "rtti=" << rtti;
  }
  */
  newplayer->load(stream);
  if (isvirtual)
  {
    newplayer->setVirtual(true);
  }
  return newplayer;
}

// ----------------- Player handling -----------------------

KPlayer * KGame::findPlayer(quint32 id) const
{
  for ( KGamePlayerList::iterator it = d->mPlayerList.begin(); it!=d->mPlayerList.end();it++ )
 {
   if ((*it)->id() == id)
   {
     return *it;
   }
 }
 for ( KGamePlayerList::iterator it = d->mInactivePlayerList.begin(); it!=d->mInactivePlayerList.end();it++ )
 {
   if ((*it)->id() == id)
   {
     return *it;
   }
 }
 return 0;
}

// it is necessary that addPlayer and systemAddPlayer are called in the same
// order. Ie if addPlayer(foo) followed by addPlayer(bar) is called, you must
// not call systemAddPlayer(bar) followed by systemAddPlayer(foo), as the
// mAddPlayerList would get confused. Should be no problem as long as comServer
// and the clients are working correctly.
// BUT: if addPlayer(foo) does not arrive by any reason while addPlayer(bar)
// does, we would be in trouble...
bool KGame::addPlayer(KPlayer* newplayer)
{
 kDebug(11001) << ":  " << "; maxPlayers=" << maxPlayers() << "playerCount=" << playerCount();
 if (!newplayer)
 {
  kFatal(11001) << "trying to add NULL player in KGame::addPlayer()";
  return false;
 }

 if (maxPlayers() >= 0 && (int)playerCount() >= maxPlayers())
 {
   kWarning(11001) << "cannot add more than" << maxPlayers() << "players - deleting...";
   return false;
 }

 if (newplayer->id() == 0)
 {
   d->mUniquePlayerNumber++;
   newplayer->setId(KGameMessage::createPlayerId(d->mUniquePlayerNumber, gameId()));
   kDebug(11001) << "NEW!!! player" << newplayer << "now has id" << newplayer->id();
 }
 else
 {
   // this could happen in games which use their own ID management by certain
   // reasons. that is NOT recommended
   kDebug(11001) << "player" << newplayer << "already has an id:" << newplayer->id();
 }

 QByteArray buffer;
 QDataStream stream(&buffer,QIODevice::WriteOnly);
 // We distinguis here what policy we have
 if (policy()==PolicyLocal || policy()==PolicyDirty)
 {
   if ( !systemAddPlayer(newplayer) )
       return false;
 }
 if (policy()==PolicyClean || policy()==PolicyDirty)
 {
   savePlayer(stream,newplayer);
   // Store the player for delayed clean adding
   if (policy()==PolicyClean)
   {
     d->mAddPlayerList.enqueue(newplayer);
   }
   sendSystemMessage(stream,(int)KGameMessage::IdAddPlayer, 0);
 }
 return true;
}

bool KGame::systemAddPlayer(KPlayer* newplayer)
{
 if (!newplayer)
 {
   kFatal(11001) << "trying to add NULL player in KGame::systemAddPlayer()";
   return false ;
 }
 if (newplayer->id() == 0)
 {
   kWarning(11001) << "player" << newplayer << "has no ID";
 }

 if (findPlayer(newplayer->id()))
 {
   kError(11001) << "ERROR: Double adding player !!!!! NOT GOOD !!!!!! " << newplayer->id() << "...I delete it again";
   delete newplayer;
   return false;
 }
 else
 {
   kDebug(11001) << "Trying to add player" << newplayer <<" maxPlayers="<<maxPlayers()<<" playerCount="<<playerCount();
   // Add the player to the game
   d->mPlayerList.append(newplayer);
   newplayer->setGame(this);
   kDebug(11001) << "Player: isVirtual=" << newplayer->isVirtual();
   kDebug(11001) << "        id=" << newplayer->id() << "  #Players="
                  << d->mPlayerList.count() << "added" << newplayer
                  << "  (virtual=" << newplayer->isVirtual() << ")";
   emit signalPlayerJoinedGame(newplayer);
 }
 return true;
}

// Called by the KPlayer destructor
void KGame::playerDeleted(KPlayer *player)
{
 kDebug(11001) << ": id (" << player->id() << ") to be removed" << player;

 if (policy()==PolicyLocal || policy()==PolicyDirty)
 {
   systemRemovePlayer(player,false);
 }
 if (policy()==PolicyClean || policy()==PolicyDirty)
 {
   if (!player->isVirtual())
   {
     kDebug(11001) << ": sending IdRemovePlayer "<<player->id();
     sendSystemMessage(player->id(), KGameMessage::IdRemovePlayer, 0);
   }
 }
}

bool KGame::removePlayer(KPlayer * player, quint32 receiver)
{//transmit to all clients, or to receiver only
 if (!player)
 {
   kFatal(11001) << "trying to remove NULL player in KGame::removePlayer()";
   return false;
 }
 kDebug(11001) << ": id (" << player->id() << ") to be removed" << player;

 if (policy()==PolicyLocal || policy()==PolicyDirty)
 {
     systemRemovePlayer(player,true);
     return true; // player is gone
 }
 if (policy()==PolicyClean || policy()==PolicyDirty)
 {
   kDebug(11001) << ": sending IdRemovePlayer "<<player->id();
   sendSystemMessage(player->id(),KGameMessage::IdRemovePlayer, receiver);
 }
 return true;
 // we will receive the message in networkTransmission()
}

void KGame::systemRemovePlayer(KPlayer* player,bool deleteit)
{
 kDebug(11001) ;
 if (!player)
 {
   kWarning(11001) << "cannot remove NULL player";
   return;
 }
 systemRemove(player,deleteit);

 if (gameStatus()==(int)Run && playerCount()<minPlayers())
 {
   kWarning(11001) << ": not enough players, PAUSING game\n";
   setGameStatus(Pause);
 }
}

bool KGame::systemRemove(KPlayer* p,bool deleteit)
{
 if (!p)
 {
   kWarning(11001) << "cannot remove NULL player";
   return false;
 }
 bool result;
 kDebug(11001) << ": Player (" << p->id() << ") to be removed" << p;

 if (d->mPlayerList.count() == 0)
 {
   result = false;
 }
 else
 {

   result = d->mPlayerList.removeAll(p);
 }

 emit signalPlayerLeftGame(p);

 p->setGame(0);
 if (deleteit)
 {
   delete p;
 }

 return result;
}

bool KGame::inactivatePlayer(KPlayer* player)
{
 if (!player)
 {
   return false;
 }
 kDebug(11001) << "Inactivate player" << player->id();

 if (policy()==PolicyLocal || policy()==PolicyDirty)
 {
   if ( !systemInactivatePlayer(player) )
       return false;
 }
 if (policy()==PolicyClean || policy()==PolicyDirty)
 {
   sendSystemMessage(player->id(), KGameMessage::IdInactivatePlayer);
 }

 return true;
}

bool KGame::systemInactivatePlayer(KPlayer* player)
{
 if (!player || !player->isActive())
 {
   return false;
 }
 kDebug(11001) << "Inactivate player" << player->id();

 int pid=player->id();
 // Virtual players cannot be deactivated. They will be removed
 if (player->isVirtual())
 {
   systemRemovePlayer(player,true);
   return false; // don't touch player after this!
 }
 else
 {
   d->mPlayerList.removeAll(player);
   d->mInactivePlayerList.prepend(player);
   player->setActive(false);
 }
 emit signalPlayerLeftGame(player);
 if (isAdmin())
 {
   d->mInactiveIdList.prepend(pid);
 }
 return true;
}

bool KGame::activatePlayer(KPlayer * player)
{
  if (!player)
  {
    return false;
  }
  kDebug(11001) << ": activate" << player->id();
  if (policy()==PolicyLocal || policy()==PolicyDirty)
  {
      if ( !systemActivatePlayer(player) )
          return false;
  }
  if (policy()==PolicyClean || policy()==PolicyDirty )
  {
    sendSystemMessage(player->id(), KGameMessage::IdActivatePlayer);
  }
 return true;
}

bool KGame::systemActivatePlayer(KPlayer* player)
{
 if (!player || player->isActive())
 {
   return false;
 }
 kDebug(11001) << ": activate" << player->id();

 d->mInactivePlayerList.removeAll(player);
 player->setActive(true);
 if ( !addPlayer(player) ) // player is gone
     return false;

 if (isAdmin())
 {
   d->mInactiveIdList.removeAll(player->id());
 }
 return true;
}

// -------------------- Properties ---------------------------

void KGame::setMaxPlayers(uint maxnumber)
{ if (isAdmin()) { d->mMaxPlayer.changeValue(maxnumber); } }

void KGame::setMinPlayers(uint minnumber)
{ if (isAdmin()) { d->mMinPlayer.changeValue(minnumber); } }

uint KGame::minPlayers() const
{ return d->mMinPlayer.value(); }

int KGame::maxPlayers() const
{ return d->mMaxPlayer.value(); }

uint KGame::playerCount() const
{ return d->mPlayerList.count(); }

int KGame::gameStatus() const
{ return d->mGameStatus.value(); }

bool KGame::isRunning() const
{ return d->mGameStatus.value() == Run; }

KGamePropertyHandler* KGame::dataHandler() const
{ return d->mProperties; }


KGame::KGamePlayerList* KGame::inactivePlayerList()
{ return &d->mInactivePlayerList; }

const KGame::KGamePlayerList* KGame::inactivePlayerList() const
{ return &d->mInactivePlayerList; }

KGame::KGamePlayerList* KGame::playerList()
{ return &d->mPlayerList; }

const KGame::KGamePlayerList* KGame::playerList() const
{ return &d->mPlayerList; }

KRandomSequence* KGame::random() const
{ return d->mRandom; }


bool KGame::sendPlayerInput(QDataStream &msg, KPlayer *player, quint32 sender)
{
 if (!player)
 {
   kError(11001) << ": NULL player";
   return false;
 }
 if (!isRunning())
 {
   kError(11001) << ": game not running";
   return false;
 }

 kDebug(11001) << ": transmitting playerInput over network";
 sendSystemMessage(msg, (int)KGameMessage::IdPlayerInput, player->id(), sender);
 return true;
}

bool KGame::systemPlayerInput(QDataStream &msg, KPlayer *player, quint32 sender)
{
 if (!player)
 {
   kError(11001) << ": NULL player";
   return false;
 }
 if (!isRunning())
 {
   kError(11001) << ": game not running";
   return false;
 }
 kDebug(11001) << "KGame: Got playerInput from messageServer... sender:" << sender;
 if (playerInput(msg,player))
 {
   playerInputFinished(player);
 }
 else
 {
   kDebug(11001) <<": switching off player input";
   // TODO: (MH 03-2003): We need an return option from playerInput so that
   // the player's is not automatically disabled here
   if (!player->asyncInput())
   {
     player->setTurn(false); // in turn based games we have to switch off input now
   }
 }
 return true;
}


KPlayer * KGame::playerInputFinished(KPlayer *player)
{
    if ( !player )
        return 0;

 kDebug(11001) <<"player input finished for "<<player->id();
 // Check for game over and if not allow the next player to move
 int gameOver = 0;
 if (gameSequence())
 {
   gameSequence()->setCurrentPlayer(player);
 }
 // do not call gameSequence()->checkGameOver() to keep backward compatibility!
 gameOver = checkGameOver(player);
 if (gameOver!=0)
 {
     player->setTurn(false);
     setGameStatus(End);
     emit signalGameOver(gameOver,player,this);
 }
 else if (!player->asyncInput())
 {
   player->setTurn(false); // in turn based games we have to switch off input now
   if (gameSequence())
   {
     QTimer::singleShot(0,this,SLOT(prepareNext()));
   }
 }
 return player;
}

// Per default we do not do anything
int KGame::checkGameOver(KPlayer *player)
{
 if (gameSequence())
 {
   return gameSequence()->checkGameOver(player);
 }
 return 0;
}

void KGame::setGameSequence(KGameSequence* sequence)
{
 delete d->mGameSequence;
 d->mGameSequence = sequence;
 if (d->mGameSequence)
 {
   d->mGameSequence->setGame(this);
 }
}

KGameSequence* KGame::gameSequence() const
{
  return d->mGameSequence;
}

void KGame::prepareNext()
{
 if (gameSequence())
 {
   // we don't call gameSequence->nextPlayer() to keep old code working
   nextPlayer(gameSequence()->currentPlayer());
 }
}

KPlayer *KGame::nextPlayer(KPlayer *last,bool exclusive)
{
 if (gameSequence())
 {
   return gameSequence()->nextPlayer(last, exclusive);
 }
 return 0;
}

void KGame::setGameStatus(int status)
{
 kDebug(11001) << ": GAMESTATUS CHANGED  to" << status;
 if (status==(int)Run && playerCount()<minPlayers())
 {
   kDebug(11001) << ": not enough players, pausing game\n";
   status=Pause;
 }
 d->mGameStatus = status;
}

void KGame::networkTransmission(QDataStream &stream, int msgid, quint32 receiver, quint32 sender, quint32 /*clientID*/)
{//clientID is unused
 // message targets a playerobject. If we find it we forward the message to the
 // player. Otherwise we proceed here and hope the best that the user processes
 // the message

//  kDebug(11001) << ": we="<<(int)gameId()<<" id="<<msgid<<" recv=" << receiver << "sender=" << sender;


 // *first* notice the game that something has changed - so no return prevents
 // this
 emit signalMessageUpdate(msgid, receiver, sender);
 if (KGameMessage::isPlayer(receiver))
 {
   //kDebug(11001) << "message id" << msgid << "seems to be for a player ("<<active=p->isActive()<<" recv="<< receiver;
   KPlayer *p=findPlayer(receiver);
   if (p && p->isActive())
   {
     p->networkTransmission(stream,msgid,sender);
     return;
   }
   if (p)
   {
      kDebug(11001) << "player is here but not active";
   }
   else
   {
      kDebug(11001) << "no player found";
   }
 }
 // If it is not for a player it is meant for us!!!! Otherwise the
 // gamenetwork would not have passed the message to us!

 // GameProperties processed
 if (d->mProperties->processMessage(stream, msgid, sender == gameId()))
 {
//   kDebug(11001 ) << "KGame: message taken by property - returning";
   return ;
 }

 switch(msgid)
 {
   case KGameMessage::IdSetupGame:  // Client: First step in setup game
   {
     qint16 v;
     qint32 c;
     stream >> v >> c;
     kDebug(11001) << " ===================> (Client) " << ": Got IdSetupGame ==================";
     kDebug(11001) << "our game id is" << gameId() << "Lib version=" << v << "App Cookie=" << c;
     // Verify identity of the network partners
     if (c!=cookie())
     {
       kError(11001) << "IdGameSetup: Negotiate Game: cookie mismatch I'am="<<cookie()<<" master="<<c;
       sendError(KGameError::Cookie, KGameError::errCookie(cookie(), c));
       disconnect(); // disconnect from master
     }
     else if (v!=KGameMessage::version())
     {
       sendError(KGameError::Version, KGameError::errVersion(v));
       disconnect(); // disconnect from master
     }
     else
     {
       setupGame(sender);
     }
     kDebug(11001) << "========== (Client) Setup game done\n";
   }
   break;
   case KGameMessage::IdSetupGameContinue:  // Master: second step in game setup
   {
     kDebug(11001) << "=====>(Master) " << " - IdSetupGameContinue";
     setupGameContinue(stream, sender);
   }
   break;
   case KGameMessage::IdActivatePlayer:  // Activate Player
   {
     int id;
     stream >> id;
     kDebug(11001) << "Got IdActivatePlayer id=" << id;
     if (sender!=gameId()  || policy()!=PolicyDirty)
     {
       systemActivatePlayer(findPlayer(id));
     }
   }
   break;
   case KGameMessage::IdInactivatePlayer:  // Inactivate Player
   {
     int id;
     stream >> id;
     kDebug(11001) << "Got IdInactivatePlayer id=" << id;
     if (sender!=gameId()  || policy()!=PolicyDirty)
     {
       systemInactivatePlayer(findPlayer(id));
     }
   }
   break;
   case KGameMessage::IdAddPlayer:
   {
     kDebug(11001) << ": Got IdAddPlayer";
     if (sender!=gameId()  || policy()!=PolicyDirty)
     {
       KPlayer *newplayer=0;
       // We sent the message so the player is already available
       if (sender==gameId())
       {
          kDebug(11001) << "dequeue previously added player";
          newplayer = d->mAddPlayerList.dequeue();
       }
       else
       {
         newplayer=loadPlayer(stream,true);
       }
       systemAddPlayer(newplayer);// the final, local, adding
       //systemAddPlayer(stream);
     }
   }
   break;
   case KGameMessage::IdRemovePlayer: // Client should delete player id
   {
     int id;
     stream >> id;
     kDebug(11001) << ": Got IdRemovePlayer" << id;
     KPlayer *p=findPlayer(id);
     if (p)
     {
       // Otherwise the player is already removed
       if (sender!=gameId()  || policy()!=PolicyDirty)
       {
         systemRemovePlayer(p,true);
       }
     }
     else
     {
       kWarning(11001) << "Cannot find player" << id;
     }
   }
   break;
   case KGameMessage::IdGameLoad:
   {
     kDebug(11001) << "====> (Client) " << ": Got IdGameLoad";
     loadgame(stream,true,false);
   }
   break;
   case KGameMessage::IdGameSetupDone:
   {
     int cid;
     stream >> cid;
     kDebug(11001) << "====> (CLIENT) " << ": Got IdGameSetupDone for client "
             << cid << "we are =" << gameId();
     sendSystemMessage(gameId(), KGameMessage::IdGameConnected, 0);
   }
   break;
   case KGameMessage::IdGameConnected:
   {
     int cid;
     stream >> cid;
     kDebug(11001) << "====> (ALL) " << ": Got IdGameConnected for client "<< cid << "we are =" << gameId();
     emit signalClientJoinedGame(cid,this);
   }
   break;

   case KGameMessage::IdSyncRandom:  // Master forces a new random seed on us
   {
     int newseed;
     stream >> newseed;
     kDebug(11001) << "CLIENT: setting random seed to" << newseed;
     d->mRandom->setSeed(newseed);
   }
   break;
   case KGameMessage::IdDisconnect:
   {
   // if we disconnect we *always* start a local game.
   // this could lead into problems if we just change the message server
     if (sender != gameId())
     {
         kDebug(11001) << "client" << sender << "leaves game";
         return;
     }
     kDebug(11001) << "leaving the game";
     // start a new local game
     // no other client is by default connected to this so this call should be
     // enough
     setMaster();
   }
   break;
   default:
    {
     if (msgid < KGameMessage::IdUser)
     {
       kError(11001) << "incorrect message id" << msgid << " - emit anyway";
     }
     kDebug(11001) << ": User data msgid" << msgid;
     emit signalNetworkData(msgid - KGameMessage::IdUser,((QBuffer*)stream.device())->readAll(),receiver,sender);
   }
   break;
 }

}

// called by the IdSetupGameContinue Message - MASTER SIDE
// Here the master needs to decide which players can take part at the game
// and which will be deactivated
void KGame::setupGameContinue(QDataStream& stream, quint32 sender)
{
  KPlayer *player;
  qint32 cnt;
  int i;
  stream >> cnt;

  QList<int> inactivateIds;

  KGamePlayerList newPlayerList;
  for (i=0;i<cnt;i++)
  {
    player=loadPlayer(stream,true);
    kDebug(11001) << "Master got player" << player->id() <<" rawgame=" << KGameMessage::rawGameId(player->id())  << "from sender" << sender;
    if (KGameMessage::rawGameId(player->id()) != sender)
    {
      kError(11001) << "Client tries to add player with wrong game id - cheat possible";
    }
    else
    {
      newPlayerList.append(player);
      kDebug(11001) << "newplayerlist appended" << player->id();
    }
  }

  newPlayersJoin(playerList(),&newPlayerList,inactivateIds);


  kDebug(11001) << "Master calculates how many players to activate client has cnt=" << cnt;
  kDebug(11001) << "The game has" << playerCount() << "active players";
  kDebug(11001) << "The user deactivated "<< inactivateIds.count() << "player already";
  kDebug(11001) << "MaxPlayers for this game is" << maxPlayers();

  // Do we have too many players? (After the programmer disabled some?)
  // MH: We cannot use have player here as it CHANGES in the loop
  // int havePlayers = cnt+playerCount()-inactivateIds.count();
  kDebug(11001) << "havePlayers" << cnt+playerCount()-inactivateIds.count();
  while (maxPlayers() > 0 && maxPlayers() < (int)(cnt+playerCount() - inactivateIds.count()))
  {
    kDebug(11001) << "  Still to deacticvate "
            << (int)(cnt+playerCount()-inactivateIds.count())-(int)maxPlayers()
;
    KPlayer *currentPlayer=0;
    int currentPriority=0x7fff; // MAX_UINT (16bit?) to get the maximum of the list
    // find lowest network priority which is not yet in the newPlayerList
    // do this for the new players
    for ( KGamePlayerList::iterator it = newPlayerList.begin(); it!=newPlayerList.end();it++ )
    {
      KPlayer* player = *it;
      // Already in the list
      if (inactivateIds.indexOf(player->id())!=-1)
      {
        continue;
      }
      if (player->networkPriority()<currentPriority)
      {
        currentPriority=player->networkPriority();
        currentPlayer=player;
      }
    }

    // find lowest network priority which is not yet in the newPlayerList
    // Do this for the network players
    for ( KGamePlayerList::iterator it = d->mPlayerList.begin(); it!=d->mPlayerList.end();it++ )
    {
      KPlayer* player = *it;
      // Already in the list
      if (inactivateIds.indexOf(player->id())!=-1)
      {
        continue;
      }
      if (player->networkPriority()<currentPriority)
      {
        currentPriority=player->networkPriority();
        currentPlayer=player;
      }
    }

    // add it to inactivateIds
    if (currentPlayer)
    {
      kDebug(11001) << "Marking player" << currentPlayer->id() << "for inactivation";
      inactivateIds.append(currentPlayer->id());
    }
    else
    {
      kError(11001) << "Couldn't find a player to dectivate..That is not so good...";
      break;
    }
  }

  kDebug(11001) << "Alltogether deactivated" << inactivateIds.count() << "players";

  QList<int>::Iterator it;
  for ( it = inactivateIds.begin(); it != inactivateIds.end(); ++it )
  {
    int pid=*it;
    kDebug(11001) << "pid=" << pid;
  }

  // Now deactivate the network players from the inactivateId list
  //QValueList<int>::Iterator it;
  for ( it = inactivateIds.begin(); it != inactivateIds.end(); ++it )
  {
    int pid=*it;
    if (KGameMessage::rawGameId(pid) == sender)
    {
      continue; // client's player
    }
    kDebug(11001) << " -> the network needs to deactivate" << pid;
    player=findPlayer(pid);
    if (player)
    {
      // We have to make REALLY sure that the player is gone. With any policy
      if (systemInactivatePlayer(player) && policy()!=PolicyLocal)
      {
        sendSystemMessage(player->id(), KGameMessage::IdInactivatePlayer);
      } else
      player = 0;
    }
    else
    {
      kError(11001) << "We should deactivate a player, but cannot find it...not good.";
    }
  }

  // Now send out the player list which the client can activate
  for ( KGamePlayerList::iterator it = newPlayerList.begin(); it!=newPlayerList.end();it++ )
  {
    KPlayer* player = *it;
    kDebug(11001) << "newplayerlist contains" << player->id();
    // Only activate what is not in the list
    if (inactivateIds.indexOf(player->id())!=-1)
    {
      continue;
    }
    kDebug(11001) << " -> the client can ******** reactivate ********  " << player->id();
    sendSystemMessage(player->id(), KGameMessage::IdActivatePlayer, sender);
  }

  // Save the game over the network
  QByteArray bufferS;
  QDataStream streamS(&bufferS,QIODevice::WriteOnly);
  // Save game over netowrk and save players
  savegame(streamS,true,true);
  sendSystemMessage(streamS,KGameMessage::IdGameLoad,sender);


  // Only to the client first , as the client will add players
  sendSystemMessage(sender, KGameMessage::IdGameSetupDone, sender);
  
  //Finally delete content of the newPlayerList
  qDeleteAll(newPlayerList);
  newPlayerList.clear();
}

// called by the IdSetupGame Message - CLIENT SIDE
// Client needs to prepare for network transfer
void KGame::setupGame(quint32 sender)
{
  QByteArray bufferS;
  QDataStream streamS(&bufferS,QIODevice::WriteOnly);

  // Deactivate all players
  KGamePlayerList mTmpList(d->mPlayerList); // we need copy otherwise the removal crashes
  qint32 cnt=mTmpList.count();
  kDebug(11001) << "Client: playerlistcount=" << d->mPlayerList.count() << "tmplistcout=" << cnt;

  streamS << cnt;

  KGamePlayerList::iterator it = mTmpList.begin();
  KPlayer *player;
  while (it!=mTmpList.end())
  {
    player=*it;
    ++it;
    --cnt;

    if (!systemInactivatePlayer(player))
	continue; // player is gone

    // Give the new game id to all players (which are inactivated now)
    player->setId(KGameMessage::createPlayerId(player->id(),gameId()));

    // Save it for the master to decide what to do
    savePlayer(streamS,player);
  }
  if (d->mPlayerList.count() > 0 || cnt!=0)
  {
    kFatal(11001) << "KGame::setupGame(): Player list is not empty! or cnt!=0=" <<cnt;
  }

  sendSystemMessage(streamS,KGameMessage::IdSetupGameContinue,sender);
}

// unused by KGame
void KGame::syncRandom()
{
 int newseed=(int)d->mRandom->getLong(65535);
 sendSystemMessage(newseed,KGameMessage::IdSyncRandom); // Broadcast
 d->mRandom->setSeed(newseed);
}

void KGame::Debug()
{
 KGameNetwork::Debug();
 kDebug(11001) << "------------------- KGAME -------------------------";
 kDebug(11001) << "this:          " << this;
 kDebug(11001) << "uniquePlayer   " << d->mUniquePlayerNumber;
 kDebug(11001) << "gameStatus     " << gameStatus();
 kDebug(11001) << "MaxPlayers :   " << maxPlayers();
 kDebug(11001) << "NoOfPlayers :  " << playerCount();
 kDebug(11001) << "NoOfInactive:  " << d->mInactivePlayerList.count();
 kDebug(11001) << "---------------------------------------------------";
}

void KGame::slotClientConnected(quint32 clientID)
{
 if (isAdmin())
 {
   negotiateNetworkGame(clientID);
 }
}

void KGame::slotServerDisconnected() // Client side
{
  kDebug(11001) << "======= SERVER DISCONNECT =======";
  kDebug(11001) << "+++ (CLIENT)++++++++" << ": our GameID="<<gameId();

  int oldgamestatus=gameStatus();


  KGamePlayerList removeList;
  kDebug(11001) << "Playerlist of client=" << d->mPlayerList.count() << "count";
  kDebug(11001) << "Inactive Playerlist of client=" << d->mInactivePlayerList.count() << "count";
  for ( KGamePlayerList::iterator it = d->mPlayerList.begin(); it!=d->mPlayerList.end();it++ )
  {
    KPlayer* player = *it;
    // TODO: CHECK: id=0, could not connect to server in the first place??
    if (KGameMessage::rawGameId(player->id()) != gameId() && gameId()!=0)
    {
      kDebug(11001) << "Player" << player->id() << "belongs to a removed game";
      removeList.append(player);
    }
  }

  for ( KGamePlayerList::iterator it = removeList.begin(); it!=removeList.end();it++ )
  {
    KPlayer* player = *it;
    bool remove = true;
    emit signalReplacePlayerIO(player, &remove);
    if (remove)
    {
      kDebug(11001) << " ---> Removing player" << player->id();
      systemRemovePlayer(player,true); // no network necessary
    }
  }

  setMaster();
  kDebug(11001) << "our game id is after setMaster" << gameId();

  KGamePlayerList mReList(d->mInactivePlayerList);
  for ( KGamePlayerList::iterator it = mReList.begin(); it!=mReList.end();it++ )
  {
    KPlayer* player = *it;
    // TODO ?check for priority? Sequence should be ok
    if ((int)playerCount()<maxPlayers() || maxPlayers()<0)
    {
      systemActivatePlayer(player);
    }
  }
  kDebug(11001) << "Players activated player-cnt=" << playerCount();

  for ( KGamePlayerList::iterator it = d->mPlayerList.begin(); it!=d->mPlayerList.end();it++ )
  {
    KPlayer* player = *it;
    int oldid=player->id();
    d->mUniquePlayerNumber++;
    player->setId(KGameMessage::createPlayerId(d->mUniquePlayerNumber,gameId()));
    kDebug(11001) << "Player id" << oldid <<" changed to" << player->id() << "as we are now local";
  }
  // TODO clear inactive lists ?
  Debug();
  for ( KGamePlayerList::iterator it = d->mPlayerList.begin(); it!=d->mPlayerList.end();it++ )
  {
    KPlayer* player = *it;
    player->Debug();
  }
  kDebug(11001) << "+++++++++++" << "DONE=";
  emit signalClientLeftGame(0,oldgamestatus,this);
}

void KGame::slotClientDisconnected(quint32 clientID,bool /*broken*/) // server side
{
 kDebug(11001) << "++++(SERVER)+++++++" << "clientId=" << clientID;

 int oldgamestatus=gameStatus();

 KPlayer *player;
 KGamePlayerList removeList;
 kDebug(11001) << "Playerlist of client=" << d->mPlayerList.count() << "count";
 for ( KGamePlayerList::iterator it = d->mPlayerList.begin(); it!=d->mPlayerList.end();it++ )
 {
   KPlayer* player = *it;
   if (KGameMessage::rawGameId(player->id())==clientID)
   {
     kDebug(11001) << "Player" << player->id() << "belongs to the removed game";
     removeList.append(player);
   }
 }

 for ( KGamePlayerList::iterator it = removeList.begin(); it!=removeList.end();it++ )
 {
   KPlayer* player = *it;
   // try to replace the KGameIO first
   bool remove = true;
   emit signalReplacePlayerIO(player, &remove);
   if (remove) {
     // otherwise (no new KGameIO) remove the player
     kDebug(11001) << " ---> Removing player" << player->id();
     removePlayer(player,0);
   }
 }

 // Now add inactive players - sequence should be ok
 // TODO remove players from removed game
 for (int idx=0;idx<d->mInactiveIdList.count();idx++)
 {
   int it1 = d->mInactiveIdList.at(idx);
   player = findPlayer(it1);
   if (((int)playerCount() < maxPlayers() || maxPlayers() < 0) && player && KGameMessage::rawGameId(it1) != clientID)
   {
     activatePlayer(player);
   }
 }
  emit signalClientLeftGame(clientID,oldgamestatus,this);
}


// -------------------- Synchronization -----------------------

// this initializes a newly connected client.
// we send the number of players (including type) as well as game status and
// properties to the client. After the initialization has been completed both
// clients should have the same status (ie players, properties, etc)
void KGame::negotiateNetworkGame(quint32 clientID)
{
 kDebug(11001) << "===========================" << ": clientID=" << clientID << " =========================== ";
 if (!isAdmin())
 {
   kError(11001) << ": Serious WARNING..only gameAdmin should call this";
   return ;
 }

 QByteArray buffer;
 QDataStream streamGS(&buffer,QIODevice::WriteOnly);

 // write Game setup specific data
 //streamGS << (qint32)maxPlayers();
 //streamGS << (qint32)minPlayers();

 // send to the newly connected client *only*
 qint16 v=KGameMessage::version();
 qint32 c=cookie();
 streamGS << v << c;
 sendSystemMessage(streamGS, KGameMessage::IdSetupGame, clientID);
}

bool KGame::sendGroupMessage(const QByteArray &msg, int msgid, quint32 sender, const QString& group)
{
// AB: group must not be i18n'ed!! we should better use an id for group and use
// a groupName() for the name // FIXME

 for ( KGamePlayerList::iterator it = d->mPlayerList.begin(); it!=d->mPlayerList.end();it++ )
 {
   KPlayer* player = *it;
   if (player && player->group()==group)
   {
     sendMessage(msg,msgid,player->id(), sender);
   }
 }
 return true;
}

bool KGame::sendGroupMessage(const QDataStream &msg, int msgid, quint32 sender, const QString& group)
{ return sendGroupMessage(((QBuffer*)msg.device())->buffer(), msgid, sender, group); }

bool KGame::sendGroupMessage(const QString& msg, int msgid, quint32 sender, const QString& group)
{
 QByteArray buffer;
 QDataStream stream(&buffer, QIODevice::WriteOnly);
 stream << msg;
 return sendGroupMessage(stream, msgid, sender, group);
}

bool KGame::addProperty(KGamePropertyBase* data)
{ return dataHandler()->addProperty(data); }

bool KGame::sendPlayerProperty(int msgid, QDataStream& s, quint32 playerId)
{ return sendSystemMessage(s, msgid, playerId); }

void KGame::sendProperty(int msgid, QDataStream& stream, bool* sent)
{
  bool s = sendSystemMessage(stream, msgid);
  if (s)
  {
    *sent = true;
  }
}

void KGame::emitSignal(KGamePropertyBase *me)
{
 emit signalPropertyChanged(me,this);
}

KGamePropertyBase* KGame::findProperty(int id) const
{ return d->mProperties->find(id); }

KGame::GamePolicy KGame::policy() const
{
 return d->mPolicy;
}
void KGame::setPolicy(GamePolicy p,bool recursive)
{
 // Set KGame policy
 d->mPolicy=p;
 if (recursive)
 {
   // Set all KGame property policy
   dataHandler()->setPolicy((KGamePropertyBase::PropertyPolicy)p,false);

   // Set all KPLayer (active or inactive) property policy
   for ( KGamePlayerList::iterator it = d->mPlayerList.begin(); it!=d->mPlayerList.end();it++ )
   {
     (*it)->dataHandler()->setPolicy((KGamePropertyBase::PropertyPolicy)p,false);
   }
   for ( KGamePlayerList::iterator it = d->mInactivePlayerList.begin(); it!=d->mInactivePlayerList.end();it++ )
   {
     (*it)->dataHandler()->setPolicy((KGamePropertyBase::PropertyPolicy)p,false);
   }
 }
}

/*
 * vim: et sw=2
 */
