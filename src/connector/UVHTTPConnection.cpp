/*
 * UVHTTPConnection.cpp
 *
 *  Created on: May 20, 2014
 *      Author: dak
 */


#include <cstdio>
#include <cstring>
#include <random>

#include "uv.h"
#include "UCLowerHeaders.h"
#include "connector/UVConnection.h"
#include "UVEventLoop.h"

#include <ctype.h>

extern UVEventLoop worker;

/**
 * @class UVHTTPCnxUpper UVConnection.h
 * @brief implements an unsecured http connection over a libuv raw tcp socket.
 *
 * this is the base http connector ... in the current case, we need a couple of these to talk http with the server the HTTPCnxLayer handles the
 * core HTTP implementation, this layer gives it the UV transport and provides error tracking, timeouts, and queueing of requests
 */
UVHTTPCnxUpper::UVHTTPCnxUpper(CnxLayerUpper* upper, const std::string host,  const std::string resource, const std::string service)
	: CnxLayer(upper, &http)
	, CnxLayerUpper()
	, uv(&http, service, host)
	, http(this, &uv)
	, retryDelay(10)
	, notifyReceipt(true)
{
	queueLock = new UVLock();
}


UVHTTPCnxUpper::~UVHTTPCnxUpper()
{
	DEBUG_OUT( "~UVHTTPCnxUpper()");
	delete queueLock;
}

/**
 * data receipt from lower layers ... only passed back up if notify flag is set
 */
int UVHTTPCnxUpper::Receive(const char *data, const size_t len)
{
	if (notifyReceipt && upper) return upper->Receive(data, len);
	return 0;
}


/**
 * open notification from lower layer(s)
 */
void UVHTTPCnxUpper::OnOpen()
{
	if (upper) upper->OnOpen(); // probably shouldn't get this ... the http layer does it's write on opens automatically though it's closed, and gives an an open response
}

void UVHTTPCnxUpper::OnClose()
{
	CheckQAndWrite();
}

void UVHTTPCnxUpper::OnOpenFailure(const std::string msg, const int status)
{
	if (status == UV_EALREADY) {
		queueLock->Lock();
		messageQueue.push_front(msg);
		queueLock->Unlock();
		worker.Schedule(retryDelay*1000, 0, [this](){
			CheckQAndWrite();
		});
	} else {
		if (upper) upper->OnOpenFailure(msg, status);
	}
}

/**
 * io error notification from lower layer(s)
 */
void UVHTTPCnxUpper::OnIOError(const std::string msg, const int status)
{
	if (upper) upper->OnIOError(msg, status);
}


/**
 * disconenction notification from lower layer(s)
 */
void UVHTTPCnxUpper::OnServerDisconnect(const std::string msg, const int status)
{
	if (upper) upper->OnServerDisconnect(msg, status);
}

/**
 * open hook
 */
int UVHTTPCnxUpper::Open()
{
	if (lower) return lower->Open();
	return 0;
}


/**
 * output hook
 */
int UVHTTPCnxUpper::Write(const char *data, const size_t len)
{
	std::string msg(data, len);
//	DEBUG_OUT("UVHTTPCnxUpper::Write " << msg );
	queueLock->Lock();
	messageQueue.push_back(msg);
	queueLock->Unlock();
	return CheckQAndWrite();
}

/**
 * http messages create a mess when several are sent at once. if the outgoing line is busy, they are queued. here we check any http messages that may be queued.
 */
int
UVHTTPCnxUpper::CheckQAndWrite()
{
	int r=0;
	std::string msg;
	bool hasFromQ = false;
	queueLock->Lock();
	if (!messageQueue.empty()) {
		msg = messageQueue.front();
		messageQueue.pop_front();
		hasFromQ = true;
	}
	queueLock->Unlock();
	if (hasFromQ) {
		DEBUG_OUT("UVHTTPCnxUpper::CheckQAndWrite() finds message" );
		if (lower) r = lower->Write(msg.c_str(), msg.size());
		if (r < 0) {
			queueLock->Lock();
			messageQueue.push_front(msg);
			queueLock->Unlock();
			worker.Schedule(retryDelay*1000, 0, [this](){
				CheckQAndWrite();
			});

			if (r == kErrTransportLayerBusy) {
			} else {
			}
		}
	}
	return r;
}

/**
 * close hook
 */
int UVHTTPCnxUpper::Close()
{
	if (lower) return lower->Close();
	return 0;
}

/**
 * sets the connection host
 * @param h the host
 */
void
UVHTTPCnxUpper::SetHost(const std::string h) const
{
	http.SetHost(h);
	uv.SetHost(h);
}

/**
 * sets the resource to access
 * @param r the resource
 */
void
UVHTTPCnxUpper::SetResource(const std::string r) const
{
	http.SetResource(r);
}

/**
 * sets the connection service
 * @param s the service to use
 */
void
UVHTTPCnxUpper::SetService(const std::string s) const
{
	uv.SetService(s);
}


/**
 * sets the http method
 * @param m
 */
void
UVHTTPCnxUpper::SetMethod(const std::string m) const
{
	http.SetMethod(m);
}

/**
 * setter to toggle receipt notification
 */
void
UVHTTPCnxUpper::SetNotifyReceipt(bool n)
{
	notifyReceipt = n;
}

/**
 * @class UPCHTTPConnection UVConnection.h
 * @brief implements a pair of unsecured http connections over a libuv raw tcp socket with a bit of upc wierdness added
 *
 * actually implements a couple of http connections, and does its best to cope with UPC's peculiarities
 */
