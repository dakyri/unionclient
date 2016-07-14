/*
 * ConnectionMonitor.cpp
 *
 *  Created on: May 13, 2014
 *      Author: dak
 */

#include <uv.h>

#include <math.h>
#include <ctime>
#include <climits>
#include <cstdlib>
#include <random>

#include "UCUpperHeaders.h"

#include "UVEventLoop.h"

const UserMessageID ConnectionMonitor::kClientHeartbeat = "CLIENT_HEARTBEAT";
/**
 * @class ConnectionMonitor ConnectionMonitor.h
 * @brief Handles the heartbeat, connection timeout and reconnection logic
 */
ConnectionMonitor::ConnectionMonitor(ClientManager& clientManager, UnionBridge& bridge, ILogger& l)
	: clientManager(clientManager)
	, unionBridge(bridge)
	, loop(nullptr)
	, log(l)
	, readyTimeout(5000)
{
	DEBUG_OUT("ConnectionMonitor::ConnectionMonitor()");
}

void
ConnectionMonitor::AddSelfListeners()
{
	beginCnxListener = unionBridge.AddUPCListener(Event::BEGIN_CONNECT,
			[this](EventType t,  StringArgs a, UPCStatus status) {
		log.Debug("ConnectionMonitor::beginCnxListener()");
		StartReadyTimer();
	});

	readyListener = unionBridge.AddUPCListener(Event::READY,
			[this](EventType e, StringArgs args, UPCStatus s) {
		log.Debug("ConnectionMonitor::readyListener()");
		StartHeartbeat();
		CancelReadyTimer();
		StopReconnect();
		log.Debug("ConnectionMonitor::readyListener() done");
	});
	heartbeatMessageListener = unionBridge.AddMessageListener(kClientHeartbeat,
			[this] (UserMessageID mid, StringArgs a) {
		if (a.size() < 1) return;
		int id = atoi(a[0].c_str());
		int ping = (int)(std::time(nullptr) - heartbeats[id]);
		GetSelf()->SetAttribute("_PING", std_to_string(ping),"", sharedPing);
		heartbeats.erase(id);
	});
	closedListener = unionBridge.AddUPCListener(Event::CONNECT_FAILURE,
			[this](EventType e, StringArgs args, UPCStatus s) {
		log.Debug("ConnectionMonitor caught the disconnection");
		StopHeartbeat();
		if (unionBridge.GetConnectionState() == ConnectionState::DISCONNECTION_IN_PROGRESS) {
			return;
		}
		if (s == UPC::Status::NO_VALID_CONNECTION_AVAILABLE) {
			return;
		}
		int numAttempts = unionBridge.GetConnectAttemptCount();
		if (numAttempts == 0) {
			SelectReconnectFrequency();
		}
		log.Debug("scheduling reconnect frequency "+std_to_string(autoReconnectFrequency));
		if (autoReconnectFrequency > -1) {
			if (!autoReconnectTimeoutRef) {
				log.Debug("have no reco timer ...");
				if (!disposed && autoReconnectFrequency != -1) {
					log.Warn("[CONNECTION_MONITOR] Disconnection detected.");
					if (autoReconnectDelayFirstAttempt
							&& (	(numAttempts == 0)
									||	(numAttempts == 1 && unionBridge.GetReadyCount() == 0))) {
						ScheduleReconnect(autoReconnectFrequency);
					} else {
						if (!DoReconnect()) {
							unionBridge.NxConnectFailure("All reconnection possibilities exhausted", UPC::Status::NO_VALID_CONNECTION_AVAILABLE);
						}
					}
				}
			}
		}
	});

}

/**
 * clean up the mess ... most of the cleanup work is done by the timer loop which atm is stopped and deleted before we delete this (inevitably and unforunately)
 */
ConnectionMonitor::~ConnectionMonitor() {
	DEBUG_OUT("ConnectionMonitor::~ConnectionMonitor()");
	disposed = true;
// cleaning up these done by event loop
//	StopHeartbeat();
//	StopReconnect();

	DEBUG_OUT("ConnectionMonitor::~ConnectionMonitor() removing callbacks ...");
// remove the callbacks
	readyListener = nullptr;
	closedListener = nullptr;

// xxx the user1 code seems full of detritus like this ... but maybe it's used in a way I haven't got yet ... u7 seems non-existent ... but keeping an eye on it unless it does
//	unionBridge.RemoveMessageListener("u7", this.u7);
	DEBUG_OUT("ConnectionMonitor::~ConnectionMonitor() done");
}

