/*
 * UnionBridge.cpp
 *
 *  Created on: Apr 9, 2014
 *      Author: dak
 */

#include "CommonTypes.h"
#include "UCUpperHeaders.h"
#include "UCLowerHeaders.h"
#include "UCLowerTypes.h"
#include "Version.h"

#include "tinyxml2.h"

/**
 * @class UnionBridge UnionBridge.h
 * @brief Core UPC message handler and protocol handler. Converts messages to and from notification callbacks on AbstractConnector
 *
 * Impements the connections with the AbstractConnector, which presents a simple pipe to a union server and communicates with NXConnection
 * callbacks. These are then processed as NXUPC callbacks. If this multiplexed becomes too much layering, perhaps convert to a mammoth switch, but in
 * this multiplexed approach we can take addition NXUPC notifications from anything that would care to generate them.
 *
 * Generates NXUPC notifications, identified by Event ids, which represent i/o events, but which are aware of the upc protocol
 * and connection regime, as distinct from the notifications of AbstractConnector, which are protocol independent. Spawns:
 * - Event::READY, {}, UPC::Status::SUCCESS ... we're ready to go
 * - Event::DISCONNECTED, {}, UPC::Status::SUCCESS ... we've successfully and totally disconnected from the server
 * - Event::SELECT_CONNECTION, {args}, status ... specific underlying connection is changed
 * - Event::CONNECT_FAILURE, {msg}, status is either an io status (-ve) or one of the following (which are signalled by UPC messages):
 *		- UPC::Status::CONNECT_REFUSED, {reason, description}, UPC::Status::UPC_ERROR ... connection wasn't allowed
 *		- UPC::Status::PROTOCOL_INCOMPATIBLE, {version} ... connection wasn't allowed
 *		- UPC::Status::SESSION_TERMINATED, {} ... our current session is dramatically cut short
 *		- UPC::Status::SESSION_NOT_FOUND, {} ... our current session is completely out of date
 *		- UPC::Status::SERVER_KILL_CONNECT, {msg} ... server has cut off the current connection
 *		- UPC::Status::CLIENT_KILL_CONNECT, {msg} ... we or our own code cut off
 *		- UPC::Status::NO_VALID_CONNECTION_AVAILABLE, {msg} ... or we ran out of options for connections
 *		- UPC::Status::CONNECT_TIMEOUT, {msg} ... or we timed out
 * - Event::BEGIN_CONNECT, {}, connectionState ... used by the timeout subsystem
 * - Event::CONNECTED, {}, connectionState ... signalled when we have
 * More detailed info about the connecting and disconnecting are obtained from the following. todo these last 3 of these are completely pointless and redundant implementations of the js client's api.
 * For full information of what's going on, the above are sufficient Maybe these are useful for an external logging app or maybe they should be wiped from existence as unnecessary stuff that nobody wants.
 * - Event::CONNECTION_STATE_CHANGE, {}, connectionState
 * - Event::SEND_DATA, {data}, UPC::Status::SUCCESS
 * - Event::RECEIVE_DATA, {data}, UPC::Status::SUCCESS
 */
