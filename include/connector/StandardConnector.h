/*
 * StandardConnector.h
 *
 *  Created on: May 2, 2014
 *      Author: dak
 */

#ifndef STANDARDCONNECTOR_H_
#define STANDARDCONNECTOR_H_
class UVWSConnection;
class UPCHTTPConnection;

class StandardConnector: public AbstractConnector {
public:
	StandardConnector(std::string dfltHost="", std::string dfltPort="");
	virtual ~StandardConnector();

	virtual bool IsReady() const override;

	virtual int Connect() override;
	virtual int Disconnect() override;
	virtual int Send(const std::string msg) override;

	virtual void SetActiveConnectionSessionID(std::string) override;
	virtual void SetConnectionAffinity(std::string affinityAddress, int durationSec) override;

	virtual void OnConnectFailure(const CnxRef cr, const int status) override;
	virtual void OnConnectSucceed(const CnxRef cr) override;
	virtual void OnDisconnectSucceed(const CnxRef cr) override;

	void SetConnectionPriorities(std::vector<ConnectionPropertySet> pri);
	int GetConnectionFailLimit() { return connectionFailLimit;  }
	void SetConnectionFailLimit(int n) { connectionFailLimit = n; }
protected:
	std::vector<ConnectionPropertySet> connectionPriorities;
	AbstractConnection* activeConnection;
	int connectionFailLimit;
	std::unordered_set<CnxRef> attempted;
	std::unordered_set<CnxRef> failedSinceConnect;
};

class StandardUVConnector: public StandardConnector {
public:
	StandardUVConnector(std::string dfltHost="", std::string dfltPort="");
	virtual ~StandardUVConnector();
	virtual void SetConnectionAffinity(std::string host, int durationSec);
protected:
	UVWSConnection* wsConnection;
	UPCHTTPConnection* httpConnection;
};

#endif /* STANDARDCONNECTOR_H_ */
