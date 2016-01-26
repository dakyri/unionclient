/*
 * HTTPCnxLayer.h
 *
 *  Created on: May 20, 2014
 *      Author: dak
 */

#ifndef HTTPCNXLAYER_H_
#define HTTPCNXLAYER_H_

#include "connector/CnxLayer.h"

class HTTPCnxLayer: public CnxLayer, public CnxLayerUpper {
public:
	HTTPCnxLayer(CnxLayerUpper* upper, CnxLayer* lower);
	virtual ~HTTPCnxLayer();

	void SetHost(const std::string h) const;
	void SetResource(const std::string r) const;
	void SetMethod(const std::string m) const;
	void SetHeaders(const HTTP::Headers m) const;
	HTTP::Response& GetResponse() const;

// from CnxLayerUpper
	virtual int Receive(const char *data, const size_t len) override;

	virtual void OnOpen() override;
	virtual void OnClose() override;
	virtual void OnOpenFailure(const std::string msg, const int status) override;
	virtual void OnIOError(const std::string msg, const int status) override;
	virtual void OnServerDisconnect(const std::string msg, const int status) override;

// from CnxLayer
	virtual int Open() override;
	virtual int Close() override;
	virtual int Write(const char *data, const size_t len) override;

protected:
	int ProcessHTTPResponseHeaders(const std::string rh, HTTP::Response& r);

	std::string mutable host;
	std::string mutable resource;
	std::string mutable method;
	HTTP::Headers mutable headers;

	std::string mutable messageBody;
	std::string mutable responseMsg;

	HTTP::Response response;

	bool waitingResponseMessageBody;
	bool hasFullResponse;

	int id=0;
};

#endif /* HTTPCNXLAYER_H_ */
