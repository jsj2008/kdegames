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

#include "kgameio.h"
#include "kgameio.moc"
#include "kgame.h"
#include "kplayer.h"
#include "kgamemessage.h"
#include "kmessageio.h"

#include <kdebug.h>

#include <QWidget>
#include <qbuffer.h>
#include <QTimer>
//Added by qt3to4:
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>

#include <stdlib.h>

class KGameIOPrivate
{
public:
  KGameIOPrivate()
    : mPlayer(0)
  {
  }

  KPlayer *mPlayer;
};


// ----------------------- Generic IO -------------------------
KGameIO::KGameIO()
  : d(new KGameIOPrivate)
{
  kDebug(11001) << ": this=" << this << ", sizeof(this)" << sizeof(KGameIO);
}

KGameIO::KGameIO(KPlayer* player)
  : d(new KGameIOPrivate)
{
  kDebug(11001) << ": this=" << this << ", sizeof(this)" << sizeof(KGameIO);
  if (player)
  {
    player->addGameIO(this);
  }
}

KGameIO::~KGameIO()
{
  kDebug(11001) << ": this=" << this;
  // unregister ourselves
  if (player())
  {
    player()->removeGameIO(this, false);
  }
  delete d;
}

KPlayer* KGameIO::player() const
{
  return d->mPlayer;
}

void KGameIO::setPlayer(KPlayer *p)
{
  d->mPlayer = p;
}

void KGameIO::initIO(KPlayer *p)
{
  setPlayer(p);
}

void KGameIO::notifyTurn(bool b)
{
  if (!player())
  {
    kWarning(11001) << ": player() is NULL";
    return;
  }
  bool sendit=false;
  QByteArray buffer;
  QDataStream stream(&buffer, QIODevice::WriteOnly);
  emit signalPrepareTurn(stream, b, this, &sendit);
  if (sendit)
  {
    QDataStream ostream(buffer);
    quint32 sender = player()->id();  // force correct sender
    kDebug(11001) << "Prepare turn sendInput";
    sendInput(ostream, true, sender);
  }
}

KGame* KGameIO::game() const
{
  if (!player())
  {
    return 0;
  }
  return player()->game();
}

bool KGameIO::sendInput(QDataStream& s, bool transmit, quint32 sender)
{
  if (!player())
  {
    return false;
  }
  return player()->forwardInput(s, transmit, sender);
}

void KGameIO::Debug()
{
  kDebug(11001) << "------------------- KGAMEINPUT --------------------";
  kDebug(11001) << "this:    " << this;
  kDebug(11001) << "rtti :   " << rtti();
  kDebug(11001) << "Player:  " << player();
  kDebug(11001) << "---------------------------------------------------";
}

// ----------------------- Key IO ---------------------------
class KGameKeyIOPrivate
{
};

KGameKeyIO::KGameKeyIO(QWidget *parent) 
   : KGameIO(), d(0)
{
  if (parent)
  {
    kDebug(11001) << "Key Event filter installed";
    parent->installEventFilter(this);
  }
}

KGameKeyIO::~KGameKeyIO()
{
  if (parent())
  {
    parent()->removeEventFilter(this);
  }
  delete d;
}

int KGameKeyIO::rtti() const { return KeyIO; }

bool KGameKeyIO::eventFilter( QObject *o, QEvent *e )
{
  if (!player())
  {
    return false;
  }

  // key press/release
  if ( e->type() == QEvent::KeyPress ||
       e->type() == QEvent::KeyRelease )
  {
     QKeyEvent *k = (QKeyEvent*)e;
  //   kDebug(11001) << "KGameKeyIO" << this << "key press/release" <<  k->key();
     QByteArray buffer;
     QDataStream stream(&buffer,QIODevice::WriteOnly);
     bool eatevent=false;
     emit signalKeyEvent(this,stream,k,&eatevent);
     QDataStream msg(buffer);

     if (eatevent && sendInput(msg))
     {
       return eatevent;
     }
     return false; // do not eat otherwise
  }
  return QObject::eventFilter( o, e );    // standard event processing
}


