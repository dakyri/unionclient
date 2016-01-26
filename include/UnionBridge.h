/*
 * UnionBridge.h
 *
 *  Created on: Apr 9, 2014
 *      Author: dak
 *
 * merged classes of MessageManager and Server from original client code
 */

#ifndef UnionBridge_H_
#define UnionBridge_H_

class UnionBridge: protected DispatchUPC, public MessageNotifier {
public:
	UnionBridge(AbstractConnector& c, RoomManager& roomManager, ClientManager& clientManager, AccountManager& accountManager, ILogger& log);
	virtual ~UnionBridge();

	void Connect();
	void Disconnect();

	int GetConnectAttemptCount() const;
	int GetConnectFailedCount() const;

	int GetConnectionState() const;

	bool IsReady() const;
	int GetReadyCount() const;

	void SetConnector(const AbstractConnector &c);

	bool AddUPCListener(const EventType eventType, const CBUPCRef listener) const;
	CBUPCRef AddUPCListener(const EventType eventType, const CBUPC listener) const;
	bool RemoveUPCListener(const EventType eventType, const CBUPCRef listener) const;

	void RemoveMessageListenersOnDisconnect(bool enabled);
	bool GetRemoveMessageListenersOnDisconnect();

	int GetNumMessagesReceived() const;
	int GetNumMessagesSent() const;
	int GetTotalMessages() const;

	void SendUPC(UPCMessageID messageID, StringArgs args);

/* from Server class */
	Version GetUPCVersion() const;
	Version GetClientVersion() const;
	std::string GetClientType() const;

	void SetClientVersion(Version v) const;
	void SetUPCVersion(Version v) const;
	void SetClientType(std::string) const;
	void SetUserAgentString(const std::string) const;
	void SetServerVersion(Version v);

	time_t GetServerTime();

	void ResetUPCStats();
	void ClearModuleCache();
	void SyncTime();

	bool IsWatchingForProcessedUPCs();
	void WatchForProcessedUPCs();
	void StopWatchingForProcessedUPCs();
	void SetIsWatchingForProcessedUPCs(bool value);

	/* from ConnectionManager class */
	void SendHelloMessage();
	/** TODO from original Client
		virtual int GetConnectionState() = 0;
		virtual float GetConnectTime() = 0;
		virtual float GetTimeOnline() = 0;
		virtual int GetPing() = 0;
		virtual std::string GetIP() = 0;
	*/
	ILogger &GetLog();
	void SetLog(ILogger&logger);
	void SetEventLoop(EventLoop *l);
	void SetQueueNotifications(bool);
	int GetAvailableConnectionCount();
	void OnReconnectFailure();
	void NxConnectFailure(const std::string msg, const UPCStatus);
protected:
	void AddSelfUPCMessageListeners(const NXUPC& notifier, const bool incoming, const bool outgoing, const bool handshake);
	void RemoveSelfUPCMessageListeners(const NXUPC& notifier, const bool incoming, const bool outgoing, const bool handshake);

	void AddSelfConnectionListeners(const NXConnection& notifier);
	void RemoveSelfConnectionListeners(const NXConnection& notifier);

	void SetConnectionState(int);

	void UpcReceivedListener(EventType t, CnxRef cnx, std::string upc, ConnectionStatus status);
	void UpcSentListener(EventType t, CnxRef cnx, std::string upc, ConnectionStatus status);
	void DisconnectListener(EventType t, CnxRef cnx, std::string, ConnectionStatus status);
	void ConnectListener(EventType t, CnxRef cnx, std::string, ConnectionStatus status);
	void SelectListener(EventType t, CnxRef cnx, std::string, ConnectionStatus status);
	void IOErrorListener(EventType t, CnxRef cnx, std::string, ConnectionStatus status);
	void ConnectFailureListener(EventType t, CnxRef cnx, std::string, ConnectionStatus status);
	void CleanupClosedConnection();

	void NxBeginConnect();
	void NxBeginHandshake();
	void NxDisconnected();
	void NxConnectionStateChange();
	void NxConnectRefused(const std::string reason, const std::string description);
	void NxProtocolIncompatible(const std::string version);
	void NxReady();
	void NxSendData(const std::string data);
	void NxUPCMethod(const int method, const StringArgs upcArgs);
	void NxReceiveData(const std::string data);
	void NxIOError(std::string err, UPCStatus status);