using namespace std::placeholders;
UnionBridge::UnionBridge(AbstractConnector &c, RoomManager &roomManager, ClientManager& clientManager, AccountManager& accountManager, ILogger &log)
	: roomManager(roomManager)
	, clientManager(clientManager)
	, accountManager(accountManager)
	, connector(c)
	, loop(nullptr)
	, log(log) {

	SetQueueNotifications(false);

	DEBUG_OUT("UnionBridge::UnionBridge();");

	removeListenersOnDisconnect = true;
	numMessagesSent = 0;
	numMessagesReceived = 0;

	clientType="Custom Client";
	clientVersion(2,1,1,302);
	upcVersion(1,10,3);
	userAgentString = "Desktop testbed";

	rxUPCListener = std::make_shared<CBConnection>(std::bind(&UnionBridge::UpcReceivedListener,this, _1, _2, _3, _4));
	txUPCListener = std::make_shared<CBConnection>(std::bind(&UnionBridge::UpcSentListener,this, _1, _2, _3, _4));
	connectListener = std::make_shared<CBConnection>(std::bind(&UnionBridge::ConnectListener,this, _1, _2, _3, _4));
	disconnectListener = std::make_shared<CBConnection>(std::bind(&UnionBridge::DisconnectListener,this, _1, _2, _3, _4));
	selectListener = std::make_shared<CBConnection>(std::bind(&UnionBridge::SelectListener,this, _1, _2, _3, _4));
	ioErrorListener = std::make_shared<CBConnection>(std::bind(&UnionBridge::IOErrorListener,this, _1, _2, _3, _4));
	connectFailureListener = std::make_shared<CBConnection>(std::bind(&UnionBridge::ConnectFailureListener,this, _1, _2, _3, _4));

	u1Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U1, this, _1, _2, _3)); /* SEND_MESSAGE_TO_ROOMS */
	u2Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U2, this, _1, _2, _3)); /* SEND_MESSAGE_TO_CLIENTS */
	u3Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U3, this, _1, _2, _3)); /* SET_CLIENT_ATTR */
	u4Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U4, this, _1, _2, _3)); /* JOIN_ROOM */
	u5Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U5, this, _1, _2, _3)); /* SET_ROOM_ATTR */
	u6Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U6, this, _1, _2, _3)); /* JOINED_ROOM */
	u7Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U7, this, _1, _2, _3)); /* RECEIVE_MESSAGE */
	u8Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U8, this, _1, _2, _3)); /* CLIENT_ATTR_UPDATE */
	u9Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U9, this, _1, _2, _3)); /* ROOM_ATTR_UPDATE */
	u10Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U10, this, _1, _2, _3)); /* LEAVE_ROOM */
	u11Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U11, this, _1, _2, _3)); /* CREATE_ACCOUNT */
	u12Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U12, this, _1, _2, _3)); /* REMOVE_ACCOUNT */
	u13Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U13, this, _1, _2, _3)); /* CHANGE_ACCOUNT_PASSWORD */
	u14Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U14, this, _1, _2, _3)); /* LOGIN */
	u18Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U18, this, _1, _2, _3)); /* GET_CLIENTCOUNT_SNAPSHOT */
	u19Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U19, this, _1, _2, _3)); /* SYNC_TIME */
	u21Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U21, this, _1, _2, _3)); /* GET_ROOMLIST_SNAPSHOT */
	u32Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U32, this, _1, _2, _3)); /* CREATE_ROOM_RESULT */
	u43Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U43, this, _1, _2, _3)); /* STOP_WATCHING_FOR_ROOMS_RESULT */
	u24Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U24, this, _1, _2, _3)); /* CREATE_ROOM */
	u25Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U25, this, _1, _2, _3)); /* REMOVE_ROOM */
	u29Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U29, this, _1, _2, _3)); /* CLIENT_METADATA */
	u26Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U26, this, _1, _2, _3)); /* WATCH_FOR_ROOMS */
	u27Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U27, this, _1, _2, _3)); /* STOP_WATCHING_FOR_ROOMS */
	u33Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U33, this, _1, _2, _3)); /* REMOVE_ROOM_RESULT */
	u34Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U34, this, _1, _2, _3)); /* CLIENTCOUNT_SNAPSHOT */
	u36Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U36, this, _1, _2, _3)); /* CLIENT_ADDED_TO_ROOM */
	u37Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U37, this, _1, _2, _3)); /* CLIENT_REMOVED_FROM_ROOM */
	u38Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U38, this, _1, _2, _3)); /* ROOMLIST_SNAPSHOT */
	u39Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U39, this, _1, _2, _3)); /* ROOM_ADDED */
	u40Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U40, this, _1, _2, _3)); /* ROOM_REMOVED */
	u42Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U42, this, _1, _2, _3)); /* WATCH_FOR_ROOMS_RESULT */
	u44Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U44, this, _1, _2, _3)); /* LEFT_ROOM */
	u46Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U46, this, _1, _2, _3)); /* CHANGE_ACCOUNT_PASSWORD_RESULT */
	u47Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U47, this, _1, _2, _3)); /* CREATE_ACCOUNT_RESULT */
	u48Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U48, this, _1, _2, _3)); /* REMOVE_ACCOUNT_RESULT */
	u49Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U49, this, _1, _2, _3)); /* LOGIN_RESULT */
	u50Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U50, this, _1, _2, _3)); /* SERVER_TIME_UPDATE */
	u54Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U54, this, _1, _2, _3)); /* ROOM_SNAPSHOT */
	u55Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U55, this, _1, _2, _3)); /* GET_ROOM_SNAPSHOT */
	u57Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U57, this, _1, _2, _3)); /* SEND_MESSAGE_TO_SERVER */
	u58Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U58, this, _1, _2, _3)); /* OBSERVE_ROOM */
	u59Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U59, this, _1, _2, _3)); /* OBSERVED_ROOM */
	u60Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U60, this, _1, _2, _3)); /* GET_ROOM_SNAPSHOT_RESULT */
	u61Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U61, this, _1, _2, _3)); /* STOP_OBSERVING_ROOM */
	u62Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U62, this, _1, _2, _3)); /* STOPPED_OBSERVING_ROOM */
	u63Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U63, this, _1, _2, _3)); /* CLIENT_READY */
	u64Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U64, this, _1, _2, _3)); /* SET_ROOM_UPDATE_LEVELS */
	u65Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U65, this, _1, _2, _3)); /* CLIENT_HELLO */
	u66Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U66, this, _1, _2, _3)); /* SERVER_HELLO */
	u67Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U67, this, _1, _2, _3)); /* REMOVE_ROOM_ATTR */
	u69Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U69, this, _1, _2, _3)); /* REMOVE_CLIENT_ATTR */
	u70Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U70, this, _1, _2, _3)); /* SEND_ROOMMODULE_MESSAGE */
	u71Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U71, this, _1, _2, _3)); /* SEND_SERVERMODULE_MESSAGE */
	u72Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U72, this, _1, _2, _3)); /* JOIN_ROOM_RESULT */
	u73Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U73, this, _1, _2, _3)); /* SET_CLIENT_ATTR_RESULT */
	u74Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U74, this, _1, _2, _3)); /* SET_ROOM_ATTR_RESULT */
	u75Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U75, this, _1, _2, _3)); /* GET_CLIENTCOUNT_SNAPSHOT_RESULT */
	u76Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U76, this, _1, _2, _3)); /* LEAVE_ROOM_RESULT */
	u77Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U77, this, _1, _2, _3)); /* OBSERVE_ROOM_RESULT */
	u78Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U78, this, _1, _2, _3)); /* STOP_OBSERVING_ROOM_RESULT */
	u79Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U79, this, _1, _2, _3)); /* ROOM_ATTR_REMOVED */
	u80Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U80, this, _1, _2, _3)); /* REMOVE_ROOM_ATTR_RESULT */
	u81Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U81, this, _1, _2, _3)); /* CLIENT_ATTR_REMOVED */
	u82Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U82, this, _1, _2, _3)); /* REMOVE_CLIENT_ATTR_RESULT */
	u83Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U83, this, _1, _2, _3)); /* TERMINATE_SESSION */
	u84Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U84, this, _1, _2, _3)); /* SESSION_TERMINATED */
	u85Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U85, this, _1, _2, _3)); /* SESSION_NOT_FOUND */
	u86Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U86, this, _1, _2, _3)); /* LOGOFF */
	u87Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U87, this, _1, _2, _3)); /* LOGOFF_RESULT */
	u88Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U88, this, _1, _2, _3)); /* LOGGED_IN */
	u89Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U89, this, _1, _2, _3)); /* LOGGED_OFF */
	u90Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U90, this, _1, _2, _3)); /* ACCOUNT_PASSWORD_CHANGED */
	u91Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U91, this, _1, _2, _3)); /* GET_CLIENTLIST_SNAPSHOT */
	u92Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U92, this, _1, _2, _3)); /* WATCH_FOR_CLIENTS */
	u93Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U93, this, _1, _2, _3)); /* STOP_WATCHING_FOR_CLIENTS */
	u94Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U94, this, _1, _2, _3)); /* GET_CLIENT_SNAPSHOT */
	u95Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U95, this, _1, _2, _3)); /* OBSERVE_CLIENT */
	u96Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U96, this, _1, _2, _3)); /* STOP_OBSERVING_CLIENT */
	u97Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U97, this, _1, _2, _3)); /* GET_ACCOUNTLIST_SNAPSHOT */
	u98Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U98, this, _1, _2, _3)); /* WATCH_FOR_ACCOUNTS */
	u99Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U99, this, _1, _2, _3)); /* STOP_WATCHING_FOR_ACCOUNTS */
	u100Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U100, this, _1, _2, _3)); /* GET_ACCOUNT_SNAPSHOT */
	u101Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U101, this, _1, _2, _3)); /* CLIENTLIST_SNAPSHOT */
	u102Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U102, this, _1, _2, _3)); /* CLIENT_ADDED_TO_SERVER */
	u103Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U103, this, _1, _2, _3)); /* CLIENT_REMOVED_FROM_SERVER */
	u104Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U104, this, _1, _2, _3)); /* CLIENT_SNAPSHOT */
	u105Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U105, this, _1, _2, _3)); /* OBSERVE_CLIENT_RESULT */
	u106Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U106, this, _1, _2, _3)); /* STOP_OBSERVING_CLIENT_RESULT */
	u107Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U107, this, _1, _2, _3)); /* WATCH_FOR_CLIENTS_RESULT */
	u108Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U108, this, _1, _2, _3)); /* STOP_WATCHING_FOR_CLIENTS_RESULT */
	u109Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U109, this, _1, _2, _3)); /* WATCH_FOR_ACCOUNTS_RESULT */
	u110Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U110, this, _1, _2, _3)); /* STOP_WATCHING_FOR_ACCOUNTS_RESULT */
	u111Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U111, this, _1, _2, _3)); /* ACCOUNT_ADDED */
	u112Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U112, this, _1, _2, _3)); /* ACCOUNT_REMOVED */
	u113Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U113, this, _1, _2, _3)); /* JOINED_ROOM_ADDED_TO_CLIENT */
	u114Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U114, this, _1, _2, _3)); /* JOINED_ROOM_REMOVED_FROM_CLIENT */
	u115Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U115, this, _1, _2, _3)); /* GET_CLIENT_SNAPSHOT_RESULT */
	u116Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U116, this, _1, _2, _3)); /* GET_ACCOUNT_SNAPSHOT_RESULT */
	u117Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U117, this, _1, _2, _3)); /* OBSERVED_ROOM_ADDED_TO_CLIENT */
	u118Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U118, this, _1, _2, _3)); /* OBSERVED_ROOM_REMOVED_FROM_CLIENT */
	u119Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U119, this, _1, _2, _3)); /* CLIENT_OBSERVED */
	u121Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U121, this, _1, _2, _3)); /* OBSERVE_ACCOUNT */
	u122Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U122, this, _1, _2, _3)); /* STOP_OBSERVING_ACCOUNT */
	u120Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U120, this, _1, _2, _3)); /* STOPPED_OBSERVING_CLIENT */
	u123Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U123, this, _1, _2, _3)); /* OBSERVE_ACCOUNT_RESULT */
	u124Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U124, this, _1, _2, _3)); /* ACCOUNT_OBSERVED */
	u125Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U125, this, _1, _2, _3)); /* STOP_OBSERVING_ACCOUNT_RESULT */
	u126Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U126, this, _1, _2, _3)); /* STOPPED_OBSERVING_ACCOUNT */
	u127Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U127, this, _1, _2, _3)); /* ACCOUNT_LIST_UPDATE */
	u128Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U128, this, _1, _2, _3)); /* UPDATE_LEVELS_UPDATE */
	u129Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U129, this, _1, _2, _3)); /* CLIENT_OBSERVED_ROOM */
	u130Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U130, this, _1, _2, _3)); /* CLIENT_STOPPED_OBSERVING_ROOM */
	u131Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U131, this, _1, _2, _3)); /* ROOM_OCCUPANTCOUNT_UPDATE */
	u132Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U132, this, _1, _2, _3)); /* ROOM_OBSERVERCOUNT_UPDATE */
	u133Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U133, this, _1, _2, _3)); /* ADD_ROLE */
	u134Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U134, this, _1, _2, _3)); /* ADD_ROLE_RESULT */
	u135Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U135, this, _1, _2, _3)); /* REMOVE_ROLE */
	u136Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U136, this, _1, _2, _3)); /* REMOVE_ROLE_RESULT */
	u137Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U137, this, _1, _2, _3)); /* BAN */
	u138Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U138, this, _1, _2, _3)); /* BAN_RESULT */
	u139Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U139, this, _1, _2, _3)); /* UNBAN */
	u140Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U140, this, _1, _2, _3)); /* UNBAN_RESULT */
	u141Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U141, this, _1, _2, _3)); /* GET_BANNED_LIST_SNAPSHOT */
	u142Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U142, this, _1, _2, _3)); /* BANNED_LIST_SNAPSHOT */
	u143Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U143, this, _1, _2, _3)); /* WATCH_FOR_BANNED_ADDRESSES */
	u144Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U144, this, _1, _2, _3)); /* WATCH_FOR_BANNED_ADDRESSES_RESULT */
	u145Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U145, this, _1, _2, _3)); /* STOP_WATCHING_FOR_BANNED_ADDRESSES */
	u146Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U146, this, _1, _2, _3)); /* STOP_WATCHING_FOR_BANNED_ADDRESSES_RESULT */
	u147Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U147, this, _1, _2, _3)); /* BANNED_ADDRESS_ADDED */
	u148Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U148, this, _1, _2, _3)); /* BANNED_ADDRESS_REMOVED */
	u149Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U149, this, _1, _2, _3)); /* KICK_CLIENT */
	u150Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U150, this, _1, _2, _3)); /* KICK_CLIENT_RESULT */
	u151Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U151, this, _1, _2, _3)); /* GET_SERVERMODULELIST_SNAPSHOT */
	u152Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U152, this, _1, _2, _3)); /* SERVERMODULELIST_SNAPSHOT */
	u154Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U154, this, _1, _2, _3)); /* GET_UPC_STATS_SNAPSHOT */
	u155Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U155, this, _1, _2, _3)); /* GET_UPC_STATS_SNAPSHOT_RESULT */
	u156Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U156, this, _1, _2, _3)); /* UPC_STATS_SNAPSHOT */
	u157Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U157, this, _1, _2, _3)); /* RESET_UPC_STATS */
	u160Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U160, this, _1, _2, _3)); /* WATCH_FOR_PROCESSED_UPCS_RESULT */
	u164Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U164, this, _1, _2, _3)); /* CONNECTION_REFUSED */
	u165Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U165, this, _1, _2, _3)); /* GET_NODELIST_SNAPSHOT */
	u153Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U153, this, _1, _2, _3)); /* CLEAR_MODULE_CACHE */
	u158Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U158, this, _1, _2, _3)); /* RESET_UPC_STATS_RESULT */
	u159Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U159, this, _1, _2, _3)); /* WATCH_FOR_PROCESSED_UPCS */
	u161Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U161, this, _1, _2, _3)); /* PROCESSED_UPC_ADDED */
	u162Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U162, this, _1, _2, _3)); /* STOP_WATCHING_FOR_PROCESSED_UPCS */
	u163Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U163, this, _1, _2, _3)); /* STOP_WATCHING_FOR_PROCESSED_UPCS_RESULT */
	u166Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U166, this, _1, _2, _3)); /* NODELIST_SNAPSHOT */
	u167Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U167, this, _1, _2, _3)); /* GET_GATEWAYS_SNAPSHOT */
	u168Listener = std::make_shared<CBUPC>(std::bind(&UnionBridge::U168, this, _1, _2, _3)); /* GATEWAYS_SNAPSHOT */

	AddSelfUPCMessageListeners(*this, true, false, true);
	AddSelfConnectionListeners(connector);
}

