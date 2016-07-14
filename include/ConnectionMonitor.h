/*
 * ConnectionMonitor.h
 *
 *  Created on: May 13, 2014
 *      Author: dak
 */

#ifndef CONNECTIONMONITOR_H_
#define CONNECTIONMONITOR_H_

#include "UVForwards.h"

class ConnectionMonitor {
public:
	ConnectionMonitor(ClientManager& clientManager, UnionBridge& bridge, ILogger& log);
	virtual ~ConnectionMonitor();

	bool HeartbeatIsEnabled() const;
	void DisableHeartbeat() const;
	void EnableHeartbeat() const;
	void DisableHeartbeatLogging() const;
	void EnableHeartbeatLogging() const;

	bool IsPingShared() const;
	void SharePing(bool share) const;

	int GetAutoReconnectAttemptLimit() const;
	int GetAutoReconnectFrequency() const;
	int GetConnectionTimeout() const;
	void SetAutoReconnectAttemptLimit(const int attempts);
	void SetAutoReconnectFrequency(int minMS, int maxMS = -1, bool delayFirstAttempt = false);
	void SetConnectionTimeout(const int milliseconds);

	int GetHeartbeatFrequency() const;
	void SetHeartbeatFrequency(const int ms);

	int GetReadyTimeout() const;
	void SetReadyTimeout(const int ms) const;

	void HeartbeatMessageListener(UserMessageID mid, StringArgs a);

	void RestoreDefaults();

	ILogger &GetLog();
	void SetLog(ILogger&logger);
	void SetEventLoop(EventLoop *l);

	void Heartbeat() const;

	ClientRef GetSelf() const; // the main reason we need the clientManager referred in this class

	static const UserMessageID kClientHeartbeat;

	static const int kDfltHeartbeatFrequency = 10000;
	static const int kDfltMinHeartbeatFrequency = 20;
	static const int kDfltAutoreconnectFrequency = 1500;
	static const int kDfltAutoreconnectAttemptLimit = 10;
	static const int kDfltConnectionTimeout = 60000;
	static const int kDefltARMinMS = 1500;
	static const int kDefltARMaxMS = 1600;

	void AddSelfListeners();
protected:
	void StartHeartbeat() const;
	void StopHeartbeat() const;
	void StartReadyTimer() const;
	void CancelReadyTimer() const;
	void SelectReconnectFrequency() const;
	void StopReconnect() const;
	void ScheduleReconnect(const int milliseconds) const;
	bool DoReconnect() const;

	bool mutable heartbeatEnabled = false;
	int mutable heartbeatCounter = 0;
	bool mutable sharedPing = false;
	bool mutable disposed = false;
	time_t mutable oldestHeartbeat = 0;

	TimerRef mutable autoReconnectTimeoutRef = nullptr;
	TimerRef mutable heartbeatTimerRef = nullptr;
	TimerRef mutable readyTimerRef = nullptr;

	std::unordered_map<int,time_t> mutable heartbeats;

	int mutable heartBeatFrequency = kDfltHeartbeatFrequency;
	int mutable autoReconnectFrequency = kDfltAutoreconnectFrequency;
	int mutable autoReconnectAttemptLimit = kDfltAutoreconnectAttemptLimit;
	int mutable connectionTimeout = kDfltConnectionTimeout;
	int mutable autoReconnectMinMS = kDefltARMinMS;
	int mutable autoReconnectMaxMS = kDefltARMaxMS;
	bool mutable autoReconnectDelayFirstAttempt = false;

	CBUPCRef readyListener;
	CBUPCRef closedListener;
	CBUPCRef beginCnxListener;
	CBModMsgRef heartbeatMessageListener;

	ClientManager& clientManager;
	UnionBridge& unionBridge;
	EventLoop* loop;
	ILogger& log;

	int mutable readyTimeout;
};

#endif /* CONNECTIONMONITOR_H_ */
