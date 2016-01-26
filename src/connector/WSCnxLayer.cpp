/*
 * WSCnxLayer.cpp
 *
 *  Created on: May 11, 2014
 *      Author: dak
 */

#include <cstdio>
#include <cstring>
#include <random>

#include "UCLowerHeaders.h"
#include "connector/WSCnxLayer.h"
#include "connector/Base64.h"

#include <ctype.h>


/**
 * @class WSCnxLayer UVConnection.h
 * @brief implements websocket in a layer that will be used by an AbstractorConnection implementation
 *
 * TODO if the frame is too short we should probably stash the bytes and see if there is an issue on the next
 * chunk of data from the lower layer ... I think it possible that ssl will split ws packets
 */
const char *WSCnxLayer::WSGUID ="258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

WSCnxLayer::WSCnxLayer(CnxLayerUpper* upper, CnxLayer* lower)
	: CnxLayer(upper, lower)
	, rxType(0)
{

}

WSCnxLayer::~WSCnxLayer()
{

}

int
WSCnxLayer::Open()
{
	DEBUG_OUT("Websocket::Open()" );
	return lower? lower->Open():-1;
}


/**
 * sending data, so do the websocket wrapping
 */
int
WSCnxLayer::Write(const char *data, const size_t len) {
	if (connectState != ConnectionState::READY) {
		if (upper) upper->OnIOError("send before web socket negotiated", -1);
		return -1;
	}
	return DoWSWrite(kWSTextData, data, len, true);
}

int
WSCnxLayer::Close() {
	DEBUG_OUT("WSCnxLayer::Close()" );
	if (connectState == ConnectionState::READY) {
		connectState = ConnectionState::DISCONNECTION_IN_PROGRESS;
		uint16_t s=1; // not especially worried about network byte order atm TODO
		DoWSWrite(kWSCnxClose, (const char*)&s, 2, true);
	}
	int r = lower? lower->Close():0;
	DEBUG_OUT("WSCnxLayer::Close() ok");
	return r;
}

int
WSCnxLayer::Receive(const char *msgBytesThisFrame, const size_t nBytesThisFrame)
{
	char *msg = (char *)msgBytesThisFrame;
	size_t msgLen = (size_t)nBytesThisFrame;

	std::vector<char> msgBytes;
	if (stashedBytes.size() > 0) {
		DEBUG_OUT("using stashed bytes on websocket");
		for (auto it : stashedBytes) {
			msgBytes.push_back(it);
		}
		stashedBytes.clear();
		for (size_t i = 0; i < msgLen; i++) {
			msgBytes.push_back(msg[i]);
		}
		msg = msgBytes.data();
		msgLen = msgBytes.size();
	}
	if (connectState == ConnectionState::CONNECTION_IN_PROGRESS) {
		std::string data(msg, msgLen);
//		DEBUG_OUT( "incoming response " << data );
		response = response + data;
		if (HTTP::IsCompleteResponse(response)) {
			ProcessHTTPResponse(response);
		}
	} else {
		int n=0;
		char *p = (char *)msg;
		size_t pn = msgLen;
		while ((n=ProcessWSRxFrame(p, pn)) > 0) {
			p += n;
			pn -= n;
		}
		if (n < 0) {
			if (n == kErrFrameTooShort) {
				if (pn > 0) {
					DEBUG_OUT("got bytes remaining " << pn);
					stashedBytes.assign(p, p + pn);
				} else {
					DoIOError(-1, "websocket error processing frames, data length too short");
				}

				// TODO if the frame is too short we should probably stash the bytes and see if there is an issue on the next
				// chunk of data from the lower layer ... I think it possible that ssl will split ws packets
			} else {
				DoIOError(-1, "websocket error processing frames, unexpected error");
				uint16_t s=1; // not especially worried about network byte order atm TODO
				DoWSWrite(kWSCnxClose, (const char*)&s, 2, true);
				if (lower) lower->Close();
			}
		}
	}
	return 0;
}

/**
 * we don't inform the upper layer of the open until we have a working websocket. first negotiate this with http
 */
void
WSCnxLayer::OnOpen()  {
	DEBUG_OUT("connected ... ready for WebSocket handshake ...\n");
	if (!lower) {
		if (upper) upper->OnOpenFailure("No transport layer", -1);
	}
	connectState = ConnectionState::CONNECTION_IN_PROGRESS;
	key = MakeWSKey();
	response = "";
	std::string msg = HTTP::Message( HTTP_METHOD_GET, host, resource, {
			{"Upgrade", "websocket"},
			{"Connection","Upgrade"},
			{"Sec-WebSocket-Key",key},
			{"Sec-WebSocket-Version","13"}
	});
	lower->Write(msg.c_str(), msg.size());
}

void
WSCnxLayer::OnClose()
{
	if (upper) upper->OnClose();
}