UPCHTTPConnection::UPCHTTPConnection(std::string host, std::string resource, const std::string service)
	: AbstractConnection(host, service)
	, CnxLayerUpper()
	, httpRx(this)
	, httpTx(this)
{
	sRequestIndex = 1;
	cRequestIndex = 1;

	SetHost(host);
	SetService(service);
	SetResource(resource);
	httpRx.SetMethod( HTTP_METHOD_POST);
	httpTx.SetMethod( HTTP_METHOD_POST);
}

UPCHTTPConnection:: ~UPCHTTPConnection()
{
	DEBUG_OUT("~UVHTTPConnection() closing connection");
	if (connectState != ConnectionState::DISCONNECTION_IN_PROGRESS && connectState != ConnectionState::NOT_CONNECTED) {
		httpRx.Close();
		httpTx.Close();
	}
	DEBUG_OUT("~UVHTTPConnection() done");
}

std::string
UPCHTTPConnection::LongName()
{
	return std::string("http connection at " + host + ":" + service);
}

std::string
UPCHTTPConnection::ShortName()
{
	return std::string("http://" + host + ":" + service);
}

/**
 * main hook to connect. doesn't actually connect the comms layers (http is stateless) just sets up constants for initial request
 */
int
UPCHTTPConnection::Connect()
{
	initialRequest = true;
	httpTx.SetNotifyReceipt(true);
	connectState = ConnectionState::READY; // we won't get errors till we try to do something
	NxConnected(this);
	return 0;
}

/**
 * main hook to disconnect. cleans up connections in case something messy has happeneds
 */
int
UPCHTTPConnection::Disconnect()
{
	DEBUG_OUT("UPCHTTPConnection disconnecting ..." );
	if (connectState != ConnectionState::DISCONNECTION_IN_PROGRESS && connectState != ConnectionState::NOT_CONNECTED) {
		connectState = ConnectionState::DISCONNECTION_IN_PROGRESS;
		httpRx.Close();
		httpTx.Close();
		connectState = ConnectionState::NOT_CONNECTED;
		NxDisconnected(this);
	}
	return 0;
}

/**
 * main hook to send data ... uses the httpTx connector with different setting depending on whether it is the first transmission or subsequent.
 * in upc, subsequent requests return no data of interest as all the interesting bits come from the long poll on httpRx
 */
int
UPCHTTPConnection::Send(const std::string msg)
{
	HTTP::PostData pd;
	if (initialRequest) {
		pd["mode"] = "d";
	} else {
		pd["mode"] = "s";
		pd["rid"] = std_to_string(sRequestIndex++);
		pd["sid"] = sessionID;
	}
	pd["data"] = msg;
	std::string ucpMsg = pd.Serialize();
	return httpTx.Write(ucpMsg.c_str(), (size_t) ucpMsg.size());
}

/**
 * data receipt from lower layers
 */
int
UPCHTTPConnection::Receive(const char *data, const size_t len) {
	std::string strData(data, len);
	DEBUG_OUT("UPCHTTPConnection::Receive" << len << " " << initialRequest);
	if (initialRequest) {
		NxReceive(strData, UPC::Status::SUCCESS);
	} else {
		if (strData.size() > 0) {
			NxReceive(strData, UPC::Status::SUCCESS);
		}
		if (connectState == ConnectionState::READY) {
			DEBUG_OUT("UPCHTTPConnection::Receive() initiate mode c request on receipt of " << strData.size() << " bytes ");
			ModeCRequest();
		}
	}
	return 0;
}

/**
 * open notification from lower layer(s)
 */
void
UPCHTTPConnection::OnOpen() {
	if (initialRequest) {
		DEBUG_OUT("initial request on open");
		connectState = ConnectionState::READY;
		NxConnected(this);
	}
}
/**
 * open failure notification from lower layer(s)
 */
void
UPCHTTPConnection::OnOpenFailure(std::string msg, int status) {
	DEBUG_OUT("UVHTTPConnection::OnOpenFailure");
	connectState = ConnectionState::NOT_CONNECTED;
	NxConnectFailure(this, msg, status);
}
/**
 * io error notification from lower layer(s)
 */
void
UPCHTTPConnection::OnIOError(std::string msg, int status) {
	NxIOError(msg, status);
}
/**
 * disconenction notification from lower layer(s)
 */
void
UPCHTTPConnection::OnServerDisconnect(std::string msg, int status) {
	connectState = ConnectionState::NOT_CONNECTED;
	NxServerHangup(this, msg, status);
}

/**
 * sets the connection host
 * @param h the host
 */
void
UPCHTTPConnection::SetHost(const std::string h) const
{
	host = h;
	httpRx.SetHost(h);
	httpTx.SetHost(h);
}

/**
 * sets the connection service
 * @param s the service to use
 */
void
UPCHTTPConnection::SetService(const std::string s) const
{
	service = s;
	httpRx.SetService(s);
	httpTx.SetService(s);
}

/**
 * sets the resource to access
 * @param r the resource
 */
void
UPCHTTPConnection::SetResource(const std::string r) const
{
	httpRx.SetResource(r);
	httpTx.SetResource(r);
}

/**
 * calling this indicates we have pulled the necessary session id from the xml. time to kick in the primary send, and start the polling connection
 * @param id the union session id
 */
void
UPCHTTPConnection::SetSessionID(const std::string id)
{
	DEBUG_OUT("get session id " << id);
	sessionID = id;
	initialRequest = false;
	httpTx.SetNotifyReceipt(false);
	ModeCRequest();
}

/**
 * initiate the long polling request
 */
int
UPCHTTPConnection::ModeCRequest()
{
	HTTP::PostData pd;
	pd["mode"] = "c";
	pd["rid"] = std_to_string(cRequestIndex++);
	pd["sid"] = sessionID;
	std::string ucpMsg = pd.Serialize();
	DEBUG_OUT("mode c request " );
	return httpRx.Write(ucpMsg.c_str(), (size_t) ucpMsg.size());

}

