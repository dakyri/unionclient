/*
 * UPC.h
 *
 *  Created on: Apr 10, 2014
 *      Author: dak
 */

#ifndef UPC_H_
#define UPC_H_

#include "CommonTypes.h"

/**
 * @class UPC UPC.h
 * wrapper classes defining constants used in upc comms
 */
class UPC {
public:
	UPC();

	/**
	 * @class UPC::ID UPC.h
	 * constants corresponding to UPC message ids, descriptive character string labels for the multitude of Uxx message labels in the xml messages sent and received
	 */
	class ID {
	public:
		static const UPCMessageID WATCH_FOR_BANNED_ADDRESSES;
		static const UPCMessageID CLIENT_READY;
		static const UPCMessageID JOINED_ROOM;
		static const UPCMessageID LEFT_ROOM;
		static const UPCMessageID ACCOUNT_REMOVED;
		static const UPCMessageID ACCOUNT_ADDED;
		static const UPCMessageID WATCH_FOR_ROOMS;
		static const UPCMessageID SEND_MESSAGE_TO_SERVER;
		static const UPCMessageID CREATE_ACCOUNT_RESULT;
		static const UPCMessageID WATCH_FOR_CLIENTS_RESULT;
		static const UPCMessageID OBSERVE_ROOM;
		static const UPCMessageID OBSERVE_ACCOUNT_RESULT;
		static const UPCMessageID CREATE_ROOM;
		static const UPCMessageID STOP_WATCHING_FOR_ACCOUNTS;
		static const UPCMessageID WATCH_FOR_ACCOUNTS;
		static const UPCMessageID GET_BANNED_LIST_SNAPSHOT;
		static const UPCMessageID CONNECTION_REFUSED;
		static const UPCMessageID CLIENT_ADDED_TO_ROOM;
		static const UPCMessageID SESSION_NOT_FOUND;
		static const UPCMessageID ACCOUNT_LIST_UPDATE;
		static const UPCMessageID LEAVE_ROOM_RESULT;
		static const UPCMessageID CLIENTLIST_SNAPSHOT;
		static const UPCMessageID SET_ROOM_UPDATE_LEVELS;
		static const UPCMessageID WATCH_FOR_PROCESSED_UPCS_RESULT;
		static const UPCMessageID CLIENT_ATTR_UPDATE;
		static const UPCMessageID CLIENT_STOPPED_OBSERVING_ROOM;
		static const UPCMessageID STOPPED_OBSERVING_CLIENT;
		static const UPCMessageID RECEIVE_MESSAGE;
		static const UPCMessageID JOIN_ROOM_RESULT;
		static const UPCMessageID BAN_RESULT;
		static const UPCMessageID OBSERVE_CLIENT;
		static const UPCMessageID STOP_OBSERVING_ACCOUNT_RESULT;
		static const UPCMessageID CLIENT_ADDED_TO_SERVER;
		static const UPCMessageID STOPPED_OBSERVING_ACCOUNT;
		static const UPCMessageID STOP_OBSERVING_ROOM;
		static const UPCMessageID TERMINATE_SESSION;
		static const UPCMessageID CHANGE_ACCOUNT_PASSWORD_RESULT;
		static const UPCMessageID GET_ROOMLIST_SNAPSHOT;
		static const UPCMessageID CLIENT_OBSERVED_ROOM;
		static const UPCMessageID STOP_WATCHING_FOR_ACCOUNTS_RESULT;
		static const UPCMessageID STOP_WATCHING_FOR_ROOMS;
		static const UPCMessageID CLEAR_MODULE_CACHE;
		static const UPCMessageID GET_GATEWAYS_SNAPSHOT;
		static const UPCMessageID ROOM_SNAPSHOT;
		static const UPCMessageID WATCH_FOR_PROCESSED_UPCS;
		static const UPCMessageID REMOVE_CLIENT_ATTR;
		static const UPCMessageID STOP_OBSERVING_CLIENT;
		static const UPCMessageID GET_CLIENT_SNAPSHOT;
		static const UPCMessageID ROOM_ADDED;
		static const UPCMessageID UNBAN;
		static const UPCMessageID GET_NODELIST_SNAPSHOT;
		static const UPCMessageID OBSERVED_ROOM_REMOVED_FROM_CLIENT;
		static const UPCMessageID SERVER_HELLO;
		static const UPCMessageID GET_ACCOUNTLIST_SNAPSHOT;
		static const UPCMessageID WATCH_FOR_CLIENTS;
		static const UPCMessageID CLIENT_OBSERVED;
		static const UPCMessageID GET_ACCOUNT_SNAPSHOT_RESULT;
		static const UPCMessageID UPDATE_LEVELS_UPDATE;
		static const UPCMessageID SEND_ROOMMODULE_MESSAGE;
		static const UPCMessageID WATCH_FOR_ROOMS_RESULT;
		static const UPCMessageID REMOVE_ROLE_RESULT;
		static const UPCMessageID STOP_OBSERVING_CLIENT_RESULT;
		static const UPCMessageID ACCOUNT_OBSERVED;
		static const UPCMessageID ROOM_OBSERVERCOUNT_UPDATE;
		static const UPCMessageID SET_CLIENT_ATTR_RESULT;
		static const UPCMessageID REMOVE_ROOM_RESULT;
		static const UPCMessageID JOIN_ROOM;
		static const UPCMessageID REMOVE_ROOM_ATTR;
		static const UPCMessageID ROOM_ATTR_REMOVED;
		static const UPCMessageID KICK_CLIENT_RESULT;
		static const UPCMessageID ACCOUNT_PASSWORD_CHANGED;
		static const UPCMessageID STOP_WATCHING_FOR_CLIENTS;
		static const UPCMessageID RESET_UPC_STATS_RESULT;
		static const UPCMessageID BANNED_LIST_SNAPSHOT;
		static const UPCMessageID ROOM_REMOVED;
		static const UPCMessageID STOP_WATCHING_FOR_CLIENTS_RESULT;
		static const UPCMessageID GET_ROOM_SNAPSHOT_RESULT;
		static const UPCMessageID STOP_OBSERVING_ROOM_RESULT;
		static const UPCMessageID STOP_WATCHING_FOR_PROCESSED_UPCS_RESULT;
		static const UPCMessageID GET_CLIENTCOUNT_SNAPSHOT;
		static const UPCMessageID LOGGED_IN;
		static const UPCMessageID STOP_WATCHING_FOR_BANNED_ADDRESSES;
		static const UPCMessageID OBSERVE_CLIENT_RESULT;
		static const UPCMessageID LOGGED_OFF;
		static const UPCMessageID SET_ROOM_ATTR;
		static const UPCMessageID CLIENT_METADATA;
		static const UPCMessageID SET_ROOM_ATTR_RESULT;
		static const UPCMessageID PROCESSED_UPC_ADDED;
		static const UPCMessageID REMOVE_ROOM_ATTR_RESULT;
		static const UPCMessageID OBSERVED_ROOM_ADDED_TO_CLIENT;
		static const UPCMessageID LOGOFF_RESULT;
		static const UPCMessageID GET_CLIENTCOUNT_SNAPSHOT_RESULT;
		static const UPCMessageID SERVERMODULELIST_SNAPSHOT;
		static const UPCMessageID CHANGE_ACCOUNT_PASSWORD;
		static const UPCMessageID CLIENT_HELLO;
		static const UPCMessageID ROOM_OCCUPANTCOUNT_UPDATE;
		static const UPCMessageID SEND_MESSAGE_TO_ROOMS;
		static const UPCMessageID GET_UPC_STATS_SNAPSHOT;
		static const UPCMessageID ADD_ROLE_RESULT;
		static const UPCMessageID LOGIN;
		static const UPCMessageID ROOMLIST_SNAPSHOT;
		static const UPCMessageID CREATE_ROOM_RESULT;
		static const UPCMessageID STOP_WATCHING_FOR_ROOMS_RESULT;
		static const UPCMessageID REMOVE_ROLE;
		static const UPCMessageID REMOVE_ACCOUNT;
		static const UPCMessageID JOINED_ROOM_REMOVED_FROM_CLIENT;
		static const UPCMessageID WATCH_FOR_BANNED_ADDRESSES_RESULT;
		static const UPCMessageID REMOVE_CLIENT_ATTR_RESULT;
		static const UPCMessageID JOINED_ROOM_ADDED_TO_CLIENT;
		static const UPCMessageID GET_ROOM_SNAPSHOT;
		static const UPCMessageID STOP_WATCHING_FOR_PROCESSED_UPCS;
		static const UPCMessageID BANNED_ADDRESS_ADDED;
		static const UPCMessageID SET_CLIENT_ATTR;
		static const UPCMessageID OBSERVE_ROOM_RESULT;
		static const UPCMessageID GET_SERVERMODULELIST_SNAPSHOT;
		static const UPCMessageID SEND_SERVERMODULE_MESSAGE;
		static const UPCMessageID ADD_ROLE;
		static const UPCMessageID STOP_WATCHING_FOR_BANNED_ADDRESSES_RESULT;
		static const UPCMessageID WATCH_FOR_ACCOUNTS_RESULT;
		static const UPCMessageID RESET_UPC_STATS;
		static const UPCMessageID ROOM_ATTR_UPDATE;
		static const UPCMessageID SERVER_TIME_UPDATE;
		static const UPCMessageID LEAVE_ROOM;
		static const UPCMessageID CLIENT_REMOVED_FROM_SERVER;
		static const UPCMessageID STOP_OBSERVING_ACCOUNT;
		static const UPCMessageID STOPPED_OBSERVING_ROOM;
		static const UPCMessageID GATEWAYS_SNAPSHOT;
		static const UPCMessageID GET_UPC_STATS_SNAPSHOT_RESULT;
		static const UPCMessageID NODELIST_SNAPSHOT;
		static const UPCMessageID SEND_MESSAGE_TO_CLIENTS;
		static const UPCMessageID CLIENT_REMOVED_FROM_ROOM;
		static const UPCMessageID GET_ACCOUNT_SNAPSHOT;
		static const UPCMessageID CLIENTCOUNT_SNAPSHOT;
		static const UPCMessageID REMOVE_ACCOUNT_RESULT;
		static const UPCMessageID LOGOFF;
		static const UPCMessageID SESSION_TERMINATED;
		static const UPCMessageID UPC_STATS_SNAPSHOT;
		static const UPCMessageID REMOVE_ROOM;
		static const UPCMessageID BANNED_ADDRESS_REMOVED;
		static const UPCMessageID GET_CLIENT_SNAPSHOT_RESULT;
		static const UPCMessageID CLIENT_SNAPSHOT;
		static const UPCMessageID CREATE_ACCOUNT;
		static const UPCMessageID UNBAN_RESULT;
		static const UPCMessageID GET_CLIENTLIST_SNAPSHOT;
		static const UPCMessageID LOGIN_RESULT;
		static const UPCMessageID OBSERVE_ACCOUNT;
		static const UPCMessageID CLIENT_ATTR_REMOVED;
		static const UPCMessageID KICK_CLIENT;
		static const UPCMessageID BAN;
		static const UPCMessageID SYNC_TIME;
		static const UPCMessageID OBSERVED_ROOM;

