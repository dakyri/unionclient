/*
 * HTTPCnxLayer.cpp
 *
 *  Created on: May 20, 2014
 *      Author: dak
 */
#include "uv.h"
#include "UCLowerHeaders.h"
#include "connector/HTTPCnxLayer.h"

static int _layerid=0;
/**
 * @class HTTPCnxLayer HTTPCnxLayer.h
 * @brief connection layer providing http connections
 */
HTTPCnxLayer::HTTPCnxLayer(CnxLayerUpper* upper, CnxLayer* lower)
	: CnxLayer(upper, lower)
	, waitingResponseMessageBody(false)
	, hasFullResponse(false)
{
	id = ++_layerid;
}

HTTPCnxLayer::~HTTPCnxLayer() {
	DEBUG_OUT("~HTTPCnxLayer() closing lower");
	if (lower) lower->Close();
	connectState = ConnectionState::NOT_CONNECTED;
	DEBUG_OUT("~HTTPCnxLayer() done");
}

// from CnxLayer
int
HTTPCnxLayer::Open()
{
	connectState = ConnectionState::READY;
// http we don't open a connection till we are sending .... and close once we've recieved
//	return lower? lower->Open():-1;
	return 1;
}


int
HTTPCnxLayer::Close()
{
	connectState = ConnectionState::NOT_CONNECTED;
// http ... close on receipt ... but just in case we're on a long poll or something
	DEBUG_OUT("HTTPCnxLayer closing lower in Close()");
	return lower? lower->Close():-1;
}


int
HTTPCnxLayer::Write(const char *data, const size_t len)
{
	if (lower) {
		if (lower->connectState == ConnectionState::READY) {
			DEBUG_OUT("HTTPCnxLayer::Write() prior request seems still in progress");
			DoIOError(kErrTransportLayerBusy, "Request already in progress ");
			return kErrTransportLayerBusy;
		}
	}
	messageBody.assign(data, len);
	waitingResponseMessageBody = false;
	hasFullResponse = false;
	responseMsg = "";
	if (lower) {
		DEBUG_OUT("HTTPCnxLayer::Write() openning transport");
		return lower->Open();
	}
	return 0;
}



int
HTTPCnxLayer::ProcessHTTPResponseHeaders(const std::string response, HTTP::Response& r)
{
	int res = HTTP::SplitResponseHeaders(response, r);
	if (res < 0) {
		DoIOError(kErrInvalidHTTPResponse, "Bad HTTP Response "+response);
		if (upper) upper->OnOpenFailure("Bad HTTP Response "+response, kErrInvalidHTTPResponse);
		connectState = ConnectionState::NOT_CONNECTED;
		DEBUG_OUT("HTTPCnxLayer closing lower as error for bad response");
		if (lower) lower->Close();
		return res;
	}
	std::string m = HTTP::ErrorResponseMessage(r.responseCode);
	if (m != "") {
		DoIOError(kErrHTTPErrorResponse, "HTTP Error "+r.responseCode+ ", "+m);
		if (upper) upper->OnOpenFailure("HTTP Error "+r.responseCode+ ", "+m, kErrHTTPErrorResponse);
		connectState = ConnectionState::NOT_CONNECTED;
		DEBUG_OUT("HTTPCnxLayer closing lower as error for http error");
		if (lower) lower->Close();
		return kErrHTTPErrorResponse;
	}

	connectState = ConnectionState::READY;
//	if (upper) upper->OnOpen();
	return res;
}

