# -*- coding: utf-8 -*-

"""Copyright (C) 2009-2012 Wolfgang Rohdewald <wolfgang@rohdewald.de>

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



Read the user manual for a description of the interface to this scoring engine
"""

from util import logDebug
from meld import Meld, meldKey, meldsContent, Pairs, CONCEALED
from rule import Score, Ruleset
from common import elements, Debug

class UsedRule(object):
    """use this in scoring, never change class Rule.
    If the rule has been used for a meld, pass it"""
    def __init__(self, rule, meld=None):
        self.rule = rule
        self.meld = meld

class Hand(object):
    """represent the hand to be evaluated"""

    # pylint: disable=R0902
    # pylint we need more than 10 instance attributes

    cache = dict()
    misses = 0
    hits = 0

    @staticmethod
    def clearCache(game):
        """clears the cache with Hands"""
        if Debug.handCache and Hand.cache:
            game.debug('cache hits:%d misses:%d' % (Hand.hits, Hand.misses))
        Hand.cache.clear()
        Hand.hits = 0
        Hand.misses = 0

    @staticmethod
    def cached(ruleset, string, computedRules=None, robbedTile=None):
        """since a Hand instance is never changed, we can use a cache"""
        cRuleHash = '&&'.join([rule.name for rule in computedRules]) if computedRules else 'None'
        if isinstance(ruleset, Hand):
            cacheId = id(ruleset.player or ruleset.ruleset)
        else:
            cacheId = id(ruleset)
        cacheKey = hash((cacheId, string, robbedTile, cRuleHash))
        cache = Hand.cache
        if cacheKey in cache:
            if cache[cacheKey] is None:
                raise Exception('recursion: Hand calls itself for same content')
            Hand.hits += 1
            return cache[cacheKey]
        Hand.misses += 1
        cache[cacheKey] = None
        result = Hand(ruleset, string,
            computedRules=computedRules, robbedTile=robbedTile)
        cache[cacheKey] = result
        return result

    def __init__(self, ruleset, string, computedRules=None, robbedTile=None):
        """evaluate string using ruleset. rules are to be applied in any case.
        ruleset can be Hand, Game or Ruleset."""
        # silence pylint. This method is time critical, so do not split it into smaller methods
        # pylint: disable=R0902,R0914,R0912,R0915
        if isinstance(ruleset, Hand):
            self.ruleset = ruleset.ruleset
            self.player = ruleset.player
            self.computedRules = ruleset.computedRules
        elif isinstance(ruleset, Ruleset):
            self.ruleset = ruleset
            self.player = None
        else:
            self.player = ruleset
            self.ruleset = self.player.game.ruleset
        self.string = string
        self.robbedTile = robbedTile
        self.computedRules = computedRules or []
        self.__won = False
        self.mjStr = ''
        self.mjRule = None
        self.ownWind = None
        self.roundWind = None
        tileStrings = []
        mjStrings = []
        haveM = False
        splits = self.string.split()
        for part in splits:
            partId = part[0]
            if partId in 'Mmx':
                haveM = True
                self.ownWind = part[1]
                self.roundWind = part[2]
                mjStrings.append(part)
                self.__won = partId == 'M'
            elif partId == 'L':
                if len(part[1:]) > 8:
                    raise Exception('last tile cannot complete a kang:' + self.string)
                mjStrings.append(part)
            else:
                tileStrings.append(part)

        if not haveM:
            raise Exception('Hand got string without mMx: %s', self.string)
        self.mjStr = ' '.join(mjStrings)
        self.lastMeld = self.lastTile = self.lastSource = None
        self.announcements = '' # is set as a side effect by __setLastTile
        self.__setLastTile()
        self.hiddenMelds = []
        self.declaredMelds = []
        self.melds = []
        tileString = ' '.join(tileStrings)
        self.bonusMelds, tileString = self.__separateBonusMelds(tileString)
        self.tileNames = Pairs(tileString.replace(' ','').replace('R', ''))
        self.tileNames.sort()
        self.values = ''.join(x[1] for x in self.tileNames)
        self.suits = set(x[0].lower() for x in self.tileNames)
        self.lenOffset = self.__computeLenOffset(tileString)
        self.dragonMelds, self.windMelds = self.__computeDragonWindMelds(tileString)
        self.inHand = []
        self.__separateMelds(tileString)
        self.hiddenMelds = sorted(self.hiddenMelds, key=meldKey)
        if self.lastTile:
            assert self.lastTile in self.tileNames, 'lastTile %s is not in tiles %s' % (
            self.lastTile, ' '.join(self.tileNames))
            if self.lastSource == 'k':
                assert self.tileNames.count(self.lastTile.lower()) + \
                    self.tileNames.count(self.lastTile.capitalize()) == 1, \
                    'Robbing kong: I cannot have lastTile %s more than once in %s' % (
                     self.lastTile, ' '.join(self.tileNames))

        self.sortedMeldsContent = meldsContent(self.melds)
        if self.bonusMelds:
            self.sortedMeldsContent += ' ' + meldsContent(self.bonusMelds)

        self.usedRules = []
        self.score = None
        oldWon = self.won
        self.__applyRules()
        if self.won != oldWon:
            # if not won after all, this might be a long hand.
            # So we might even have to unapply meld rules and
            # bonus points. Instead just recompute all again.
            # This should only happen with scoring manual games
            # and with scoringtest - normally kajongg would not
            # let you declare an invalid mah jongg
            self.__applyRules()

    @apply
    def won(): # pylint: disable=E0202
        """have we been modified since load or last save?
        The "won" value is set to True when instantiating the hand,
        according to the mMx in the init string. Later on, it may
        only be cleared."""
        def fget(self):
            # pylint: disable=W0212
            return self.__won
        def fset(self, value):
            # pylint: disable=W0212
            # pylint: disable=W0142
            value = bool(value)
            assert not value
            self.__won = value
            self.string = self.string.replace(' M', ' m')
            self.mjStr = self.mjStr.replace(' M', ' m')
        return property(**locals())

    def debug(self, msg, btIndent=None):
        """try to use Game.debug so we get a nice prefix"""
        if self.player:
            self.player.game.debug(msg, btIndent=btIndent)
        else:
            logDebug(msg, btIndent=btIndent)

    def __applyRules(self):
        """find out which rules apply, collect in self.usedRules.
        This may change self.won"""
        self.usedRules = list([UsedRule(rule) for rule in self.computedRules])
        if self.__hasExclusiveRules():
            return
        self.__applyMeldRules()
        self.__applyHandRules()
        if self.__hasExclusiveRules():
            return
        self.score = self.__totalScore()
        if self.won:
            matchingMJRules = self.__maybeMahjongg()
            if not matchingMJRules:
                self.won = False
                self.score = self.__totalScore()
                return
            self.mjRule = matchingMJRules[0]
            self.__setLastMeld() # TODO: only if needed, use a property
            self.usedRules.append(UsedRule(self.mjRule))
            if self.__hasExclusiveRules():
                return
            self.usedRules.extend(self.matchingWinnerRules())
            self.score = self.__totalScore()
        else: # not self.won
            assert self.mjRule is None
            loserRules = self.__matchingRules(self.ruleset.loserRules)
            if loserRules:
                self.usedRules.extend(list(UsedRule(x) for x in loserRules))
                self.score = self.__totalScore()

    def matchingWinnerRules(self):
        """returns a list of matching winner rules"""
        matching = self.__matchingRules(self.ruleset.winnerRules)
        for rule in matching:
            if (self.ruleset.limit and rule.score.limits >= 1) or 'absolute' in rule.options:
                return [UsedRule(rule)]
        return list(UsedRule(x) for x in matching)

    def __hasExclusiveRules(self):
        """if we have one, remove all others"""
        exclusive = list(x for x in self.usedRules if 'absolute' in x.rule.options)
        if exclusive:
            self.usedRules = exclusive
            self.score = self.__totalScore()
            self.won = self.__maybeMahjongg()
        return bool(exclusive)

    def __setLastTile(self):
        """sets lastTile, lastSource, announcements"""
        parts = self.mjStr.split()
        for part in parts:
            if part[0] == 'L':
                part = part[1:]
                if len(part) > 2:
                    self.lastMeld = Meld(part[2:])
                self.lastTile = part[:2]
            elif part[0] == 'M':
                if len(part) > 3:
                    self.lastSource = part[3]
                    if len(part) > 4:
                        self.announcements = part[4:]

    def __setLastMeld(self):
        """sets best last meld"""
        if self.lastTile and self.won and not self.lastMeld:
            if hasattr(self.mjRule.function, 'computeLastMelds'):
                lastMelds = self.mjRule.function.computeLastMelds(self)
                if lastMelds:
                    # syncHandBoard may return nothing
                    self.lastMeld = lastMelds[0] # TODO: use the one with highest score
            if not self.lastMeld:
                self.lastMeld = Meld([self.lastTile])

    def __sub__(self, tiles):
        """returns a copy of self minus tiles. Case of tiles (hidden
        or exposed) is ignored. If the tile is not hidden
        but found in an exposed meld, this meld will be hidden with
        the tile removed from it. Exposed melds of length<3 will also
        be hidden."""
        # pylint: disable=R0912
        # pylint says too many branches
        if not isinstance(tiles, list):
            tiles = list([tiles])
        hidden = 'R' + ''.join(self.inHand)
        # exposed is a deep copy of declaredMelds. If lastMeld is given, it
        # must be first in the list.
        exposed = (Meld(x) for x in self.declaredMelds)
        if self.lastMeld:
            exposed = sorted(exposed, key=lambda x: (x.pairs != self.lastMeld.pairs, meldKey(x)))
        else:
            exposed = sorted(exposed, key=meldKey)
        bonus = sorted(Meld(x) for x in self.bonusMelds)
        for tile in tiles:
            assert isinstance(tile, str) and len(tile) == 2, 'Hand.__sub__:%s' % tiles
            if tile.capitalize() in hidden:
                hidden = hidden.replace(tile.capitalize(), '', 1)
            elif tile[0] in 'fy': # bonus tile
                for idx, meld in enumerate(bonus):
                    if tile == meld.pairs[0]:
                        del bonus[idx]
                        break
            else:
                for idx, meld in enumerate(exposed):
                    if tile.lower() in meld.pairs:
                        del meld.pairs[meld.pairs.index(tile.lower())]
                        del exposed[idx]
                        meld.conceal()
                        hidden += meld.joined
                        break
        for idx, meld in enumerate(exposed):
            if len(meld.pairs) < 3:
                del exposed[idx]
                meld.conceal()
                hidden += meld.joined
        mjStr = self.mjStr
        if self.lastTile in tiles:
            parts = mjStr.split()
            newParts = []
            for idx, part in enumerate(parts):
                if part[0] == 'M':
                    part = 'm' + part[1:]
                    if len(part) > 3 and part[3] == 'k':
                        part = part[:3]
                elif part[0] == 'L':
                    continue
                newParts.append(part)
            mjStr = ' '.join(newParts)
        newString = ' '.join([hidden, meldsContent(exposed), meldsContent(bonus), mjStr])
        return Hand.cached(self, newString, self.computedRules)

    def manualRuleMayApply(self, rule):
        """returns True if rule has selectable() and applies to this hand"""
        if self.won and rule in self.ruleset.loserRules:
            return False
        if not self.won and rule in self.ruleset.winnerRules:
            return False
        return rule.selectable(self) or rule.appliesToHand(self) # needed for activated rules

    def callingHands(self, wanted=1, excludeTile=None, mustBeAvailable=False):
        """the hand is calling if it only needs one tile for mah jongg.
        Returns up to 'wanted' hands which would only need one tile.
        If mustBeAvailable is True, make sure the missing tile might still
        be available.
        """
        result = []
        string = self.string
        if ' x' in string or self.lenOffset:
            return result
        for rule in self.ruleset.mjRules:
            # sort only for reproducibility
            if not hasattr(rule, 'winningTileCandidates'):
                raise Exception('rule %s, code=%s has no winningTileCandidates' % (
                    rule.name, rule.function))
            candidates = sorted(x.capitalize() for x in rule.winningTileCandidates(self))
            for tileName in candidates:
                if excludeTile and tileName == excludeTile.capitalize():
                    continue
                if mustBeAvailable and not self.player.tileAvailable(tileName, self):
                    continue
                hand = self.picking(tileName)
                if hand.won:
                    result.append(hand)
                    if len(result) == wanted:
                        break
            if len(result) == wanted:
                break
        return result

    def __maybeMahjongg(self):
        """check if this is a mah jongg hand.
        Return a sorted list of matching MJ rules, highest
        total first"""
        if not self.won:
            return []
        if self.lenOffset != 1:
            return []
        matchingMJRules = [x for x in self.ruleset.mjRules if x.appliesToHand(self)]
        if self.robbedTile and self.robbedTile.istitle():
            # Millington 58: robbing hidden kong is only allowed for 13 orphans
            matchingMJRules = [x for x in matchingMJRules if 'mayrobhiddenkong' in x.options]
        return sorted(matchingMJRules, key=lambda x: -x.score.total())

    def splitRegex(self, rest):
        """split rest into melds as good as possible"""
        rest = ''.join(rest)
        melds = []
        for rule in self.ruleset.splitRules:
            splits = rule.apply(rest)
            while len(splits) >1:
                for split in splits[:-1]:
                    melds.append(Meld(split))
                rest = splits[-1]
                splits = rule.apply(rest)
            if len(splits) == 0:
                break
        if len(splits) == 1 :
            assert Meld(splits[0]).isValid()   # or the splitRules are wrong
        return melds

    def genVariants(self, original0, maxPairs=1):
        """generates all possible meld variants out of original
        where original is a list of tile values like ['1','1','2']"""
        color = original0[0][0]
        original = [x[1] for x in original0]
        def recurse(cVariants, foundMelds, rest):
            """build the variants recursively"""
            values = set(rest)
            melds = []
            for value in values:
                intValue = int(value)
                if rest.count(value) == 3:
                    melds.append([value] * 3)
                elif rest.count(value) == 2:
                    melds.append([value] * 2)
                if rest.count(str(intValue + 1)) and rest.count(str(intValue + 2)):
                    melds.append([value, str(intValue+1), str(intValue+2)])
            pairsFound = sum(len(x) == 2 for x in foundMelds)
            for meld in (m for m in melds if len(m) !=2 or pairsFound < maxPairs):
                restCopy = rest[:]
                for value in meld:
                    restCopy.remove(value)
                newMelds = foundMelds[:]
                newMelds.append(meld)
                if restCopy:
                    recurse(cVariants, newMelds, restCopy)
                else:
                    for idx, newMeld in enumerate(newMelds):
                        newMelds[idx] = ''.join(color+x for x in newMeld)
                    cVariants.append(' '.join(sorted(newMelds )))
        cVariants = []
        recurse(cVariants, [], original)
        gVariants = []
        for cVariant in set(cVariants):
            melds = [Meld(x) for x in cVariant.split()]
            gVariants.append(melds)
        if not gVariants:
            gVariants.append(self.splitRegex(original0)) # fallback: nothing useful found
        return gVariants