		static constexpr int ToCode(const UPCMessageID);
	};

	/**
	 * @class UPC::Status UPC.h
	 * constants corresponding to the UPC status codes which are returned in the xml packets. the UPCStatus constants are for the numeric version
	 * used internally, and the xxx_STR char* constants are the actual constants which are passed back in the xml comms packets. negative values
	 * are reserved for the low level comms subsystems, and includes things like libuv error codes, positive values > UPC::Status::UPC_STATUS_BASE
	 * are error states as returned in xml comms, and postitive values in the range [CNX_ERROR_BASE, UPC_STATUS_BASE) are protocol and comms errors
	 * messaged in UPC packets
	 */
	class Status {
	private:
	public:
		enum {
		 CNX_ERROR_BASE=100,
		 UPC_STATUS_BASE=200,
		/** @constant */
		 SUCCESS = 0,
		/** @constant */
		 UNKNOWN_ERROR = -1,

		/** @constant */
		 CLIENT_KILL_CONNECT = CNX_ERROR_BASE+0,
		/** @constant */
		 SERVER_KILL_CONNECT = CNX_ERROR_BASE+1,
		/** @constant */
		 SESSION_TERMINATED = CNX_ERROR_BASE+2,
		/** @constant */
		 SESSION_NOT_FOUND = CNX_ERROR_BASE+3,
		/** @constant */
		 PROTOCOL_INCOMPATIBLE = CNX_ERROR_BASE+4,
		/** @constant */
		 CONNECT_REFUSED = CNX_ERROR_BASE+5,
		/** @constant */
		XML_ERROR = CNX_ERROR_BASE + 6,
		/** @constant */
		NO_VALID_CONNECTION_AVAILABLE = CNX_ERROR_BASE + 7,
		/** @constant */
		CONNECT_TIMEOUT = CNX_ERROR_BASE + 8,
		