// from CnxLayerUpper
int
HTTPCnxLayer::Receive(const char *msgBytes, const size_t msgLen)
{
	std::string data(msgBytes, msgLen);

	responseMsg = responseMsg + data;

	if (waitingResponseMessageBody) {
		if (responseMsg.size() >= response.contentLength) {
			response.body.assign(responseMsg.c_str(), response.contentLength);
//			responseMsg.erase(responseMsg.begin(), responseMsg.begin()+response.contentLength);
			std::string m(responseMsg.c_str()+response.contentLength, responseMsg.size()-response.contentLength);
			if (responseMsg.size() > response.contentLength) {
				std::string m(responseMsg.c_str(), responseMsg.size()-response.contentLength);
				responseMsg = m;
			} else {
				responseMsg = "";
			}
			hasFullResponse = true;
			waitingResponseMessageBody = false;
		}
	} else if (HTTP::IsCompleteResponse(responseMsg)) {
		int i = ProcessHTTPResponseHeaders(responseMsg, response);
		if (i < 0) {
			return -1;
		}
//		responseMsg.erase(responseMsg.begin(), responseMsg.begin()+i);
		if (responseMsg.size() > (unsigned)i) {
			std::string m(responseMsg.c_str()+i, responseMsg.size()-i);
			responseMsg = m;
		} else {
			responseMsg = "";
		}
		if (response.contentLength > 0) {
			if (responseMsg.size() >= response.contentLength) {
				waitingResponseMessageBody = false;
				response.body.assign(responseMsg.c_str(), response.contentLength);
//				responseMsg.erase(responseMsg.begin(), responseMsg.begin()+response.contentLength);
				if (responseMsg.size() > response.contentLength) {
					std::string m(responseMsg.c_str()+response.contentLength, responseMsg.size()-response.contentLength);
					responseMsg = m;
				} else {
					responseMsg = "";
				}
				hasFullResponse = true;
//				if (upper) upper->Receive(response.body.c_str(), response.contentLength);
			} else {
				waitingResponseMessageBody = true;
			}
		} else {
			response.body = "";
			hasFullResponse = true;
			waitingResponseMessageBody = false;
		}
	}
	if (!waitingResponseMessageBody) {
		DEBUG_OUT("HTTPCnxLayer " << id << " closing lower in Receive " << response.contentLength << " " << hasFullResponse << " bytes" << response.body);
		if (lower) lower->Close();
	}
	return 0;
}

/**
 * called by the lower layer when we're ready to write: the socket, or whatever we happen to be using is open  ... perhaps it is a cage full of carrier pigeons carrying usb sticks
 *  ...now we send the stuff we asked to send in the previous write
 */
void
HTTPCnxLayer::OnOpen()
{
	DEBUG_OUT("HTTPCnxLayer::OnOpen() " << id);
	responseMsg = "";
	if (!lower) {
		if (upper) upper->OnOpenFailure("No transport layer", kErrNoTransport);
	}
	std::string msg = HTTP::Message(method, host, resource, headers, messageBody);
	lower->Write(msg.c_str(), msg.size());
}

/**
 * the lower layer has successfully closed
 */
void
HTTPCnxLayer::OnClose()
{
	DEBUG_OUT("HTTPCnxLayer::OnClose() " << id << " length" << response.contentLength);
	if (upper) {
		bool doNotifyReceipt = hasFullResponse;
		hasFullResponse = false;
		if (doNotifyReceipt) upper->Receive(response.body.c_str(), response.contentLength);
		response.body = "";
		response.contentLength = 0;
		responseMsg = "";
		upper->OnClose();
	}
}

void
HTTPCnxLayer::OnOpenFailure(const std::string msg, const int status)
{
	if (status == UV_EALREADY) {
		if (upper) upper->OnOpenFailure(messageBody, status);
	} else {
		if (upper) upper->OnOpenFailure(msg, status);
	}
}


void
HTTPCnxLayer::OnIOError(const std::string msg, const int status)
{
	DEBUG_OUT("HTTPCnxLayer::OnIOError() " << id << " length" << response.contentLength);
	if (upper) upper->OnIOError(msg, status);
}


void
HTTPCnxLayer::OnServerDisconnect(const std::string msg, const int status)
{
	if (upper) upper->OnServerDisconnect(msg, status);
}


void
HTTPCnxLayer::SetHost(const std::string h) const
{
	host = h;
}

void
HTTPCnxLayer::SetResource(const std::string r) const
{
	resource = r;
}

void
HTTPCnxLayer::SetMethod(const std::string m) const
{
	method = m;
}

void
HTTPCnxLayer::SetHeaders(const HTTP::Headers h) const
{
	headers = h;
}

