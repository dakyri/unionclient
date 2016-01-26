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
#include "UVEventLoop.h"

#include <ctype.h>

extern UVEventLoop worker;

/**
 * @class UVCnxLayer UVConnection.h
 * @brief Class performing raw socket io using libuv. Designed to plug into other layers implementing protocols like http and websocket over the top
 *
 * Request* methods make the basic libuv call and set up call backs to a bound std::function, which is call by the On* methods, which
 * are static C style call back functions.
 *
 * Running this on a separate event loop thread ...  The run thread is static, and shared between UVCnxLayer instances
 */

int _layer_id=0;
UVCnxLayer::UVCnxLayer(CnxLayerUpper* upper, std::string s, std::string h)
	: CnxLayer(upper, nullptr) {

	service = s;
	host = h;
	hasResolvedAddress = false;
	connectState = ConnectionState::NOT_CONNECTED;
	id = ++_layer_id;
}

UVCnxLayer::~UVCnxLayer() {
	DEBUG_OUT("~UVCnxLayer()");
}



/**
 * setter for the host ... sets a flag so that we make sure we redo the getaddr call
 */
void
UVCnxLayer::SetHost(const std::string h) const
{
	hasResolvedAddress = false;
	host = h;
}

/**
 * setter for the target service ... sets a flag so that we make sure we redo the getaddr call
 */
void
UVCnxLayer::SetService(const std::string s) const
{
	hasResolvedAddress = false;
	service = s;
}

/**
 * open hook. will call get addr if needed ... otherwise strives for socket connection nirvana
 */
int
UVCnxLayer::Open()
{
	DEBUG_OUT("UVCnxLayer::Opend()" );
	DEBUG_OUT("connect request ... :" << host << ":" << service << " resolved " << hasResolvedAddress << " layer " << id);
	if (hasResolvedAddress) {
		char addr[17] = {'\0'};
		uv_ip4_name((struct sockaddr_in*) address, addr, 16);
		DEBUG_OUT("connecting to "<< addr <<" family "<< address->sa_family <<" port "<<(((struct sockaddr_in*) address)->sin_port)<<" connecting ...\n");
		int r= DoConnection();
		return r;
	}
	worker.Resolve(this, host, service, [this] (sockaddr *adr, int status) {
		DEBUG_OUT("UVCnx resolve" << status);
		if (status < 0) {
			DoOpenFailure(status, "uv_getaddrinfo() callback error %s\n", uv_strerror(status));
			return;
		}
		hasResolvedAddress = true;
		*address = *adr;
		char addr[17] = {'\0'};
		uv_ip4_name((struct sockaddr_in*) address, addr, 16);
		DEBUG_OUT("resolved to "<< addr <<" family "<< address->sa_family <<" port "<<(((struct sockaddr_in*) address)->sin_port)<<" connecting ...\n");
		DoConnection();
	});
	return 0;
}

/**
 * main write hook. doesn't run on main thread, so shouldn't need to acquire mutex, and at the moment doesn't set up anything other than a basic static cleanup callback
 */
int
UVCnxLayer::Write(const char *data, const size_t len) {
	DEBUG_OUT("UVCnxLayer::Write() " << len << " on " << " layer " << id);
	worker.Write(this, data, len);
	return 0;
}

/**
 * main close hook
 * uncertain about the mutex here ... XXX it is possible for this entry point to be triggered in an error callback ie at a point
 * where we already have the mutex ... uv mutex locks aren't guaranteed to be recursive
 */
int
UVCnxLayer::Close() {
	DEBUG_OUT("UVCnxLayer::Close" << " layer " << id);
	int r = 0;
	if (connectState == ConnectionState::NOT_CONNECTED) {
		DEBUG_OUT("UVCnxLayer::Close() already closed" << " layer " << id);
		return 0;
	}
	if (connectState == ConnectionState::DISCONNECTION_IN_PROGRESS) {
		DEBUG_OUT("UVCnxLayer::Close() already disconnecting" << " layer " << id);
		return kCloseOnClosedLayer; // xxx perhaps we shouldn't regard this as an error?
	}
	connectState = ConnectionState::DISCONNECTION_IN_PROGRESS;
	worker.Close(this, [this] (uv_handle_t* h) {
		DEBUG_OUT("UVCnxLayer::Close close callback" << " layer " << id);
		connectState = ConnectionState::NOT_CONNECTED;
		if (upper) upper->OnClose();
	});
//	if (connectState == ConnectionState::READY) {
//	} else {
//	}
	return r;
}

/**
 * does the actual work of the connection. assumes we have an inet address, and kicks in a read callback if we succeed
 */
int
UVCnxLayer::DoConnection()
{
	DEBUG_OUT("DoConnection() ... " << hasResolvedAddress << id);
	if (!hasResolvedAddress) {
		DoIOError(-1, "MakeConnection:: address unresolved\n");
		return -1;
	}
	DEBUG_OUT("DoConnection() ... " << host << ":" << service << " layer " << id);
	worker.Connect(this,
			[this] (uv_connect_t *req, int status) {
				if (status < 0) {
					DoOpenFailure(status, "connect failed error %s\n", uv_strerror(status));
					DEBUG_OUT("UVCnxLayer::DoConnection() error ..." << uv_strerror(status) << " layer " << id);
					return;
				}
				DEBUG_OUT("UVCnxLayer::DoConnection() ready  ..." << " layer " << id );
				connectState = ConnectionState::READY;
				if (upper) upper->OnOpen();
			},
			[this](uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
				if (nread < 0) {
					if (nread != UV_EOF) {
						DEBUG_OUT("UVCnxLayer::Read() io error  ..." << uv_strerror((int)nread) << " layer " << id );
						DoIOError(-1, "Read error %s\n", uv_strerror((int)nread));
					} else {
						DEBUG_OUT("UVCnxLayer::Read() eof error  ..."<< " layer " << id);
						worker.Close(this, [this] (uv_handle_t* h) {
							connectState = ConnectionState::NOT_CONNECTED;
							DoServerDisconnect(-1, "Unexpected end of file on uv read");
						});
//						DoServerDisconnect(-1, "Unexpected end of file on uv read");
//						worker.Close(this, [this] (uv_handle_t* h) {
//							connectState = ConnectionState::NOT_CONNECTED;
//						});
					}
					return;
				} else if (nread > 0) {
					if (upper) upper->Receive(buf->base, nread);
				}
			}
		);
	return 0;
}