/**
 *
 */
UnionBridge::~UnionBridge() {
	DEBUG_OUT("UnionBridge::~UnionBridge()");
	RemoveSelfUPCMessageListeners(*this, true, false, true);
	Disconnect();
	DEBUG_OUT("UnionBridge::~UnionBridge() done");
}

/**
 * set notification mode true to queue notifications ... the notification queue is handled in to the event loop, but the queue won't be processed until after the io callback
 * which clears up a lot of the hierarchy ... particularly useful with http connections
 */
void
UnionBridge::SetQueueNotifications(bool b)
{
	queueNotifications = b;
}

/**
 * begin a connection using the curent connector
 */
void
UnionBridge::Connect()
{
	NxBeginConnect();
	SetConnectionState(ConnectionState::CONNECTION_IN_PROGRESS);
	connector.Connect();
	log.Debug("UnionBridge: connection in progress");
	connectAttemptCount++;
}

/**
 * end a connection using the curent connector
 */
void
UnionBridge::Disconnect()
{
	if (connectionState != ConnectionState::NOT_CONNECTED) {
		SetConnectionState(ConnectionState::DISCONNECTION_IN_PROGRESS);
		log.Debug("UnionBridge::Disconnect()");
		if (connectionState != ConnectionState::CONNECTION_IN_PROGRESS) {
			NxConnectFailure("Clean disconnection in progress via Connector::Disconnect()", UPC::Status::CLIENT_KILL_CONNECT);
		}
		connector.Disconnect();
	}
}


int
UnionBridge::GetAvailableConnectionCount()
{
	std::cerr << "got " << connector.NConnections() << " available connections" << endl;
	return connector.NConnections();
}

/**
 * sets the connection state, and generates a connection state change notification
 * @param state
 */
void
UnionBridge::SetConnectionState(int state)
{
	connectionState=state;
	NxConnectionStateChange();
}

/**
 * server version setter, as recieved in the first server response
 */
void
UnionBridge::SetServerVersion(Version v)
{
	serverVersion = v;
}

/**
 * our own version, or at least the one we're pretending to have ... sent to server with UA string
 */
Version
UnionBridge::GetClientVersion() const
{
	return clientVersion;
}

/**
 * @return version of upc we think we are speaking
 */
Version
UnionBridge::GetUPCVersion() const
{
	return upcVersion;
}

/**
 * @return our personal tag as reported to Union
 */
std::string
UnionBridge::GetClientType() const
{
	return clientType;
}

/**
 * @return the current connection state
 */
int
UnionBridge::GetConnectionState() const
{
	return connectionState;
}

/**
 * @param v this libraries version as reported to Union
 */
void
UnionBridge::SetClientVersion(Version v) const
{
	clientVersion = v;
}

/**
 * @param v the upc version we are communicating with
 */
void
UnionBridge::SetUPCVersion(Version v) const
{
	upcVersion = v;
}

/**
 * @param t the client
 */
void
UnionBridge::SetClientType(std::string t) const
{
	clientType = t;
}


void
UnionBridge::SetEventLoop(EventLoop *l)
{
	loop = l;
	if (queueNotifications) {
		if (loop != nullptr) {
			loop->Worker([this]() {
				ProcessDispatches();
			});
		}
	}
}


void
UnionBridge::AddSelfConnectionListeners(const NXConnection& notifier)
{
	connector.AddListener(Event::RECEIVE_DATA, rxUPCListener);
	connector.AddListener(Event::SEND_DATA, txUPCListener);
	connector.AddListener(Event::DISCONNECTED, disconnectListener);
	connector.AddListener(Event::CONNECTED, connectListener);
	connector.AddListener(Event::SELECT_CONNECTION, selectListener);
	connector.AddListener(Event::IO_ERROR, ioErrorListener);
	connector.AddListener(Event::CONNECT_FAILURE, connectFailureListener);
}

void
UnionBridge::RemoveSelfConnectionListeners(const NXConnection& notifier)
{
	log.Debug("UnionBridge::Removing listeners");
	connector.RemoveListener(Event::RECEIVE_DATA, rxUPCListener);
	connector.RemoveListener(Event::SEND_DATA, txUPCListener);
	connector.RemoveListener(Event::DISCONNECTED, disconnectListener);
	connector.RemoveListener(Event::CONNECTED, connectListener);
	connector.RemoveListener(Event::SELECT_CONNECTION, selectListener);
	connector.RemoveListener(Event::IO_ERROR, ioErrorListener);
	connector.RemoveListener(Event::CONNECT_FAILURE, connectFailureListener);
	log.Debug("Done removing listeners");
}


bool
UnionBridge::IsReady() const
{
	return connector.IsReady() && connectionState == ConnectionState::READY;
}

void
UnionBridge::SendHelloMessage()
{
	SendUPC(UPC::ID::CLIENT_HELLO,
			{	GetClientType(),
				(userAgentString!=""?(userAgentString+";"):"")+GetClientVersion().ToVerboseString(),
				GetUPCVersion().ToString()
			});
}


/**
 * main entry point for sending UPC messages. sends a given message to the server, and after sending, checks for
 * any special messages that may want further interest, intention or notification ... in particular module message messages.
 * @param messageID what to send
 * @param args a vector of string arguments added to the message
 */
void
UnionBridge::SendUPC(UPCMessageID messageID, StringArgs args)
{
	// Quit if the connection isn't ready...
	std::string msgID(messageID);
	if (!connector.IsReady()) {
		log.Warn("[UNION_BRIDGE] Connection not ready. UPC not sent. Message: " + msgID);
		return;
	}

	std::string theUPC = "<U><M>" + msgID + "</M>";

	if (args.size() > 0) {
		theUPC += "<L>";
		for (StringArgs::iterator arg=args.begin(); arg!=args.end(); ++arg) {
			// Wrap any non-filter argument that contains a start tag ("<") in CDATA
			std::string a = *arg;
			if (a.find("<") != std::string::npos) {
				if (a.find("<f t=") != 0) {
					a = "<![CDATA[" + a + "]]>";
				}
			}
			theUPC += "<A>" + a + "</A>";
		}
		theUPC += "</L>";
	}
	theUPC += "</U>";

	numMessagesSent++;
	log.Debug("[UNION_BRIDGE] UPC sent: " + theUPC);
	connector.Send(theUPC);
}

/**
 * getter for the logger
 * @return the logger
 */
ILogger&
UnionBridge::GetLog()
{
	return log;
}

/**
 * setter for the logger
 * @param l the new logger
 */
void
UnionBridge::SetLog(ILogger &l) {
	log = l;
	connector.SetLog(&log);
}

/**
 * sets the current connector, and does the basic setup ... discconnecting the old, and then reconnecting if we are already connected
 * @param c the AbstractConnector
 */
