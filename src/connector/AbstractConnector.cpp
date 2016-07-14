/*
 * AbstractConnector.cpp
 *
 *  Created on: Apr 7, 2014
 *      Author: dak
 */

#include "UCLowerHeaders.h"
/**
 * @class AbstractConnector AbstractConnector.h
 * @brief Abstract base class for that will present a set of AbstractConnection objects for I/O.
 *
 * Implementations of this class should be unaware of the details of the underlying protocols implemented by the subclasses
 * of AbstractConnection.
 *
 * Event's spawned:
 * - Event::RECEIVE_DATA .. on successful read
 * - Event::SEND_DATA .. on successful write
 * - Event::CONNECTED .. on successful connection
 * - Event::DISCONNECTED .. on successful disconnect
 * - Event::SELECT_CONNECTION .. on successful renegotiation of connection
 * - Event::IO_ERROR .. on any error on successful connection
 * - Event::CONNECT_FAILURE .. error during connect phase or if connection is lost. status is negative if it is an io error defined elsewhere, else:
 *		- UPC::Status::CONNECT_REFUSED, reason+description ... connection wasn't allowed
 *		- UPC::Status::PROTOCOL_INCOMPATIBLE, version ... connection wasn't allowed
 *		- UPC::Status::SESSION_TERMINATED, msg ... our current session is dramatically cut short
 *		- UPC::Status::SESSION_NOT_FOUND, "" ... our current session is completely out of date
 *		- UPC::Status::SERVER_KILL_CONNECT, msg ... server has cut off the current connection
 *		- UPC::Status::CLIENT_KILL_CONNECT, msg ... server has cut off the current connection

 */

/**
 *
 */
AbstractConnector::AbstractConnector(std::string dfltHost, std::string dfltPort)
	: defaultHost(dfltHost)
	, defaultPort(dfltPort)
{
}

AbstractConnector::~AbstractConnector() {
}

/**
 * @param cr a connection to this connector
 * @param ind at this index
 */

/**
 * @param host the default host to be set
 */
void
AbstractConnector::SetHost(const std::string host)
{
	defaultHost = host;
}

/**
 * @return the default hose
 */
std::string
AbstractConnector::GetHost() const
{
	return defaultHost;
}

void
AbstractConnector::AddConnection(CnxRef cr, int ind)
{
	cr->c = this;
	if (cr->host == "") cr->host = defaultHost;
	if (ind < 0 || ind >= (int)connections.size())
		connections.push_back(cr);
	else
		connections.insert(connections.begin() + ind, cr);
}

CnxRef
AbstractConnector::Connection(int n)
{
	if (n < 0 || n >= connections.size()) return nullptr;
	return connections[n];
}

CnxRef
AbstractConnector::RemoveConnection(int n)
{
	if (n < 0 || n >= connections.size()) return nullptr;
	CnxRef c = connections[n];
	connections.erase(connections.begin()+n);
	return c;
}

int
AbstractConnector::NConnections()
{
	return (int) connections.size();
}

void
AbstractConnector::SetAllConnectionHost(std::string host)
{
	for (auto it : connections) {
		it->SetHost(host);
	}
}

void
AbstractConnector::SetAllConnectionService(std::string service)
{
	for (auto it : connections) {
		it->SetService(service);
	}

}

void
AbstractConnector::NotifyCheckConnectFailure(const CnxRef cr, const std::string msg, const int status)
{
	NotifyListeners(Event::CONNECT_FAILURE, cr, msg, status);
	OnConnectFailure(cr, status);
}

//////////////////////////////////////////////////////////////

/**
 * @class AbstractConnection AbstractConnector.h
 * @brief Abstract base for classes that will implement a particular network connection transport protocol for AbstractConnector
 *
 * This class generates the actual notifications spawned by the AbstractConnector through the Nx* class.
 */

/**
 *
 */
AbstractConnection::AbstractConnection(std::string host, std::string service)
	: host(host)
	, service(service)
	, properties(properties)
	, connectState(ConnectionState::UNKNOWN)
	, c(nullptr){

	Reset();
}

AbstractConnection::~AbstractConnection() {
}

void
AbstractConnection::Reset()
{
	nCnxSucceed = 0;
	nCnxFail = 0;
	nIOError = 0;
	nTimeout = 0;
}

/**
 * @return the current connection state
 */
int
AbstractConnection::GetConnectState() {
	return connectState;
}

/**
 * @return the current connection properties (eg persistent, secure)
 */
ConnectionPropertySet
AbstractConnection::GetProperties() {
	return properties;
}

/**
 * sets the properties for this connection. each connection has a set of connection properties which are used to decide which among a set of connections is the preferred.
 * at the moment, the pre-defined ones are kPersistent ("persistent"), and kSecure ("secure" ie uses an SSL layer). Potentially other properties possible for extensibility, or for
 * a finer grain of selection
 * @param p a set indicationg current properties for this connection
 */
