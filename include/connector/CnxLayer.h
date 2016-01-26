/*
 * CnxLayer.h
 *
 *  Created on: May 11, 2014
 *      Author: dak
 */

#ifndef CNXLAYER_H_
#define CNXLAYER_H_

#include "connector/HTTP.h"

/**
 * @interface CnxLayerUpper CnxLayer.h
 * @brief interface for receipt of notifications from a lower protocol layer
 * typically a cnx layer implementation inherits this, unless it is the lowest level in the stack. other objects that are talking to the top level of the
 * stack may also inherit it
 */
class CnxLayerUpper {
public:
	CnxLayerUpper() {}
	virtual ~CnxLayerUpper() {}

	/** data received from the lower layer, which retains all ownership of buffers */
	virtual int Receive(const char *data, const size_t len)=0;
	/** lower layer successfully opened */
	virtual void OnOpen() {}
	/** lower layer successfully closed */
	virtual void OnClose() {}
	/** lower layer failed to open */
	virtual void OnOpenFailure(const std::string msg, const int status) {}
	/** lower layer had a more generic error */
	virtual void OnIOError(const std::string msg, const int status) {}
	/** lower layer was disconnected by the network */
	virtual void OnServerDisconnect(const std::string msg, const int status) {}
};

/**
 * @class CnxLayer CnxLayer.h
 */
class CnxLayer {
public:
	CnxLayer(CnxLayerUpper* upper, CnxLayer* lower);
	virtual ~CnxLayer();

	/** open this layer, and lower layers if needed */
	virtual int Open()=0;
	/** close this layer, and lower layers if needed */
	virtual int Close()=0;
	/** write on this layer, assuming that the data will be pushed through the protocol stack */
	virtual int Write(const char *data, const size_t len)=0;

//	int SendHttp(std::string req, std::string host, std::string res, HTTP::Headers headers, std::string body="");
	void DoIOError(int status, const std::string, ...) const;
	void DoOpenFailure(int status, const std::string, ...) const;
	void DoServerDisconnect(int status, const std::string, ...) const;

	CnxLayerUpper* upper;
	CnxLayer* lower;

	int connectState;

	static const int kErrInvalidHTTPResponse=-1;
	static const int kErrNoTransport=-2;
	static const int kErrTransportLayerBusy=-3;
	static const int kErrHTTPErrorResponse=-4;
	static const int kCloseOnClosedLayer=-5;
};



#endif /* CNXLAYER_H_ */