// ----------------------- Mouse IO ---------------------------
class KGameMouseIOPrivate
{
};

KGameMouseIO::KGameMouseIO(QWidget *parent,bool trackmouse) 
   : KGameIO(), d(0)
{
  if (parent)
  {
    kDebug(11001) << "Mouse Event filter installed tracking=" << trackmouse;
    parent->installEventFilter(this);
    parent->setMouseTracking(trackmouse);
  }
}

KGameMouseIO::KGameMouseIO(QGraphicsScene *parent,bool /*trackmouse*/) 
   : KGameIO(), d(0)
{
  if (parent)
  {
    //kDebug(11001) << "Mouse Event filter installed tracking=" << trackmouse;
    parent->installEventFilter(this);
//     parent->setMouseTracking(trackmouse);
  }
}

KGameMouseIO::~KGameMouseIO()
{
  if (parent())
  {
    parent()->removeEventFilter(this);
  }
  delete d;
}

int KGameMouseIO::rtti() const
{
  return MouseIO;
}

void KGameMouseIO::setMouseTracking(bool b)
{
  if (parent())
  {
    ((QWidget*)parent())->setMouseTracking(b);
  }
}

bool KGameMouseIO::eventFilter( QObject *o, QEvent *e )
{
  if (!player())
  {
    return false;
  }
  //kDebug(11001) << "KGameMouseIO" << this << " " << e->type();

  // mouse action
  if ( e->type() == QEvent::MouseButtonPress ||
       e->type() == QEvent::MouseButtonRelease ||
       e->type() == QEvent::MouseButtonDblClick ||
       e->type() == QEvent::Wheel ||
       e->type() == QEvent::MouseMove ||
       e->type() == QEvent::GraphicsSceneMousePress ||
       e->type() == QEvent::GraphicsSceneMouseRelease ||
       e->type() == QEvent::GraphicsSceneMouseDoubleClick ||
       e->type() == QEvent::GraphicsSceneWheel ||
       e->type() == QEvent::GraphicsSceneMouseMove
       )
  {
     QMouseEvent *k = (QMouseEvent*)e;
     // kDebug(11001) << "KGameMouseIO" << this;
     QByteArray buffer;
     QDataStream stream(&buffer,QIODevice::WriteOnly);
     bool eatevent=false;
     emit signalMouseEvent(this,stream,k,&eatevent);
//     kDebug(11001) << "################# eatevent=" << eatevent;
     QDataStream msg(buffer);
     if (eatevent && sendInput(msg))
     {
       return eatevent;
     }
     return false; // do not eat otherwise
  }
  return QObject::eventFilter( o, e );    // standard event processing
}


// ----------------------- KGameProcesPrivate ---------------------------
class KGameProcessIOPrivate
{
public:
  KGameProcessIOPrivate()
  {
    //mMessageServer = 0;
    //mMessageClient = 0;
    mProcessIO=0;
  }
  //KMessageServer *mMessageServer;
  //KMessageClient *mMessageClient;
  KMessageProcess *mProcessIO;
};

// ----------------------- Process IO ---------------------------
KGameProcessIO::KGameProcessIO(const QString& name) 
   : KGameIO(), d(new KGameProcessIOPrivate)
{
  kDebug(11001) << ": this=" << this << ", sizeof(this)=" << sizeof(KGameProcessIO);

  //kDebug(11001) << "================= KMEssageServer ====================";
  //d->mMessageServer=new KMessageServer(0,this);
  //kDebug(11001) << "================= KMEssageClient ====================";
  //d->mMessageClient=new KMessageClient(this);
  kDebug(11001) << "================= KMEssageProcessIO ====================";
  d->mProcessIO=new KMessageProcess(this,name);
  kDebug(11001) << "================= KMEssage Add client ====================";
  //d->mMessageServer->addClient(d->mProcessIO);
  //kDebug(11001) << "================= KMEssage SetSErver ====================";
  //d->mMessageClient->setServer(d->mMessageServer);
  kDebug(11001) << "================= KMEssage: Connect ====================";
  //connect(d->mMessageClient, SIGNAL(broadcastReceived(const QByteArray&, quint32)),
  //        this, SLOT(clientMessage(const QByteArray&, quint32)));
  //connect(d->mMessageClient, SIGNAL(forwardReceived(const QByteArray&, quint32, const QValueList <quint32> &)),
  //        this, SLOT(clientMessage(const QByteArray&, quint32, const QValueList <quint32> &)));
  connect(d->mProcessIO, SIGNAL(received(const QByteArray&)),
          this, SLOT(receivedMessage(const QByteArray&)));
  // Relay signal
  connect(d->mProcessIO, SIGNAL(signalReceivedStderr(QString)),
          this, SIGNAL(signalReceivedStderr(QString)));
  //kDebug(11001) << "Our client is id="<<d->mMessageClient->id();
}

