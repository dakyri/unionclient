/*
 * UnionClient.cpp
 *
 *  Created on: Apr 7, 2014
 *      Author: dak
 */


#include <uv.h>

#include "UnionClient.h"

#include "UVEventLoop.h"

UVEventLoop worker;


/** @mainpage Union Client library

@section intro_section Introduction
A C++ library for communicating with the Union chat server, and maintaining a cached model of the server data.

@section prezi_section Prezi Specific Notes
This is a standard union server version 2.1.0, with our own authentication code. The authentication flow is the following:
- The client connects to the server
- Then it joins the special room called 'lobby'
- The join message has two arguments, named policy and signature
- The policy is base64 encoded and it contains a field called oid, which is the name of the room to be connected
- The signature is base64 encoded as well, and it is used to verify the authentication of the client
- If the authentication is successful, then the server makes the client connected to the room with the oid, and makes the client leave the lobby
The signature is a HMAC digest of the policy with a shared secret key. The shared secret is 'secret' in the development server.
 */

/**
 * @class UnionClient UnionClient.h
 * @brief Core class packaging all the managers of various resources including comms
 *
 * Generates NXStatusMessage notifications, identified by Event ids, which correspond to i/o events, but which are aware of the upc protocol
 * and connection regime, as distinct from the notifications of AbstractConnector, which are protocol independent. These are echoed from the
 * UnionBridge:
 * - Event::READY, "", UPC::Status::SUCCESS ... UPC handshake succeeded and we're ready to go
 * - Event::DISCONNECTED, "", UPC::Status::SUCCESS ... we've successfully and totally disconnected from the server
 * - Event::SELECT_CONNECTION, args, status ... specific underlying connection is changed
 * - Event::CONNECT_FAILURE, msg, status ... generic connection failure. status is negative if from an io error, or if +ve it should be one of
 *		- UPC::Status::CONNECT_REFUSED, reason+description ... connection wasn't allowed
 *		- UPC::Status::PROTOCOL_INCOMPATIBLE, version ... connection wasn't allowed
 *		- UPC::Status::SESSION_TERMINATED, msg ... our current session is dramatically cut short
 *		- UPC::Status::SESSION_NOT_FOUND, "" ... our current session is completely out of date
 *		- UPC::Status::SERVER_KILL_CONNECT, msg ... server has cut off the current connection
 *		- UPC::Status::CLIENT_KILL_CONNECT, msg ... server has cut off the current connection
 *		- UPC::Status::NO_VALID_CONNECTION_AVAILABLE, msg ... or we ran out of options for connections
 *		- UPC::Status::CONNECT_TIMEOUT, msg ... or we timed out
 *      - UPC::Status::NO_VALID_CONNECTION_AVAILABLE, msg ... this is the one to give up on. other CONNECT_FAILURE messages give state information and we try to autoreconnect
 * - Event::BEGIN_CONNECT, "", connectionState ... used by the timeout subsystem
 * - Event::CONNECTED, "", connectionState ... signalled when we have established communications, and just before we do UPC handshake
 * - Event::IO_ERROR, msg, status ... signalled when we have a recoverable io error on a working connection, status will be an error code from the io subsystem
 */

UnionClient::UnionClient(AbstractConnector &c)
	: log(defaultLogger)
	, roomManager(clientManager, accountManager, unionBridge, defaultLogger)
	, clientManager(roomManager, accountManager, unionBridge, defaultLogger)
	, accountManager(roomManager, clientManager, unionBridge, defaultLogger)
	, unionBridge(c, roomManager, clientManager, accountManager, defaultLogger)
	, connectionMonitor(clientManager, unionBridge, defaultLogger)
{
	connectionMonitor.SetEventLoop(&worker);
	unionBridge.SetEventLoop(&worker);

	connectionMonitor.AddSelfListeners();

	DEBUG_OUT("adding default listeners ... ");

	readyListener = std::make_shared<CBUPC>([this](EventType t,  StringArgs a, UPCStatus status) {
		connectionMonitor.EnableHeartbeat();
		NotifyListeners(t, a.size()>0?a[0]:"", status);
	});
	disconnectListener = std::make_shared<CBUPC>([this](EventType t,  StringArgs a, UPCStatus status) {
		connectionMonitor.DisableHeartbeat();
		NotifyListeners(t, a.size()>0?a[0]:"", status);
	});
	selectListener = std::make_shared<CBUPC>([this](EventType t,  StringArgs a, UPCStatus status) {
		NotifyListeners(t, a.size()>0?a[0]:"", status);
	});
	cnxFailListener = std::make_shared<CBUPC>([this](EventType t,  StringArgs a, UPCStatus status) {
		// this is a bad idea, as it will supress the reasons for the connection failure
//		if (status == UPC::Status::NO_VALID_CONNECTION_AVAILABLE) {
			NotifyListeners(t, a.size() > 0 ? a[0] : "", status);
//		}
		// if CONNECT_REFUSED there are two params ? either join them or use the second? the first param is the short form, and is the one passed on for now
	});
	echoListener = std::make_shared<CBUPC>([this](EventType t,  StringArgs a, UPCStatus status) {
		NotifyListeners(t, a.size()>0?a[0]:"", status);
	});

	unionBridge.AddUPCListener(Event::READY, readyListener);
	unionBridge.AddUPCListener(Event::DISCONNECTED, disconnectListener);
	unionBridge.AddUPCListener(Event::SELECT_CONNECTION, selectListener);
	unionBridge.AddUPCListener(Event::CONNECT_FAILURE, cnxFailListener);
	unionBridge.AddUPCListener(Event::CONNECTED, echoListener);
	unionBridge.AddUPCListener(Event::BEGIN_CONNECT, echoListener);
	unionBridge.AddUPCListener(Event::IO_ERROR, echoListener);
	defaultLogger.SetLevel(DefaultLogger::kLogNothing);

	worker.StartUVRunner();
}