ClientRef
ConnectionMonitor::GetSelf() const
{
	return clientManager.Self();
}

void
ConnectionMonitor::SharePing(bool share) const {
	sharedPing = share;
}

bool
ConnectionMonitor::IsPingShared() const {
	return sharedPing;
}

void
ConnectionMonitor::SetEventLoop(EventLoop *l)
{
	loop = l;
}


void
ConnectionMonitor::DisableHeartbeatLogging() const
{
	log.AddSuppressionTerm("<A>CLIENT_HEARTBEAT</A>");
	log.AddSuppressionTerm("<A>_PING</A>");
	log.AddSuppressionTerm("[_PING]");
	log.AddSuppressionTerm("<![CDATA[_PING]]>");
}

void
ConnectionMonitor::EnableHeartbeatLogging() const
{
	log.RemoveSuppressionTerm("<A>CLIENT_HEARTBEAT</A>");
	log.RemoveSuppressionTerm("<A>_PING</A>");
	log.RemoveSuppressionTerm("[_PING]");
	log.RemoveSuppressionTerm("<![CDATA[_PING]]>");
}

bool
ConnectionMonitor::HeartbeatIsEnabled() const
{
	return heartbeatEnabled;
}

void
ConnectionMonitor::EnableHeartbeat() const {
	log.Info("[CONNECTION_MONITOR] Heartbeat enabled.");
	heartbeatEnabled = true;
	StartHeartbeat();
}
void
ConnectionMonitor::DisableHeartbeat() const {
	log.Info("[CONNECTION_MONITOR] Heartbeat disabled.");
	heartbeatEnabled = false;
	StopHeartbeat();
}

void
ConnectionMonitor::StartHeartbeat() const {
	if (!heartbeatEnabled) {
		log.Info("[CONNECTION_MONITOR] Heartbeat is currently disabled. Ignoring start request.");
		return;
	} else {
		log.Info("[CONNECTION_MONITOR] Starting heartbeat.");
	}

	StopHeartbeat();
	heartbeats.clear();
	if (loop != nullptr) {
		heartbeatTimerRef = loop->Schedule(heartBeatFrequency, heartBeatFrequency, [this] () {
			Heartbeat();
		});
	}

}

void
ConnectionMonitor::StopHeartbeat() const {
	return;
	log.Debug("stopping heartbeat ");
	if (heartbeatTimerRef != nullptr) {
		if (loop != nullptr) {
			loop->CancelTimer(heartbeatTimerRef);
		}
		heartbeatTimerRef = nullptr;
	}
	heartbeats.clear();
	log.Debug("stopped heartbeat " );
}


void
ConnectionMonitor::Heartbeat() const
{
	log.Debug( "ConnectionMonitor::Heartbeat()");
	if (!unionBridge.IsReady()) {
		log.Info("[CONNECTION_MONITOR] Orbiter is not connected. Stopping heartbeat.");
		StopHeartbeat();
		return;
	}
	log.Debug("ConnectionMonitor::Heartbeat() IsReady!!");

	time_t timeSinceOldestHeartbeat;
	time_t now = std::time(nullptr);

	heartbeats[heartbeatCounter] = now;
	unionBridge.SendUPC("u2", {
			kClientHeartbeat,
			clientManager.Self()->GetClientID(),
			"",
			std_to_string(heartbeatCounter)
	});
	heartbeatCounter++;
	log.Debug("sent!" );

	// Assign the oldest heartbeat
	if (heartbeats.size() == 1) {
		oldestHeartbeat = now;
	} else {
		oldestHeartbeat = UINT_MAX;
		for (auto it: heartbeats) {
			if (it.second < oldestHeartbeat) {
				oldestHeartbeat = it.second;
			}
		}
	}
	// Close connection if too much time has passed since the last response
	timeSinceOldestHeartbeat = now - oldestHeartbeat;
	if (timeSinceOldestHeartbeat > connectionTimeout) {
		log.Warn("[CONNECTION_MONITOR] No response from server in " +
				std_to_string((int)timeSinceOldestHeartbeat) + "ms. Starting automatic disconnect.");
		unionBridge.Disconnect();
	}
	log.Debug("done");
}