	void U1(EventType t, std::vector<std::string> args, UPCStatus status); /* SEND_MESSAGE_TO_ROOMS */
	void U2(EventType t, std::vector<std::string> args, UPCStatus status); /* SEND_MESSAGE_TO_CLIENTS */
	void U3(EventType t, std::vector<std::string> args, UPCStatus status); /* SET_CLIENT_ATTR */
	void U4(EventType t, std::vector<std::string> args, UPCStatus status); /* JOIN_ROOM */
	void U5(EventType t, std::vector<std::string> args, UPCStatus status); /* SET_ROOM_ATTR */
	void U6(EventType t, std::vector<std::string> args, UPCStatus status); /* JOINED_ROOM */
	void U7(EventType t, std::vector<std::string> args, UPCStatus status); /* RECEIVE_MESSAGE */
	void U8(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_ATTR_UPDATE */
	void U9(EventType t, std::vector<std::string> args, UPCStatus status); /* ROOM_ATTR_UPDATE */
	void U10(EventType t, std::vector<std::string> args, UPCStatus status); /* LEAVE_ROOM */
	void U11(EventType t, std::vector<std::string> args, UPCStatus status); /* CREATE_ACCOUNT */
	void U12(EventType t, std::vector<std::string> args, UPCStatus status); /* REMOVE_ACCOUNT */
	void U13(EventType t, std::vector<std::string> args, UPCStatus status); /* CHANGE_ACCOUNT_PASSWORD */
	void U14(EventType t, std::vector<std::string> args, UPCStatus status); /* LOGIN */
	void U18(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_CLIENTCOUNT_SNAPSHOT */
	void U19(EventType t, std::vector<std::string> args, UPCStatus status); /* SYNC_TIME */
	void U21(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_ROOMLIST_SNAPSHOT */
	void U32(EventType t, std::vector<std::string> args, UPCStatus status); /* CREATE_ROOM_RESULT */
	void U43(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_WATCHING_FOR_ROOMS_RESULT */
	void U24(EventType t, std::vector<std::string> args, UPCStatus status); /* CREATE_ROOM */
	void U25(EventType t, std::vector<std::string> args, UPCStatus status); /* REMOVE_ROOM */
	void U29(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_METADATA */
	void U26(EventType t, std::vector<std::string> args, UPCStatus status); /* WATCH_FOR_ROOMS */
	void U27(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_WATCHING_FOR_ROOMS */
	void U33(EventType t, std::vector<std::string> args, UPCStatus status); /* REMOVE_ROOM_RESULT */
	void U34(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENTCOUNT_SNAPSHOT */
	void U36(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_ADDED_TO_ROOM */
	void U37(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_REMOVED_FROM_ROOM */
	void U38(EventType t, std::vector<std::string> args, UPCStatus status); /* ROOMLIST_SNAPSHOT */
	void U39(EventType t, std::vector<std::string> args, UPCStatus status); /* ROOM_ADDED */
	void U40(EventType t, std::vector<std::string> args, UPCStatus status); /* ROOM_REMOVED */
	void U42(EventType t, std::vector<std::string> args, UPCStatus status); /* WATCH_FOR_ROOMS_RESULT */
	void U44(EventType t, std::vector<std::string> args, UPCStatus status); /* LEFT_ROOM */
	void U46(EventType t, std::vector<std::string> args, UPCStatus status); /* CHANGE_ACCOUNT_PASSWORD_RESULT */
	void U47(EventType t, std::vector<std::string> args, UPCStatus status); /* CREATE_ACCOUNT_RESULT */
	void U48(EventType t, std::vector<std::string> args, UPCStatus status); /* REMOVE_ACCOUNT_RESULT */
	void U49(EventType t, std::vector<std::string> args, UPCStatus status); /* LOGIN_RESULT */
	void U50(EventType t, std::vector<std::string> args, UPCStatus status); /* SERVER_TIME_UPDATE */
	void U54(EventType t, std::vector<std::string> args, UPCStatus status); /* ROOM_SNAPSHOT */
	void U55(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_ROOM_SNAPSHOT */
	void U57(EventType t, std::vector<std::string> args, UPCStatus status); /* SEND_MESSAGE_TO_SERVER */
	void U58(EventType t, std::vector<std::string> args, UPCStatus status); /* OBSERVE_ROOM */
	void U59(EventType t, std::vector<std::string> args, UPCStatus status); /* OBSERVED_ROOM */
	void U60(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_ROOM_SNAPSHOT_RESULT */
	void U61(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_OBSERVING_ROOM */
	void U62(EventType t, std::vector<std::string> args, UPCStatus status); /* STOPPED_OBSERVING_ROOM */
	void U63(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_READY */
	void U64(EventType t, std::vector<std::string> args, UPCStatus status); /* SET_ROOM_UPDATE_LEVELS */
	void U65(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_HELLO */
	void U66(EventType t, std::vector<std::string> args, UPCStatus status); /* SERVER_HELLO */
	void U67(EventType t, std::vector<std::string> args, UPCStatus status); /* REMOVE_ROOM_ATTR */
	void U69(EventType t, std::vector<std::string> args, UPCStatus status); /* REMOVE_CLIENT_ATTR */
	void U70(EventType t, std::vector<std::string> args, UPCStatus status); /* SEND_ROOMMODULE_MESSAGE */
	void U71(EventType t, std::vector<std::string> args, UPCStatus status); /* SEND_SERVERMODULE_MESSAGE */
	void U72(EventType t, std::vector<std::string> args, UPCStatus status); /* JOIN_ROOM_RESULT */
	void U73(EventType t, std::vector<std::string> args, UPCStatus status); /* SET_CLIENT_ATTR_RESULT */
	void U74(EventType t, std::vector<std::string> args, UPCStatus status); /* SET_ROOM_ATTR_RESULT */
	void U75(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_CLIENTCOUNT_SNAPSHOT_RESULT */
	void U76(EventType t, std::vector<std::string> args, UPCStatus status); /* LEAVE_ROOM_RESULT */
	void U77(EventType t, std::vector<std::string> args, UPCStatus status); /* OBSERVE_ROOM_RESULT */
	void U78(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_OBSERVING_ROOM_RESULT */
	void U79(EventType t, std::vector<std::string> args, UPCStatus status); /* ROOM_ATTR_REMOVED */
	void U80(EventType t, std::vector<std::string> args, UPCStatus status); /* REMOVE_ROOM_ATTR_RESULT */
	void U81(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_ATTR_REMOVED */
	void U82(EventType t, std::vector<std::string> args, UPCStatus status); /* REMOVE_CLIENT_ATTR_RESULT */
	void U83(EventType t, std::vector<std::string> args, UPCStatus status); /* TERMINATE_SESSION */
	void U84(EventType t, std::vector<std::string> args, UPCStatus status); /* SESSION_TERMINATED */
	void U85(EventType t, std::vector<std::string> args, UPCStatus status); /* SESSION_NOT_FOUND */
	void U86(EventType t, std::vector<std::string> args, UPCStatus status); /* LOGOFF */
	void U87(EventType t, std::vector<std::string> args, UPCStatus status); /* LOGOFF_RESULT */
	void U88(EventType t, std::vector<std::string> args, UPCStatus status); /* LOGGED_IN */
	void U89(EventType t, std::vector<std::string> args, UPCStatus status); /* LOGGED_OFF */
	void U90(EventType t, std::vector<std::string> args, UPCStatus status); /* ACCOUNT_PASSWORD_CHANGED */
	void U91(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_CLIENTLIST_SNAPSHOT */
	void U92(EventType t, std::vector<std::string> args, UPCStatus status); /* WATCH_FOR_CLIENTS */
	void U93(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_WATCHING_FOR_CLIENTS */
	void U94(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_CLIENT_SNAPSHOT */
	void U95(EventType t, std::vector<std::string> args, UPCStatus status); /* OBSERVE_CLIENT */
	void U96(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_OBSERVING_CLIENT */
	void U97(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_ACCOUNTLIST_SNAPSHOT */
	void U98(EventType t, std::vector<std::string> args, UPCStatus status); /* WATCH_FOR_ACCOUNTS */
	void U99(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_WATCHING_FOR_ACCOUNTS */
	void U100(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_ACCOUNT_SNAPSHOT */
	void U101(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENTLIST_SNAPSHOT */
	void U102(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_ADDED_TO_SERVER */
	void U103(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_REMOVED_FROM_SERVER */
	void U104(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_SNAPSHOT */
	void U105(EventType t, std::vector<std::string> args, UPCStatus status); /* OBSERVE_CLIENT_RESULT */
	void U106(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_OBSERVING_CLIENT_RESULT */
	void U107(EventType t, std::vector<std::string> args, UPCStatus status); /* WATCH_FOR_CLIENTS_RESULT */
	void U108(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_WATCHING_FOR_CLIENTS_RESULT */
	void U109(EventType t, std::vector<std::string> args, UPCStatus status); /* WATCH_FOR_ACCOUNTS_RESULT */
	void U110(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_WATCHING_FOR_ACCOUNTS_RESULT */
	void U111(EventType t, std::vector<std::string> args, UPCStatus status); /* ACCOUNT_ADDED */
	void U112(EventType t, std::vector<std::string> args, UPCStatus status); /* ACCOUNT_REMOVED */
	void U113(EventType t, std::vector<std::string> args, UPCStatus status); /* JOINED_ROOM_ADDED_TO_CLIENT */
	void U114(EventType t, std::vector<std::string> args, UPCStatus status); /* JOINED_ROOM_REMOVED_FROM_CLIENT */
	void U115(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_CLIENT_SNAPSHOT_RESULT */
	void U116(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_ACCOUNT_SNAPSHOT_RESULT */
	void U117(EventType t, std::vector<std::string> args, UPCStatus status); /* OBSERVED_ROOM_ADDED_TO_CLIENT */
	void U118(EventType t, std::vector<std::string> args, UPCStatus status); /* OBSERVED_ROOM_REMOVED_FROM_CLIENT */
	void U119(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_OBSERVED */
	void U121(EventType t, std::vector<std::string> args, UPCStatus status); /* OBSERVE_ACCOUNT */
	void U122(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_OBSERVING_ACCOUNT */
	void U120(EventType t, std::vector<std::string> args, UPCStatus status); /* STOPPED_OBSERVING_CLIENT */
	void U123(EventType t, std::vector<std::string> args, UPCStatus status); /* OBSERVE_ACCOUNT_RESULT */
	void U124(EventType t, std::vector<std::string> args, UPCStatus status); /* ACCOUNT_OBSERVED */
	void U125(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_OBSERVING_ACCOUNT_RESULT */
	void U126(EventType t, std::vector<std::string> args, UPCStatus status); /* STOPPED_OBSERVING_ACCOUNT */
	void U127(EventType t, std::vector<std::string> args, UPCStatus status); /* ACCOUNT_LIST_UPDATE */
	void U128(EventType t, std::vector<std::string> args, UPCStatus status); /* UPDATE_LEVELS_UPDATE */
	void U129(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_OBSERVED_ROOM */
	void U130(EventType t, std::vector<std::string> args, UPCStatus status); /* CLIENT_STOPPED_OBSERVING_ROOM */
	void U131(EventType t, std::vector<std::string> args, UPCStatus status); /* ROOM_OCCUPANTCOUNT_UPDATE */
	void U132(EventType t, std::vector<std::string> args, UPCStatus status); /* ROOM_OBSERVERCOUNT_UPDATE */
	void U133(EventType t, std::vector<std::string> args, UPCStatus status); /* ADD_ROLE */
	void U134(EventType t, std::vector<std::string> args, UPCStatus status); /* ADD_ROLE_RESULT */
	void U135(EventType t, std::vector<std::string> args, UPCStatus status); /* REMOVE_ROLE */
	void U136(EventType t, std::vector<std::string> args, UPCStatus status); /* REMOVE_ROLE_RESULT */
	void U137(EventType t, std::vector<std::string> args, UPCStatus status); /* BAN */
	void U138(EventType t, std::vector<std::string> args, UPCStatus status); /* BAN_RESULT */
	void U139(EventType t, std::vector<std::string> args, UPCStatus status); /* UNBAN */
	void U140(EventType t, std::vector<std::string> args, UPCStatus status); /* UNBAN_RESULT */
	void U141(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_BANNED_LIST_SNAPSHOT */
	void U142(EventType t, std::vector<std::string> args, UPCStatus status); /* BANNED_LIST_SNAPSHOT */
	void U143(EventType t, std::vector<std::string> args, UPCStatus status); /* WATCH_FOR_BANNED_ADDRESSES */
	void U144(EventType t, std::vector<std::string> args, UPCStatus status); /* WATCH_FOR_BANNED_ADDRESSES_RESULT */
	void U145(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_WATCHING_FOR_BANNED_ADDRESSES */
	void U146(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_WATCHING_FOR_BANNED_ADDRESSES_RESULT */
	void U147(EventType t, std::vector<std::string> args, UPCStatus status); /* BANNED_ADDRESS_ADDED */
	void U148(EventType t, std::vector<std::string> args, UPCStatus status); /* BANNED_ADDRESS_REMOVED */
	void U149(EventType t, std::vector<std::string> args, UPCStatus status); /* KICK_CLIENT */
	void U150(EventType t, std::vector<std::string> args, UPCStatus status); /* KICK_CLIENT_RESULT */
	void U151(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_SERVERMODULELIST_SNAPSHOT */
	void U152(EventType t, std::vector<std::string> args, UPCStatus status); /* SERVERMODULELIST_SNAPSHOT */
	void U154(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_UPC_STATS_SNAPSHOT */
	void U155(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_UPC_STATS_SNAPSHOT_RESULT */
	void U156(EventType t, std::vector<std::string> args, UPCStatus status); /* UPC_STATS_SNAPSHOT */
	void U157(EventType t, std::vector<std::string> args, UPCStatus status); /* RESET_UPC_STATS */
	void U160(EventType t, std::vector<std::string> args, UPCStatus status); /* WATCH_FOR_PROCESSED_UPCS_RESULT */
	void U164(EventType t, std::vector<std::string> args, UPCStatus status); /* CONNECTION_REFUSED */
	void U165(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_NODELIST_SNAPSHOT */
	void U153(EventType t, std::vector<std::string> args, UPCStatus status); /* CLEAR_MODULE_CACHE */
	void U158(EventType t, std::vector<std::string> args, UPCStatus status); /* RESET_UPC_STATS_RESULT */
	void U159(EventType t, std::vector<std::string> args, UPCStatus status); /* WATCH_FOR_PROCESSED_UPCS */
	void U161(EventType t, std::vector<std::string> args, UPCStatus status); /* PROCESSED_UPC_ADDED */
	void U162(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_WATCHING_FOR_PROCESSED_UPCS */
	void U163(EventType t, std::vector<std::string> args, UPCStatus status); /* STOP_WATCHING_FOR_PROCESSED_UPCS_RESULT */
	void U166(EventType t, std::vector<std::string> args, UPCStatus status); /* NODELIST_SNAPSHOT */
	void U167(EventType t, std::vector<std::string> args, UPCStatus status); /* GET_GATEWAYS_SNAPSHOT */
	void U168(EventType t, std::vector<std::string> args, UPCStatus status); /* GATEWAYS_SNAPSHOT */


	bool removeListenersOnDisconnect = true;
	int numMessagesSent = 0;
	int numMessagesReceived = 0;

	int connectionState = ConnectionState::UNKNOWN;
	int readyCount = 0;
	bool mostRecentConnectAchievedReady = false;
	int connectAttemptCount = 0;
	int connectFailCount = 0;
	bool queueNotifications;

	Version mutable serverVersion;
	Version mutable clientVersion;
	Version mutable upcVersion;
	std::string mutable clientType;
	std::string mutable userAgentString;
	std::string mutable sessionID;

	RoomManager& roomManager;
	ClientManager& clientManager;
	AccountManager& accountManager;
	AbstractConnector& connector;
	EventLoop* loop;
	ILogger& log;

	CBConnectionRef rxUPCListener;
	CBConnectionRef txUPCListener;
	CBConnectionRef disconnectListener;
	CBConnectionRef connectListener;
	CBConnectionRef selectListener;
	CBConnectionRef ioErrorListener;
	CBConnectionRef connectFailureListener;

	CBUPCRef u1Listener; /* SEND_MESSAGE_TO_ROOMS */
	CBUPCRef u2Listener; /* SEND_MESSAGE_TO_CLIENTS */
	CBUPCRef u3Listener; /* SET_CLIENT_ATTR */
	CBUPCRef u4Listener; /* JOIN_ROOM */
	CBUPCRef u5Listener; /* SET_ROOM_ATTR */
	CBUPCRef u6Listener; /* JOINED_ROOM */
	CBUPCRef u7Listener; /* RECEIVE_MESSAGE */
	CBUPCRef u8Listener; /* CLIENT_ATTR_UPDATE */
	CBUPCRef u9Listener; /* ROOM_ATTR_UPDATE */
	CBUPCRef u10Listener; /* LEAVE_ROOM */
	CBUPCRef u11Listener; /* CREATE_ACCOUNT */
	CBUPCRef u12Listener; /* REMOVE_ACCOUNT */
	CBUPCRef u13Listener; /* CHANGE_ACCOUNT_PASSWORD */
	CBUPCRef u14Listener; /* LOGIN */
	CBUPCRef u18Listener; /* GET_CLIENTCOUNT_SNAPSHOT */
	CBUPCRef u19Listener; /* SYNC_TIME */
	CBUPCRef u21Listener; /* GET_ROOMLIST_SNAPSHOT */
	CBUPCRef u32Listener; /* CREATE_ROOM_RESULT */
	CBUPCRef u43Listener; /* STOP_WATCHING_FOR_ROOMS_RESULT */
	CBUPCRef u24Listener; /* CREATE_ROOM */
	CBUPCRef u25Listener; /* REMOVE_ROOM */
	CBUPCRef u29Listener; /* CLIENT_METADATA */
	CBUPCRef u26Listener; /* WATCH_FOR_ROOMS */
	CBUPCRef u27Listener; /* STOP_WATCHING_FOR_ROOMS */
	CBUPCRef u33Listener; /* REMOVE_ROOM_RESULT */
	CBUPCRef u34Listener; /* CLIENTCOUNT_SNAPSHOT */
	CBUPCRef u36Listener; /* CLIENT_ADDED_TO_ROOM */
	CBUPCRef u37Listener; /* CLIENT_REMOVED_FROM_ROOM */
	CBUPCRef u38Listener; /* ROOMLIST_SNAPSHOT */
	CBUPCRef u39Listener; /* ROOM_ADDED */
	CBUPCRef u40Listener; /* ROOM_REMOVED */
	CBUPCRef u42Listener; /* WATCH_FOR_ROOMS_RESULT */
	CBUPCRef u44Listener; /* LEFT_ROOM */
	CBUPCRef u46Listener; /* CHANGE_ACCOUNT_PASSWORD_RESULT */
	CBUPCRef u47Listener; /* CREATE_ACCOUNT_RESULT */
	CBUPCRef u48Listener; /* REMOVE_ACCOUNT_RESULT */
	CBUPCRef u49Listener; /* LOGIN_RESULT */
	CBUPCRef u50Listener; /* SERVER_TIME_UPDATE */
	CBUPCRef u54Listener; /* ROOM_SNAPSHOT */
	CBUPCRef u55Listener; /* GET_ROOM_SNAPSHOT */
	CBUPCRef u57Listener; /* SEND_MESSAGE_TO_SERVER */
	CBUPCRef u58Listener; /* OBSERVE_ROOM */
	CBUPCRef u59Listener; /* OBSERVED_ROOM */
	CBUPCRef u60Listener; /* GET_ROOM_SNAPSHOT_RESULT */
	CBUPCRef u61Listener; /* STOP_OBSERVING_ROOM */
	CBUPCRef u62Listener; /* STOPPED_OBSERVING_ROOM */
	CBUPCRef u63Listener; /* CLIENT_READY */
	CBUPCRef u64Listener; /* SET_ROOM_UPDATE_LEVELS */
	CBUPCRef u65Listener; /* CLIENT_HELLO */
	CBUPCRef u66Listener; /* SERVER_HELLO */
	CBUPCRef u67Listener; /* REMOVE_ROOM_ATTR */
	CBUPCRef u69Listener; /* REMOVE_CLIENT_ATTR */
	CBUPCRef u70Listener; /* SEND_ROOMMODULE_MESSAGE */
	CBUPCRef u71Listener; /* SEND_SERVERMODULE_MESSAGE */
	CBUPCRef u72Listener; /* JOIN_ROOM_RESULT */
	CBUPCRef u73Listener; /* SET_CLIENT_ATTR_RESULT */
	CBUPCRef u74Listener; /* SET_ROOM_ATTR_RESULT */
	CBUPCRef u75Listener; /* GET_CLIENTCOUNT_SNAPSHOT_RESULT */
	CBUPCRef u76Listener; /* LEAVE_ROOM_RESULT */
	CBUPCRef u77Listener; /* OBSERVE_ROOM_RESULT */
	CBUPCRef u78Listener; /* STOP_OBSERVING_ROOM_RESULT */
	CBUPCRef u79Listener; /* ROOM_ATTR_REMOVED */
	CBUPCRef u80Listener; /* REMOVE_ROOM_ATTR_RESULT */
	CBUPCRef u81Listener; /* CLIENT_ATTR_REMOVED */
	CBUPCRef u82Listener; /* REMOVE_CLIENT_ATTR_RESULT */
	CBUPCRef u83Listener; /* TERMINATE_SESSION */
	CBUPCRef u84Listener; /* SESSION_TERMINATED */
	CBUPCRef u85Listener; /* SESSION_NOT_FOUND */
	CBUPCRef u86Listener; /* LOGOFF */
	CBUPCRef u87Listener; /* LOGOFF_RESULT */
	CBUPCRef u88Listener; /* LOGGED_IN */
	CBUPCRef u89Listener; /* LOGGED_OFF */
	CBUPCRef u90Listener; /* ACCOUNT_PASSWORD_CHANGED */
	CBUPCRef u91Listener; /* GET_CLIENTLIST_SNAPSHOT */
	CBUPCRef u92Listener; /* WATCH_FOR_CLIENTS */
	CBUPCRef u93Listener; /* STOP_WATCHING_FOR_CLIENTS */
	CBUPCRef u94Listener; /* GET_CLIENT_SNAPSHOT */
	CBUPCRef u95Listener; /* OBSERVE_CLIENT */
	CBUPCRef u96Listener; /* STOP_OBSERVING_CLIENT */
	CBUPCRef u97Listener; /* GET_ACCOUNTLIST_SNAPSHOT */
	CBUPCRef u98Listener; /* WATCH_FOR_ACCOUNTS */
	CBUPCRef u99Listener; /* STOP_WATCHING_FOR_ACCOUNTS */
	CBUPCRef u100Listener; /* GET_ACCOUNT_SNAPSHOT */
	CBUPCRef u101Listener; /* CLIENTLIST_SNAPSHOT */
	CBUPCRef u102Listener; /* CLIENT_ADDED_TO_SERVER */
	CBUPCRef u103Listener; /* CLIENT_REMOVED_FROM_SERVER */
	CBUPCRef u104Listener; /* CLIENT_SNAPSHOT */
	CBUPCRef u105Listener; /* OBSERVE_CLIENT_RESULT */
	CBUPCRef u106Listener; /* STOP_OBSERVING_CLIENT_RESULT */
	CBUPCRef u107Listener; /* WATCH_FOR_CLIENTS_RESULT */
	CBUPCRef u108Listener; /* STOP_WATCHING_FOR_CLIENTS_RESULT */
	CBUPCRef u109Listener; /* WATCH_FOR_ACCOUNTS_RESULT */
	CBUPCRef u110Listener; /* STOP_WATCHING_FOR_ACCOUNTS_RESULT */
	CBUPCRef u111Listener; /* ACCOUNT_ADDED */
	CBUPCRef u112Listener; /* ACCOUNT_REMOVED */
	CBUPCRef u113Listener; /* JOINED_ROOM_ADDED_TO_CLIENT */
	CBUPCRef u114Listener; /* JOINED_ROOM_REMOVED_FROM_CLIENT */
	CBUPCRef u115Listener; /* GET_CLIENT_SNAPSHOT_RESULT */
	CBUPCRef u116Listener; /* GET_ACCOUNT_SNAPSHOT_RESULT */
	CBUPCRef u117Listener; /* OBSERVED_ROOM_ADDED_TO_CLIENT */
	CBUPCRef u118Listener; /* OBSERVED_ROOM_REMOVED_FROM_CLIENT */
	CBUPCRef u119Listener; /* CLIENT_OBSERVED */
	CBUPCRef u121Listener; /* OBSERVE_ACCOUNT */
	CBUPCRef u122Listener; /* STOP_OBSERVING_ACCOUNT */
	CBUPCRef u120Listener; /* STOPPED_OBSERVING_CLIENT */
	CBUPCRef u123Listener; /* OBSERVE_ACCOUNT_RESULT */
	CBUPCRef u124Listener; /* ACCOUNT_OBSERVED */
	CBUPCRef u125Listener; /* STOP_OBSERVING_ACCOUNT_RESULT */
	CBUPCRef u126Listener; /* STOPPED_OBSERVING_ACCOUNT */
	CBUPCRef u127Listener; /* ACCOUNT_LIST_UPDATE */
	CBUPCRef u128Listener; /* UPDATE_LEVELS_UPDATE */
	CBUPCRef u129Listener; /* CLIENT_OBSERVED_ROOM */
	CBUPCRef u130Listener; /* CLIENT_STOPPED_OBSERVING_ROOM */
	CBUPCRef u131Listener; /* ROOM_OCCUPANTCOUNT_UPDATE */
	CBUPCRef u132Listener; /* ROOM_OBSERVERCOUNT_UPDATE */
	CBUPCRef u133Listener; /* ADD_ROLE */
	CBUPCRef u134Listener; /* ADD_ROLE_RESULT */
	CBUPCRef u135Listener; /* REMOVE_ROLE */
	CBUPCRef u136Listener; /* REMOVE_ROLE_RESULT */
	CBUPCRef u137Listener; /* BAN */
	CBUPCRef u138Listener; /* BAN_RESULT */
	CBUPCRef u139Listener; /* UNBAN */
	CBUPCRef u140Listener; /* UNBAN_RESULT */
	CBUPCRef u141Listener; /* GET_BANNED_LIST_SNAPSHOT */
	CBUPCRef u142Listener; /* BANNED_LIST_SNAPSHOT */
	CBUPCRef u143Listener; /* WATCH_FOR_BANNED_ADDRESSES */
	CBUPCRef u144Listener; /* WATCH_FOR_BANNED_ADDRESSES_RESULT */
	CBUPCRef u145Listener; /* STOP_WATCHING_FOR_BANNED_ADDRESSES */
	CBUPCRef u146Listener; /* STOP_WATCHING_FOR_BANNED_ADDRESSES_RESULT */
	CBUPCRef u147Listener; /* BANNED_ADDRESS_ADDED */
	CBUPCRef u148Listener; /* BANNED_ADDRESS_REMOVED */
	CBUPCRef u149Listener; /* KICK_CLIENT */
	CBUPCRef u150Listener; /* KICK_CLIENT_RESULT */
	CBUPCRef u151Listener; /* GET_SERVERMODULELIST_SNAPSHOT */
	CBUPCRef u152Listener; /* SERVERMODULELIST_SNAPSHOT */
	CBUPCRef u154Listener; /* GET_UPC_STATS_SNAPSHOT */
	CBUPCRef u155Listener; /* GET_UPC_STATS_SNAPSHOT_RESULT */
	CBUPCRef u156Listener; /* UPC_STATS_SNAPSHOT */
	CBUPCRef u157Listener; /* RESET_UPC_STATS */
	CBUPCRef u160Listener; /* WATCH_FOR_PROCESSED_UPCS_RESULT */
	CBUPCRef u164Listener; /* CONNECTION_REFUSED */
	CBUPCRef u165Listener; /* GET_NODELIST_SNAPSHOT */
	CBUPCRef u153Listener; /* CLEAR_MODULE_CACHE */
	CBUPCRef u158Listener; /* RESET_UPC_STATS_RESULT */
	CBUPCRef u159Listener; /* WATCH_FOR_PROCESSED_UPCS */
	CBUPCRef u161Listener; /* PROCESSED_UPC_ADDED */
	CBUPCRef u162Listener; /* STOP_WATCHING_FOR_PROCESSED_UPCS */
	CBUPCRef u163Listener; /* STOP_WATCHING_FOR_PROCESSED_UPCS_RESULT */
	CBUPCRef u166Listener; /* NODELIST_SNAPSHOT */
	CBUPCRef u167Listener; /* GET_GATEWAYS_SNAPSHOT */
	CBUPCRef u168Listener; /* GATEWAYS_SNAPSHOT */

};

#endif /* UnionBridge_H_ */
