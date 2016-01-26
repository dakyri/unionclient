/*
 * UnionClient.h
 *
 *  Created on: Apr 7, 2014
 *      Author: dak
 */

#ifndef UNIONCLIENT_H_
#define UNIONCLIENT_H_

#include "UCUpperHeaders.h"
#include "connector/AbstractConnector.h"

class UnionClient: public NotifyStatusMessage {
public:
	UnionClient(AbstractConnector &c);
	virtual ~UnionClient();

	void SetConnector(const AbstractConnector &c);
	AbstractConnector &GetConnector() const;
	ILogger &GetLog();
	DefaultLogger &GetDefaultLog();
	void SetLog(ILogger&logger);

	Version GetServerUPCVersion() const;

	void DisableHeartbeat();
	void EnableHeartbeat();

//	void DisableHeartbeatLogging();
//	void EnableHeartbeatLogging();

	int GetAutoReconnectAttemptLimit() const;
	int GetAutoReconnectFrequency() const;
	int GetConnectionTimeout() const;
	int GetHeartbeatFrequency() const;
	void SetAutoReconnectAttemptLimit(const int attempts);
	void SetAutoReconnectFrequency(const int minMS, const int maxMS = -1, bool const delayFirstAttempt = false);
	void SetConnectionTimeout(const int milliseconds);
	void SetHeartbeatFrequency(const int ms);

	bool IsReady() const;
	int GetReadyCount() const;
	int GetReadyTimeout() const;
	void SetReadyTimeout(const int ms);

	int GetConnectAbortCount() const;
	int GetConnectAttemptCount() const;
	int GetConnectFailedCount() const;
	int GetConnectionState() const;

	CnxRef GetActiveConnection();
	CnxRef GetInProgressConnection();

	std::string GetAffinity(const std::string host) const;
	void SetGlobalAffinity(const bool enabled);

	RoomManager& GetRoomManager();
	AccountManager& GetAccountManager();
	ClientManager& GetClientManager();
	ConnectionMonitor& GetConnectionMonitor();
	UnionBridge& GetUnionBridge();

	ClientRef Self() const;

	void Connect();
	void Disconnect();
protected:

	DefaultLogger defaultLogger;
	ILogger& log;

	RoomManager	roomManager;
	ClientManager clientManager;
	AccountManager accountManager;
	UnionBridge unionBridge;
	ConnectionMonitor connectionMonitor;

	CBUPCRef readyListener;
	CBUPCRef disconnectListener;
	CBUPCRef selectListener;
	CBUPCRef cnxFailListener;
	CBUPCRef echoListener;
};
#endif /* UNIONCLIENT_H_ */

