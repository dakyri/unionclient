/*
 * WSCnxLayer.h
 *
 *  Created on: May 11, 2014
 *      Author: dak
 */

#ifndef WSCNXLAYER_H_
#define WSCNXLAYER_H_

#include "CnxLayer.h"

class WSCnxLayer: public CnxLayer, public CnxLayerUpper
{
public:
	WSCnxLayer(CnxLayerUpper* upper, CnxLayer* lower);
	virtual ~WSCnxLayer();

	virtual int Open() override;
	virtual int Write(const char *data, const size_t len) override;
	virtual int Close() override;

	virtual int Receive(const char *data, const size_t len) override;
	virtual void OnOpen() override;
	virtual void OnClose() override;
	virtual void OnOpenFailure(const std::string msg, const int status) override;
	virtual void OnIOError(const std::string msg, const int status) override;
	virtual void OnServerDisconnect(const std::string msg, const int status) override;

	void SetHost(const std::string h) const;
	void SetResource(const std::string r) const;

	static const char* WSGUID;

	static const int kErrFrameTooShort = -101;
	static const int kErrBadOpcode = -102;
	static const int kErrWebsocketNotAccepted = -103;

protected:
	static const unsigned kWSContinue = 0x0;
	static const unsigned kWSTextData = 0x1;
	static const unsigned kWSBinaryData = 0x2;
	static const unsigned kWSCnxClose = 0x8;
	static const unsigned kWSPing = 0x9;
	static const unsigned kWSPong = 0xa;

	void ProcessHTTPResponse(const std::string& response);
	int ProcessWSRxFrame(char *msgBytes, const uint64_t msgLen);
	int DoWSWrite(const int msgType, const char *msg, const uint64_t len, const bool doMask);

	std::string MakeWSKey() const;

	std::string mutable host;

	std::string mutable key;

	std::string mutable resource;
	std::string mutable response;

	std::vector<char> stashedBytes;
	std::vector<char> rxData;
	int rxType;
};




#endif /* WSCNXLAYER_H_ */