# TODO: get rid of __split, the mjRules should do that if they need it at all
# only __split at end of Hand.__init__, now we do it twice for winning hands
    def __split(self, rest):
        """work hard to always return the variant with the highest Mah Jongg value.
        Adds melds to self.melds.
        only one special mjRule may try to rearrange melds.
        A rest will be rearranged by standard rules."""
        if 'Xy' in rest:
            # hidden tiles of other players:
            self.melds.extend(self.splitRegex(rest))
            return
        arrangements = []
        for mjRule in self.ruleset.mjRules:
            func = mjRule.function
            if func.__class__.__name__ == 'StandardMahJongg':
                stdMJ = func
        if self.mjRule:
            rules = [self.mjRule]
        else:
            rules = self.ruleset.mjRules
        for mjRule in rules:
            func = mjRule.function
            if func != stdMJ and hasattr(func, 'rearrange'):
                if ((self.lenOffset == 1 and func.appliesToHand(self))
                        or (self.lenOffset < 1 and func.shouldTry(self))):
                    melds, pairs = func.rearrange(self, rest[:])
                    if melds:
                        arrangements.append((mjRule, melds, pairs))
        if arrangements:
# TODO: we should know for each arrangement how many tiles for MJ are still needed.
# If len(pairs) == 4, one or up to three might be needed. That would allow for better AI.
# TODO: if hand just completed and we did not win, only try stdmj
            arrangement = sorted(arrangements, key=lambda x: len(x[2]))[0]
            self.melds.extend(arrangement[1])
            self.melds.extend([Meld(x) for x in arrangement[2]])
            assert len(''.join(x.joined for x in self.melds)) == len(self.tileNames) * 2, '%s != %s' % (
                meldsContent(self.melds), self.tileNames)
        else:
            # stdMJ is special because it might build more than one pair
            # the other special hands would put that into the rest
            # if the above TODO is done, stdMJ does not have to be special anymore
            melds, _ = stdMJ.rearrange(self, rest[:])
            self.melds.extend(melds)
            assert len(''.join(x.joined for x in self.melds)) == len(self.tileNames) * 2, '%s != %s' % (
                meldsContent(self.melds), self.tileNames)

    def countMelds(self, key):
        """count melds having key"""
        result = 0
        if isinstance(key, str):
            for meld in self.melds:
                if meld.tileType() in key:
                    result += 1
        else:
            for meld in self.melds:
                if key(meld):
                    result += 1
        return result

    def __matchingRules(self, rules):
        """return all matching rules for this hand"""
        return list(rule for rule in rules if rule.appliesToHand(self))

    def __applyMeldRules(self):
        """apply all rules for single melds"""
        for rule in self.ruleset.meldRules:
            for meld in self.melds + self.bonusMelds:
                if rule.appliesToMeld(self, meld):
                    self.usedRules.append(UsedRule(rule, meld))

    def __applyHandRules(self):
        """apply all hand rules for both winners and losers"""
        for rule in self.ruleset.handRules:
            if rule.appliesToHand(self):
                self.usedRules.append(UsedRule(rule))

    def __totalScore(self):
        """use all used rules to compute the score"""
        pointsTotal = Score(ruleset=self.ruleset)
        maxLimit = 0.0
        maxRule = None
        for usedRule in self.usedRules:
            score = usedRule.rule.score
            if score.limits:
                # we assume that a hand never gets different limits combined
                maxLimit = max(maxLimit, score.limits)
                maxRule = usedRule
            else:
                pointsTotal += score
        if maxLimit:
            if maxLimit >= 1.0 or maxLimit * self.ruleset.limit > pointsTotal.total():
                self.usedRules =  [maxRule]
                return Score(ruleset=self.ruleset, limits=maxLimit)
        return pointsTotal

    def total(self):
        """total points of hand"""
        return self.score.total()

    def __computeLenOffset(self, tileString):
        """lenOffset is <0 for short hand, 0 for correct calling hand, >0 for long hand.
        Of course ignoring bonus tiles.
        if there are no kongs, 13 tiles will return 0"""
        result = len(self.tileNames) - 13
        for split in tileString.split():
            if split[0] != 'R':
                if Meld(split).isKong():
                    result -= 1
        return result

    @staticmethod
    def __computeDragonWindMelds(tileString):
        """returns lists with melds containing all (even single)
        dragons respective winds"""
        dragonMelds = []
        windMelds = []
        for split in tileString.split():
            if split[0] == 'R':
                pairs = Pairs(split[1:])
                for lst, tiles in ((windMelds, elements.wINDS), (dragonMelds, elements.dRAGONS)):
                    for tile in tiles:
                        count = pairs.count(tile)
                        if count:
                            lst.append(Meld([tile] * count))
            elif split[0] in 'dD':
                dragonMelds.append(Meld(split))
            elif split[0] in 'wW':
                windMelds.append(Meld(split))
        return dragonMelds, windMelds

    @staticmethod
    def __separateBonusMelds(tileString):
        """keep them separate. One meld per bonus tile. Others depend on that."""
        result = []
        if 'f' in tileString or 'y' in tileString:
            for pair in Pairs(tileString.replace(' ','').replace('R', '')):
                if pair[0] in 'fy':
                    result.append(Meld(pair))
                    tileString = tileString.replace(pair, '', 1)
        return result, tileString

    def __separateMelds(self, tileString):
        """build a meld list from the hand string"""
        # no matter how the tiles are grouped make a single
        # meld for every bonus tile
        # we need to remove spaces from the hand string first
        # for building only pairs with length 2
        splits = tileString.split()
        rest = ''
        for split in splits:
            if split[0] == 'R':
                rest = split[1:]
            else:
                meld = Meld(split)
                self.melds.append(meld)
                self.declaredMelds.append(meld)
        if rest:
            rest = sorted([rest[x:x+2] for x in range(0, len(rest), 2)])
            self.inHand = rest
            self.__split(rest)
        self.melds = sorted(self.melds, key=meldKey)
        for meld in self.melds:
            if not meld.isValid():
                raise Exception('%s has an invalid meld: %s' % (self.string, meld.joined))
        self.__categorizeMelds()

    def picking(self, tileName):
        """returns a new Hand built from this one plus tileName"""
        assert tileName.istitle(), 'tileName %s should be title:' % tileName
        parts = self.string.split()
        mPart = ''
        rPart = 'R%s' % tileName
        unchanged = []
        for part in parts:
            if part[0] in 'SBCDW':
                rPart += part
            elif part[0] == 'R':
                rPart += part[1:]
            elif part[0].lower() == 'm':
                mPart = part
            elif part[0] == 'L':
                pass
            else:
                unchanged.append(part)
        # combine all parts about hidden tiles plus the new one to one part
        # because something like DrDrS8S9 plus S7 will have to be reordered
        # anyway
        # set the "won" flag M
        parts = unchanged
        parts.extend([rPart, mPart.capitalize(), 'L%s' % tileName])
        return Hand.cached(self, ' '.join(parts))

    def __categorizeMelds(self):
        """categorize: hidden, declared"""
        self.hiddenMelds = []
        self.declaredMelds = []
        for meld in self.melds:
            if meld.state == CONCEALED and not meld.isKong():
                self.hiddenMelds.append(meld)
            else:
                self.declaredMelds.append(meld)

    def explain(self):
        """explain what rules were used for this hand"""
        result = [x.rule.explain() for x in self.usedRules
            if x.rule.score.points]
        result.extend([x.rule.explain() for x in self.usedRules
            if x.rule.score.doubles])
        result.extend([x.rule.explain() for x in self.usedRules
            if not x.rule.score.points and not x.rule.score.doubles])
        if any(x.rule.debug for x in self.usedRules):
            result.append(str(self))
        return result

    def doublesEstimate(self):
        """this is only an estimate because it only uses meldRules and handRules,
        but not things like mjRules, winnerRules, loserRules"""
        result = 0
        for meld in self.dragonMelds + self.windMelds:
            for rule in self.ruleset.doublingMeldRules:
                if rule.appliesToMeld(self, meld):
                    result += rule.score.doubles
        for rule in self.ruleset.doublingHandRules:
            if rule.appliesToHand(self):
                result += rule.score.doubles
        return result

    def __str__(self):
        """hand as a string"""
        return u' '.join([self.sortedMeldsContent, self.mjStr])
