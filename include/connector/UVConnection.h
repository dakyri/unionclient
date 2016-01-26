/*
 * UVConnector.h
 *
 *  Created on: Apr 28, 2014
 *	  Author: dak
 */

#ifndef UVCONNECTOR_H_
#define UVCONNECTOR_H_

#include "AbstractConnector.h"
#include "connector/CnxLayer.h"
#include "connector/HTTPCnxLayer.h"

#include <stdint.h>
#include <list>

#include "UVForwards.h"


class UVCnxLayer: public CnxLayer, public UVTCPClient
{
public:
	UVCnxLayer(CnxLayerUpper* upper, std::string s, std::string h);
	virtual ~UVCnxLayer();

	virtual int Open() override;
	virtual int Write(const char *data, const size_t len) override;
	virtual int Close() override;

	void SetHost(const std::string h) const;
	void SetService(const std::string s) const;
protected:
	int	DoConnection();

	std::string mutable host;
	std::string mutable service;

	int id;

	bool mutable hasResolvedAddress=false;
};


class UVWSConnection: public AbstractConnection, public CnxLayerUpper  {
public:
	UVWSConnection(std::string service="", std::string host="");
	~UVWSConnection();

protected:
	virtual int Connect()override;
	virtual int Disconnect()override;
	virtual int Send(const std::string msg)override;

	virtual std::string LongName() override;
	virtual std::string ShortName() override;

	virtual void SetHost(const std::string host) const override;
	virtual void SetService(const std::string service) const override;

	virtual int Receive(const char *data, const size_t len) override;
	virtual void OnOpen() override;
	virtual void OnClose() override;
	virtual void OnOpenFailure(const std::string msg, const int status) override;
	virtual void OnIOError(const std::string msg, const int status) override;
	virtual void OnServerDisconnect(const std::string msg, const int status) override;

	WSCnxLayer ws;
	UVCnxLayer uv;
};

class UVHTTPCnxUpper: public CnxLayer, public CnxLayerUpper
{
public:
	UVHTTPCnxUpper(CnxLayerUpper* upper, const std::string host="",  const std::string resource="", const std::string service="");
	virtual ~UVHTTPCnxUpper();

	virtual int Open() override;
	virtual int Write(const char *data, const size_t len) override;
	virtual int Close() override;

	void SetHost(const std::string host) const;
	void SetResource(const std::string resource) const;
	void SetMethod(const std::string m) const;
	void SetService(const std::string service) const;

	void SetNotifyReceipt(bool);
protected:
	virtual int Receive(const char *data, const size_t len) override;
	virtual void OnOpen() override;
	virtual void OnClose() override;
	virtual void OnOpenFailure(const std::string msg, const int status) override;
	virtual void OnIOError(const std::string msg, const int status) override;
	virtual void OnServerDisconnect(const std::string msg, const int status) override;

	int CheckQAndWrite();

	UVCnxLayer uv;
	HTTPCnxLayer http;

	class UVLock *queueLock;
	std::list<std::string> messageQueue;

	int mutable retryDelay;

	bool notifyReceipt;
};

class UPCHTTPConnection: public AbstractConnection, public CnxLayerUpper
{
public:
	UPCHTTPConnection(const std::string host="",  const std::string resource="", const std::string service="");
	virtual ~UPCHTTPConnection();

	virtual void SetSessionID(const std::string) override;
	void SetResource(const std::string) const;
	virtual void SetHost(const std::string host) const override;
	virtual void SetService(const std::string service) const override;

protected:
	virtual int Connect()override;
	virtual int Disconnect()override;
	virtual int Send(const std::string msg)override;

	virtual std::string LongName() override;
	virtual std::string ShortName() override;

	virtual int Receive(const char *data, const size_t len) override;
	virtual void OnOpen() override;
	virtual void OnOpenFailure(const std::string msg, const int status) override;
	virtual void OnIOError(const std::string msg, const int status) override;
	virtual void OnServerDisconnect(const std::string msg, const int status) override;

	int ModeCRequest();

	bool initialRequest = true;

	int sRequestIndex;
	int cRequestIndex;

	UVHTTPCnxUpper httpRx;
	UVHTTPCnxUpper httpTx;

	std::string mutable sessionID;
};

#endif /* UVCONNECTOR_H_ */