void
AbstractConnection::SetProperties(ConnectionPropertySet p) {
	properties = p;
}

/**
 * @param p a set of ConnectionProperty to match against
 * @return true if all of the given properties are in this connection's properties
 */
bool
AbstractConnection::HasProperties(ConnectionPropertySet p) {
	for (auto cp : p) {
		if (properties.find(cp) == properties.end()) {
			return false;
		}
	}
	return true;
}


/**
 * @param h the host for this connection
 */
void
AbstractConnection::SetHost(const std::string h) const
{
	host = h;
}

/**
 * @param s the service to use on this connection
 */
void
AbstractConnection::SetService(const std::string s) const
{
	service = s;
}

/**
 * @return the current host of this connection
 */
std::string
AbstractConnection::GetHost() const
{
	return host;
}

/**
 * @return the current service offered by this connection
 */
std::string
AbstractConnection::GetService() const
{
	return service;
}

/**
 * wrapper to make a log error note via the parent AbstractConnector
 */
void
AbstractConnection::LogError(const std::string s, ...) const
{
	if (c == nullptr) {
		std::clog << "**** null connector in connect log error " << s;
		return;
	}
	va_list args;
	va_start (args, s);

#ifdef _MSC_VER
	vsnprintf_s(c->logBuf, AbstractConnector::kLogBufLen, s.c_str(), args);
#else
	vsnprintf(c->logBuf, AbstractConnector::kLogBufLen, s.c_str(), args);
#endif
	if (c->log)
		c->log->Error(c->logBuf);
	else
		std::clog << c->logBuf;
	va_end(args);
}

/**
 * wrapper to send a Event::RECEIVE_DATA notification via the parent AbstractConnector
 */
void
AbstractConnection::NxReceive(const std::string data, const int status)
{
	if (c == nullptr) {
		std::clog << "**** null connector in connect NxRecieve";
		return;
	}
	c->NotifyListeners(Event::RECEIVE_DATA, nullptr, data, status);
}

/**
 * wrapper to send a Event::SEND_DATA notification via the parent AbstractConnector
 */
void
AbstractConnection::NxSend(const std::string data, const int status)
{
	if (c == nullptr) {
		std::clog << "**** null connector in connect NxSend";
		return;
	}
	c->NotifyListeners(Event::SEND_DATA, nullptr, data, status);
}

/**
 * wrapper to send a Event::CONNECTED notification via the parent AbstractConnector. and keeps track of the connection count
 */
void
AbstractConnection::NxConnected(const CnxRef cr)
{
	if (c == nullptr) {
		std::clog << "**** null connector in connect NxConnected";
		return;
	}
	nCnxSucceed++;
	c->NotifyListeners(Event::CONNECTED, cr, "", 0);
	c->OnConnectSucceed(cr);
}

/**
 * wrapper to send a Event::DISCONNECTED notification via the parent AbstractConnector
 */
void
AbstractConnection::NxDisconnected(const CnxRef cr)
{
	if (c == nullptr) {
		std::clog << "**** null connector in connect NxDisconnected";
		return;
	}
	c->NotifyListeners(Event::DISCONNECTED, cr, "", 0);
	c->OnDisconnectSucceed(cr);
}


/**
 * wrapper to send a Event::IO_ERROR notification via the parent AbstractConnector. also keeps track of the io error count.
 */
void
AbstractConnection::NxIOError(const std::string msg, const int status)
{
	if (c == nullptr) {
		std::clog << "**** null connector in connect NxIOError";
		return;
	}
	nIOError++;
	c->NotifyListeners(Event::IO_ERROR, nullptr, msg, 0);
}

/**
 * wrapper to send a Event::CONNECT_FAILURE notification via the parent AbstractConnector. called when we fail to connect, or when a connection breaks
 * unexpectedly. increments the connection fail count
 */
void
AbstractConnection::NxConnectFailure(const CnxRef cr, const std::string msg, const int status)
{
	if (c == nullptr) {
		std::clog << "**** null connector in connect NxConnectFailure";
		return;
	}
	nCnxFail++;
	c->NotifyListeners(Event::CONNECT_FAILURE, cr, msg, status);
	c->OnConnectFailure(cr, status);
}


/**
 * wrapper to send a Event::SERVER_KILL_CONNECT notification via the parent AbstractConnector. a clean server hangup doesn't typically imply the connection has
 * failed ... just that it needs to be renegotiated ... so the connection fail count not incremented
 */
void
AbstractConnection::NxServerHangup(const CnxRef cr, const std::string msg, const int status)
{
	if (c == nullptr) {
		std::clog << "**** null connector in connect NxConnectFailure";
		return;
	}
	c->NotifyListeners(Event::CONNECT_FAILURE, cr, msg, UPC::Status::SERVER_KILL_CONNECT);
	c->OnConnectFailure(cr, status);
}

/**
 * @param lp an ILogger module to use for messages
 */
void
AbstractConnector::SetLog(ILogger *lp){
	log = lp;
}
