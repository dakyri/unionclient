/*
 * StandardConnector.cpp
 *
 *  Created on: May 2, 2014
 *      Author: dak
 */
#include "UCLowerHeaders.h"
#include "connector/UVConnection.h"

/**
 * @class StandardConnector StandardConnector.h
 * @brief default base class for managing and selecting connections, depending on circumstances, properties and connectivity.
 * this is the main base class providing control and connectivity with a set of AbstractConnection objects.
 *
 * Events spawned (NXConnection)
 *     - CONNECT_FAILURE if there are no available connections
 *           - status of UPC::Status::NO_VALID_CONNECTION_AVAILABLE if all options are exhausted, else a protocol specific error number
 *     - SELECT_CONNECTION when the current valid connection changes from the previous
 */

/**
 *
 */
StandardConnector::StandardConnector(std::string dfltHost, std::string dfltPort)
	: AbstractConnector(dfltHost, dfltPort) {
	SetConnectionFailLimit(-1);
	activeConnection = nullptr;
}

/**
 *
 */
StandardConnector::~StandardConnector() {
	DEBUG_OUT("StandardConnector::~StandardConnector()");
	for (auto it: connections) {
		delete it;
	}
	DEBUG_OUT("StandardConnector::~StandardConnector() done");
}

/**
 * 
 */
void
StandardConnector::SetConnectionPriorities(std::vector<ConnectionPropertySet> pri)
{
	connectionPriorities = pri;
}

/**
 * mainly for internal and informational purposes. hint! don't poll this to check that the connection is ready. listen for the events.
 */
bool
StandardConnector::IsReady() const
{
	if (activeConnection != nullptr) {
		return activeConnection->GetConnectState() == ConnectionState::READY;
	}
	return false;
}


/**
 * connects this connector making appropriate searches for a compatible connection, searching for connections with appropriate properties
 */
int
StandardConnector::Connect()
{
	if (connections.size() == 0) {
		NotifyListeners(Event::CONNECT_FAILURE, nullptr, "No Connections Available", -1);
		Disconnect();
		return -1;
	}
	CnxRef foundConnection = nullptr;
	for (auto it: connectionPriorities) {
		for (auto jt : connections) {
			if ((connectionFailLimit <= 0 || jt->GetConnectFailCount() < connectionFailLimit)
					&& jt->HasProperties(it)
					&& (attempted.find(jt) == attempted.end())) {
				foundConnection = jt;
				break;
			}
		}
		if (foundConnection != nullptr) {
			break;
		}
	}
	if (foundConnection == nullptr) {
		for (auto jt : connections) {
			if ((connectionFailLimit <= 0 || jt->GetConnectFailCount() < connectionFailLimit)
					&& (attempted.find(jt) == attempted.end())) {
				foundConnection = jt;
				break;
			}
		}
	}
	if (foundConnection != nullptr) {
		if (foundConnection != activeConnection) {
			NotifyListeners(Event::SELECT_CONNECTION, foundConnection, foundConnection->LongName(), 0);
		}
	} else {
		NotifyListeners(Event::SELECT_CONNECTION, nullptr, "No viable connections available", UPC::Status::NO_VALID_CONNECTION_AVAILABLE);
		NotifyListeners(Event::CONNECT_FAILURE, nullptr, "No viable connections available", UPC::Status::NO_VALID_CONNECTION_AVAILABLE);
		NotifyListeners(Event::DISCONNECTED, nullptr, "No viable connections available", UPC::Status::SUCCESS);
		Disconnect();
		return -1;
	}
	attempted.insert(foundConnection);
	activeConnection = foundConnection;
	return activeConnection->Connect();
}

/**
*
*/
void
StandardConnector::OnConnectFailure(const CnxRef cr, const int status) {
	failedSinceConnect.insert(cr);
}

/**
*
*/
void
StandardConnector::OnConnectSucceed(const CnxRef cr) {
	attempted.clear();
	failedSinceConnect.clear();
}

/**
*
*/
void
StandardConnector::OnDisconnectSucceed(const CnxRef cr) {
	attempted.clear();
	failedSinceConnect.clear();
}


/**
 *
 */
int
StandardConnector::Disconnect()
{
	DEBUG_OUT("StandardConnector::Disconnect()");
	int r = 0;
	if (activeConnection != nullptr) {
		r = activeConnection->Disconnect();
		activeConnection = nullptr;
	}
	DEBUG_OUT("StandardConnector::Disconnect() done");
	return r;
}

/**
 *
 */
int
StandardConnector::Send(const std::string msg)
{
	if (activeConnection == nullptr) {
		NotifyListeners(Event::IO_ERROR, nullptr, "Send while no current connection", -1);
		return -1;
	}
	return activeConnection->Send(msg);
}


/**
 * sets the session id for the http connection which, if it is active, will trigger the polling reader and allow the http upc connection sends which need the session id
 */
void StandardConnector::SetActiveConnectionSessionID(std::string s) {
	activeConnection->SetSessionID(s);
}

/**
 * some uncertainty here as to how this should or would (or even if required) from websocket connections (TODO) ... I suspect that this will only be
 * required from http connections, in which case it should be a non issue, as they are not permanently connecting.
 */
void StandardConnector::SetConnectionAffinity(std::string host, int durationSec) {

	if (host != "") {
		if (activeConnection != nullptr) {
			if (activeConnection->GetHost() != host) {
			}
		}
	}
}

/**
 * @class StandardUVConnector StandardConnector.h
 * @brief StandardConnector that adds a default UVWSConnection and UPCHTTPConnection out of the box
 */

/**
 *
 */
StandardUVConnector::StandardUVConnector(std::string dfltHost, std::string dfltPort)
	: StandardConnector(dfltHost, dfltPort) {
	wsConnection = new UVWSConnection(dfltPort, dfltHost);
	httpConnection = new UPCHTTPConnection(dfltHost, "", dfltPort);
	AddConnection(wsConnection);
	AddConnection(httpConnection);
}

/**
 *
 */
StandardUVConnector::~StandardUVConnector() {
	DEBUG_OUT("StandardUVConnector::~StandardUVConnector()");
}

/**
 * I'm assuming for the moment that this will only be an issue on the http connection, for which there's a clear and simple approach. The web socket would
 * seem to require a full reclose and restart, which may bring in different affinities. For the http connection, it seems that it's simply a question
 * of setting the host and service appropriately, potentially setting a timer to reset the affinity host to the original. there doesn't seem to be a way
 * of getting a different affinity address or knowing what the new affinity address should be in the union protocol (TODO XXX)
 *  @see StandardConnector::SetConnectionAffinity
 */
void StandardUVConnector::SetConnectionAffinity(std::string host, int durationSec) {
	if (host != "") {
		if (activeConnection != nullptr) {
			if (activeConnection->GetHost() != host) {
				if (activeConnection == httpConnection) {
					httpConnection->SetHost(host);
				}
			}
		}
	}
}