KGameProcessIO::~KGameProcessIO()
{
  kDebug(11001) << ": this=" << this;
  kDebug(11001) << "player="<<player();
  if (player())
  {
    player()->removeGameIO(this,false);
  }
  if (d->mProcessIO)
  {
    delete d->mProcessIO;
    d->mProcessIO=0;
  }
  delete d;
}

int KGameProcessIO::rtti() const
{
  return ProcessIO;
}

void KGameProcessIO::initIO(KPlayer *p)
{
  KGameIO::initIO(p);
  // Send 'hello' to process
  QByteArray buffer;
  QDataStream stream(&buffer, QIODevice::WriteOnly);
  qint16 id = p->userId();
  stream << id;

  bool sendit=true;
  if (p)
  {
    emit signalIOAdded(this,stream,p,&sendit);
    if (sendit )
    {
      quint32 sender = p->id();
      kDebug(11001) <<  "Sending IOAdded to process player !!!!!!!!!!!!!! ";
      sendSystemMessage(stream, KGameMessage::IdIOAdded, 0, sender);
    }
  }
}

void KGameProcessIO::notifyTurn(bool b)
{
  if (!player())
  {
    kWarning(11001) << ": player() is NULL";
    return;
  }
  bool sendit=true;
  QByteArray buffer;
  QDataStream stream(&buffer,QIODevice::WriteOnly);
  stream << (qint8)b;
  emit signalPrepareTurn(stream,b,this,&sendit);
  if (sendit)
  {
    quint32 sender=player()->id();
    kDebug(11001) <<  "Sending Turn to process player !!!!!!!!!!!!!! ";
    sendSystemMessage(stream, KGameMessage::IdTurn, 0, sender);
  }
}

void KGameProcessIO::sendSystemMessage(QDataStream &stream,int msgid, quint32 receiver, quint32 sender)
{
  sendAllMessages(stream, msgid, receiver, sender, false);
}

void KGameProcessIO::sendMessage(QDataStream &stream,int msgid, quint32 receiver, quint32 sender)
{
  sendAllMessages(stream, msgid, receiver, sender, true);
}

void KGameProcessIO::sendAllMessages(QDataStream &stream,int msgid, quint32 receiver, quint32 sender, bool usermsg)
{
  kDebug(11001) << "==============>  KGameProcessIO::sendMessage (usermsg="<<usermsg<<")";
  // if (!player()) return ;
  //if (!player()->isActive()) return ;

  if (usermsg)
  {
    msgid+=KGameMessage::IdUser;
  }

  kDebug(11001) << "=============* ProcessIO (" << msgid << "," << receiver << "," << sender << ") ===========";

  QByteArray buffer;
  QDataStream ostream(&buffer,QIODevice::WriteOnly);
  QBuffer *device=(QBuffer *)stream.device();
  QByteArray data=device->buffer();;

  KGameMessage::createHeader(ostream,sender,receiver,msgid);
  // ostream.writeRawBytes(data.data()+device->at(),data.size()-device->at());
  ostream.writeRawData(data.data(),data.size());
  kDebug(11001) << "   Adding user data from pos="<< device->pos() <<" amount=" << data.size() << "byte";
  //if (d->mMessageClient) d->mMessageClient->sendBroadcast(buffer);
  if (d->mProcessIO)
  {
    d->mProcessIO->send(buffer);
  }
}