void
UnionBridge::SetConnector(const AbstractConnector &c)
{
	bool reconnect=false;
	RemoveSelfConnectionListeners(connector);
	switch (connectionState) {
	case ConnectionState::CONNECTION_IN_PROGRESS:
	case ConnectionState::READY:
	case ConnectionState::LOGGED_IN:
		connector.Disconnect();
		reconnect = true;
		break;
	}
	connector = c;
	AddSelfConnectionListeners(connector);
	if (reconnect) {
		connector.Connect();
	}
}

/**
 * @return the total messages recieved
 */
int
UnionBridge::GetNumMessagesReceived() const {
  return numMessagesReceived;
}

/**
 * @return the total messages sent
 */
int
UnionBridge::GetNumMessagesSent() const {
  return numMessagesSent;
}

/**
 * @return the total messages received and sent
 */
int
UnionBridge::GetTotalMessages() const {
  return numMessagesSent + numMessagesReceived;
}

int
UnionBridge::GetConnectAttemptCount() const {
	return connectAttemptCount;
}
int
UnionBridge::GetConnectFailedCount() const {
	return connectFailCount;
}


/**
 * @return the ready count
 */
int
UnionBridge::GetReadyCount() const
{
	return readyCount;
}

/**
 * Event::CONNECTED listener
 * @param t EventType
 * @param c the connection if any
 * @param args the message
 * @param status io status
 */
void
UnionBridge::ConnectListener(EventType t, CnxRef c, std::string args, ConnectionStatus status) {
	log.Info("Commencing UPC handshake ...");
	NxBeginHandshake();
	SendHelloMessage();
}

/**
 * Event::SELECT_CONNECTION listener
 * @param t EventType
 * @param c the connection if any
 * @param args the message
 * @param status io status
 */
void
UnionBridge::SelectListener(EventType t, CnxRef c, std::string args, ConnectionStatus status) {
	NotifyListeners(Event::SELECT_CONNECTION, {args}, status);
}
/**
 * Event::DISCONNECTED listener
 * @param t EventType
 * @param c the connection if any
 * @param args the message
 * @param status io status
 */
void
UnionBridge::DisconnectListener(EventType t, CnxRef c, std::string, ConnectionStatus status) {
	NxDisconnected();
	CleanupClosedConnection();
}

/**
 * Event::CONNECT_FAILURE listener
 * @param t EventType
 * @param c the connection if any
 * @param args the message
 * @param status io status
 */
void
UnionBridge::ConnectFailureListener(EventType t, CnxRef c, std::string msg, ConnectionStatus status) {
	log.Info("[UNION_BRIDGE] Cleaning up after unfortunate connection failure ...");
	NxConnectFailure(msg, status);
	CleanupClosedConnection();
	numMessagesReceived = 0;
	numMessagesSent = 0;
	connectFailCount++;
}


/**
 * Event::IO_ERROR listener
 * @param t EventType
 * @param c the connection if any
 * @param args the message
 * @param status io status
 */
void
UnionBridge::IOErrorListener(EventType t, CnxRef c, std::string err, ConnectionStatus status) {
	NxIOError(err, status);
	log.Error("Connector IO Error "+err);
}

/**
 * Event::SEND_DATA listener
 * @param t EventType
 * @param c the connection if any
 * @param args the message
 * @param status io status
 */
void
UnionBridge::UpcSentListener(EventType t, CnxRef c, std::string upc, ConnectionStatus status) {
	NxSendData(upc);
}
/**
 * Event::RECEIVE_DATA listener
 * @param t EventType
 * @param c the connection if any
 * @param args the message
 * @param status io status
 */
void
UnionBridge::UpcReceivedListener(EventType t, CnxRef c, std::string upc, ConnectionStatus status) {
	numMessagesReceived++;

	log.Debug("[UNION_BRIDGE] Message received: " + upc );

	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError err=doc.Parse((const char*)upc.c_str());
	if (err != tinyxml2::XML_NO_ERROR) {
		log.Error("UPC error, XML parse fails");
		NxIOError("XML error in "+upc, UPC::Status::XML_ERROR);
		return;
	}
	std::vector<std::string> upcArgs;
	tinyxml2::XMLElement* root = doc.RootElement();
	while (root != nullptr) {
		if (strcmp(root->Value(), "U")==0) {
			tinyxml2::XMLElement* methodElement = root->FirstChildElement("M");
			if (methodElement == nullptr) {
				log.Error("UPC error, no method");
				return;
			}
			upcArgs.clear();
			const char *methodText = methodElement->GetText();
			int method = atoi(methodText+1);
			tinyxml2::XMLElement* listElement = root->FirstChildElement("L");
			if (listElement != nullptr) {
				tinyxml2::XMLElement* dataElement = listElement->FirstChildElement("A");
				while (dataElement != nullptr) {
					tinyxml2::XMLNode* firstNode = dataElement->FirstChild();
					if (firstNode != nullptr) {
						tinyxml2::XMLText* textNode = dataElement->FirstChild()->ToText();
						if (textNode != nullptr) {
							upcArgs.push_back(textNode->Value());
						} else {
							upcArgs.push_back("");
						}
					} else {
						upcArgs.push_back("");
					}
					dataElement = dataElement->NextSiblingElement("A");
				}
			}
			NxUPCMethod(method, upcArgs);
		} else {
			log.Error("UPC error, not a UPC message");
		}

		root = (root->NextSibling()?root->NextSibling()->ToElement():nullptr);
	}
//	NxReceiveData(upc);

}


/**
 * clean up after an unpleasant occurence
 */
void
UnionBridge::CleanupClosedConnection() {
	SetConnectionState(ConnectionState::NOT_CONNECTED);
	if (removeListenersOnDisconnect) {
		log.Info("[UNION_BRIDGE] Removing registered message listeners.");
		RemoveSelfConnectionListeners(connector);
	} else {
		log.Warn("[UNION_BRIDGE] Leaving message listeners registered. \n Be sure to remove any unwanted message listeners manually.");
	}
}

/**
 * add the corresponding type of UPC callback ... really we are only interested in the incoming and handshake/system requests
 */