		/** @constant */
		 ACCOUNT_EXISTS = UPC_STATUS_BASE+0,
		/** @constant */
		 ACCOUNT_NOT_FOUND = UPC_STATUS_BASE+1,
		/** @constant */
		 AUTHORIZATION_REQUIRED = UPC_STATUS_BASE+2,
		/** @constant */
		 AUTHORIZATION_FAILED = UPC_STATUS_BASE+3,
		/** @constant */
		 ALREADY_ASSIGNED = UPC_STATUS_BASE+4,
		/** @constant */
		 ALREADY_BANNED = UPC_STATUS_BASE+5,
		/** @constant */
		 ALREADY_IN_ROOM = UPC_STATUS_BASE+6,
		/** @constant */
		 ALREADY_LOGGED_IN = UPC_STATUS_BASE+7,
		/** @constant */
		 ALREADY_OBSERVING = UPC_STATUS_BASE+8,
		/** @constant */
		 ALREADY_SYNCHRONIZED = UPC_STATUS_BASE+9,
		/** @constant */
		 ALREADY_WATCHING = UPC_STATUS_BASE+10,
		/** @constant */
		 ATTR_NOT_FOUND = UPC_STATUS_BASE+11,
		/** @constant */
		 CLIENT_NOT_FOUND = UPC_STATUS_BASE+12,
		/** @constant */
		 UPC_ERROR = UPC_STATUS_BASE+13,
		/** @constant */
		 EVALUATION_FAILED = UPC_STATUS_BASE+14,
		/** @constant */
		 DUPLICATE_VALUE = UPC_STATUS_BASE+15,
		/** @constant */
		 IMMUTABLE = UPC_STATUS_BASE+16,
		/** @constant */
		 INVALID_QUALIFIER = UPC_STATUS_BASE+17,
		/** @constant */
		 NAME_NOT_FOUND = UPC_STATUS_BASE+18,
		/** @constant */
		 NAME_EXISTS = UPC_STATUS_BASE+19,
		/** @constant */
		 NOT_ASSIGNED = UPC_STATUS_BASE+20,
		/** @constant */
		 NOT_BANNED = UPC_STATUS_BASE+21,
		/** @constant */
		 NOT_IN_ROOM = UPC_STATUS_BASE+22,
		/** @constant */
		 NOT_LOGGED_IN = UPC_STATUS_BASE+23,
		/** @constant */
		 NOT_OBSERVING = UPC_STATUS_BASE+24,
		/** @constant */
		 NOT_WATCHING = UPC_STATUS_BASE+25,
		/** @constant */
		 PERMISSION_DENIED = UPC_STATUS_BASE+26,
		/** @constant */
		 REMOVED = UPC_STATUS_BASE+27,
		/** @constant */
		 ROLE_NOT_FOUND = UPC_STATUS_BASE+28,
		/** @constant */
		 ROOM_EXISTS = UPC_STATUS_BASE+29,
		/** @constant */
		 ROOM_FULL = UPC_STATUS_BASE+30,
		/** @constant */
		 ROOM_NOT_FOUND = UPC_STATUS_BASE+31,
		/** @constant */
		 SERVER_ONLY = UPC_STATUS_BASE+32,
		};

