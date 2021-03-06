# -*- coding: utf-8 -*-

"""
Copyright (C) 2008-2012 Wolfgang Rohdewald <wolfgang@rohdewald.de>

The function isAlive() is taken from the book
"Rapid GUI Programming with Python and Qt"
by Mark Summerfield.

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

from __future__ import print_function
import logging, socket, logging.handlers, traceback, os, datetime, shutil
import time

from locale import getpreferredencoding
from sys import stdout
try:
    STDOUTENCODING = stdout.encoding
except AttributeError:
    STDOUTENCODING = None
if not STDOUTENCODING:
    STDOUTENCODING = getpreferredencoding()

SERVERMARK = '&&SERVER&&'

# util must not import twisted or we need to change kajongg.py

import sip

import common

try:
    from kde import i18n, i18nc
    HAVE_KDE = True
except ImportError:
    # a server might not have KDE4
    # pylint thinks those are already defined
    # pylint: disable=E0102
    HAVE_KDE = False
    def i18n(englishIn, *args):
        """dummy for server"""
        result = englishIn
        if '%' in result:
            for idx, arg in enumerate(args):
                arg = xToUtf8(arg)
                result = result.replace('%' + str(idx+1), unicode(arg))
        if '%' in result:
            for ignore in ['numid', 'filename']:
                result = result.replace('<%s>' % ignore, '')
                result = result.replace('</%s>' % ignore, '')
        return result
    def i18nc(dummyContext, englishIn, *args):
        """dummy for server"""
        return i18n(englishIn, *args)

if not common.InternalParameters.isServer:
    from kde import KMessageBox, KGlobal
else:
    class KMessageBox(object):
        """dummy for server, just show on stdout"""
        @staticmethod
        def sorry(dummy, *args):
            """just output to stdout"""
            kprint(*args)
        @staticmethod
        def information(dummy, *args):
            """just output to stdout"""
            kprint(*args)

def appdataDir():
    """the per user directory with kajongg application information like the database"""
    if common.InternalParameters.isServer:
        # the server might or might not have KDE installed, so to be on
        # the safe side we use our own .kajonggserver directory
        # the following code moves an existing kajonggserver.db to .kajonggserver
        # but only if .kajonggserver does not yet exist
        kdehome = os.environ.get('KDEHOME', '~/.kde')
        oldPath = os.path.expanduser(kdehome + '/share/apps/kajongg/kajonggserver.db')
        if not os.path.exists(oldPath):
            oldPath = os.path.expanduser('~/.kde4/share/apps/kajongg/kajonggserver.db')
        newPath = os.path.expanduser('~/.kajonggserver/')
        if os.path.exists(oldPath) and not os.path.exists(newPath):
            # upgrading an old kajonggserver installation
            os.makedirs(newPath)
            shutil.move(oldPath, newPath)
            logInfo('moved %s to %s' % (oldPath,  newPath))
        if not os.path.exists(newPath):
            os.makedirs(newPath)
        return newPath
    else:
        result = os.path.dirname(str(KGlobal.dirs().locateLocal("appdata", ""))) + '/'
        return result

def cacheDir():
    """the cache directory for this user"""
    if common.InternalParameters.isServer:
        result = os.path.join(appdataDir(), 'cache')
    else:
        result = os.path.dirname(str(KGlobal.dirs().locateLocal("cache", "")))
        result = os.path.join(result, 'kajongg')
    if not os.path.exists(result):
        os.makedirs(result)
    return result

ENGLISHDICT = {}

LOGGER = None

def english(i18nstring):
    """translate back from local language"""
    return ENGLISHDICT.get(i18nstring, i18nstring)

def translateServerMessage(msg):
    """because a PB exception can not pass a list of arguments, the server
    encodes them into one string using SERVERMARK as separator. That
    string is always english. Here we unpack and translate it into the
    client language."""
    if msg.find(SERVERMARK) >= 0:
        return m18n(*tuple(msg.split(SERVERMARK)[1:-1]))
    return msg

def stack(msg, limit=6):
    """returns a list of lines with msg as prefix"""
    result = []
    for idx, values in enumerate(traceback.extract_stack(limit=limit+2)[:-2]):
        fileName, line, function, txt = values
        result.append('%2d: %s %s/%d %s: %s' % (idx, msg, os.path.splitext(os.path.basename(fileName))[0],
                                line, function, txt))
    return result

def initLog(logName):
    """init the loggers"""
    global LOGGER # pylint: disable=W0603
    LOGGER = logging.getLogger(logName)
    try:
        handler = logging.handlers.SysLogHandler('/dev/log')
    except (AttributeError, socket.error):
        handler = logging.handlers.RotatingFileHandler('kajongg.log', maxBytes=100000000, backupCount=10)
    LOGGER.addHandler(handler)
    LOGGER.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(name)s: %(levelname)s %(message)s")
    handler.setFormatter(formatter)

def __logUnicodeMessage(prio, msg):
    """if we can encode the unicode msg to ascii, do so.
    Otherwise convert the unicode object into an utf-8 encoded
    str object.
    The logger module would log the unicode object with the
    marker feff at the beginning of every message, we do not want that."""
    msg = msg.encode(getpreferredencoding(), 'ignore')[:4000]
    kprint(msg)
    LOGGER.log(prio, msg)

def xToUtf8(msg, args=None):
    """makes sure msg and all args are utf-8"""
    if isinstance(msg, unicode):
        msg = msg.encode('utf-8')
    if args:
        args = args[:]
        for arg in args:
            if isinstance(arg, unicode):
                arg = arg.encode('utf-8')
    return msg, args

def logMessage(msg, prio, showDialog, showStack=False, withGamePrefix=True):
    """writes info message to log and to stdout"""
    if isinstance(msg, Exception):
        msg = ' '.join(unicode(x.decode(getpreferredencoding()) \
             if isinstance(x, str) else unicode(x)) for x in msg.args if x is not None)
    if isinstance(msg, str):
        msg = unicode(msg, 'utf-8')
    elif not isinstance(msg, unicode):
        msg = unicode(str(msg), 'utf-8')
    msg = translateServerMessage(msg)
    logMsg = msg
    if withGamePrefix and common.InternalParameters.logPrefix:
        logMsg = '%s: %s' % (common.InternalParameters.logPrefix, msg)
    __logUnicodeMessage(prio, logMsg)
    if showStack:
        for line in traceback.format_stack()[2:-3]:
            if not 'logException' in line:
                __logUnicodeMessage(prio, '  ' + line.strip())
    if common.InternalParameters.hasGUI and showDialog:
        if prio == logging.INFO:
            KMessageBox.information(None, msg)
        else:
            KMessageBox.sorry(None, msg)

def logInfo(msg, showDialog=False, withGamePrefix=True):
    """log an info message"""
    logMessage(msg, logging.INFO, showDialog, withGamePrefix=withGamePrefix)

def logError(msg, withGamePrefix=True):
    """log an error message"""
    logMessage(msg, logging.ERROR, True, showStack=True, withGamePrefix=withGamePrefix)

def logDebug(msg, showStack=False, withGamePrefix=True, btIndent=None):
    """log this message and show it on stdout
    if btIndent is set, message is indented by depth(backtrace)-btIndent"""
    if btIndent:
        depth = traceback.extract_stack()
        msg = ' ' * (len(depth) - btIndent) + msg
    logMessage(msg, logging.DEBUG, False, showStack=showStack, withGamePrefix=withGamePrefix)

def logWarning(msg, withGamePrefix=True):
    """log this message and show it on stdout"""
    logMessage(msg, logging.WARNING, True, withGamePrefix=withGamePrefix)

def logException(exception, withGamePrefix=True):
    """logs error message and re-raises exception"""
    logMessage(exception, logging.ERROR, True, showStack=True, withGamePrefix=withGamePrefix)
    if isinstance(exception, (str, unicode)):
        msg = exception.encode('utf-8', 'replace')
        exception = Exception(msg)
    raise exception

def m18n(englishText, *args):
    """wrapper around i18n converting QString into a Python unicode string"""
    englishText, args = xToUtf8(englishText, args)
    result = unicode(i18n(englishText, *args))
    if not args:
        ENGLISHDICT[result] = englishText
    return result

def m18nc(context, englishText, *args):
    """wrapper around i18nc converting QString into a Python unicode string"""
    englishText, args = xToUtf8(englishText, args)
    result = unicode(i18nc(context, englishText, *args))
    if not args:
        ENGLISHDICT[result] = englishText
    return result

def m18nE(englishText):
    """use this if you want to get the english text right now but still have the string translated"""
    return englishText

def m18ncE(dummyContext, englishText):
    """use this if you want to get the english text right now but still have the string translated"""
    return englishText

def isAlive(qobj):
    """is the underlying C++ object still valid?"""
    if qobj is None:
        return False
    try:
        sip.unwrapinstance(qobj)
    except RuntimeError:
        return False
    else:
        return True

def socketName():
    """client and server process use this socket to talk to each other"""
    serverDir = os.path.expanduser('~/.kajonggserver')
    if not os.path.exists(serverDir):
        appdataDir() # allocate the directory and possibly move old databases there
    if common.InternalParameters.socket:
        return common.InternalParameters.socket
    else:
        return os.path.join(serverDir, 'socket')

def which(program):
    """returns the full path for the binary or None"""
    for path in os.environ['PATH'].split(os.pathsep):
        fullName = os.path.join(path, program)
        if os.path.exists(fullName):
            return fullName

def removeIfExists(filename):
    """remove file if it exists. Returns True if it existed"""
    exists = os.path.exists(filename)
    if exists:
        os.remove(filename)
    return exists

def uniqueList(seq):
    """makes list content unique, keeping only the first occurrence"""
    seen = set()
    seen_add = seen.add
    return [ x for x in seq if x not in seen and not seen_add(x)]

import gc

def _getr(slist, olist, seen):
    """Recursively expand slist's objects into olist, using seen to track
    already processed objects."""
    for elment in slist:
        if id(elment) in seen:
            continue
        seen[id(elment)] = None
        olist.append(elment)
        tlist = gc.get_referents(elment)
        if tlist:
            _getr(tlist, olist, seen)

# The public function.
def get_all_objects():
    """Return a list of all live Python objects, not including the
    list itself. May use this in Duration for showing where
    objects are leaking"""
    gc.collect()
    gcl = gc.get_objects()
    olist = []
    seen = {}
    # Just in case:
    seen[id(gcl)] = None
    seen[id(olist)] = None
    seen[id(seen)] = None
    # _getr does the real work.
    _getr(gcl, olist, seen)
    return olist

def kprint(*args, **kwargs):
    """a wrapper around print, always encoding unicode to something sensible"""
    newArgs = []
    for arg in args:
        try:
            arg = arg.decode('utf-8')
        except BaseException:
            arg = repr(arg)
        arg = arg.encode(STDOUTENCODING, 'ignore')
        newArgs.append(arg)
    # we need * magic: pylint: disable=W0142
    try:
        print(*newArgs, sep=kwargs.get('sep', ' '), end=kwargs.get('end', '\n'), file=kwargs.get('file'))
    except IOError, exception:
        # very big konsole, busy system: sometimes Python says
        # resource temporarily not available
        time.sleep(0.1)
        print(exception)
        print(*newArgs, sep=kwargs.get('sep', ' '), end=kwargs.get('end', '\n'), file=kwargs.get('file'))

class Duration(object):
    """a helper class for checking code execution duration"""
    def __init__(self, name, threshold=None, bug=False):
        """name describes where in the source we are checking
        threshold in seconds: do not warn below
        if bug is True, throw an exception if threshold is exceeded"""
        self.name = name
        self.threshold = threshold or 1.0
        self.bug = bug
        self.__start = datetime.datetime.now()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, trback):
        """now check time passed"""
        diff = datetime.datetime.now() - self.__start
        if diff > datetime.timedelta(seconds=self.threshold):
            msg = '%s took %d.%06d seconds' % (self.name, diff.seconds, diff.microseconds)
            if self.bug:
                logException(msg)
            else:
                logDebug(msg)