void
UnionBridge::AddSelfUPCMessageListeners(const NXUPC& notifier, const bool incoming, const bool outgoing, const bool handshake)
{
	if (outgoing) {
		notifier.AddListener(1, u1Listener); /* SEND_MESSAGE_TO_ROOMS */
		notifier.AddListener(2, u2Listener); /* SEND_MESSAGE_TO_CLIENTS */
		notifier.AddListener(3, u3Listener); /* SET_CLIENT_ATTR */
		notifier.AddListener(4, u4Listener); /* JOIN_ROOM */
		notifier.AddListener(5, u5Listener); /* SET_ROOM_ATTR */
	}
	if (incoming) {
		notifier.AddListener(6, u6Listener); /* JOINED_ROOM */
		notifier.AddListener(7, u7Listener); /* RECEIVE_MESSAGE */
		notifier.AddListener(8, u8Listener); /* CLIENT_ATTR_UPDATE */
		notifier.AddListener(9, u9Listener); /* ROOM_ATTR_UPDATE */
	}
	if (outgoing) {
		notifier.AddListener(10, u10Listener); /* LEAVE_ROOM */
		notifier.AddListener(11, u11Listener); /* CREATE_ACCOUNT */
		notifier.AddListener(12, u12Listener); /* REMOVE_ACCOUNT */
		notifier.AddListener(13, u13Listener); /* CHANGE_ACCOUNT_PASSWORD */
		notifier.AddListener(14, u14Listener); /* LOGIN */
		notifier.AddListener(18, u18Listener); /* GET_CLIENTCOUNT_SNAPSHOT */
		notifier.AddListener(19, u19Listener); /* SYNC_TIME */
		notifier.AddListener(21, u21Listener); /* GET_ROOMLIST_SNAPSHOT */
		notifier.AddListener(24, u24Listener); /* CREATE_ROOM */
		notifier.AddListener(25, u25Listener); /* REMOVE_ROOM */
		notifier.AddListener(26, u26Listener); /* WATCH_FOR_ROOMS */
		notifier.AddListener(27, u27Listener); /* STOP_WATCHING_FOR_ROOMS */
	}
	if (incoming) {
		notifier.AddListener(29, u29Listener); /* CLIENT_METADATA */
		notifier.AddListener(32, u32Listener); /* CREATE_ROOM_RESULT */
		notifier.AddListener(33, u33Listener); /* REMOVE_ROOM_RESULT */
		notifier.AddListener(34, u34Listener); /* CLIENTCOUNT_SNAPSHOT */
		notifier.AddListener(36, u36Listener); /* CLIENT_ADDED_TO_ROOM */
		notifier.AddListener(37, u37Listener); /* CLIENT_REMOVED_FROM_ROOM */
		notifier.AddListener(38, u38Listener); /* ROOMLIST_SNAPSHOT */
		notifier.AddListener(39, u39Listener); /* ROOM_ADDED */
		notifier.AddListener(40, u40Listener); /* ROOM_REMOVED */
		notifier.AddListener(42, u42Listener); /* WATCH_FOR_ROOMS_RESULT */
		notifier.AddListener(43, u43Listener); /* STOP_WATCHING_FOR_ROOMS_RESULT */
		notifier.AddListener(44, u44Listener); /* LEFT_ROOM */
		notifier.AddListener(46, u46Listener); /* CHANGE_ACCOUNT_PASSWORD_RESULT */
		notifier.AddListener(47, u47Listener); /* CREATE_ACCOUNT_RESULT */
		notifier.AddListener(48, u48Listener); /* REMOVE_ACCOUNT_RESULT */
		notifier.AddListener(49, u49Listener); /* LOGIN_RESULT */
	}
	if (outgoing)
		notifier.AddListener(50, u50Listener); /* SERVER_TIME_UPDATE */
	if (incoming)
		notifier.AddListener(54, u54Listener); /* ROOM_SNAPSHOT */

	if (outgoing) {
		notifier.AddListener(55, u55Listener); /* GET_ROOM_SNAPSHOT */
		notifier.AddListener(57, u57Listener); /* SEND_MESSAGE_TO_SERVER */
		notifier.AddListener(58, u58Listener); /* OBSERVE_ROOM */
	}
	if (incoming) {
		notifier.AddListener(59, u59Listener); /* OBSERVED_ROOM */
		notifier.AddListener(60, u60Listener); /* GET_ROOM_SNAPSHOT_RESULT */
	}
	if (outgoing)
		notifier.AddListener(61, u61Listener); /* STOP_OBSERVING_ROOM */
	if (incoming)
		notifier.AddListener(62, u62Listener); /* STOPPED_OBSERVING_ROOM */
	if (handshake) {
		notifier.AddListener(63, u63Listener); /* CLIENT_READY */
	}
	if (outgoing) {
		notifier.AddListener(64, u64Listener); /* SET_ROOM_UPDATE_LEVELS */
		notifier.AddListener(65, u65Listener); /* CLIENT_HELLO */
	}
	if (handshake)
		notifier.AddListener(66, u66Listener); /* SERVER_HELLO */
	if (outgoing) {
		notifier.AddListener(67, u67Listener); /* REMOVE_ROOM_ATTR */
		notifier.AddListener(69, u69Listener); /* REMOVE_CLIENT_ATTR */
		notifier.AddListener(70, u70Listener); /* SEND_ROOMMODULE_MESSAGE */
		notifier.AddListener(71, u71Listener); /* SEND_SERVERMODULE_MESSAGE */
	}
	if (incoming) {
		notifier.AddListener(72, u72Listener); /* JOIN_ROOM_RESULT */
		notifier.AddListener(73, u73Listener); /* SET_CLIENT_ATTR_RESULT */
		notifier.AddListener(74, u74Listener); /* SET_ROOM_ATTR_RESULT */
		notifier.AddListener(75, u75Listener); /* GET_CLIENTCOUNT_SNAPSHOT_RESULT */
		notifier.AddListener(76, u76Listener); /* LEAVE_ROOM_RESULT */
		notifier.AddListener(77, u77Listener); /* OBSERVE_ROOM_RESULT */
		notifier.AddListener(78, u78Listener); /* STOP_OBSERVING_ROOM_RESULT */
		notifier.AddListener(79, u79Listener); /* ROOM_ATTR_REMOVED */
		notifier.AddListener(80, u80Listener); /* REMOVE_ROOM_ATTR_RESULT */
		notifier.AddListener(81, u81Listener); /* CLIENT_ATTR_REMOVED */
		notifier.AddListener(82, u82Listener); /* REMOVE_CLIENT_ATTR_RESULT */
	}
	if (handshake) {
		notifier.AddListener(83, u83Listener); /* TERMINATE_SESSION */
		notifier.AddListener(84, u84Listener); /* SESSION_TERMINATED */
		notifier.AddListener(85, u85Listener); /* SESSION_NOT_FOUND */
	}
	if (outgoing) {
		notifier.AddListener(86, u86Listener); /* LOGOFF */
	}
	if (incoming) {
		notifier.AddListener(87, u87Listener); /* LOGOFF_RESULT */
		notifier.AddListener(88, u88Listener); /* LOGGED_IN */
		notifier.AddListener(89, u89Listener); /* LOGGED_OFF */
		notifier.AddListener(90, u90Listener); /* ACCOUNT_PASSWORD_CHANGED */
	}
	if (outgoing) {
		notifier.AddListener(91, u91Listener); /* GET_CLIENTLIST_SNAPSHOT */
		notifier.AddListener(92, u92Listener); /* WATCH_FOR_CLIENTS */
		notifier.AddListener(93, u93Listener); /* STOP_WATCHING_FOR_CLIENTS */
		notifier.AddListener(94, u94Listener); /* GET_CLIENT_SNAPSHOT */
		notifier.AddListener(95, u95Listener); /* OBSERVE_CLIENT */
		notifier.AddListener(96, u96Listener); /* STOP_OBSERVING_CLIENT */
		notifier.AddListener(97, u97Listener); /* GET_ACCOUNTLIST_SNAPSHOT */
		notifier.AddListener(98, u98Listener); /* WATCH_FOR_ACCOUNTS */
		notifier.AddListener(99, u99Listener); /* STOP_WATCHING_FOR_ACCOUNTS */
		notifier.AddListener(100, u100Listener); /* GET_ACCOUNT_SNAPSHOT */
	}
	if (incoming) {
		notifier.AddListener(101, u101Listener); /* CLIENTLIST_SNAPSHOT */
		notifier.AddListener(102, u102Listener); /* CLIENT_ADDED_TO_SERVER */
		notifier.AddListener(103, u103Listener); /* CLIENT_REMOVED_FROM_SERVER */
		notifier.AddListener(104, u104Listener); /* CLIENT_SNAPSHOT */
		notifier.AddListener(105, u105Listener); /* OBSERVE_CLIENT_RESULT */
		notifier.AddListener(106, u106Listener); /* STOP_OBSERVING_CLIENT_RESULT */
		notifier.AddListener(107, u107Listener); /* WATCH_FOR_CLIENTS_RESULT */
		notifier.AddListener(108, u108Listener); /* STOP_WATCHING_FOR_CLIENTS_RESULT */
		notifier.AddListener(109, u109Listener); /* WATCH_FOR_ACCOUNTS_RESULT */
		notifier.AddListener(110, u110Listener); /* STOP_WATCHING_FOR_ACCOUNTS_RESULT */
		notifier.AddListener(111, u111Listener); /* ACCOUNT_ADDED */
		notifier.AddListener(112, u112Listener); /* ACCOUNT_REMOVED */
		notifier.AddListener(113, u113Listener); /* JOINED_ROOM_ADDED_TO_CLIENT */
		notifier.AddListener(114, u114Listener); /* JOINED_ROOM_REMOVED_FROM_CLIENT */
		notifier.AddListener(115, u115Listener); /* GET_CLIENT_SNAPSHOT_RESULT */
		notifier.AddListener(116, u116Listener); /* GET_ACCOUNT_SNAPSHOT_RESULT */
		notifier.AddListener(117, u117Listener); /* OBSERVED_ROOM_ADDED_TO_CLIENT */
		notifier.AddListener(118, u118Listener); /* OBSERVED_ROOM_REMOVED_FROM_CLIENT */
		notifier.AddListener(119, u119Listener); /* CLIENT_OBSERVED */
		notifier.AddListener(120, u120Listener); /* STOPPED_OBSERVING_CLIENT */
	}
	if (outgoing) {
		notifier.AddListener(121, u121Listener); /* OBSERVE_ACCOUNT */
		notifier.AddListener(122, u122Listener); /* STOP_OBSERVING_ACCOUNT */
	}
	if (incoming) {
		notifier.AddListener(123, u123Listener); /* OBSERVE_ACCOUNT_RESULT */
		notifier.AddListener(124, u124Listener); /* ACCOUNT_OBSERVED */
		notifier.AddListener(125, u125Listener); /* STOP_OBSERVING_ACCOUNT_RESULT */
		notifier.AddListener(126, u126Listener); /* STOPPED_OBSERVING_ACCOUNT */
		notifier.AddListener(127, u127Listener); /* ACCOUNT_LIST_UPDATE */
		notifier.AddListener(128, u128Listener); /* UPDATE_LEVELS_UPDATE */
		notifier.AddListener(129, u129Listener); /* CLIENT_OBSERVED_ROOM */
		notifier.AddListener(130, u130Listener); /* CLIENT_STOPPED_OBSERVING_ROOM */
		notifier.AddListener(131, u131Listener); /* ROOM_OCCUPANTCOUNT_UPDATE */
		notifier.AddListener(132, u132Listener); /* ROOM_OBSERVERCOUNT_UPDATE */
	}
	if (outgoing) {
		notifier.AddListener(133, u133Listener); /* ADD_ROLE */
	}

	if (incoming)
		notifier.AddListener(134, u134Listener); /* ADD_ROLE_RESULT */

	if (outgoing)
		notifier.AddListener(135, u135Listener); /* REMOVE_ROLE */
	if (incoming)
		notifier.AddListener(136, u136Listener); /* REMOVE_ROLE_RESULT */
	if (outgoing)
		notifier.AddListener(137, u137Listener); /* BAN */
	if (incoming)
		notifier.AddListener(138, u138Listener); /* BAN_RESULT */
	if (outgoing)
		notifier.AddListener(139, u139Listener); /* UNBAN */
	if (incoming)
		notifier.AddListener(140, u140Listener); /* UNBAN_RESULT */
	if (outgoing)
		notifier.AddListener(141, u141Listener); /* GET_BANNED_LIST_SNAPSHOT */
	if (incoming)
		notifier.AddListener(142, u142Listener); /* BANNED_LIST_SNAPSHOT */
	if (outgoing)
		notifier.AddListener(143, u143Listener); /* WATCH_FOR_BANNED_ADDRESSES */
	if (incoming)
		notifier.AddListener(144, u144Listener); /* WATCH_FOR_BANNED_ADDRESSES_RESULT */
	if (outgoing)
		notifier.AddListener(145, u145Listener); /* STOP_WATCHING_FOR_BANNED_ADDRESSES */
	if (incoming) {
		notifier.AddListener(146, u146Listener); /* STOP_WATCHING_FOR_BANNED_ADDRESSES_RESULT */
		notifier.AddListener(147, u147Listener); /* BANNED_ADDRESS_ADDED */
		notifier.AddListener(148, u148Listener); /* BANNED_ADDRESS_REMOVED */
	}
	if (outgoing)
		notifier.AddListener(149, u149Listener); /* KICK_CLIENT */
	notifier.AddListener(150, u150Listener); /* KICK_CLIENT_RESULT */
	if (outgoing)
		notifier.AddListener(151, u151Listener); /* GET_SERVERMODULELIST_SNAPSHOT */
	notifier.AddListener(152, u152Listener); /* SERVERMODULELIST_SNAPSHOT */
	if (outgoing) {
		notifier.AddListener(153, u153Listener); /* CLEAR_MODULE_CACHE */
		notifier.AddListener(154, u154Listener); /* GET_UPC_STATS_SNAPSHOT */
	}
	if (incoming) {
		notifier.AddListener(155, u155Listener); /* GET_UPC_STATS_SNAPSHOT_RESULT */
		notifier.AddListener(156, u156Listener); /* UPC_STATS_SNAPSHOT */
	}
	if (outgoing)
		notifier.AddListener(157, u157Listener); /* RESET_UPC_STATS */
	if (incoming)
		notifier.AddListener(158, u158Listener); /* RESET_UPC_STATS_RESULT */
	if (outgoing)
		notifier.AddListener(159, u159Listener); /* WATCH_FOR_PROCESSED_UPCS */
	if (incoming) {
		notifier.AddListener(160, u160Listener); /* WATCH_FOR_PROCESSED_UPCS_RESULT */
		notifier.AddListener(161, u161Listener); /* PROCESSED_UPC_ADDED */
	}
	if (outgoing)
		notifier.AddListener(162, u162Listener); /* STOP_WATCHING_FOR_PROCESSED_UPCS */
	if (incoming)
		notifier.AddListener(163, u163Listener); /* STOP_WATCHING_FOR_PROCESSED_UPCS_RESULT */
	if (handshake)
		notifier.AddListener(164, u164Listener); /* CONNECTION_REFUSED */
	if (outgoing) {
		notifier.AddListener(165, u165Listener); /* GET_NODELIST_SNAPSHOT */
	}
	if (incoming)
		notifier.AddListener(166, u166Listener); /* NODELIST_SNAPSHOT */
	if (outgoing)
		notifier.AddListener(167, u167Listener); /* GET_GATEWAYS_SNAPSHOT */
	if (incoming)
		notifier.AddListener(168, u168Listener); /* GATEWAYS_SNAPSHOT */

}

