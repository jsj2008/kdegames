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

#include "kgamenetwork.h"
#include "kgamenetwork.moc"
#include "kgamemessage.h"
#include "kgameerror.h"

#include "kmessageserver.h"
#include "kmessageclient.h"
#include "kmessageio.h"
#include <dnssd/publicservice.h>

#include <kdebug.h>

#include <qbuffer.h>
//Added by qt3to4:
#include <QList>


class KGameNetworkPrivate
{
public:
        KGameNetworkPrivate()
        {
                mMessageClient = 0;
                mMessageServer = 0;
                mDisconnectId = 0;
		mService = 0;
        }

public:
        KMessageClient* mMessageClient;
        KMessageServer* mMessageServer;
        quint32 mDisconnectId;  // Stores gameId() over a disconnect process
	DNSSD::PublicService* mService;
	QString mType;
	QString mName;

        int mCookie;
};

// ------------------- NETWORK GAME ------------------------
KGameNetwork::KGameNetwork(int c, QObject* parent) 
    : QObject(parent),
      d( new KGameNetworkPrivate )
{
 d->mCookie = (qint16)c;

 // Init the game as a local game, i.e.
 // create your own KMessageServer and a KMessageClient connected to it.
 setMaster();

 kDebug(11001) << "this=" << this <<", cookie=" << cookie() << "sizeof(this)="<<sizeof(KGameNetwork);
}

KGameNetwork::~KGameNetwork()
{
 kDebug(11001) << "this=" << this;
// Debug();
 delete d->mService;
 delete d;
}

// ----------------------------- status methods
bool KGameNetwork::isNetwork() const
{ return isOfferingConnections() || d->mMessageClient->isNetwork();}

quint32 KGameNetwork::gameId() const
{
  //return d->mMessageClient->id() ;
  // Return stored id in the case of disconnect. In any other
  // case the disconnect id is 0
  if (d->mMessageClient->id()!=0 ) {
    return d->mMessageClient->id() ;
  } else {
    return d->mDisconnectId;
  }
}

int KGameNetwork::cookie() const
{ return d->mCookie; }

bool KGameNetwork::isMaster() const
{ return (d->mMessageServer != 0); }

bool KGameNetwork::isAdmin() const
{ return (d->mMessageClient->isAdmin()); }

KMessageClient* KGameNetwork::messageClient() const
{ return d->mMessageClient; }

KMessageServer* KGameNetwork::messageServer() const
{ return d->mMessageServer; }

// ----------------------- network init
void KGameNetwork::setMaster()
{
 if (!d->mMessageServer) {
   d->mMessageServer = new KMessageServer (cookie(), this);
 } else {
   kWarning(11001) << "Server already running!!";
 }
 if (!d->mMessageClient) {
   d->mMessageClient = new KMessageClient (this);
   connect (d->mMessageClient, SIGNAL(broadcastReceived(const QByteArray&, quint32)),
            this, SLOT(receiveNetworkTransmission(const QByteArray&, quint32)));
   connect (d->mMessageClient, SIGNAL(connectionBroken()),
            this, SIGNAL(signalConnectionBroken()));
   connect (d->mMessageClient, SIGNAL(aboutToDisconnect(quint32)),
            this, SLOT(aboutToLoseConnection(quint32)));
   connect (d->mMessageClient, SIGNAL(connectionBroken()),
            this, SLOT(slotResetConnection()));

   connect (d->mMessageClient, SIGNAL(adminStatusChanged(bool)),
            this, SLOT(slotAdminStatusChanged(bool)));
   connect (d->mMessageClient, SIGNAL(eventClientConnected(quint32)),
            this, SIGNAL(signalClientConnected(quint32)));
   connect (d->mMessageClient, SIGNAL(eventClientDisconnected(quint32, bool)),
            this, SIGNAL(signalClientDisconnected(quint32, bool)));

   // broacast and direct messages are treated equally on receive.
   connect (d->mMessageClient, SIGNAL(forwardReceived(const QByteArray&, quint32, const QList<quint32>&)),
            d->mMessageClient, SIGNAL(broadcastReceived(const QByteArray&, quint32)));

 } else {
   // should be no problem but still has to be tested
   kDebug(11001) << "Client already exists!";
 }
 d->mMessageClient->setServer(d->mMessageServer);
}