void
ConnectionMonitor::RestoreDefaults() {
	SetAutoReconnectFrequency(kDefltARMinMS, kDefltARMaxMS, 0);
	SetAutoReconnectAttemptLimit(kDfltAutoreconnectAttemptLimit);
	SetConnectionTimeout(kDfltConnectionTimeout);
	SetHeartbeatFrequency(kDfltHeartbeatFrequency);
}

void
ConnectionMonitor::SetHeartbeatFrequency(int milliseconds) {
	if (milliseconds >= kDfltMinHeartbeatFrequency) {
		heartBeatFrequency = milliseconds;
		log.Info("[CONNECTION_MONITOR] Heartbeat frequency set to " + std_to_string((int)milliseconds) + " ms.");
		if (milliseconds >= kDfltMinHeartbeatFrequency && milliseconds < 1000) {
			log.Info("[CONNECTION_MONITOR] HEARTBEAT FREQUENCY WARNING: "
					+ std_to_string((int)milliseconds) + " ms. Current frequency will generate "
					+ std_to_string((float)floor((1000/milliseconds)*10)/10)
					+ " messages per second per connected client.");
		}

		if (unionBridge.IsReady()) {
			StartHeartbeat();
		}
	} else {
		log.Warn("[CONNECTION_MONITOR] Invalid heartbeat frequency specified: "
				+ std_to_string((int)milliseconds) + ". Frequency must be "
				+ std_to_string((int)kDfltMinHeartbeatFrequency) + " or greater.");
	}
}

int
ConnectionMonitor::GetHeartbeatFrequency() const {
	return heartBeatFrequency;
}

void
ConnectionMonitor::SetAutoReconnectFrequency(int minMS, int maxMS, bool delayFirstAttempt) {
	if (minMS == 0 || minMS < -1) {
		log.Warn("[CONNECTION_MONITOR] Invalid auto-reconnect minMS specified: ["
				+ std_to_string((int)minMS) + "]. Value must not be zero or less than -1. Value adjusted"
				+ " to [-1] (no reconnect).");
		minMS = -1;
	}
	if (minMS == -1) {
		StopReconnect();
	} else {
		if (maxMS == -1) {
			maxMS = minMS;
		}
		if (maxMS < minMS) {
			log.Warn("[CONNECTION_MONITOR] Invalid auto-reconnect maxMS specified: ["
					+ std_to_string((int)maxMS) + "]." + " Value of maxMS must be greater than or equal "
					+ "to minMS. Value adjusted to [" + std_to_string((int)minMS) + "].");
			maxMS = minMS;
		}
	}

	autoReconnectDelayFirstAttempt = delayFirstAttempt;
	autoReconnectMinMS = minMS;
	autoReconnectMaxMS = maxMS;

	log.Info("[CONNECTION_MONITOR] Assigning auto-reconnect frequency settings: [minMS: "
			+ std_to_string((int)minMS) + ", maxMS: " + std_to_string((int)maxMS) + ", delayFirstAttempt: "
			+ bool_to_string((int)delayFirstAttempt) + "].");
	if (minMS > 0 && minMS < 1000) {
		log.Info("[CONNECTION_MONITOR] RECONNECT FREQUENCY WARNING: "
				+ std_to_string((int)minMS) + " minMS specified. Current frequency will cause "
				+ std_to_string((float)floor((1000/minMS)*10)/10)
				+ " reconnection attempts per second.");
	}
	SelectReconnectFrequency();
}

/**
 * @return the ready timeout in msec
 */
int
ConnectionMonitor::GetReadyTimeout() const
{
	return readyTimeout;
}

/**
 * @param ms new timeout in msec
 */
void
ConnectionMonitor::SetReadyTimeout(const int ms) const
{
	readyTimeout = ms;
}

/**
 * starts a timer to see if something goes awol while we connect
 */
void
ConnectionMonitor::StartReadyTimer() const {
	CancelReadyTimer();
	if (readyTimeout <= 0) {
	log.Debug("timeout is <= 0");
		return;
	}
	if (loop != nullptr) {
		readyTimerRef = loop->Schedule((unsigned)readyTimeout, 0, [this] () {
			unionBridge.NxConnectFailure("Connection failed while waiting for ready", UPC::Status::CONNECT_TIMEOUT);
		});
	}
	log.Debug("started ready timer");

}

