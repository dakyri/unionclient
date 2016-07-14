/*
 * AbstractConnector.h
 *
 *  Created on: Apr 7, 2014
 *      Author: dak
 */

#ifndef ABSTRACTCONNECTOR_H_
#define ABSTRACTCONNECTOR_H_

class AbstractConnector;
class AbstractConnection;

typedef std::unordered_map<std::string, std::string> ConnectionAttributes;

typedef std::string ConnectionProperty;
typedef std::unordered_set<ConnectionProperty> ConnectionPropertySet;

#define CONNECTION_PERSISTENT "persistent"
#define CONNECTION_SECURE "secure"

/**
 * @class AbstractConnector AbstractConnector.h
 */
class AbstractConnector: public NXConnection {
	friend class AbstractConnection;
public:
	AbstractConnector(std::string dfltHost="", std::string dfltPort="");
	virtual ~AbstractConnector();

	virtual void SetHost(const std::string host);
	virtual std::string GetHost() const;

	/**
	 * virtual method implemented by subclasses to indicate the connection is ready
	 */
	virtual bool IsReady() const=0;
	/**
	 * virtual method implemented by subclasses to initiate a connection
	 */
	virtual int Connect()=0;
	/**
	 * virtual method implemented by subclasses to end a connection
	 */
	virtual int Disconnect()=0;
	/**
	 * virtual method implemented by subclasses to send data along a connection
	 */
	virtual int Send(const std::string msg)=0;

	/**
	 * virtual method implemented by subclasses to send data along a connection
	 */
	virtual void SetActiveConnectionSessionID(std::string)=0;
	/**
	 * virtual method implemented by subclasses
	 */
	virtual void SetConnectionAffinity(std::string host, int durationSec)=0;

	/**
	 * virtual method implemented by subclasses if some special cleanup is done on connection failure
	 */
	virtual void OnConnectFailure(const CnxRef cr, const int status) {}
	virtual void OnConnectSucceed(const CnxRef cr) {}
	virtual void OnDisconnectSucceed(const CnxRef cr) {}

	void NotifyCheckConnectFailure(const CnxRef cr, const std::string msg, const int status);

	void SetLog(ILogger *log);

	void AddConnection(CnxRef, int ind=-1);
	CnxRef Connection(int n);
	CnxRef RemoveConnection(int n);
	int NConnections();

	void SetAllConnectionHost(std::string host);
	void SetAllConnectionService(std::string service);

protected:
	std::vector<CnxRef> connections; // or std::unique_ptr TODO
	std::string defaultHost;
	std::string defaultPort;

	ILogger *log = nullptr;
	static const int kLogBufLen=100;
	char logBuf[kLogBufLen];
};

class AbstractConnection
{
	friend class AbstractConnector;

public:
	AbstractConnection(const std::string service="", const std::string host="");
	virtual ~AbstractConnection();

	virtual int Connect()=0;
	virtual int Disconnect()=0;
	virtual int Send(const std::string msg)=0;

	virtual std::string LongName() = 0;
	virtual std::string ShortName() = 0;

	virtual void SetHost(const std::string host) const;
	virtual void SetService(const std::string service) const;
	virtual std::string GetHost() const;
	virtual std::string GetService() const;
	virtual void SetSessionID(const std::string s) {};

	int GetConnectState();
	ConnectionPropertySet GetProperties();
	void SetProperties(ConnectionPropertySet);
	bool HasProperties(ConnectionPropertySet);

	int GetConnectSucceedCount() { return nCnxSucceed; }
	int GetConnectFailCount() { return nCnxFail; }
	int GetIOErrorCount() { return nIOError; }
	int GetConnectTimeoutCount() { return nTimeout; }

	void Reset();

protected:
	void LogError(const std::string, ...) const;

	void NxReceive(const std::string data, const int status);
	void NxSend(const std::string data, const int status);
	void NxConnected(const CnxRef c);
	void NxDisconnected(const CnxRef c);
	void NxSelectConnection(const CnxRef c);
	void NxIOError(const std::string msg, const int status);
	void NxConnectFailure(const CnxRef c, const std::string msg, const int status);
	void NxServerHangup(const CnxRef c,const  std::string msg, const int status);

	int nCnxSucceed;
	int nCnxFail;
	int nIOError;
	int nTimeout;

	std::string mutable host;
	std::string mutable service;

	ConnectionPropertySet properties;
	int connectState;

	AbstractConnector* c;
};


#endif /* ABSTRACTCONNECTOR_H_ */