void
UnionBridge::RemoveSelfUPCMessageListeners(const NXUPC& notifier, const bool incoming, const bool outgoing, bool const handshake)
{
	if (outgoing) {
		notifier.RemoveListener(1, u1Listener); /* SEND_MESSAGE_TO_ROOMS */
		notifier.RemoveListener(2, u2Listener); /* SEND_MESSAGE_TO_CLIENTS */
		notifier.RemoveListener(3, u3Listener); /* SET_CLIENT_ATTR */
		notifier.RemoveListener(4, u4Listener); /* JOIN_ROOM */
		notifier.RemoveListener(5, u5Listener); /* SET_ROOM_ATTR */
	}
	if (incoming) {
		notifier.RemoveListener(6, u6Listener); /* JOINED_ROOM */
		notifier.RemoveListener(7, u7Listener); /* RECEIVE_MESSAGE */
		notifier.RemoveListener(8, u8Listener); /* CLIENT_ATTR_UPDATE */
		notifier.RemoveListener(9, u9Listener); /* ROOM_ATTR_UPDATE */
	}
	if (outgoing) {
		notifier.RemoveListener(10, u10Listener); /* LEAVE_ROOM */
		notifier.RemoveListener(11, u11Listener); /* CREATE_ACCOUNT */
		notifier.RemoveListener(12, u12Listener); /* REMOVE_ACCOUNT */
		notifier.RemoveListener(13, u13Listener); /* CHANGE_ACCOUNT_PASSWORD */
		notifier.RemoveListener(14, u14Listener); /* LOGIN */
		notifier.RemoveListener(18, u18Listener); /* GET_CLIENTCOUNT_SNAPSHOT */
		notifier.RemoveListener(19, u19Listener); /* SYNC_TIME */
		notifier.RemoveListener(21, u21Listener); /* GET_ROOMLIST_SNAPSHOT */
		notifier.RemoveListener(24, u24Listener); /* CREATE_ROOM */
		notifier.RemoveListener(25, u25Listener); /* REMOVE_ROOM */
		notifier.RemoveListener(26, u26Listener); /* WATCH_FOR_ROOMS */
		notifier.RemoveListener(27, u27Listener); /* STOP_WATCHING_FOR_ROOMS */
	}

	if (incoming) {
		notifier.RemoveListener(29, u29Listener); /* CLIENT_METADATA */
		notifier.RemoveListener(32, u32Listener); /* CREATE_ROOM_RESULT */
		notifier.RemoveListener(33, u33Listener); /* REMOVE_ROOM_RESULT */
		notifier.RemoveListener(34, u34Listener); /* CLIENTCOUNT_SNAPSHOT */
		notifier.RemoveListener(36, u36Listener); /* CLIENT_ADDED_TO_ROOM */
		notifier.RemoveListener(37, u37Listener); /* CLIENT_REMOVED_FROM_ROOM */
		notifier.RemoveListener(38, u38Listener); /* ROOMLIST_SNAPSHOT */
		notifier.RemoveListener(39, u39Listener); /* ROOM_ADDED */
		notifier.RemoveListener(40, u40Listener); /* ROOM_REMOVED */
		notifier.RemoveListener(42, u42Listener); /* WATCH_FOR_ROOMS_RESULT */
		notifier.RemoveListener(43, u43Listener); /* STOP_WATCHING_FOR_ROOMS_RESULT */
		notifier.RemoveListener(44, u44Listener); /* LEFT_ROOM */
		notifier.RemoveListener(46, u46Listener); /* CHANGE_ACCOUNT_PASSWORD_RESULT */
		notifier.RemoveListener(47, u47Listener); /* CREATE_ACCOUNT_RESULT */
		notifier.RemoveListener(48, u48Listener); /* REMOVE_ACCOUNT_RESULT */
		notifier.RemoveListener(49, u49Listener); /* LOGIN_RESULT */
	}
	if (outgoing) {
		notifier.RemoveListener(50, u50Listener); /* SERVER_TIME_UPDATE */
	}
	if (incoming) {
		notifier.RemoveListener(54, u54Listener); /* ROOM_SNAPSHOT */
	}
	if (outgoing) {
		notifier.RemoveListener(55, u55Listener); /* GET_ROOM_SNAPSHOT */
		notifier.RemoveListener(57, u57Listener); /* SEND_MESSAGE_TO_SERVER */
		notifier.RemoveListener(58, u58Listener); /* OBSERVE_ROOM */
	}
	if (incoming) {
		notifier.RemoveListener(59, u59Listener); /* OBSERVED_ROOM */
		notifier.RemoveListener(60, u60Listener); /* GET_ROOM_SNAPSHOT_RESULT */
	}
	if (outgoing) {
		notifier.RemoveListener(61, u61Listener); /* STOP_OBSERVING_ROOM */
	}
	if (incoming) {
		notifier.RemoveListener(62, u62Listener); /* STOPPED_OBSERVING_ROOM */
	}
	if (handshake)
		notifier.RemoveListener(63, u63Listener); /* CLIENT_READY */
	if (outgoing) {
		notifier.RemoveListener(64, u64Listener); /* SET_ROOM_UPDATE_LEVELS */
		notifier.RemoveListener(65, u65Listener); /* CLIENT_HELLO */
	}
	if (handshake) {
		notifier.RemoveListener(66, u66Listener); /* SERVER_HELLO */
	}
	if (outgoing) {
		notifier.RemoveListener(67, u67Listener); /* REMOVE_ROOM_ATTR */
		notifier.RemoveListener(69, u69Listener); /* REMOVE_CLIENT_ATTR */
		notifier.RemoveListener(70, u70Listener); /* SEND_ROOMMODULE_MESSAGE */
		notifier.RemoveListener(71, u71Listener); /* SEND_SERVERMODULE_MESSAGE */
	}
	if (incoming) {
		notifier.RemoveListener(72, u72Listener); /* JOIN_ROOM_RESULT */
		notifier.RemoveListener(73, u73Listener); /* SET_CLIENT_ATTR_RESULT */
		notifier.RemoveListener(74, u74Listener); /* SET_ROOM_ATTR_RESULT */
		notifier.RemoveListener(75, u75Listener); /* GET_CLIENTCOUNT_SNAPSHOT_RESULT */
		notifier.RemoveListener(76, u76Listener); /* LEAVE_ROOM_RESULT */
		notifier.RemoveListener(77, u77Listener); /* OBSERVE_ROOM_RESULT */
		notifier.RemoveListener(78, u78Listener); /* STOP_OBSERVING_ROOM_RESULT */
		notifier.RemoveListener(79, u79Listener); /* ROOM_ATTR_REMOVED */
		notifier.RemoveListener(80, u80Listener); /* REMOVE_ROOM_ATTR_RESULT */
		notifier.RemoveListener(81, u81Listener); /* CLIENT_ATTR_REMOVED */
		notifier.RemoveListener(82, u82Listener); /* REMOVE_CLIENT_ATTR_RESULT */
	}

	if (handshake) {
		notifier.RemoveListener(83, u83Listener); /* TERMINATE_SESSION */
		notifier.RemoveListener(84, u84Listener); /* SESSION_TERMINATED */
		notifier.RemoveListener(85, u85Listener); /* SESSION_NOT_FOUND */
	}
	if (outgoing) {
		notifier.RemoveListener(86, u86Listener); /* LOGOFF */
		notifier.RemoveListener(87, u87Listener); /* LOGOFF_RESULT */
		notifier.RemoveListener(88, u88Listener); /* LOGGED_IN */
		notifier.RemoveListener(89, u89Listener); /* LOGGED_OFF */
		notifier.RemoveListener(90, u90Listener); /* ACCOUNT_PASSWORD_CHANGED */
		notifier.RemoveListener(91, u91Listener); /* GET_CLIENTLIST_SNAPSHOT */
		notifier.RemoveListener(92, u92Listener); /* WATCH_FOR_CLIENTS */
		notifier.RemoveListener(93, u93Listener); /* STOP_WATCHING_FOR_CLIENTS */
		notifier.RemoveListener(94, u94Listener); /* GET_CLIENT_SNAPSHOT */
		notifier.RemoveListener(95, u95Listener); /* OBSERVE_CLIENT */
		notifier.RemoveListener(96, u96Listener); /* STOP_OBSERVING_CLIENT */
		notifier.RemoveListener(97, u97Listener); /* GET_ACCOUNTLIST_SNAPSHOT */
		notifier.RemoveListener(98, u98Listener); /* WATCH_FOR_ACCOUNTS */
		notifier.RemoveListener(99, u99Listener); /* STOP_WATCHING_FOR_ACCOUNTS */
		notifier.RemoveListener(100, u100Listener); /* GET_ACCOUNT_SNAPSHOT */
	}
	if (incoming) {
		notifier.RemoveListener(101, u101Listener); /* CLIENTLIST_SNAPSHOT */
		notifier.RemoveListener(102, u102Listener); /* CLIENT_ADDED_TO_SERVER */
		notifier.RemoveListener(103, u103Listener); /* CLIENT_REMOVED_FROM_SERVER */
		notifier.RemoveListener(104, u104Listener); /* CLIENT_SNAPSHOT */
		notifier.RemoveListener(105, u105Listener); /* OBSERVE_CLIENT_RESULT */
		notifier.RemoveListener(106, u106Listener); /* STOP_OBSERVING_CLIENT_RESULT */
		notifier.RemoveListener(107, u107Listener); /* WATCH_FOR_CLIENTS_RESULT */
		notifier.RemoveListener(108, u108Listener); /* STOP_WATCHING_FOR_CLIENTS_RESULT */
		notifier.RemoveListener(109, u109Listener); /* WATCH_FOR_ACCOUNTS_RESULT */
		notifier.RemoveListener(110, u110Listener); /* STOP_WATCHING_FOR_ACCOUNTS_RESULT */
		notifier.RemoveListener(111, u111Listener); /* ACCOUNT_ADDED */
		notifier.RemoveListener(112, u112Listener); /* ACCOUNT_REMOVED */
		notifier.RemoveListener(113, u113Listener); /* JOINED_ROOM_ADDED_TO_CLIENT */
		notifier.RemoveListener(114, u114Listener); /* JOINED_ROOM_REMOVED_FROM_CLIENT */
		notifier.RemoveListener(115, u115Listener); /* GET_CLIENT_SNAPSHOT_RESULT */
		notifier.RemoveListener(116, u116Listener); /* GET_ACCOUNT_SNAPSHOT_RESULT */
		notifier.RemoveListener(117, u117Listener); /* OBSERVED_ROOM_ADDED_TO_CLIENT */
		notifier.RemoveListener(118, u118Listener); /* OBSERVED_ROOM_REMOVED_FROM_CLIENT */
		notifier.RemoveListener(119, u119Listener); /* CLIENT_OBSERVED */
		notifier.RemoveListener(120, u120Listener); /* STOPPED_OBSERVING_CLIENT */
	}
	if (outgoing) {
		notifier.RemoveListener(121, u121Listener); /* OBSERVE_ACCOUNT */
		notifier.RemoveListener(122, u122Listener); /* STOP_OBSERVING_ACCOUNT */
	}
	if (incoming) {
		notifier.RemoveListener(123, u123Listener); /* OBSERVE_ACCOUNT_RESULT */
		notifier.RemoveListener(124, u124Listener); /* ACCOUNT_OBSERVED */
		notifier.RemoveListener(125, u125Listener); /* STOP_OBSERVING_ACCOUNT_RESULT */
		notifier.RemoveListener(126, u126Listener); /* STOPPED_OBSERVING_ACCOUNT */
		notifier.RemoveListener(127, u127Listener); /* ACCOUNT_LIST_UPDATE */
		notifier.RemoveListener(128, u128Listener); /* UPDATE_LEVELS_UPDATE */
		notifier.RemoveListener(129, u129Listener); /* CLIENT_OBSERVED_ROOM */
		notifier.RemoveListener(130, u130Listener); /* CLIENT_STOPPED_OBSERVING_ROOM */
		notifier.RemoveListener(131, u131Listener); /* ROOM_OCCUPANTCOUNT_UPDATE */
		notifier.RemoveListener(132, u132Listener); /* ROOM_OBSERVERCOUNT_UPDATE */
	}
	if (outgoing) {
		notifier.RemoveListener(133, u133Listener); /* ADD_ROLE */
	}
	if (incoming)
		notifier.RemoveListener(134, u134Listener); /* ADD_ROLE_RESULT */
	if (outgoing)
		notifier.RemoveListener(135, u135Listener); /* REMOVE_ROLE */
	if (incoming)
		notifier.RemoveListener(136, u136Listener); /* REMOVE_ROLE_RESULT */
	if (outgoing)
		notifier.RemoveListener(137, u137Listener); /* BAN */
	if (incoming)
		notifier.RemoveListener(138, u138Listener); /* BAN_RESULT */
	if (outgoing)
		notifier.RemoveListener(139, u139Listener); /* UNBAN */
	if (incoming)
		notifier.RemoveListener(140, u140Listener); /* UNBAN_RESULT */
	if (outgoing)
		notifier.RemoveListener(141, u141Listener); /* GET_BANNED_LIST_SNAPSHOT */
	if (incoming)
		notifier.RemoveListener(142, u142Listener); /* BANNED_LIST_SNAPSHOT */
	if (outgoing)
		notifier.RemoveListener(143, u143Listener); /* WATCH_FOR_BANNED_ADDRESSES */
	if (incoming)
		notifier.RemoveListener(144, u144Listener); /* WATCH_FOR_BANNED_ADDRESSES_RESULT */
	if (outgoing)
		notifier.RemoveListener(145, u145Listener); /* STOP_WATCHING_FOR_BANNED_ADDRESSES */
	if (incoming) {
		notifier.RemoveListener(146, u146Listener); /* STOP_WATCHING_FOR_BANNED_ADDRESSES_RESULT */
		notifier.RemoveListener(147, u147Listener); /* BANNED_ADDRESS_ADDED */
		notifier.RemoveListener(148, u148Listener); /* BANNED_ADDRESS_REMOVED */
	}
	if (outgoing)
		notifier.RemoveListener(149, u149Listener); /* KICK_CLIENT */
	if (incoming)
		notifier.RemoveListener(150, u150Listener); /* KICK_CLIENT_RESULT */
	if (outgoing)
		notifier.RemoveListener(151, u151Listener); /* GET_SERVERMODULELIST_SNAPSHOT */
	if (incoming)
		notifier.RemoveListener(152, u152Listener); /* SERVERMODULELIST_SNAPSHOT */
	if (outgoing) {
		notifier.RemoveListener(153, u153Listener); /* CLEAR_MODULE_CACHE */
		notifier.RemoveListener(154, u154Listener); /* GET_UPC_STATS_SNAPSHOT */
	}
	if (incoming) {
		notifier.RemoveListener(155, u155Listener); /* GET_UPC_STATS_SNAPSHOT_RESULT */
		notifier.RemoveListener(156, u156Listener); /* UPC_STATS_SNAPSHOT */
	}
	if (outgoing)
		notifier.RemoveListener(157, u157Listener); /* RESET_UPC_STATS */
	if (incoming)
		notifier.RemoveListener(158, u158Listener); /* RESET_UPC_STATS_RESULT */
	if (outgoing)
		notifier.RemoveListener(159, u159Listener); /* WATCH_FOR_PROCESSED_UPCS */
	if (incoming) {
		notifier.RemoveListener(160, u160Listener); /* WATCH_FOR_PROCESSED_UPCS_RESULT */
		notifier.RemoveListener(161, u161Listener); /* PROCESSED_UPC_ADDED */
	}
	if (outgoing)
		notifier.RemoveListener(162, u162Listener); /* STOP_WATCHING_FOR_PROCESSED_UPCS */
	if (incoming)
		notifier.RemoveListener(163, u163Listener); /* STOP_WATCHING_FOR_PROCESSED_UPCS_RESULT */
	if (handshake)
		notifier.RemoveListener(164, u164Listener); /* CONNECTION_REFUSED */
	if (outgoing) {
		notifier.RemoveListener(165, u165Listener); /* GET_NODELIST_SNAPSHOT */
	}
	if (incoming)
		notifier.RemoveListener(166, u166Listener); /* NODELIST_SNAPSHOT */
	if (outgoing)
		notifier.RemoveListener(167, u167Listener); /* GET_GATEWAYS_SNAPSHOT */
	if (incoming)
		notifier.RemoveListener(168, u168Listener); /* GATEWAYS_SNAPSHOT */
}