void
ConnectionMonitor::CancelReadyTimer() const {
	if (readyTimerRef != nullptr && loop != nullptr) {
		loop->CancelTimer(readyTimerRef);
		readyTimerRef = nullptr;
	}
	log.Debug("cancelled ready timer");

}

/**
 * @return the auto reconnect frequency in ms
 */
int
ConnectionMonitor::GetAutoReconnectFrequency() const {
  return autoReconnectFrequency;
}

/**
 * calaculates a reconnect rate
 */
void
ConnectionMonitor::SelectReconnectFrequency() const {
	if (autoReconnectMinMS == -1) {
		autoReconnectFrequency = -1;
	} else if (autoReconnectMinMS == autoReconnectMaxMS) {
		autoReconnectFrequency = autoReconnectMinMS;
	} else {
		std::mt19937 eng((std::random_device())());
		std::uniform_int_distribution<> randomInt(autoReconnectMinMS,autoReconnectMaxMS);
		autoReconnectFrequency = randomInt(eng);
		log.Info("[CONNECTION_MONITOR] Random auto-reconnect frequency selected: [" +
				std_to_string(autoReconnectFrequency) + "] ms.");
	}
}

void
ConnectionMonitor::SetAutoReconnectAttemptLimit(const int attempts) {
	if (attempts < -1 || attempts == 0) {
		log.Warn("[CONNECTION_MONITOR] Invalid Auto-reconnect attempt limit specified: "
				+ std_to_string(attempts) + ". Limit must -1 or greater than 1.");
		return;
	}

	autoReconnectAttemptLimit = attempts;

	if (attempts == -1) {
		log.Info("[CONNECTION_MONITOR] Auto-reconnect attempt limit set to none.");
	} else {
		log.Info("[CONNECTION_MONITOR] Auto-reconnect attempt limit set to "
				+ std_to_string(attempts) + " attempt(s).");
	}
}

int
ConnectionMonitor::GetAutoReconnectAttemptLimit() const {
  return autoReconnectAttemptLimit;
}

void
ConnectionMonitor::SetConnectionTimeout(const int milliseconds) {
	if (milliseconds > 0) {
		connectionTimeout = milliseconds;
		log.Info("[CONNECTION_MONITOR] Connection timeout set to "
				+ std_to_string(milliseconds) + " ms.");
	} else {
		log.Warn("[CONNECTION_MONITOR] Invalid connection timeout specified: "
				+ std_to_string(milliseconds) + ". Frequency must be greater "
				 "than zero.");
	}
}

int
ConnectionMonitor::GetConnectionTimeout() const {
	return connectionTimeout;
}

bool
ConnectionMonitor::DoReconnect() const {
	log.Debug("ConnectionMonitor::DoReconnect()");
	int numActualAttempts = unionBridge.GetConnectAttemptCount();
	int numReconnectAttempts;

	if (unionBridge.GetReadyCount() == 0) {
		numReconnectAttempts = numActualAttempts - 1;
	} else {
		numReconnectAttempts = numActualAttempts;
	}

	if (autoReconnectAttemptLimit != -1
			&& numReconnectAttempts > 0
			&& numReconnectAttempts % (autoReconnectAttemptLimit) == 0) {
		log.Warn("[CONNECTION_MONITOR] Automatic reconnect attempt limit reached."
				 " No further automatic connection attempts will be made until"
				 " the next manual connection attempt.");
		return false;
	}

	ScheduleReconnect(autoReconnectFrequency);

	log.Warn("[CONNECTION_MONITOR] Attempting automatic reconnect. (Next attempt in "
			+ std_to_string(autoReconnectFrequency) + "ms.)");
	unionBridge.Connect();
	return true;
}

void
ConnectionMonitor::StopReconnect() const {
	if (autoReconnectTimeoutRef != nullptr && loop != nullptr) {
		loop->CancelTimer(autoReconnectTimeoutRef);
		autoReconnectTimeoutRef = nullptr;
	}
}


void
ConnectionMonitor::ScheduleReconnect(const int milliseconds) const {
	StopReconnect();
	if (loop != nullptr) {
		log.Debug("scheduling reconnect");
		autoReconnectTimeoutRef = loop->Schedule(milliseconds, 0, [this]() {
			StopReconnect();
			if (unionBridge.GetConnectionState() == ConnectionState::NOT_CONNECTED) {
				DoReconnect();
			}
		});
	}
};


