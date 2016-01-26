/*
 * UVConnector.cpp
 *
 *  Created on: Apr 28, 2014
 *	  Author: dak
 */
#include <cstdio>
#include <cstring>
#include <random>

#include "uv.h"
#include "UCLowerHeaders.h"
#include "connector/UVConnection.h"

#include <ctype.h>


/**
 * @class UVWSConnection UVConnection.h
 * @brief implements an unsecured WebSocket connection over a libuv raw tcp socket.
 * sets up two connected CnxLayers ... a websocket layer, which will talk to a uv layer ... the object using us will request stuff which we push to the ws layer which then
 * talks over uv.
 *
 * connectState will be READY only once we have a correctly negotiated web socket
 */
UVWSConnection::UVWSConnection(std::string service, std::string host)
	: AbstractConnection(host, service)
	, CnxLayerUpper()
	, ws(this, &uv)
	, uv(&ws, service, host)
{
	SetHost(host);
	SetService(service);
	ConnectionPropertySet s;
	s.insert(CONNECTION_PERSISTENT);
	SetProperties(s);
}

UVWSConnection:: ~UVWSConnection()
{
	DEBUG_OUT("~UVWSConnection() closing connection");
	if (connectState != ConnectionState::DISCONNECTION_IN_PROGRESS && connectState != ConnectionState::NOT_CONNECTED) {
		ws.Close();
	}
	DEBUG_OUT("~UVWSConnection() done" );
}

std::string
UVWSConnection::LongName()
{
	return std::string("web socket at " + host + ":" + service);
}

std::string
UVWSConnection::ShortName()
{
	return std::string("ws://" + host + ":" + service);
}



/**
 * main hook to start it off
 */
int
UVWSConnection::Connect()
{
	return ws.Open();
}

/**
 * main hook to say goodbye
 */
int
UVWSConnection::Disconnect()
{
	int r=0;
	if (connectState != ConnectionState::DISCONNECTION_IN_PROGRESS && connectState != ConnectionState::NOT_CONNECTED) {
		connectState = ConnectionState::DISCONNECTION_IN_PROGRESS;
		r = ws.Close();
	}
	return r;
}

/**
 * main hook to actually do stuff
 */
int
UVWSConnection::Send(const std::string msg)
{
	int r = ws.Write(msg.c_str(), (size_t) msg.size());
	return r;
}

/**
 * callback to get whatever is coming back from the websocket with all the ws frills removed
 */
int
UVWSConnection::Receive(const char *data, const size_t len) {
	std::string strData(data, len);
	NxReceive(strData, UPC::Status::SUCCESS);
	return 0;
}

/**
 * callback for a successful open
 */
void
UVWSConnection::OnOpen() {
	connectState = ConnectionState::READY;
	NxConnected(this);
}

/**
* callback for a successful open
*/
void
UVWSConnection::OnClose() {
	connectState = ConnectionState::NOT_CONNECTED;
	NxDisconnected(this);
}

/**
 * callback for a tragic open
 */
void
UVWSConnection::OnOpenFailure(std::string msg, int status) {
	DEBUG_OUT("UVWSConnection::OnOpenFailure" );
	connectState = ConnectionState::NOT_CONNECTED;
	NxConnectFailure(this, msg, status);
}

/**
 * callback for a generic io crisis
 */
void
UVWSConnection::OnIOError(std::string msg, int status) {
	if (connectState == ConnectionState::DISCONNECTION_IN_PROGRESS) {
		connectState = ConnectionState::NOT_CONNECTED;
		NxDisconnected(this);
	} else {
		NxIOError(msg, status);
	}
}

/**
 * callback for a clean close
 */
void
UVWSConnection::OnServerDisconnect(std::string msg, int status) {
	if (connectState == ConnectionState::DISCONNECTION_IN_PROGRESS) {
		connectState = ConnectionState::NOT_CONNECTED;
		NxDisconnected(this);
	} else {
		connectState = ConnectionState::NOT_CONNECTED;
		NxServerHangup(this, msg, status);
	}
}

/**
 * sets destination host on all our child layers
 */
void
UVWSConnection::SetHost(const std::string newHost) const
{
	host = newHost;
	uv.SetHost(host);
	ws.SetHost(host);
}

/**
 * sets destination service on all our child layers
 */
void
UVWSConnection::SetService(const std::string newService) const
{
	service = newService;
	uv.SetService(service);
}