void KGameNetwork::setDiscoveryInfo(const QString& type, const QString& name)
{
 kDebug() << type << ":" << name;
 d->mType = type;
 d->mName = name;
 tryPublish();
}

void KGameNetwork::tryPublish()
{
 if (d->mType.isNull() || !isOfferingConnections()) return;
 if (!d->mService) d->mService = new DNSSD::PublicService(d->mName,d->mType,port());
 else {
   if (d->mType!=d->mService->type()) d->mService->setType(d->mType);
   if (d->mName!=d->mService->serviceName()) d->mService->setServiceName(d->mName);
   }
 if (!d->mService->isPublished()) d->mService->publishAsync();
}

void KGameNetwork::tryStopPublishing()
{
 if (d->mService) d->mService->stop();
}

bool KGameNetwork::offerConnections(quint16 port)
{
 kDebug (11001) << "on port" << port;
 if (!isMaster()) {
   setMaster();
 }

 // Make sure this is 0
 d->mDisconnectId = 0;

 // FIXME: This debug message can be removed when the program is working correct.
 if (d->mMessageServer && d->mMessageServer->isOfferingConnections()) {
   kDebug (11001) << "Already running as server! Changing the port now!";
 }

 tryStopPublishing();
 kDebug (11001) << "before Server->initNetwork";
 if (!d->mMessageServer->initNetwork (port)) {
   kError (11001) << "Unable to bind to port" << port << "!";
   // no need to delete - we just cannot listen to the port
//   delete d->mMessageServer;
//   d->mMessageServer = 0;
//   d->mMessageClient->setServer((KMessageServer*)0);
   return false;
 }
 kDebug (11001) << "after Server->initNetwork";
 tryPublish();
 return true;
}

bool KGameNetwork::connectToServer (const QString& host, quint16 port)
{
 if (host.isEmpty()) {
   kError(11001) << "No hostname given";
   return false;
 }
 if (connectToServer(new KMessageSocket (host, port)))
 {
  kDebug(11001) << "connected to" << host << ":" << port;
  return true;
 }
 else
 {
   return false;
 }
}

bool KGameNetwork::connectToServer (KMessageIO *connection)
{
  // Make sure this is 0
  d->mDisconnectId = 0;
  
  // if (!d->mMessageServer) {
 //   // FIXME: What shall we do here? Probably must stop a running game.
 //   kWarning (11001) << "We are already connected to another server!";
 /// }
 
 if (d->mMessageServer) {
   // FIXME: What shall we do here? Probably must stop a running game.
   kWarning(11001) << "we are server but we are trying to connect to another server! "
   << "make sure that all clients connect to that server! "
   << "quitting the local server now...";
   stopServerConnection();
   d->mMessageClient->setServer((KMessageIO*)0);
   delete d->mMessageServer;
   d->mMessageServer = 0;
 }
 
 kDebug(11001) << "    about to set server";
 d->mMessageClient->setServer(connection);
 emit signalAdminStatusChanged(false); // as we delete the connection above isAdmin() is always false now!
 
 // OK: We say that we already have connected, but this isn't so yet!
 // If the connection cannot be established, it will look as being disconnected
 // again ("slotConnectionLost" is called).
 // Shall we differ between these?
 kDebug(11001) << "connected";
 return true;
}

quint16 KGameNetwork::port() const
{
 if (isNetwork()) {
   if (isOfferingConnections()) {
     return d->mMessageServer->serverPort();
   } else {
     return d->mMessageClient->peerPort();
   }
 }
 return 0;
}

QString KGameNetwork::hostName() const
{
 return d->mMessageClient->peerName();
}

bool KGameNetwork::stopServerConnection()
{
 // We still are the Master, we just don't accept further connections!
 tryStopPublishing();
 if (d->mMessageServer) {
   d->mMessageServer->stopNetwork();
   return true;
 }
 return false;
}

bool KGameNetwork::isOfferingConnections() const
{ return (d->mMessageServer && d->mMessageServer->isOfferingConnections()); }