		static UPCStatus GetStatusCode(const char *str);
		static std::string GetStatusString(const UPCStatus status);
	private:
		static const char* ACCOUNT_EXISTS_STR;
		static const char* ACCOUNT_NOT_FOUND_STR;
		static const char* AUTHORIZATION_REQUIRED_STR;
		static const char* AUTHORIZATION_FAILED_STR;
		static const char* ALREADY_ASSIGNED_STR;
		static const char* ALREADY_BANNED_STR;
		static const char* ALREADY_IN_ROOM_STR;
		static const char* ALREADY_LOGGED_IN_STR;
		static const char* ALREADY_OBSERVING_STR;
		static const char* ALREADY_SYNCHRONIZED_STR;
		static const char* ALREADY_WATCHING_STR;
		static const char* ATTR_NOT_FOUND_STR;
		static const char* CLIENT_NOT_FOUND_STR;
		static const char* ERROR_STR;
		static const char* EVALUATION_FAILED_STR;
		static const char* DUPLICATE_VALUE_STR;
		static const char* IMMUTABLE_STR;
		static const char* INVALID_QUALIFIER_STR;
		static const char* NAME_NOT_FOUND_STR;
		static const char* NAME_EXISTS_STR;
		static const char* NOT_ASSIGNED_STR;
		static const char* NOT_BANNED_STR;
		static const char* NOT_IN_ROOM_STR;
		static const char* NOT_LOGGED_IN_STR;
		static const char* NOT_OBSERVING_STR;
		static const char* NOT_WATCHING_STR;
		static const char* PERMISSION_DENIED_STR;
		static const char* REMOVED_STR;
		static const char* ROLE_NOT_FOUND_STR;
		static const char* ROOM_EXISTS_STR;
		static const char* ROOM_FULL_STR;
		static const char* ROOM_NOT_FOUND_STR;
		static const char* SERVER_ONLY_STR;
		static const char* SUCCESS_STR;
	};


};
#endif /* UPC_H_ */