/**
 * failure ... pass it back up the line
 */
void
WSCnxLayer::OnOpenFailure(const std::string msg, const int status) {
	DEBUG_OUT("WSCnxLayer::open failure");
	if (upper) upper->OnOpenFailure(msg, status);
}

/**
 * failure ... pass it back up the line
 */
void
WSCnxLayer::OnIOError(const std::string msg, const int status) {
	DEBUG_OUT("WSCnxLayer::io error");
	if (upper) upper->OnIOError(msg, status);
}

/**
 * failure ... pass it back up the line
 */
void
WSCnxLayer::OnServerDisconnect(std::string msg, int status) {
	DEBUG_OUT("WSCnxLayer::server disconect" );
	if (upper) upper->OnServerDisconnect(msg, status);
}



void
WSCnxLayer::ProcessHTTPResponse(const std::string& response)
{
	HTTP::Response r;
	int res = HTTP::SplitResponseHeaders(response, r);
	if (res < 0) {
		DoIOError(-1, "Bad HTTP Response "+response);
		if (upper) upper->OnOpenFailure("Bad HTTP Response, no message "+response, kErrInvalidHTTPResponse);
		connectState = ConnectionState::NOT_CONNECTED;
		if (lower) lower->Close();
		return;
	}
	if (r.responseCode !=  HTTP_RESPONSE_SWITCHING_PROTOCOLS) {
		DoIOError(-1, "Unfavourable HTTP Response "+r.statusMessage);
		if (upper) upper->OnOpenFailure("Unfavourable HTTP Response "+r.statusMessage, kErrWebsocketNotAccepted);
		connectState = ConnectionState::NOT_CONNECTED;
		if (lower) lower->Close();
		return;
	}
	auto it = r.headers.begin();
	++it;
	while (it != r.headers.end()) {
		++it;
	}
	connectState = ConnectionState::READY;
	if (upper) upper->OnOpen();
}


/**
 *  processes a single websocket frame contained in the given data
 * @param msgBytes raw data
 * @param msgLen length thereof
 * @return the length of the packet just processed, less than zero if a problem, kErrFrameShort if the frame is too short
 */
int
WSCnxLayer::ProcessWSRxFrame(char *msgBytes, const uint64_t msgLen)
{
	if (msgLen <= 0) {
		return 0;
	}
	if (msgLen < 2) {
		return kErrFrameTooShort;
	}
	bool isFinal = ((msgBytes[0]&0x80) != 0);
	bool doMask = ((msgBytes[1]&0x80) != 0);
	uint64_t dataLen=0;
	size_t headerLen=doMask?4:0;
	if ((msgBytes[1] & 0x7f) == 127) {
		headerLen += 10;
		if (msgLen < headerLen) {
			return kErrFrameTooShort;
		}

		dataLen |= (((uint64_t)msgBytes[2]) << 56);
		dataLen |= (((uint64_t)msgBytes[3]) << 48);
		dataLen |= (((uint64_t)msgBytes[4]) << 40);
		dataLen |= (((uint64_t)msgBytes[5]) << 32);
		dataLen |= (((uint64_t)msgBytes[6]) << 24);
		dataLen |= (((uint64_t)msgBytes[7]) << 16);
		dataLen |= (((uint64_t)msgBytes[8]) << 8);
		dataLen |= (((uint64_t)msgBytes[9]));
	} else if ((msgBytes[1] & 0x7f) == 126) {
		headerLen += 4;
		if (msgLen < headerLen) {
			return kErrFrameTooShort;
		}
		dataLen |= (((unsigned char)msgBytes[2]) << 8);
		dataLen |= (((unsigned char)msgBytes[3]));
	} else {
		headerLen += 2;
		if (msgLen < headerLen) {
			return kErrFrameTooShort;
		}
		dataLen = (msgBytes[1] & 0x7f);
	}
	if (msgLen < headerLen+dataLen) {
		return kErrFrameTooShort;
	}
	char *msgData = msgBytes+headerLen;
	if (doMask) {
		char *masks = msgBytes + headerLen;
		for (unsigned i=0; i<dataLen; i++) {
			msgData[i] ^= masks[i%4];
		}
	}
	int msgType = msgBytes[0]&0x0f;
	switch (msgType) {
	case kWSContinue:
	case kWSTextData:
	case kWSBinaryData: {
//		DEBUG_OUt("data frame " << msgLen);
		if (isFinal) {
			if (rxData.size() > 0) {
				for (size_t i=0; i<dataLen; i++) {
					rxData.push_back(msgData[i]);
				}
				if (rxType == kWSTextData) {
					if (upper) {
						upper->Receive(rxData.data(), (size_t)rxData.size());
					}
					rxData.clear();
				}
			} else {
				if (msgType == kWSTextData) {
					if (upper) {
						upper->Receive(msgData,  (size_t)dataLen);
					}
					rxData.clear(); // just to be sure ...
				}
			}
		} else {
			if (msgType != kWSContinue) {
				rxType = msgType;
				rxData.assign(msgData, msgData+dataLen);
			} else {
				for (size_t i=0; i<dataLen; i++) {
					rxData.push_back(msgData[i]);
				}
			}
		}
		break;
	}
	case kWSCnxClose: {
//		DEBUG_OUT("close frame");
		if (dataLen > 125) { // either someone hasn't read rfc6455, or we have screwed up royally
		}
		unsigned status = 0;
		std::string reason;
		if (dataLen >= 2) {
			status = (msgData[0]<<8) | msgData[1];
			if (dataLen > 2) {
				reason.assign(msgData, dataLen);
			}
		}
		if (connectState != ConnectionState::DISCONNECTION_IN_PROGRESS) { // we didn't generate this so we should do it politely
			if (upper) upper->OnServerDisconnect(reason, status);
			DoWSWrite(kWSCnxClose, msgData, dataLen, true);
			if (lower) lower->Close();
		}
		break;
	}
	case kWSPing:  { // we are now morally and technically required to pong
//		DEBUG_OUT("ping frame");
		if (dataLen > 125) { // either someone hasn't read rfc6455, or we have screwed up royally
// TODO double check whether it would be better to pong straight away, or wait until a more relaxed opportunity, like when we get out of this callback
			DoWSWrite(kWSPong, msgData, dataLen, true);
		}
		break;
	}

	case kWSPong: { // we have pinged, and all is ok
//		DEBUG_OUT("pong frame");
		if (dataLen > 125) { // either someone hasn't read rfc6455, or we have screwed up royally

		}
		break;
	}
	default: {
		DoIOError(kErrBadOpcode, "Unknown websocket opcode %d", msgType);
		return kErrBadOpcode;
	}
	}
//	DEBUG_OUT( "done frame" );
	return (int)(dataLen+headerLen);
}