UnionClient::~UnionClient() {
	DEBUG_OUT("UnionClient::~UnionClient()");
	// TODO XXX FIXME this is the extreme sports cleanup option ... if there are multiple instances of union client, it will kill their connections too.
	// we should only really stop the bits that relate to this client, and stop the whole thread if we are the last remaining client instance
	worker.ForceStopAndClose();
	connectionMonitor.SetEventLoop(nullptr);
	unionBridge.SetEventLoop(nullptr);
}

/**
 * set the given i/o connector
 * @param c an instance of AbstractConnector
 */
void
UnionClient::SetConnector(const AbstractConnector &c)
{
	unionBridge.SetConnector(c);
}

/**
 * passes a connect request onto the i/o connector via the UnionBridge
 */
void
UnionClient::Connect()
{
	unionBridge.Connect();
}

/**
 * passes a disconnect request onto the i/o connector via the UnionBridge
 */
void
UnionClient::Disconnect()
{
	unionBridge.Disconnect();
}

/**
* calls the unionBridge.IsReady() function
*/
bool
UnionClient::IsReady() const
{
	return unionBridge.IsReady();
}

/**
 * @return the RoomManager which will allow detailed operations on rooms
 */
RoomManager&
UnionClient::GetRoomManager()
{
	return roomManager;
}

/**
 * @return the AccountManager which will allow detailed operations on accounts
 */
AccountManager&
UnionClient::GetAccountManager()
{
	return accountManager;
}

/**
 * @return the UnionBridge which will allow detailed operations on the comms subsystem
 */
UnionBridge&
UnionClient::GetUnionBridge()
{
	return unionBridge;
}

/**
 * @return the ClientManager which will allow detailed operations on clients
 */
ClientManager&
UnionClient::GetClientManager()
{
	return clientManager;
}

/**
 * @return the UPC version that the current server connection is using
 */
Version
UnionClient::GetServerUPCVersion() const
{
	return unionBridge.GetUPCVersion();
}

/**
 * sets a connection timeout in the ConnectionMonitor
 * @param timeout in ms
 */
void
UnionClient::SetConnectionTimeout(const int ms)
{
	connectionMonitor.SetConnectionTimeout(ms);
}

/**
 * sets a ready timeout in the ConnectionMonitor
 * @param ms timeout in ms
 */
void
UnionClient::SetReadyTimeout(const int ms)
{
	connectionMonitor.SetReadyTimeout(ms);
}

/**
 * sets a reconnect period in the ConnectionMonitor
 * @param ms min period in ms
 * @param maxMS max period in ms
 * @param delayFirstAttempt whether to delay the first connection attempt
 */
void
UnionClient::SetAutoReconnectFrequency(const int ms, const int maxMS, const bool delayFirstAttempt)
{
	connectionMonitor.SetAutoReconnectFrequency(ms, maxMS, delayFirstAttempt);
}

/**
 * sets the rate at which heartbeat signals are sent by the ConnectionMonitor
 * @param ms min period in ms
 */
void
UnionClient::SetHeartbeatFrequency(const int ms)
{
	connectionMonitor.SetHeartbeatFrequency(ms);
}

/**
* call the connection monitor to disable heartbeats
*/
void
UnionClient::DisableHeartbeat()
{
	connectionMonitor.DisableHeartbeat();
}

/**
 * call the connection monitor to disable heartbeats
 */
void
UnionClient::EnableHeartbeat()
{
	connectionMonitor.EnableHeartbeat();
}

/**
 * @return the connection monitor
 */
ConnectionMonitor&
UnionClient::GetConnectionMonitor()
{
	return connectionMonitor;
}

/**
 * @return a reference to the client object assigned by Union to this library when it connects
 */
ClientRef
UnionClient::Self() const
{
	return clientManager.Self();
}

/**
 * @return the current ILogger
 */
ILogger&
UnionClient::GetLog()
{
	return log;
}

/**
 * @return the current ILogger
 */
DefaultLogger&
UnionClient::GetDefaultLog()
{
	return defaultLogger;
}

/**
 * @param logger a more tailored logger to be used than the default one
 */
void
UnionClient::SetLog(ILogger&logger)
{
	log = logger;
	accountManager.SetLog(logger);
	roomManager.SetLog(logger);
	clientManager.SetLog(logger);
}