////////////////////////////////////
// notify and event hooks
///////////////////////////////////

bool
UnionBridge::AddUPCListener(const EventType eventType, const CBUPCRef listener) const {
	if (queueNotifications && loop != nullptr) {
		loop->Lock();
	}
	auto r = AddListener(eventType, listener);
	if (queueNotifications && loop != nullptr) {
		loop->Unlock();
	}
	return r;
}

bool
UnionBridge::RemoveUPCListener(const EventType eventType, const CBUPCRef listener) const {
	if (queueNotifications && loop != nullptr) {
		if (loop == nullptr) return false;
		loop->Lock();
	}
	RemoveListener(eventType, listener);
	if (queueNotifications && loop != nullptr) {
		loop->Unlock();
	}
	return true;
}

CBUPCRef
UnionBridge::AddUPCListener(const EventType eventType, const CBUPC listener) const {
	if (queueNotifications && loop != nullptr) {
		if (loop == nullptr) return CBUPCRef();
		loop->Lock();
	}
	auto r = AddEventListener(eventType, listener);
	if (queueNotifications && loop != nullptr) {
		loop->Unlock();
	}
	return r;
}

/**
 * NotifyUPCMessage hook
 *
 * this is now doing the job of NxClientKillConnect, NxServerKillConnect ... xxx for the moment, if it becomes important
 * note that they (following Orbiter semantics) also generated Event::DISCONNECTED
 */
