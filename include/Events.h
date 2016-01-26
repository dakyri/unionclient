/*
 * Events.h
 *
 *  Created on: Apr 13, 2014
 *      Author: dak
 */

#ifndef EVENTS_H_
#define EVENTS_H_

/**
 * @class Event Event.h
 * wrapper class for event ids. codes in the range of 1-200 are reserved for UPC opcodes. there is some overlap, as many of these
 * events are generated in a 1-1 correspondence with the UPC events, though several are internal events (log and collection range events, and some are generated to a variety
 * of upc events). the names correspond more or less precisely to the event ids of the original javascript and flash union clients
 */
class Event
{
public:
//---Connection events--------
	static const int CONNECTION_EVENT_ID_BASE = 200;
/** @constant */
	static const int BEGIN_CONNECT = CONNECTION_EVENT_ID_BASE+0;
/** @constant */
	static const int READY = CONNECTION_EVENT_ID_BASE+1;
/** @constant */
	static const int CONNECT_FAILURE = CONNECTION_EVENT_ID_BASE+2;
/** @constant */
	static const int CONNECTED = CONNECTION_EVENT_ID_BASE+3;
/** @constant */
	static const int DISCONNECTED = CONNECTION_EVENT_ID_BASE+4;
/** @constant */
	static const int RECEIVE_MSG = CONNECTION_EVENT_ID_BASE+5;
/** @constant */
	static const int SEND_DATA = CONNECTION_EVENT_ID_BASE+6;
/** @constant */
	static const int RECEIVE_DATA = CONNECTION_EVENT_ID_BASE+7;
/** @constant */
	static const int SELECT_CONNECTION = CONNECTION_EVENT_ID_BASE+8;
/** @constant */
	static const int CONNECTION_STATE_CHANGE = CONNECTION_EVENT_ID_BASE+9;
/** @constant */
	static const int IO_ERROR = CONNECTION_EVENT_ID_BASE+10;

//------common upc events---------------------
	static const int COMMON_EVENT_ID_BASE = 230;
/** @constant */
	static const int SYNCHRONIZED = COMMON_EVENT_ID_BASE+0;
/** @constant */
	static const int OBSERVE = COMMON_EVENT_ID_BASE+1;
/** @constant */
	static const int STOP_OBSERVING = COMMON_EVENT_ID_BASE+2;
/** @constant */
	static const int OBSERVE_RESULT = COMMON_EVENT_ID_BASE+3;
/** @constant */
	static const int STOP_OBSERVING_RESULT = COMMON_EVENT_ID_BASE+4;

//------room events---------------------
	static const int ROOM_EVENT_ID_BASE = 240;
/** @constant room event signalled when a room is successfully joined */
	static const int JOIN = ROOM_EVENT_ID_BASE+0;
/** @constant */
	static const int JOIN_RESULT = ROOM_EVENT_ID_BASE+1;
/** @constant */
	static const int LEAVE = ROOM_EVENT_ID_BASE+2;
/** @constant */
	static const int LEAVE_RESULT = ROOM_EVENT_ID_BASE+3;
/** @constant */
	static const int UPDATE_CLIENT_ATTRIBUTE = ROOM_EVENT_ID_BASE+4;
/** @constant */
	static const int DELETE_CLIENT_ATTRIBUTE = ROOM_EVENT_ID_BASE+5;
/** @constant */
	static const int ADD_OCCUPANT = ROOM_EVENT_ID_BASE+6;
/** @constant */
	static const int REMOVE_OCCUPANT = ROOM_EVENT_ID_BASE+7;
/** @constant */
	static const int ADD_OBSERVER = ROOM_EVENT_ID_BASE+8;
/** @constant */
	static const int REMOVE_OBSERVER = ROOM_EVENT_ID_BASE+9;
/** @constant */
	static const int OCCUPANT_COUNT = ROOM_EVENT_ID_BASE+10;
/** @constant */
	static const int OBSERVER_COUNT = ROOM_EVENT_ID_BASE+11;
/** @constant */
	static const int REMOVED = ROOM_EVENT_ID_BASE+12;
/** @constant */
	static const int CREATE_ROOM_RESULT = ROOM_EVENT_ID_BASE+13;
/** @constant */
	static const int REMOVE_ROOM_RESULT = ROOM_EVENT_ID_BASE+14;
/** @constant */
	static const int WATCH_FOR_ROOMS_RESULT = ROOM_EVENT_ID_BASE+15;
/** @constant */
	static const int STOP_WATCHING_FOR_ROOMS_RESULT = ROOM_EVENT_ID_BASE+16;
/** @constant */
	static const int ROOM_ADDED = ROOM_EVENT_ID_BASE+17;
/** @constant */
	static const int ROOM_REMOVED = ROOM_EVENT_ID_BASE+18;
/** @constant */
	static const int ROOM_COUNT = ROOM_EVENT_ID_BASE+19;

//------client events---------------------
	static const int CLIENT_EVENT_ID_BASE = 270;
/** @constant client event received when a client successfully joins a room */
	static const int JOIN_ROOM = CLIENT_EVENT_ID_BASE+0;
/** @constant */
	static const int LEAVE_ROOM = CLIENT_EVENT_ID_BASE+1;
/** @constant */
	static const int OBSERVE_ROOM = CLIENT_EVENT_ID_BASE+2;
/** @constant */
	static const int STOP_OBSERVING_ROOM = CLIENT_EVENT_ID_BASE+3;
/** @constant */
	static const int WATCH_FOR_CLIENTS_RESULT = CLIENT_EVENT_ID_BASE+4;
/** @constant */
	static const int STOP_WATCHING_FOR_CLIENTS_RESULT = CLIENT_EVENT_ID_BASE+5;
/** @constant */
	static const int CLIENT_DISCONNECTED = CLIENT_EVENT_ID_BASE+6;
/** @constant */
	static const int CLIENT_CONNECTED = CLIENT_EVENT_ID_BASE+7;
/** @constant */
	static const int KICK_RESULT = CLIENT_EVENT_ID_BASE+8;
/** @constant */
	static const int BAN_RESULT = CLIENT_EVENT_ID_BASE+9;
/** @constant */
	static const int UNBAN_RESULT = CLIENT_EVENT_ID_BASE+10;
/** @constant */
	static const int WATCH_FOR_BANNED_ADDRESSES_RESULT = CLIENT_EVENT_ID_BASE+11;
/** @constant */
	static const int STOP_WATCHING_FOR_BANNED_ADDRESSES_RESULT = CLIENT_EVENT_ID_BASE+12;
/** @constant */
	static const int ADDRESS_BANNED = CLIENT_EVENT_ID_BASE+13;
/** @constant */
	static const int ADDRESS_UNBANNED = CLIENT_EVENT_ID_BASE+14;
/** @constant */
	static const int SYNCHRONIZE_BANLIST = CLIENT_EVENT_ID_BASE+15;

//------account events---------------------
	static const int ACCOUNT_EVENT_ID_BASE = 290;
/** @constant */
	static const int LOGIN_RESULT = ACCOUNT_EVENT_ID_BASE+0;
/** @constant */
	static const int LOGIN  = ACCOUNT_EVENT_ID_BASE+1;
/** @constant */
	static const int LOGOFF_RESULT = ACCOUNT_EVENT_ID_BASE+2;
/** @constant */
	static const int LOGOFF = ACCOUNT_EVENT_ID_BASE+3;
/** @constant */
	static const int CHANGE_PASSWORD_RESULT = ACCOUNT_EVENT_ID_BASE+4;
/** @constant */
	static const int CHANGE_PASSWORD = ACCOUNT_EVENT_ID_BASE+5;
/** @constant */
	static const int ADD_ROLE_RESULT = ACCOUNT_EVENT_ID_BASE+6;
/** @constant */
	static const int REMOVE_ROLE_RESULT = ACCOUNT_EVENT_ID_BASE+7;
/** @constant */
	static const int CREATE_ACCOUNT_RESULT = ACCOUNT_EVENT_ID_BASE+8;
/** @constant */
	static const int REMOVE_ACCOUNT_RESULT = ACCOUNT_EVENT_ID_BASE+9;
/** @constant */
	static const int ACCOUNT_ADDED = ACCOUNT_EVENT_ID_BASE+10;
/** @constant */
	static const int ACCOUNT_REMOVED = ACCOUNT_EVENT_ID_BASE+11;
/** @constant */
	static const int WATCH_FOR_ACCOUNTS_RESULT = ACCOUNT_EVENT_ID_BASE+12;
/** @constant */
	static const int STOP_WATCHING_FOR_ACCOUNTS_RESULT = ACCOUNT_EVENT_ID_BASE+13;

//------attribute events---------------------
	static const int ATTRIBUTE_EVENT_ID_BASE = 310;
/** @constant */
	static const int UPDATE_ATTR = ATTRIBUTE_EVENT_ID_BASE+0;
/** @constant */
	static const int REMOVE_ATTR = ATTRIBUTE_EVENT_ID_BASE+1;
/** @constant */
	static const int UPDATE_ATTR_RESULT = ATTRIBUTE_EVENT_ID_BASE+2;
/** @constant */
	static const int REMOVE_ATTR_RESULT = ATTRIBUTE_EVENT_ID_BASE+3;

//------collection events---------------------
	static const int COLLECTION_EVENT_ID_BASE = 320;
/** @constant */
	static const int REMOVE_ITEM = COLLECTION_EVENT_ID_BASE+0;
/** @constant */
	static const int ADD_ITEM = COLLECTION_EVENT_ID_BASE+1;

//------collection events-------------------
	static const int LOG_EVENT_ID_BASE = 330;
/** @constant */
	static const int UPDATE = LOG_EVENT_ID_BASE+0;
/** @constant */
	static const int LEVEL_CHANGE = LOG_EVENT_ID_BASE+1;
};

#endif /* EVENTS_H_ */