void KGameNetwork::disconnect()
{
 // TODO MH
 kDebug(11001) ;
 stopServerConnection();
 if (d->mMessageServer) {
    QList <quint32> list=d->mMessageServer->clientIDs();
    QList<quint32>::Iterator it;
    for( it = list.begin(); it != list.end(); ++it )
    {
      kDebug(11001) << "Client id=" << (*it);
      KMessageIO *client=d->mMessageServer->findClient(*it);
      if (!client)
      {
        continue;
      }
      kDebug(11001) << "   rtti=" << client->rtti();
      if (client->rtti()==2)
      {
        kDebug(11001) << "DIRECT IO";
      }
      else
      {
        d->mMessageServer->removeClient(client,false);
      }
    }
 }
 else
 {
   kDebug(11001) << "before client->disconnect() id="<<gameId();
   //d->mMessageClient->setServer((KMessageIO*)0);
   kDebug(11001) << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++";
   d->mMessageClient->disconnect();

   kDebug(11001) << "++++++--------------------------------------------+++++";
 }
 //setMaster();
 /*
 if (d->mMessageServer) {
  //delete d->mMessageServer;
  //d->mMessageServer=0;
  server=true;
  kDebug(11001) << "  server true";
  d->mMessageServer->deleteClients();
  kDebug(11001) << "  server deleteClients";
 }
 */
 kDebug(11001) << "DONE";
}

void KGameNetwork::aboutToLoseConnection(quint32 clientID)
{
  kDebug(11001) << "Storing client id of connection "<<clientID;
  d->mDisconnectId = clientID;
}

void KGameNetwork::slotResetConnection()
{
  kDebug(11001) << "Resseting client disconnect id";
  d->mDisconnectId = 0;
}

void KGameNetwork::electAdmin(quint32 clientID)
{
 if (!isAdmin()) {
	kWarning(11001) << "only ADMIN is allowed to call this!";
	return;
 }
 QByteArray buffer;
 QDataStream stream(&buffer,QIODevice::WriteOnly);
 stream << static_cast<quint32>( KMessageServer::REQ_ADMIN_CHANGE );
 stream << clientID;
 d->mMessageClient->sendServerMessage(buffer);
}

void KGameNetwork::setMaxClients(int max)
{
 if (!isAdmin()) {
	kWarning(11001) << "only ADMIN is allowed to call this!";
	return;
 }
 QByteArray buffer;
 QDataStream stream(&buffer,QIODevice::WriteOnly);
 stream << static_cast<quint32>( KMessageServer::REQ_MAX_NUM_CLIENTS );
 stream << (qint32)max;
 d->mMessageClient->sendServerMessage(buffer);
}

void KGameNetwork::lock()
{
 if (messageClient()) {
   messageClient()->lock();
 }
}

void KGameNetwork::unlock()
{
 if (messageClient()) {
   messageClient()->unlock();
 }
}

// --------------------- send messages ---------------------------

bool KGameNetwork::sendSystemMessage(int data, int msgid, quint32 receiver, quint32 sender)
{
 QByteArray buffer;
 QDataStream stream(&buffer,QIODevice::WriteOnly);
 stream << data;
 return sendSystemMessage(buffer,msgid,receiver,sender);
}

bool KGameNetwork::sendSystemMessage(const QString &msg, int msgid, quint32 receiver, quint32 sender)
{
 QByteArray buffer;
 QDataStream stream(&buffer, QIODevice::WriteOnly);
 stream << msg;
 return sendSystemMessage(buffer, msgid, receiver, sender);
}

bool KGameNetwork::sendSystemMessage(const QDataStream &msg, int msgid, quint32 receiver, quint32 sender)
{ return sendSystemMessage(((QBuffer*)msg.device())->buffer(), msgid, receiver, sender); }