void
UnionBridge::NxConnectFailure(const std::string msg, const UPCStatus s) {
	if (queueNotifications) {
		DispatchEvent(Event::CONNECT_FAILURE, {msg}, s);
	} else {
		NotifyListeners(Event::CONNECT_FAILURE, { msg }, s);
	}
}

/**
 * NotifyUPCMessage hook
 */
void
UnionBridge::NxBeginConnect() {
	if (queueNotifications) {
		DispatchEvent(Event::BEGIN_CONNECT, {}, connectionState);
	} else {
		NotifyListeners(Event::BEGIN_CONNECT, {}, connectionState);
	}
}

/**
 * NotifyUPCMessage hook
 */
void
UnionBridge::NxBeginHandshake() {
	if (queueNotifications) {
		DispatchEvent(Event::CONNECTED, {}, connectionState);
	} else {
		NotifyListeners(Event::CONNECTED, {}, connectionState);
	} 
}

/**
 * NotifyUPCMessage hook
 */
void
UnionBridge::NxConnectionStateChange() {
	if (queueNotifications) {
		DispatchEvent(Event::CONNECTION_STATE_CHANGE, {}, connectionState);
	} else {
		NotifyListeners(Event::CONNECTION_STATE_CHANGE, {}, connectionState);
	} 
}


/**
 * NotifyUPCMessage hook
 */
void
UnionBridge::NxDisconnected() {
	if (queueNotifications) {
		DispatchEvent(Event::DISCONNECTED, {}, UPC::Status::SUCCESS);
	} else {
		NotifyListeners(Event::DISCONNECTED, {}, UPC::Status::SUCCESS);
	} 
}

/**
 * NotifyUPCMessage hook
 */
void
UnionBridge::NxSendData(std::string data) {
	if (queueNotifications) {
		DispatchEvent(Event::SEND_DATA, {data}, UPC::Status::SUCCESS);
	} else {
		NotifyListeners(Event::SEND_DATA, {data}, UPC::Status::SUCCESS);
	} 
}

/**
 * NotifyUPCMessage hook
 */
void
UnionBridge::NxReceiveData(std::string data) {
	if (queueNotifications) {
		DispatchEvent(Event::RECEIVE_DATA, {data}, UPC::Status::SUCCESS);
	} else {
		NotifyListeners(Event::RECEIVE_DATA, {data}, UPC::Status::SUCCESS);
	} 
}

/**
 * NotifyUPCMessage hook
 */
void
UnionBridge::NxReady() {
	if (queueNotifications) {
		DispatchEvent(Event::READY, {}, UPC::Status::SUCCESS);
	} else {
		NotifyListeners(Event::READY, {}, UPC::Status::SUCCESS);
	} 
}

/**
 * NotifyUPCMessage hook
 */
void
UnionBridge::NxProtocolIncompatible(std::string version) {
	if (queueNotifications) {
		DispatchEvent(Event::CONNECT_FAILURE, {version}, UPC::Status::PROTOCOL_INCOMPATIBLE);
	} else {
		NotifyListeners(Event::CONNECT_FAILURE, {version}, UPC::Status::PROTOCOL_INCOMPATIBLE);
	} 
}

/**
* NotifyUPCMessage hook
*/
void
UnionBridge::NxConnectRefused(std::string reason, std::string description) {
	if (queueNotifications) {
		DispatchEvent(Event::CONNECT_FAILURE, { reason, description }, UPC::Status::CONNECT_TIMEOUT);
	}
	else {
		NotifyListeners(Event::CONNECT_FAILURE, { reason, description }, UPC::Status::CONNECT_TIMEOUT);
	}
}

/**
 * NotifyUPCMessage hook
 */
void
UnionBridge::NxUPCMethod(const int method, const StringArgs upcArgs)
{
	if (queueNotifications) {
		DispatchEvent(method, upcArgs, UPC::Status::SUCCESS);
	}
	else {
		NotifyListeners(method, upcArgs, UPC::Status::SUCCESS);
	} 
}

/**
 * NotifyUPCMessage hook
 */
void
UnionBridge::NxIOError(std::string err, UPCStatus status)
{
	if (queueNotifications) {
		DispatchEvent(Event::IO_ERROR, {err}, status);
	} else {
		NotifyListeners(Event::IO_ERROR, {err}, status);
	} 
}