/**
 * sending data, via the websocket wrapping
 * @param msgType the web socket message type
 * @param msg the message
 * @param length it's length
 * @param doMask if we need to mask the data ... which we must on client side
 */
int
WSCnxLayer::DoWSWrite(const int msgType, const char *msg, const uint64_t len, const bool doMask)
{
	char *msgBytes;
	size_t msgLen = len;
	int headerLen;
	if (len <= 125) {
		headerLen = 2;
	} else if (len <= 0xffff) {
		headerLen = 4;
	} else {
		headerLen = 10;
	}
	if (doMask) {
		msgLen += headerLen+4;
	}
	msgBytes = new char[msgLen];
	char *msgData =  msgBytes + headerLen + (doMask?4:0);

	memcpy(msgData, msg, len);
	if (doMask) {
		char *masks = msgBytes + headerLen;
		std::mt19937 eng((std::random_device())());
		std::uniform_int_distribution<> randomByte(0,255);
		for (unsigned i=0; i<4; i++) {
			masks[i] = randomByte(eng);
		}
		for (unsigned i=0; i<len; i++) {
			msgData[i] ^= masks[i%4];
		}
	}
	msgBytes[0] = msgType|0x80; // final fragment of text
	if (headerLen == 2) {
		msgBytes[1] = (char)(len|(doMask?0x80:0));
	} else if (headerLen == 4) {
		msgBytes[1] = 126|(doMask?0x80:0);
		msgBytes[2] = (len >> 8)&0xff;
		msgBytes[3] = (len)&0xff;
	} else {
		msgBytes[1] = 127|(doMask?0x80:0);
// FIXME size_t is 32 bits
		msgBytes[2] = (len >> 56)&0xff;
		msgBytes[3] = (len >> 48)&0xff;
		msgBytes[4] = (len >> 40)&0xff;
		msgBytes[5] = (len >> 32)&0xff;
		msgBytes[6] = (len >> 24)&0xff;
		msgBytes[7] = (len >> 16)&0xff;
		msgBytes[8] = (len >> 8)&0xff;
		msgBytes[9] = (len)&0xff;
	}
	size_t n = lower?lower->Write(msgBytes, msgLen):msgLen;
	delete [] msgBytes;
	return (int)n;
}

std::string
WSCnxLayer::MakeWSKey() const
{
	std::mt19937 eng((std::random_device())());
	std::uniform_int_distribution<> randomByte(0,255);
	unsigned char rawKey[16];
	for (int i=0; i<16; i++) {
		rawKey[i] = randomByte(eng);
	}

	return Base64::Encode(rawKey, 16);
}

void
WSCnxLayer::SetHost(const std::string h) const
{
	host = h;
}

void
WSCnxLayer::SetResource(const std::string r) const
{
	resource = r;
}