//void KGameProcessIO::clientMessage(const QByteArray& receiveBuffer, quint32 clientID, const QValueList <quint32> &recv)
void KGameProcessIO::receivedMessage(const QByteArray& receiveBuffer)
{
  QDataStream stream(receiveBuffer);
  int msgid;
  quint32 sender;
  quint32 receiver;
  KGameMessage::extractHeader(stream,sender,receiver,msgid);

  kDebug(11001) << "************* Got process message sender =" << sender 
          << "receiver=" << receiver << "   msgid=" << msgid;


  // Cut out the header part...to not confuse network code
  QBuffer *buf=(QBuffer *)stream.device();
  QByteArray newbuffer;
  newbuffer = QByteArray::fromRawData(buf->buffer().data()+buf->pos(),buf->size()-buf->pos());
  QDataStream ostream(newbuffer);
  kDebug(11001) << "Newbuffer size=" << newbuffer.size();

// This is a dummy message which allows us the process to talk with its owner
  if (msgid==KGameMessage::IdProcessQuery)
  {
    emit signalProcessQuery(ostream,this);
  }
  else if (player())
  {
    sender = player()->id();  // force correct sender
    if (msgid==KGameMessage::IdPlayerInput) 
    {
      sendInput(ostream,true,sender);
    }
    else
    {
      player()->forwardMessage(ostream,msgid,receiver,sender);
    }
  }
  else
  {
    kDebug(11001) << ": Got message from process but no player defined!";
  }
  newbuffer.clear();
}


// ----------------------- Computer IO --------------------------
class KGameComputerIOPrivate
{
//TODO: maybe these should be KGameProperties!!
public:
  KGameComputerIOPrivate()
  {
    mAdvanceCounter = 0;
    mReactionPeriod = 0;

    mPauseCounter = 0;

    mAdvanceTimer = 0;
  }
  int mAdvanceCounter;
  int mReactionPeriod;

  int mPauseCounter;

  QTimer* mAdvanceTimer;
};

KGameComputerIO::KGameComputerIO()
    : KGameIO(), d(new KGameComputerIOPrivate)
{
}

KGameComputerIO::KGameComputerIO(KPlayer *p)
    : KGameIO(p), d(new KGameComputerIOPrivate)
{
}

KGameComputerIO::~KGameComputerIO()
{
  if (d->mAdvanceTimer)
  {
    delete d->mAdvanceTimer;
  }
  delete d;
}

int KGameComputerIO::rtti() const
{
  return ComputerIO;
}

void KGameComputerIO::setReactionPeriod(int calls)
{
 d->mReactionPeriod = calls;
}

int KGameComputerIO::reactionPeriod() const
{
  return d->mReactionPeriod;
}

void KGameComputerIO::setAdvancePeriod(int ms)
{
  stopAdvancePeriod();
  d->mAdvanceTimer = new QTimer(this);
  connect(d->mAdvanceTimer, SIGNAL(timeout()), this, SLOT(advance()));
  d->mAdvanceTimer->start(ms);
}

void KGameComputerIO::stopAdvancePeriod()
{
  if (d->mAdvanceTimer)
  {
    d->mAdvanceTimer->stop();
    delete d->mAdvanceTimer;
  }
}

void KGameComputerIO::pause(int calls)
{
  d->mPauseCounter = calls;
}

void KGameComputerIO::unpause()
{
  pause(0);
}

void KGameComputerIO::advance()
{
  if (d->mPauseCounter > 0)
  {
    d->mPauseCounter--;
    return;
  }
  else if (d->mPauseCounter < 0)
  {
    return;
  }
  d->mAdvanceCounter++;
  if (d->mAdvanceCounter >= d->mReactionPeriod)
  {
    d->mAdvanceCounter = 0;
    reaction();
  }
}

void KGameComputerIO::reaction()
{
  emit signalReaction();
}