bool KGameNetwork::sendSystemMessage(const QByteArray& data, int msgid, quint32 receiver, quint32 sender)
{
 QByteArray buffer;
 QDataStream stream(&buffer,QIODevice::WriteOnly);
 if (!sender) {
   sender = gameId();
 }

 quint32 receiverClient = KGameMessage::rawGameId(receiver); // KGame::gameId()
 int receiverPlayer = KGameMessage::rawPlayerId(receiver); // KPlayer::id()

 KGameMessage::createHeader(stream, sender, receiver, msgid);
 stream.writeRawData(data.data(), data.size());

 /*
 kDebug(11001) << "transmitGameClientMessage msgid=" << msgid << "recv="
                << receiver << "sender=" << sender << "Buffersize="
                << buffer.size();
  */

 if (!d->mMessageClient) {
   // No client created, this should never happen!
   // Having a local game means we have our own
   // KMessageServer and we are the only client.
   kWarning (11001) << "We don't have a client! Should never happen!";
   return false;
 }

 if (receiverClient == 0 || receiverPlayer != 0)
 {
   // if receiverClient == 0 this is a broadcast message. if it is != 0 but
   // receiverPlayer is also != 0 we have to send broadcast anyway, because the
   // KPlayer object on all clients needs to receive the message.
   d->mMessageClient->sendBroadcast(buffer);
 }
 else
 {
   d->mMessageClient->sendForward(buffer, receiverClient);
 }
 return true;
}

bool KGameNetwork::sendMessage(int data, int msgid, quint32 receiver, quint32 sender)
{ return sendSystemMessage(data,msgid+KGameMessage::IdUser,receiver,sender); }

bool KGameNetwork::sendMessage(const QString &msg, int msgid, quint32 receiver, quint32 sender)
{ return sendSystemMessage(msg,msgid+KGameMessage::IdUser,receiver,sender); }

bool KGameNetwork::sendMessage(const QDataStream &msg, int msgid, quint32 receiver, quint32 sender)
{ return sendSystemMessage(msg, msgid+KGameMessage::IdUser, receiver, sender); }

bool KGameNetwork::sendMessage(const QByteArray &msg, int msgid, quint32 receiver, quint32 sender)
{ return sendSystemMessage(msg, msgid+KGameMessage::IdUser, receiver, sender); }

void KGameNetwork::sendError(int error,const QByteArray& message, quint32 receiver, quint32 sender)
{
 QByteArray buffer;
 QDataStream stream(&buffer,QIODevice::WriteOnly);
 stream << (qint32) error;
 stream.writeRawData(message.data(), message.size());
 sendSystemMessage(stream,KGameMessage::IdError,receiver,sender);
}


// ----------------- receive messages from the network
void KGameNetwork::receiveNetworkTransmission(const QByteArray& receiveBuffer, quint32 clientID)
{
 QDataStream stream(receiveBuffer);
 int msgid;
 quint32 sender; // the id of the KGame/KPlayer who sent the message
 quint32 receiver; // the id of the KGame/KPlayer the message is for 
 KGameMessage::extractHeader(stream, sender, receiver, msgid);
// kDebug(11001) << "id=" << msgid << "sender=" << sender << "recv=" << receiver;

 // No broadcast : receiver==0
 // No player isPlayer(receiver)
 // Different game gameId()!=receiver
 if (receiver &&  receiver!=gameId() && !KGameMessage::isPlayer(receiver) )
 {
   // receiver=0 is broadcast or player message
   kDebug(11001) << "Message not meant for us "
            << gameId() << "!=" << receiver << "rawid="
            << KGameMessage::rawGameId(receiver);
   return;
 }
 else if (msgid==KGameMessage::IdError)
 {
   QString text;
   qint32 error;
   stream >> error;
   kDebug(11001) << "Got IdError" << error;
   text = KGameError::errorText(error, stream);
   kDebug(11001) << "Error text:" << text.toLatin1();
   emit signalNetworkErrorMessage((int)error,text);
 }
 else
 {
   networkTransmission(stream, msgid, receiver, sender, clientID);
 }
}

// -------------- slots for the signals of the client
void KGameNetwork::slotAdminStatusChanged(bool isAdmin)
{
 emit signalAdminStatusChanged(isAdmin);

// TODO: I'm pretty sure there are a lot of things that should be done here...
}

void KGameNetwork::Debug()
{
 kDebug(11001) << "------------------- KNETWORKGAME -------------------------";
 kDebug(11001) << "gameId         " << gameId();
 kDebug(11001) << "gameMaster     " << isMaster();
 kDebug(11001) << "gameAdmin      " << isAdmin();
 kDebug(11001) << "---------------------------------------------------";
}

/*
 * vim: et sw=2
 */
