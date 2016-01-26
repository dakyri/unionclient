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

#undef MESSAGE_FILTERS_ON_LISTENERS
#undef SNAPSHOT_MANAGER

/**
 * @class UnionBridge UnionBridge.h
 * callbacks for class UnionBridge ... responders for the plethora of UPC messages, numbered according to the respective upc
 * originally the headers were generated automatically from the list of possible messages ... atm the outgoing classes are superfluous, but perhaps at some stage, may
 * be useful to keep these as entry points to trigger server side actions by message
 */
void
UnionBridge::U1(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SEND_MESSAGE_TO_ROOMS */
{
}
void
UnionBridge::U2(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SEND_MESSAGE_TO_CLIENTS */
{
}
void
UnionBridge::U3(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SET_CLIENT_ATTR */
{
}
void
UnionBridge::U4(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* JOIN_ROOM */
{
}
void
UnionBridge::U5(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SET_ROOM_ATTR */
{
}

/**
 * handle a union u6, joined room message
 * @param t the union command converted to a digit
 * @param args various data elements provided in the upc xml packet
 * @param ioStatus any io event status recieved
 */
void
UnionBridge::U6(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* JOINED_ROOM */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] u6, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	RoomRef room = roomManager.AddOccupiedRoom(roomID);
	if (!room) return;
	room->OnJoin();
	// Fire JOIN through the client
	ClientRef selfClient = clientManager.Self();
	if (selfClient) {
		log.Debug("triggering self join room");
		selfClient->OnJoinRoom(room, roomID);
	}
}

/**
 * we received a message message, a upc u7  ... generate a notification by the more clunky interface used for messages, but also check and send for
 * the more useful and polite message specific notifications
 * prezi custom and module messages also come back through here ... prezi module messages come in as broadcast type 2 (client) but with an empty client ...
 * presumably this means to the self() client ... or perhaps all clients? ... in any case, the prezi messages seen so far are more system oriented
 * @param t the union command converted to a digit
 * @param args various data elements provided in the upc xml packet
 * @param ioStatus any io event status recieved
 */
void
UnionBridge::U7(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* RECEIVE_MESSAGE */
{
	if (args.size() < 4)  {
		log.Error("[UNION BRIDGE] u7, malformed packet, argument mismatch, ignoring");
		return;
	}
	/*
	 * xxx some confusion here between user1 source and between what the upc protocol implies ... source is quite clear that there is a singular room or
	 * client ... upc spec implies potentially (RS separated?) lists ... uncertain of what the custom prezi code does
	 */
	std::string messageID(args[0]);
	std::string broadcastType(args[1]);
	ClientID clientID(args[2]);
	RoomID roomID(args[3]);

	bool wasListenerError=false;
	std::exception listenerError;
	ClientRef client;
	RoomRef room;

	StringArgs messageBody;
	for (unsigned int i=4; i<args.size(); i++) {
		messageBody.push_back(args[i]);
	}

	if (clientID != "") {
		client = clientManager.Request(clientID);
	}
	// ===== To Clients, or To Server =====
	// If the message was sent to a specific client, a list of specific clients,
	// or to the whole server, then args passed to registered message listeners
	// are: the Client object plus all user-defined arguments originally passed
	// to sendMessage().
	if (broadcastType == RxMsgBroadcastType::TO_CLIENTS) {
		log.Debug("broadcasting to clients " + clientID);
		if (clientID != "") {
			NotifyMessageListeners(messageID, {clientID}, {}, messageBody);
		} else {
			NotifyMessageListeners(messageID, {}, {}, messageBody);
		}
	} else if (broadcastType == RxMsgBroadcastType::TO_SERVER) {
		log.Debug("broadcasting to server");
		NotifyMessageListeners(messageID, {}, {}, messageBody);
	} else if (broadcastType == RxMsgBroadcastType::TO_ROOMS){
		log.Debug("broadcasting to rooms" + roomID);
		NotifyMessageListeners(messageID, {}, {roomID}, messageBody);

#ifdef MESSAGE_FILTERS_ON_LISTENERS
		UPCStatus status = UPC::Status::SUCCESS;
		room = roomManager.Get(roomID);
		if (!room) {
			log.Warn("Message (u7) received for unknown room: [" + roomID + "]" + "Message: [" + messageID + "]");
			return;
		}

		// RECEIVE_MESSAGE was a response to SEND_MESSAGE_TO_ROOMS, so
		// we notify listeners only if they asked to be told about messages
		// sent to the recipient room.

		RoomID toRoomSimpleID  = UPCUtils::GetSimpleRoomID(roomID);
		RoomQualifier toRoomQualifier = UPCUtils::GetRoomQualifier(roomID);

		bool listenerFound = false;
		bool listenerIgnoredMessage = false;

		auto jt = messageBody.begin();
		jt = messageBody.insert(jt, roomID);
		jt = messageBody.insert(jt, clientID);
		messageBody.insert(jt, messageID);

		bool hasRoomIDFilter = false;

		if (hasRoomIDFilter) {
			for (auto it=listeners.begin(); it!=listeners.end(); it++) {
				listenerIgnoredMessage = true;

				listenerFound = true;
				listenerIgnoredMessage = false;

				// --- Has a "forRoomIDs" filter ---
				// If the message was sent to any of the rooms the listener is
				// interested in, then notify that listener. Note that a listener
				// for messages sent to room foo.* means the listener wants
				// notifications for all rooms whose ids start with foo.
				var listenerRoomIDs  = messageListener.GetForRoomIDs();
				var listenerRoomQualifier;
				var listenerRoomSimpleID;
				// ===== Run once for each room id =====
				var listenerRoomIDString;
				for (var j = 0; j < listenerRoomIDs.length; j++) {
					listenerRoomIDString = listenerRoomIDs[j];
					// Split the room id
					listenerRoomQualifier = UPCUtils::GetRoomQualifier(listenerRoomIDString);
					listenerRoomSimpleID  = UPCUtils::GetSimpleRoomID(listenerRoomIDString);

					// Check if the listener is interested in the recipient room...
					if (listenerRoomQualifier == toRoomQualifier &&
							(listenerRoomSimpleID == toRoomSimpleID || listenerRoomSimpleID == "*")) {
						if (listenerRoomIDs.length == 1) {
							// The listener is interested in messages sent to a
							// specific room only, so omit the "toRoom" arg.
							// (The listener already knows the target room because
							// it's only notified if the message was sent to that room.)
							args = [client].concat(messageBody);
						} else {
							// The listener is interested in messages sent to
							// multiple rooms. In this case, we have to
							// include the "toRoom" arg so the listener knows
							// which room received the message.
							args = [client, room].concat(messageBody);
						}

						try {
							messageListener.GetListenerFunction().apply(messageListener.GetThisArg(), args);
						} catch (e) {
							listenerError = e;
						}
						listenerFound = true;
						listenerIgnoredMessage = false;
						break; // Stop looking at this listener's room ids
					}
				}
			}
			if (listenerIgnoredMessage) {
				log.Debug("Message listener ignored message: " + messageID +
						". Listener registered to receive messages sent, but message was sent to: " + roomID);
			}
		}
		if (!listenerFound) {
			log.Warn("No message listener handled incoming message: " + messageID + ", sent to: " + roomID);
		}
#endif
	}
	if (wasListenerError) {
		log.Error("A message listener for incoming message [" + messageID + "]" +
			(client? "" : (", received from client [" + client->GetClientID() + "],"))+
			" encountered an error:\n\n" + listenerError.what());

//		"\n\nEnsure that all [" + message + "] listeners supply a first"
//		" parameter whose datatype is Client (or a compatible type). Listeners"
//		" that registered for the message via MessageManager's addMessageListener()"
//		" with anything other than a single roomID for the toRoomIDs parameter must"
//		" also define a second paramter whose"
//		" datatype is Room (or a compatible type). Finally, ensure that"
//		" the listener's declared message parameters match the following actual message"
//		" arguments:\n    " + userDefinedArgs
//		+ (typeof listenerError.stack === "undefined" ? "" : "\n\nStack trace follows:\n" + listenerError.stack)
	}
}

/**
 * callback for client attribute update
 */
void
UnionBridge::U8(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_ATTR_UPDATE */
{
	if (args.size() < 6)  {
		log.Error("[UNION BRIDGE] u8, malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string attrScope(args[0]);
	ClientID clientID(args[1]);
	UserID userID(args[2]);
	AttrName attrName(args[3]);
	AttrVal attrVal(args[4]);
	int attrOptions = atoi(args[5].c_str());

	log.Info("[UNION BRIDGE] U8() setting attribute "+attrName+" to "+attrVal+" for "+clientID);

	ClientRef client;
	AccountRef account;
	if (attrOptions &Attribute::FLAG_PERSISTENT) {
		account = accountManager.GetAccount(userID);
		if (account) {
			account->SetAttributeLocal(attrName, attrVal, attrScope);
		} else {
			log.Error("[CORE_MESSAGE_LISTENER] Received an attribute update for an unknown user account [" + userID + "].");
			return;
		}

	} else {
		client = clientManager.Get(clientID);
		if (client) {
			client->SetAttributeLocal(attrName, attrVal, attrScope);
		} else {
			log.Error("[CORE_MESSAGE_LISTENER] Received an attribute update for an unknown client [" + clientID + "]. ");
			return;
		}

	}
}
void
UnionBridge::U9(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ROOM_ATTR_UPDATE */
{
	if (args.size() < 4)  {
		log.Error("[UNION BRIDGE] u9, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	ClientID byClientID(args[1]);
	AttrName attrName(args[2]);
	AttrVal attrVal(args[3]);

	RoomRef theRoom = roomManager.Get(roomID);
	ClientRef byClient;

	// Quit if the room isn't found
	if (!theRoom) {
		log.Warn("Room attribute update received for server-side room with no"
				" matching client-side Room object. Room ID [" +
				roomID + "]. Attribute: [" + attrName + "].");
		return;
	}

	// Retrieve the Client object for the sender
	if (byClientID == "") {
		// No client ID was supplied, so the message was generated by the
		// server, not by a client, so set fromClient to null.
		byClient = nullptr;
	} else {
		// A valid ID was supplied, so find or create the matching IClient object
		byClient = clientManager.Request(byClientID);
	}

	theRoom->SetAttributeLocal(attrName, attrVal, Token::GLOBAL_ATTR, byClient);
}

void
UnionBridge::U10(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* LEAVE_ROOM */
{
}
void
UnionBridge::U11(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CREATE_ACCOUNT */
{
}
void
UnionBridge::U12(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* REMOVE_ACCOUNT */
{
}
void
UnionBridge::U13(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CHANGE_ACCOUNT_PASSWORD */
{
}
void
UnionBridge::U14(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* LOGIN */
{
}
void
UnionBridge::U18(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_CLIENTCOUNT_SNAPSHOT */
{
}
void
UnionBridge::U19(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SYNC_TIME */
{
}
void
UnionBridge::U21(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_ROOMLIST_SNAPSHOT */
{
}
void
UnionBridge::U24(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CREATE_ROOM */
{
}
void
UnionBridge::U25(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* REMOVE_ROOM */
{
}
void
UnionBridge::U26(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* WATCH_FOR_ROOMS */
{
}
void
UnionBridge::U27(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_WATCHING_FOR_ROOMS */
{
}

void
UnionBridge::U29(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_METADATA */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] u29, malformed packet, argument mismatch, ignoring");
		return;
	}

	log.Debug("Client Metadata "+args[0]);
	ClientRef theClient = clientManager.Request(args[0]);
	clientManager.SetSelf(theClient);
}

void
UnionBridge::U32(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CREATE_ROOM_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u32, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());
	RoomRef theRoom = roomManager.Get(roomID);
	switch (status) {
	case UPC::Status::UPC_ERROR:
	case UPC::Status::SUCCESS:
	case UPC::Status::ROOM_EXISTS:
	case UPC::Status::PERMISSION_DENIED:
		roomManager.OnCreateRoomResult(UPCUtils::GetRoomQualifier(roomID), UPCUtils::GetSimpleRoomID(roomID), status);
		break;

	default:
		log.Warn("Unrecognized status code for u32. Room ID: [" + roomID + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U33(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* REMOVE_ROOM_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u33, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());
	roomManager.OnRemoveRoomResult(UPCUtils::GetRoomQualifier(roomID),
			UPCUtils::GetSimpleRoomID(roomID),
			status);
	switch (status) {
	case UPC::Status::UPC_ERROR:
		log.Warn("Server error for room removal attempt: " + roomID);
		break;
	case UPC::Status::PERMISSION_DENIED:
		log.Info("Attempt to remove room [" + roomID + "] failed. Permission denied. See server log for details.");
		break;

	case UPC::Status::SUCCESS:
	case UPC::Status::ROOM_NOT_FOUND:
		if (roomManager.Get(roomID)) {
			roomManager.Dispose(roomID);
		}
		break;

	case UPC::Status::AUTHORIZATION_REQUIRED:
	case UPC::Status::AUTHORIZATION_FAILED:
		break;

	default:
		log.Warn("Unrecognized status code for u33. Room ID: [" + roomID + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U34(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENTCOUNT_SNAPSHOT */
{
	if (args.size() < 2) {
		log.Error("[UNION BRIDGE] u34, malformed packet, argument mismatch, ignoring");
		return;
	}

#ifdef SNAPSHOT_MANAGER
	std::string requestID(args[0]);
	int numClients = atoi(args[1].c_str());
	snapshotManager.RecieveClientCountSnapshot(requestID, numClients);
#endif
}
void
UnionBridge::U36(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_ADDED_TO_ROOM */
{
	if (args.size() < 5) {
		log.Error("[UNION BRIDGE] u36, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	ClientID clientID(args[1]);
	UserID userID(args[2]);
	std::string globalAttributes(args[3]);
	std::string roomAttributes(args[4]);

	ClientRef theClient = clientManager.Request(clientID);
	AccountRef account = accountManager.Request(userID);
	ClientManifest clientManifest;
	if (account && theClient && theClient->GetAccount() != account) {
		theClient->SetAccount(account);
	}

	// If it's not the current client, set the client's attributes.
	// (The current client obtains its own attributes through separate u8s.)
	RoomRef theRoom = roomManager.Get(roomID);
	if (!theClient->IsSelf()) {
		clientManifest.Deserialize(clientID, userID, "",  "", globalAttributes, {roomID, roomAttributes});
		theClient->Synchronize(clientManifest);

		// If the client is observed, don't fire JOIN; observed clients always
		// fire JOIN based on observation updates. Likewise, don't fire JOIN
		// on self; self fires JOIN when it receives a u6.
		if (!clientManager.IsObservingClient(clientID)) {
			theClient->OnJoinRoom(theRoom, roomID);
		}
	}

	// Add the client to the given room.
	theRoom->AddOccupant(theClient);

}
void
UnionBridge::U37(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_REMOVED_FROM_ROOM */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u37, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	ClientID clientID(args[1]);

	ClientRef theClient = clientManager.Request(clientID);
	RoomRef theRoom = roomManager.Get(roomID);
	theRoom->RemoveOccupant(clientID);

	// Don't fire LEAVE on self; self fires LEAVE when it receives a u44.
	if (!theClient->IsSelf()) {
		// If the client is observed, don't fire LEAVE; observed clients always
		// fire LEAVE based on observation updates.
		if (!clientManager.IsObservingClient(clientID)) {
			theClient->OnLeaveRoom(theRoom, roomID);
		}
	}
}
void
UnionBridge::U38(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ROOMLIST_SNAPSHOT */
{
	if (args.size() < 3)  {
		log.Error("[UNION BRIDGE] u38, malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string requestID(args[0]);
	std::string requestedRoomIDQualifier(args[1]);
//	bool recursive = (args[2] == "true");

	if (requestID == "") { // Synchronize
		for (size_t i = 3; i < args.size(); i+=2) {
			RoomQualifier roomQualifier;
			std::vector<RoomID> roomIDs;
			roomQualifier = args[i];
			std::stringstream idss(args[i+1]);
			std::string rid;
			char c = *Token::RS;
			while (std::getline(idss, rid, c)) {
				roomIDs.push_back(rid);
			}
			roomManager.SetWatchedRooms(roomQualifier, roomIDs);
		}
	} else {
#ifdef SNAPSHOT_MANAGER
		var roomList = [];
		for (i = 0; i < args.length; i+=2) {
			roomQualifier = args[i];
			roomIDs = args[i+1].split(Token::RS);
			for (var j = 0; j < roomIDs.length; j++) {
				roomList.push(roomQualifier + (roomQualifier == "" ? "" : ".") + roomIDs[j]);
			}
		}
		snapshotManager.RecieveRoomListSnapshot(requestID, roomList, requestedRoomIDQualifier, recursive);
#endif
	}
}
void
UnionBridge::U39(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ROOM_ADDED */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] u39, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	roomManager.AddWatchedRoom(roomID);

}
void
UnionBridge::U40(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ROOM_REMOVED */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] u40, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	roomManager.RemoveWatchedRoom(roomID);
	if (roomManager.Get(roomID)) {
		roomManager.Dispose(roomID);
	}
}
void
UnionBridge::U42(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* WATCH_FOR_ROOMS_RESULT */
{
	if (args.size() < 3)  {
		log.Error("[UNION BRIDGE] u42, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomQualifier roomIdQualifier(args[0]);
	bool recursive = args[1] == "true";
	UPCStatus status = UPC::Status::GetStatusCode(args[2].c_str());

	// Broadcast the result of the observation attempt.
	roomManager.OnWatchForRoomsResult(roomIdQualifier, status);
	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::INVALID_QUALIFIER:
	case UPC::Status::ALREADY_WATCHING:
	case UPC::Status::PERMISSION_DENIED:
	break;

	default:
		log.Warn("Unrecognized status code for u42. Room ID Qualifier: [" + roomIdQualifier + "], recursive: ["
				+ bool_to_string(recursive) + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U43(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_WATCHING_FOR_ROOMS_RESULT */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] u43, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomQualifier roomIdQualifier(args[0]);
	bool recursive = args[1] == "true";
	UPCStatus status = UPC::Status::GetStatusCode(args[2].c_str());

	switch (status) {
	case UPC::Status::SUCCESS:
	if (roomIdQualifier == "" && recursive) {
		roomManager.DisposeWatchedRooms();
	} else {
		// Remove all watched rooms for the qualifier
		roomManager.SetWatchedRooms(roomIdQualifier, {});
	}
	case UPC::Status::UPC_ERROR:
	case UPC::Status::NOT_WATCHING:
	case UPC::Status::INVALID_QUALIFIER:
	roomManager.OnStopWatchingForRoomsResult(roomIdQualifier, status);
	break;

	default:
		log.Warn("Unrecognized status code for u43. Room ID Qualifier: [" + roomIdQualifier + "], recursive: ["+
				  bool_to_string(recursive) + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U44(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* LEFT_ROOM */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] u44, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	RoomRef leftRoom = roomManager.Get(roomID);
	roomManager.RemoveOccupiedRoom(roomID);
	if (leftRoom) {
		leftRoom->OnLeave();
		ClientRef self=clientManager.Self();
		if (self) self->OnLeaveRoom(leftRoom, roomID);
	}
}
void
UnionBridge::U46(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CHANGE_ACCOUNT_PASSWORD_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u46, malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());

	AccountRef account = accountManager.GetAccount(userID);
	if (account) {
		account->OnChangePasswordResult(status);
	}
	accountManager.OnChangePasswordResult(userID, status);

}
void
UnionBridge::U47(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CREATE_ACCOUNT_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u47, malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());

	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ACCOUNT_EXISTS:
	case UPC::Status::PERMISSION_DENIED:
	accountManager.OnCreateAccountResult(userID, status);
	break;
	default:
		log.Warn("Unrecognized status code for u47. Account: [" + userID + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U48(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* REMOVE_ACCOUNT_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u48, malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());

	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ACCOUNT_NOT_FOUND:
	case UPC::Status::AUTHORIZATION_FAILED:
	case UPC::Status::PERMISSION_DENIED:
		accountManager.OnRemoveAccountResult(userID, status);
	break;
	default:
		log.Warn("Unrecognized status code for u48. Account: [" + userID + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U49(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* LOGIN_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u49, malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());

	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ALREADY_LOGGED_IN:
	case UPC::Status::ACCOUNT_NOT_FOUND:
	case UPC::Status::AUTHORIZATION_FAILED:
	case UPC::Status::PERMISSION_DENIED:
		accountManager.OnLoginResult(userID, status);
	break;
	default:
		log.Warn("Unrecognized status code for u49. Account: [" + userID + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U50(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SERVER_TIME_UPDATE */
{
}

void
UnionBridge::U54(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ROOM_SNAPSHOT */
{
	if (args.size() < 5)  {
		log.Error("[UNION BRIDGE] u54, malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string requestID(args[0]);
	RoomID roomID(args[1]);
	int occupantCount = atoi(args[2].c_str());
	int observerCount = atoi(args[3].c_str());
	std::string roomAttributes(args[4]);

	std::vector<ClientID> clientList;
	for (size_t i=5; i<args.size(); i++)
		clientList.push_back(args[i]);
	ClientManifest clientManifest;
	RoomManifest roomManifest;
	RoomRef theRoom;
	roomManifest.Deserialize(roomID,
			roomAttributes,
			clientList,
			occupantCount,
			observerCount);

	if (requestID == "") { // Synchronize
		theRoom = roomManager.Get(roomID);

		if (!theRoom) {
			// If the server makes the current client join or observe a room, it
			// will first send a u54 before sending the u6 or u59 notice. In that
			// case, the room might be unknown briefly, so create a cached room
			// then wait for the u6 or u59 to arrive.
			theRoom = roomManager.Request(roomID);
		}

		theRoom->Synchronize(roomManifest);
	} else {
#ifdef SNAPSHOT_MANAGER
		snapshotManager.RecieveRoomSnapshot(requestID, roomManifest);
#endif
	}
}
void
UnionBridge::U55(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_ROOM_SNAPSHOT */
{
}
void
UnionBridge::U57(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SEND_MESSAGE_TO_SERVER */
{
}
void
UnionBridge::U58(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* OBSERVE_ROOM */
{
}
void
UnionBridge::U59(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* OBSERVED_ROOM */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] u59, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	RoomRef room = roomManager.AddObservedRoom(roomID);
	if (room)
		room->OnObserve();
	// Fire OBSERVE through the client
	ClientRef self = clientManager.Self();
	if (self) {
		self->OnObserveRoom(room, roomID);
	}
}
void
UnionBridge::U60(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_ROOM_SNAPSHOT_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u60, malformed packet, argument mismatch, ignoring");
		return;
	}

#ifdef SNAPSHOT_MANAGER
	std::string requestID(args[0]);
	RoomID roomID(args[1]);

	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ROOM_NOT_FOUND:
	case UPC::Status::AUTHORIZATION_REQUIRED:
	case UPC::Status::AUTHORIZATION_FAILED:
	case UPC::Status::PERMISSION_DENIED:
	snapshotManager.RecieveSnapshotResult(requestID, status);
	break;
	default:
		log.Warn("Unrecognized status code for u60."
				+ " Request ID: [" + requestID + "], Room ID: ["
				+ roomID + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
#endif
}
void
UnionBridge::U61(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_OBSERVING_ROOM */
{
}
void
UnionBridge::U62(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOPPED_OBSERVING_ROOM */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] u62, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);

	RoomRef theRoom = roomManager.Get(roomID);
	roomManager.RemoveObservedRoom(roomID);
	if (theRoom) {
		theRoom->OnStopObserving();
		// self() might return null if a STOP_OBSERVING listener has closed the connection
		if (clientManager.Self())
			clientManager.Self()->OnStopObservingRoom(theRoom, roomID);
	}
}
void
UnionBridge::U63(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_READY */
{
	SetConnectionState(ConnectionState::READY);
	mostRecentConnectAchievedReady = true;
	readyCount++;
	connectAttemptCount = 0;
	NxReady();
}
void
UnionBridge::U64(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SET_ROOM_UPDATE_LEVELS */
{
}
void
UnionBridge::U65(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_HELLO */
{
}

/**
 * @private
 * the first command recieved!
 * apart from saying hello in UPC, and telling anyone who is interested in that fact that it is, it will also set up a few server side properties
 * to do with affinity, and sessionID
 */
void
UnionBridge::U66(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SERVER_HELLO */
{
	if (args.size() < 6)  {
		log.Error("[UNION BRIDGE] u66, malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string serverVersion(args[0]);
	std::string sessionID(args[1]);
	std::string serverUPCVersionString(args[2]);
	bool protocolCompatible = (args[3]!="false");
	std::string affinityAddress(args[4]);
	int affinityDurationSec = (int)(atof(args[5].c_str())*60.0);

	log.Debug("Server hello from " + args[0]);

	log.Info("[ORBITER] Server version: " + serverVersion);
	log.Info("[ORBITER] Server UPC version: " + serverUPCVersionString);

	SetServerVersion(Version::FromVersionString(serverVersion));
	SetUPCVersion(Version::FromVersionString(serverUPCVersionString));

	connector.SetActiveConnectionSessionID(sessionID);
	connector.SetConnectionAffinity(affinityAddress, affinityDurationSec);
	if (!protocolCompatible) {
		NxProtocolIncompatible(serverUPCVersionString);
	}
}

void
UnionBridge::U67(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* REMOVE_ROOM_ATTR */
{
}
void
UnionBridge::U69(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* REMOVE_CLIENT_ATTR */
{
}
void
UnionBridge::U70(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SEND_ROOMMODULE_MESSAGE */
{
}
void
UnionBridge::U71(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SEND_SERVERMODULE_MESSAGE */
{
}

void
UnionBridge::U72(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* JOIN_ROOM_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u72, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());

	RoomRef theRoom = roomManager.Get(roomID);
	if (theRoom) {
		log.Debug("got a ref for " + roomID);
	}
	switch (status) {
	case UPC::Status::ROOM_NOT_FOUND:
		if (roomManager.Get(roomID)) {
			roomManager.Dispose(roomID);
		}

	case UPC::Status::UPC_ERROR:
	case UPC::Status::ROOM_FULL:
	case UPC::Status::AUTHORIZATION_REQUIRED:
	case UPC::Status::AUTHORIZATION_FAILED:
	case UPC::Status::SUCCESS:
	case UPC::Status::ALREADY_IN_ROOM:
	case UPC::Status::PERMISSION_DENIED:
		roomManager.OnJoinRoomResult(roomID, status);
		if (theRoom) {
			theRoom->OnJoinResult(status);
		}
		break;

	default:
		log.Warn("Unrecognized status code for u72. Room ID: [" + roomID + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U73(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SET_CLIENT_ATTR_RESULT */
{
	log.Debug("doing a client attribute update");
	if (args.size() < 6)  {
		log.Error("[UNION BRIDGE] u73, malformed packet, argument mismatch, ignoring");
		return;
	}

	AttrScope attrScope(args[0]);
	ClientID clientID(args[1]);
	UserID userID(args[2]);
	AttrName attrName(args[3]);
	int attrOptions=atoi(args[4].c_str());
	UPCStatus status = UPC::Status::GetStatusCode(args[5].c_str());

	ClientRef theClient;
	AccountRef theAccount;

	switch (status) {
	case UPC::Status::CLIENT_NOT_FOUND:
	case UPC::Status::ACCOUNT_NOT_FOUND:
	break;

	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::DUPLICATE_VALUE:
	case UPC::Status::IMMUTABLE:
	case UPC::Status::SERVER_ONLY:
	case UPC::Status::EVALUATION_FAILED:
	case UPC::Status::PERMISSION_DENIED:
		if ( attrOptions & Attribute::FLAG_PERSISTENT) {
			// Persistent attr
			theAccount = accountManager.Request(userID);
			theAccount->OnSetAttributeResult(attrName, attrScope, status);
		} else {
			// Non-persistent attr
			theClient = clientManager.Request(clientID);
			theClient->OnSetAttributeResult(attrName, attrScope, status);
		}
		break;

	default:
		log.Warn("Unrecognized status received for u73: " + UPC::Status::GetStatusString(status));
	}
	log.Debug("done a client attribute update");
}
void
UnionBridge::U74(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SET_ROOM_ATTR_RESULT */
{
	if (args.size() < 3)  {
		log.Error("[UNION BRIDGE] u74, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	AttrName attrName(args[1]);
	UPCStatus status = UPC::Status::GetStatusCode(args[2].c_str());

	RoomRef theRoom = roomManager.Get(roomID);

	if (!theRoom) {
		log.Warn("Room attribute update received for room with no client-side Room object. Room ID [" +
				roomID + "]. Attribute: [" + attrName + "]. Status: ["
				+ UPC::Status::GetStatusString(status) + "].");
		return;
	}

	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::IMMUTABLE:
	case UPC::Status::SERVER_ONLY:
	case UPC::Status::EVALUATION_FAILED:
	case UPC::Status::ROOM_NOT_FOUND:
	case UPC::Status::PERMISSION_DENIED:
		theRoom->OnSetAttributeResult(attrName, Token::GLOBAL_ATTR, status);
	break;

	default:
		log.Warn("Unrecognized status received for u74: " + UPC::Status::GetStatusString(status));
	}
}
void
UnionBridge::U75(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_CLIENTCOUNT_SNAPSHOT_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u75, malformed packet, argument mismatch, ignoring");
		return;
	}

#ifdef SNAPSHOT_MANAGER
	std::string requestID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());
	snapshotManager.RecieveSnapshotResult(requestID, status);
#endif
}
void
UnionBridge::U76(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* LEAVE_ROOM_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u76, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());

	RoomRef leftRoom = roomManager.Get(roomID);

	switch (status) {
	case UPC::Status::ROOM_NOT_FOUND:
	if (leftRoom) {
		roomManager.Dispose(roomID);
	}

	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::NOT_IN_ROOM:
	roomManager.OnLeaveRoomResult(roomID, status);
	if (leftRoom) {
		leftRoom->OnLeaveResult(status);
	}
	break;

	default:
		log.Warn("Unrecognized status code for u76.  Room ID: [" + roomID + "]. Status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U77(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* OBSERVE_ROOM_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u77, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());
	RoomRef theRoom = roomManager.Get(roomID);
	switch (status) {
	case UPC::Status::ROOM_NOT_FOUND:
		if (theRoom) {
			roomManager.Dispose(roomID);
		}

	case UPC::Status::UPC_ERROR:
	case UPC::Status::AUTHORIZATION_REQUIRED:
	case UPC::Status::AUTHORIZATION_FAILED:
	case UPC::Status::SUCCESS:
	case UPC::Status::ALREADY_OBSERVING:
	case UPC::Status::PERMISSION_DENIED:
		roomManager.OnObserveRoomResult(roomID, status);

		if (theRoom) {
			theRoom->OnObserveResult(status);
		}
		break;

	default:
		log.Warn("Unrecognized status code for u77.  Room ID: [" + roomID + "], status: " + UPC::Status::GetStatusString(status) + ".");
	}
}
void
UnionBridge::U78(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_OBSERVING_ROOM_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u78, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());
	RoomRef theRoom = roomManager.Get(roomID);

	switch (status) {
	case UPC::Status::ROOM_NOT_FOUND:
	if (theRoom) {
		roomManager.Dispose(roomID);
	}

	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::NOT_OBSERVING:
	roomManager.OnStopObservingRoomResult(roomID, status);

	if (theRoom) {
		theRoom->OnStopObservingResult(status);
	}
	break;

	default:
		log.Warn("Unrecognized status code for u78. Room ID: [" + roomID + "], status: " + UPC::Status::GetStatusString(status) + ".");
	}
}
void
UnionBridge::U79(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ROOM_ATTR_REMOVED */
{
	if (args.size() < 3)  {
		log.Error("[UNION BRIDGE] u79, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	ClientID byClientID(args[1]);
	AttrName attrName(args[2]);
	RoomRef theRoom = roomManager.Get(roomID);

	ClientRef theClient;

	if (!theRoom) {
		log.Warn("Room attribute removal notification received for room with no client-side Room object. Room ID [" +
				roomID + "]. Attribute: [" + attrName + "].");
		return;
	}

	// If the clientID is "", the server removed the room, so there's no
	// corresponding client.
	if (byClientID != "") {
		theClient =clientManager.Request(byClientID);
	}
	theRoom->RemoveAttributeLocal(attrName, Token::GLOBAL_ATTR, theClient);
}
void
UnionBridge::U80(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* REMOVE_ROOM_ATTR_RESULT */
{
	if (args.size() < 3)  {
		log.Error("[UNION BRIDGE] u80, malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	AttrName attrName(args[1]);
	UPCStatus status = UPC::Status::GetStatusCode(args[2].c_str());

	RoomRef theRoom = roomManager.Get(roomID);
	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::IMMUTABLE:
	case UPC::Status::SERVER_ONLY:
	case UPC::Status::ROOM_NOT_FOUND:
	case UPC::Status::ATTR_NOT_FOUND:
	case UPC::Status::PERMISSION_DENIED:
	if (theRoom) {
		theRoom->OnDeleteAttributeResult(attrName, Token::GLOBAL_ATTR, status);
	}
	break;

	default:
		log.Warn("Unrecognized status received for u80: " + UPC::Status::GetStatusString(status));
	}
}
void
UnionBridge::U81(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_ATTR_REMOVED */
{
	if (args.size() < 5)  {
		log.Error("[UNION BRIDGE] u81, malformed packet, argument mismatch, ignoring");
		return;
	}

	AttrScope attrScope(args[0]);
	ClientID clientID(args[1]);
	UserID userID(args[2]);
	AttrName attrName(args[3]);
	int attrOptions = atoi(args[4].c_str());

	ClientRef client;
	AccountRef account;

	if (attrOptions & Attribute::FLAG_PERSISTENT) {
		// Persistent attr
		account = accountManager.Request(userID);
		account->RemoveAttributeLocal(attrName, attrScope);
	} else {
		// Non-persistent attr
		client = clientManager.Request(clientID);
		client->RemoveAttributeLocal(attrName, attrScope);
	}
}
void
UnionBridge::U82(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* REMOVE_CLIENT_ATTR_RESULT */
{
	if (args.size() < 6)  {
		log.Error("[UNION BRIDGE] u82, malformed packet, argument mismatch, ignoring");
		return;
	}

	AttrScope attrScope(args[0]);
	ClientID clientID(args[1]);
	UserID userID(args[2]);
	AttrName attrName(args[3]);
	int attrOptions = atoi(args[4].c_str());
	UPCStatus status = UPC::Status::GetStatusCode(args[5].c_str());

	ClientRef client;
	AccountRef account;

	switch (status) {
	case UPC::Status::CLIENT_NOT_FOUND:
	case UPC::Status::ACCOUNT_NOT_FOUND:
	break;
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::IMMUTABLE:
	case UPC::Status::SERVER_ONLY:
	case UPC::Status::ATTR_NOT_FOUND:
	case UPC::Status::EVALUATION_FAILED:
	case UPC::Status::PERMISSION_DENIED:
	if (attrOptions & Attribute::FLAG_PERSISTENT) {
		// Persistent attr
		account = accountManager.Request(userID);
		account->OnDeleteAttributeResult(attrName, attrScope, status);
	} else {
		// Non-persistent attr
		client = clientManager.Request(clientID);
		client->OnDeleteAttributeResult(attrName, attrScope, status);
	}
	break;

	default:
		log.Warn("Unrecognized status received for u82: " + UPC::Status::GetStatusString(status));
	}
}
void
UnionBridge::U83(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* TERMINATE_SESSION */
{
}
void
UnionBridge::U84(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SESSION_TERMINATED */
{
	log.Debug("server koff");
	int state = connectionState;
	connector.Disconnect();
	SetConnectionState(ConnectionState::DISCONNECTION_IN_PROGRESS);
	if (state == ConnectionState::CONNECTION_IN_PROGRESS) {
		NxConnectFailure("Server terminated session before READY state was achieved.", UPC::Status::SESSION_TERMINATED);
	} else {
		NxConnectFailure("Session teminated", UPC::Status::SESSION_TERMINATED);
	}
}

void
UnionBridge::U85(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SESSION_NOT_FOUND */
{
	log.Debug("server koff++");
	int state = connectionState;
	connector.Disconnect();
	SetConnectionState(ConnectionState::DISCONNECTION_IN_PROGRESS);
	if (state == ConnectionState::CONNECTION_IN_PROGRESS) {
		NxConnectFailure("Client attempted to reestablish an expired session or establish an unknown session.", UPC::Status::SESSION_NOT_FOUND);
	} else {
		NxConnectFailure("Session not found", UPC::Status::SESSION_NOT_FOUND);
	}
}

void
UnionBridge::U86(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* LOGOFF */
{
}
void
UnionBridge::U87(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* LOGOFF_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u87, malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());
	AccountRef account = accountManager.GetAccount(userID);

	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::AUTHORIZATION_FAILED:
	case UPC::Status::ACCOUNT_NOT_FOUND:
	case UPC::Status::NOT_LOGGED_IN:
	case UPC::Status::PERMISSION_DENIED:
	if (account) {
		account->OnLogoffResult(status);
	}
	// Tell the account manager
	accountManager.OnLogoffResult(userID, status);
	break;
	default:
		log.Warn("Unrecognized status received for u87: " + UPC::Status::GetStatusString(status));
	}
}
void
UnionBridge::U88(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* LOGGED_IN */
{
	if (args.size() < 3)  {
		log.Error("[UNION BRIDGE] u88, malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID clientID(args[0]);
	UserID userID(args[1]);
	std::string globalAttrs(args[2]);

	std::vector<std::string> roomAttrs;
	for (unsigned int i=3; i<args.size(); i++) // TODO is this from 3 or 4 ... original seems to include arguments[3] ie args[2]
		roomAttrs.push_back(args[i]);
	AccountRef account = accountManager.Request(userID);
	ClientRef client = clientManager.Request(clientID);
	ClientManifest clientManifest;
	clientManifest.Deserialize(clientID, userID, "", "", globalAttrs, roomAttrs);
	// Update the account
	std::vector<AttrScope> scopes = clientManifest.persistentAttributes.GetScopes();
	AttributeCollection& accountAttrs = account->GetAttributeCollection();
	for (auto it: scopes) {
		accountAttrs.SynchronizeScope(it, clientManifest.persistentAttributes);
	}

	if (client && !client->GetAccount()) {
		// Client doesn't know about this account yet
		client->SetAccount(account);
		client->OnLogin();
		account->OnLogin();
		accountManager.OnLogin(account, clientID);
	} else {
		// Do nothing if the account is known. Logins are reported for
		// observe-account, observe-client, and watch-for-clients, so a
		// client might receive multiple login notifications.
	}
}
void
UnionBridge::U89(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* LOGGED_OFF */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u89, malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID clientID(args[0]);
	UserID userID(args[1]);

	ClientRef client = clientManager.Get(clientID);
	AccountRef account = accountManager.GetAccount(userID);

	if (account) {
		if (account->GetConnectionState() == ConnectionState::LOGGED_IN) {
			if (client) {
				account->OnLogoff();
			}
			accountManager.OnLogoff(account, clientID);
		} else {
			// Do nothing if the account is unknown. Logoffs are reported for
			// observe-account, observe-client, and watch-for-clients, so a
			// client might receive multiple logoff notifications.
		}
	} else {
		log.Error("LOGGED_OFF (u89) received for an unknown user: [" + userID + "].");
	}
}
void
UnionBridge::U90(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ACCOUNT_PASSWORD_CHANGED */
{
	AccountRef selfAccount = clientManager.SelfAccount();
	if (selfAccount) {
		selfAccount->OnChangePassword();
	}
	accountManager.OnChangePassword(selfAccount ? selfAccount->GetUserID() : UserID());

}
void
UnionBridge::U91(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_CLIENTLIST_SNAPSHOT */
{
}
void
UnionBridge::U92(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* WATCH_FOR_CLIENTS */
{
}
void
UnionBridge::U93(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_WATCHING_FOR_CLIENTS */
{
}
void
UnionBridge::U94(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_CLIENT_SNAPSHOT */
{
}
void
UnionBridge::U95(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* OBSERVE_CLIENT */
{
}
void
UnionBridge::U96(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_OBSERVING_CLIENT */
{
}
void
UnionBridge::U97(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_ACCOUNTLIST_SNAPSHOT */
{
}
void
UnionBridge::U98(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* WATCH_FOR_ACCOUNTS */
{
}
void
UnionBridge::U99(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_WATCHING_FOR_ACCOUNTS */
{
}
void
UnionBridge::U100(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_ACCOUNT_SNAPSHOT */
{
}
void
UnionBridge::U101(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENTLIST_SNAPSHOT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u101, malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string requestID(args[0]);
	std::string serializedIDs(args[1]);

	if (requestID == "") { // Synchronize
		clientManager.DeserializeWatchedClients(serializedIDs);
	} else {
#ifdef SNAPSHOT_MANAGER
		var clientList;
		var thisUserID;
		clientList = [];
		for (var i = ids.length-1; i >= 0; i-=2) {
			thisUserID = ids[i];
			thisUserID = thisUserID == "" ? null : thisUserID;
			clientList.push({clientID:ids[i-1], userID:thisUserID});
		}
		snapshotManager.RecieveClientListSnapshot(requestID, clientList);
#endif
	}
}
void
UnionBridge::U102(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_ADDED_TO_SERVER */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] u102, malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID clientID(args[0]);

	clientManager.AddWatchedClient(clientManager.Request(clientID));

}
void
UnionBridge::U103(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_REMOVED_FROM_SERVER */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] u103, malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID clientID(args[0]);

	ClientRef client = clientManager.Get(clientID);

	if (clientManager.HasWatchedClient(clientID)) {
		clientManager.RemoveWatchedClient(clientID);
	}
	if (clientManager.IsObservingClient(clientID)) {
		clientManager.RemoveObservedClient(clientID);
	}

	// If the current client is both observing a client and watching for clients,
	// it will receive two u103 notifications. When the second one arrives, the
	// client will be unknown, so no disconnection event should be dispatched.
	if (client) {
		client->SetConnectionState(ConnectionState::NOT_CONNECTED);
		// Retrieve the client reference using getClient() here so that the
		// ClientManagerEvent.CLIENT_DISCONNECTED event provides the application
		// with access to the custom client, if available.
		clientManager.OnClientDisconnected(clientManager.Get(clientID));
	}
}
void
UnionBridge::U104(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_SNAPSHOT */
{
	if (args.size() < 6)  {
		log.Error("[UNION BRIDGE] u104, malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string requestID(args[0]);
	std::string clientID(args[1]);
	std::string userID(args[2]);
	std::string serializedOccupiedRoomIDs(args[3]);
	std::string serializedObservedRoomIDs(args[4]);
	std::string globalAttrs(args[5]);

	std::vector<std::string> roomAttrs;
	for (size_t i=7; i<args.size(); i++) {
		roomAttrs.push_back(args[i]);
	}
	ClientRef theClient;
	AccountRef account = accountManager.Request(userID);
	ClientManifest clientManifest;
	clientManifest.Deserialize(clientID, userID, serializedOccupiedRoomIDs,
			serializedObservedRoomIDs, globalAttrs, roomAttrs);

	if (clientID != "") {
		// --- Client update ---

		if (requestID == "") { // Synchronize
			theClient = clientManager.Request(clientID);
			theClient->SetAccount(account);
			theClient->Synchronize(clientManifest);
			theClient->OnSynchronize();
		} else {
#ifdef SNAPSHOT_MANAGER
			snapshotManager.RecieveClientSnapshot(requestID, clientManifest);
#endif
		}
	} else {
		// --- User account update ---

		if (requestID == "") { // Synchronize
			std::vector<AttrScope> scopes = clientManifest.persistentAttributes.GetScopes();
			for (int i = (int)scopes.size(); --i >= 0;) {
				account->GetAttributeCollection().SynchronizeScope(scopes[i], clientManifest.persistentAttributes);
			}
			account->OnSynchronize();
		} else {
#ifdef SNAPSHOT_MANAGER
			snapshotManager.RecieveAccountSnapshot(requestID, clientManifest);
#endif
		}
	}
}
void
UnionBridge::U105(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* OBSERVE_CLIENT_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] u105, malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID clientID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());

	ClientRef theClient = clientManager.Get(clientID);
	switch (status) {
	case UPC::Status::CLIENT_NOT_FOUND:
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ALREADY_OBSERVING:
	case UPC::Status::PERMISSION_DENIED:
	clientManager.OnObserveClientResult(clientID, status);
	if (theClient) {
		theClient->OnObserveResult(status);
	}
	break;

	default:
		log.Warn("Unrecognized status code for u105.  Client ID: [" + clientID + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U106(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_OBSERVING_CLIENT_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID clientID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());

	ClientRef theClient = clientManager.Get(clientID);
	switch (status) {
	case UPC::Status::CLIENT_NOT_FOUND:
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::NOT_OBSERVING:
		clientManager.OnStopObservingClientResult(clientID, status);
		if (theClient) {
			theClient->OnStopObservingResult(status);
		}
	break;

	default:
		log.Warn("Unrecognized status code for u106. Client ID: [" + clientID + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U107(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* WATCH_FOR_CLIENTS_RESULT */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UPCStatus status = UPC::Status::GetStatusCode(args[0].c_str());
	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ALREADY_WATCHING:
		clientManager.OnWatchForClientsResult(status);
	break;

	default:
		log.Warn("Unrecognized status code for u107.Status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U108(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_WATCHING_FOR_CLIENTS_RESULT */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UPCStatus status = UPC::Status::GetStatusCode(args[0].c_str());
	switch (status) {
	case UPC::Status::SUCCESS:
		clientManager.SetIsWatchingForClients(false);
		clientManager.RemoveAllWatchedClients();
	case UPC::Status::UPC_ERROR:
	case UPC::Status::NOT_WATCHING:
		clientManager.OnStopWatchingForClientsResult(status);
		break;

	default:
		log.Warn("Unrecognized status code for u108. Status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U109(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* WATCH_FOR_ACCOUNTS_RESULT */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UPCStatus status = UPC::Status::GetStatusCode(args[0].c_str());
	switch (status) {
	case UPC::Status::SUCCESS:
		accountManager.SetIsWatchingForAccounts(true);
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ALREADY_WATCHING:
		accountManager.OnWatchForAccountsResult(status);
		break;

	default:
		log.Warn("Unrecognized status code for u109. Status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U110(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_WATCHING_FOR_ACCOUNTS_RESULT */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UPCStatus status = UPC::Status::GetStatusCode(args[0].c_str());

	switch (status) {
	case UPC::Status::SUCCESS:
		accountManager.SetIsWatchingForAccounts(false);
		accountManager.RemoveAllWatchedAccounts();
	case UPC::Status::UPC_ERROR:
	case UPC::Status::NOT_WATCHING:
		accountManager.OnStopWatchingForAccountsResult(status);
		break;

	default:
		log.Warn("Unrecognized status code for u110. Status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U111(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ACCOUNT_ADDED */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	accountManager.AddWatchedAccount(accountManager.Request(userID));

}
void
UnionBridge::U112(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ACCOUNT_REMOVED */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	AccountRef account;
	if (accountManager.HasWatchedAccount(userID)) {
		account = accountManager.RemoveWatchedAccount(userID);
	}
	if (accountManager.IsObservingAccount(userID)) {
		account = accountManager.RemoveObservedAccount(userID);
	}
	accountManager.OnAccountRemoved(userID, account);

}
void
UnionBridge::U113(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* JOINED_ROOM_ADDED_TO_CLIENT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID clientID(args[0]);
	RoomID roomID(args[1]);
	ClientRef client = clientManager.Request(clientID);
	if (client) {
		client->AddOccupiedRoomID(roomID);
		client->OnJoinRoom(roomManager.Get(roomID), roomID);
	}
}
void
UnionBridge::U114(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* JOINED_ROOM_REMOVED_FROM_CLIENT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID clientID(args[0]);
	RoomID roomID(args[1]);
	ClientRef client = clientManager.Request(clientID);
	if (client) {
		client->RemoveOccupiedRoomID(roomID);
		client->OnLeaveRoom(roomManager.Get(roomID), roomID);
	}
}
void
UnionBridge::U115(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_CLIENT_SNAPSHOT_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string requestID(args[0]);
	ClientID clientID(args[1]);
#ifdef SNAPSHOT_MANAGER
	snapshotManager.RecieveSnapshotResult(requestID, status);
#endif

}
void
UnionBridge::U116(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_ACCOUNT_SNAPSHOT_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string requestID(args[0]);
	UserID userID(args[1]);
#ifdef SNAPSHOT_MANAGER
	snapshotManager.RecieveSnapshotResult(requestID, status);
#endif
}
void
UnionBridge::U117(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* OBSERVED_ROOM_ADDED_TO_CLIENT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID clientID(args[0]);
	RoomID roomID(args[1]);
	ClientRef client = clientManager.Request(clientID);
	if (client) {
		client->AddObservedRoomID(roomID);
		client->OnObserveRoom(roomManager.Get(roomID), roomID);
	}
}
void
UnionBridge::U118(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* OBSERVED_ROOM_REMOVED_FROM_CLIENT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID clientID(args[0]);
	RoomID roomID(args[1]);
	ClientRef client = clientManager.Request(clientID);
	if (client) {
		client->RemoveObservedRoomID(roomID);
		client->OnStopObservingRoom(roomManager.Get(roomID), roomID);
	}
}
void
UnionBridge::U119(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_OBSERVED */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID clientID(args[0]);

	ClientRef client = clientManager.Request(clientID);
	clientManager.AddObservedClient(client);
	client->OnObserve();

}
void
UnionBridge::U120(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOPPED_OBSERVING_CLIENT */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID clientID(args[0]);
	ClientRef client = clientManager.Get(clientID);
	clientManager.RemoveObservedClient(clientID);
	if (client) {
		client->OnStopObserving();
	}
}
void
UnionBridge::U121(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* OBSERVE_ACCOUNT */
{

}
void
UnionBridge::U122(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_OBSERVING_ACCOUNT */
{
}
void
UnionBridge::U123(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* OBSERVE_ACCOUNT_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());

	AccountRef theAccount = accountManager.GetAccount(userID);

	switch (status) {
	case UPC::Status::ACCOUNT_NOT_FOUND:
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ALREADY_OBSERVING:
		accountManager.OnObserveAccountResult(userID, status);
		if (theAccount) {
			theAccount->OnObserveResult(status);
		}
	break;

	default:
		log.Warn("Unrecognized status code for u123. User ID: [" + userID + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U124(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ACCOUNT_OBSERVED */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	AccountRef theAccount = accountManager.Request(userID);
	accountManager.AddObservedAccount(theAccount);
	theAccount->OnObserve();

}
void
UnionBridge::U125(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_OBSERVING_ACCOUNT_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());

	AccountRef theAccount = accountManager.GetAccount(userID);
	switch (status) {
	case UPC::Status::ACCOUNT_NOT_FOUND:
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ALREADY_OBSERVING:
	accountManager.OnStopObservingAccountResult(userID, status);
	if (theAccount) {
		theAccount->OnStopObservingResult(status);
	}
	break;

	default:
		log.Warn("Unrecognized status code for u125. User ID: [" + userID + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U126(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOPPED_OBSERVING_ACCOUNT */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	AccountRef account = accountManager.GetAccount(userID);
	accountManager.RemoveObservedAccount(userID);
	if (account) {
		account->OnStopObserving();
	}

}
void
UnionBridge::U127(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ACCOUNT_LIST_UPDATE */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string requestID(args[0]);
	std::string serializedIDs(args[1]);

	std::vector<UserID> ids;
	std::stringstream idss(serializedIDs);
	std::string item;
	char c = *Token::RS;
	while (std::getline(idss, item, c)) {
		ids.push_back(item);
	}
	AccountRef accountList;

	if (requestID == "") {
		accountManager.DeserializeWatchedAccounts(serializedIDs);
	} else {
#ifdef SNAPSHOT_MANAGER
		accountList = [];
		for (var i = ids.length; --i >= 0;) {
			accountList.push(ids[i]);
		}
		snapshotManager.RecieveAccountListSnapshot(requestID, accountList);
#endif
	}
}
void
UnionBridge::U128(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* UPDATE_LEVELS_UPDATE */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	unsigned int updateLevels = atoi(args[0].c_str());
	RoomID roomID(args[1]);

	Room* room = const_cast<Room*>(roomManager.Get(roomID).get());
	if (room) {
		if (!UpdateOccupantList(updateLevels)) {
			std::vector<RoomID> occupantIDs = room->GetOccupantIDs();
			for (auto it=occupantIDs.begin(); it!=occupantIDs.end(); ++it) {
				room->RemoveOccupant(*it);
			}
		}
		if (!UpdateObserverList(updateLevels)) {
			std::vector<RoomID> observerIDs= room->GetObserverIDs();
			for (auto it=observerIDs.begin(); it!=observerIDs.end(); ++it) {
				room->RemoveObserver(*it);
			}
		}
		if (!UpdateSharedRoomAttributes(updateLevels) && !UpdateAllRoomAttributes(updateLevels)) {
			room->RemoveAllAttributes();
		}
	}
}
void
UnionBridge::U129(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_OBSERVED_ROOM */
{
	if (args.size() < 5)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	ClientID clientID(args[1]);
	UserID userID(args[2]);
	std::string globalAttributes(args[3]);
	std::string roomAttributes(args[4]);

	ClientRef theClient = clientManager.Request(clientID);
	AccountRef account = accountManager.Request(userID);
	ClientManifest clientManifest;
	if (account
			&& theClient->GetAccount() != account) {
		theClient->SetAccount(account);
	}

	// If it's not the current client, set the client's attributes.
	// (The current client obtains its own attributes through separate u8s.)
	RoomRef theRoom = roomManager.Get(roomID);
	if (!theClient->IsSelf()) {
		clientManifest.Deserialize(clientID, userID, "", "", globalAttributes, {roomID, roomAttributes});
		theClient->Synchronize(clientManifest);

		// If the client is observed, don't fire OBSERVE_ROOM; observed clients always
		// fire OBSERVE_ROOM based on observation updates. Likewise, don't fire OBSERVE_ROOM
		// on self; self fires OBSERVE_ROOM when it receives a u59.
		if (!clientManager.IsObservingClient(clientID)) {
			theClient->OnObserveRoom(theRoom, roomID);
		}
	}

	if (theRoom)
		theRoom->AddObserver(theClient);
}
void
UnionBridge::U130(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLIENT_STOPPED_OBSERVING_ROOM */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	ClientID clientID(args[1]);

	// Remove the room from the client's list of observed rooms
	ClientRef theClient = clientManager.Request(clientID);
	RoomRef theRoom = roomManager.Get(roomID);

	// Remove the client from the given room
	theRoom->RemoveObserver(clientID);

	// Don't fire STOP_OBSERVING_ROOM on self; self fires STOP_OBSERVING_ROOM
	// when it receives a u62.
	if (!theClient->IsSelf()) {
		// If the client is observed, don't fire STOP_OBSERVING_ROOM; observed
		// clients always fire STOP_OBSERVING_ROOM based on observation updates.
		if (!clientManager.IsObservingClient(clientID)) {
			theClient->OnStopObservingRoom(theRoom, roomID);
		}
	}
}
void
UnionBridge::U131(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ROOM_OCCUPANTCOUNT_UPDATE */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	int numClients = atoi(args[1].c_str());

	UpdateLevels levels = 0;
	if (clientManager.Self()) {
		levels = clientManager.Self()->GetUpdateLevels(roomID);
	}
	if (levels) {
		if (!UpdateOccupantList(levels)) {
			RoomRef room = roomManager.Get(roomID);
			if (room) room->SetNumOccupants(numClients);
		}
	} else {
		log.Error("[CORE_MESSAGE_LISTENER] Received a room occupant count "
				" update (u131), but update levels are unknown for the room. Synchronization"
				" error. Please report this error to union@user1.net.");
	}
}
void
UnionBridge::U132(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ROOM_OBSERVERCOUNT_UPDATE */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	RoomID roomID(args[0]);
	int numClients = atoi(args[1].c_str());

	UpdateLevels levels = 0;
	if (clientManager.Self()) {
		levels = clientManager.Self()->GetUpdateLevels(roomID);
	}
	if (levels) {
		if (!UpdateObserverList(levels)) {
			RoomRef room = roomManager.Get(roomID);
			if (room) {
				room->SetNumObservers(numClients);
			}
		}
	} else {
		log.Error("[CORE_MESSAGE_LISTENER] Received a room observer count"
				" update (u132), but update levels are unknown for the room. Synchronization"
				" error. Please report this error to union@user1.net.");
	}
}
void
UnionBridge::U133(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ADD_ROLE */
{
}
void
UnionBridge::U134(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* ADD_ROLE_RESULT */
{
	if (args.size() < 3)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	Role role(args[1]);
	UPCStatus status = UPC::Status::GetStatusCode(args[2].c_str());

	AccountRef theAccount = accountManager.GetAccount(userID);
	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ACCOUNT_NOT_FOUND:
	case UPC::Status::ROLE_NOT_FOUND:
	case UPC::Status::ALREADY_ASSIGNED:
	case UPC::Status::PERMISSION_DENIED:
		accountManager.OnAddRoleResult(userID, role, status);
		if (theAccount) {
			theAccount->OnAddRoleResult(role, status);
		}
	break;

	default:
		log.Warn("Unrecognized status code for u134. User ID: [" + userID + "], role: [" + role + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U135(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* REMOVE_ROLE */
{
}
void
UnionBridge::U136(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* REMOVE_ROLE_RESULT */
{
	if (args.size() < 3)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UserID userID(args[0]);
	Role role(args[1]);
	UPCStatus status = UPC::Status::GetStatusCode(args[2].c_str());

	AccountRef theAccount = accountManager.GetAccount(userID);
	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ACCOUNT_NOT_FOUND:
	case UPC::Status::ROLE_NOT_FOUND:
	case UPC::Status::NOT_ASSIGNED:
	case UPC::Status::PERMISSION_DENIED:
		accountManager.OnRemoveRoleResult(userID, role, status);
		if (theAccount) {
			theAccount->OnRemoveRoleResult(role, status);
		}
	break;

	default:
		log.Warn("Unrecognized status code for u136  User ID: [" + userID + "], role: [" + role + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U137(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* BAN */
{
}
void
UnionBridge::U138(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* BAN_RESULT */
{
	if (args.size() < 3)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	Address address(args[0]);
	ClientID clientID(args[1]);
	UPCStatus status = UPC::Status::GetStatusCode(args[2].c_str());

	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::CLIENT_NOT_FOUND:
	case UPC::Status::ALREADY_BANNED:
	case UPC::Status::PERMISSION_DENIED:
		clientManager.OnBanClientResult(address, clientID, status);
	break;

	default:
		log.Warn("Unrecognized status code for u138. Address: [" + address + "], clientID: [" + clientID + "], status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U139(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* UNBAN */
{
}
void
UnionBridge::U140(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* UNBAN_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	Address address(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());

	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::NOT_BANNED:
	case UPC::Status::PERMISSION_DENIED:
		clientManager.OnUnbanClientResult(address, status);
	break;

	default:
		log.Warn("Unrecognized status code for u140. Address: [" + address + "],  status: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U141(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_BANNED_LIST_SNAPSHOT */
{
}
void
UnionBridge::U142(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* BANNED_LIST_SNAPSHOT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string requestID(args[0]);

	std::stringstream idss(args[1]);
	std::string item;
	char c = *Token::RS;
	Set<Address> bannedList;
	while (std::getline(idss, item, c)) {
		bannedList.Add(item);
	}

	if (requestID == "") {
		clientManager.SetWatchedBannedAddresses(bannedList);
	} else {
#ifdef SNAPSHOT_MANAGER
		snapshotManager.RecieveBannedListSnapshot(requestID, bannedList);
#endif
	}
}
void
UnionBridge::U143(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* WATCH_FOR_BANNED_ADDRESSES */
{
}
void
UnionBridge::U144(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* WATCH_FOR_BANNED_ADDRESSES_RESULT */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UPCStatus status = UPC::Status::GetStatusCode(args[0].c_str());
	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ALREADY_WATCHING:
	case UPC::Status::PERMISSION_DENIED:
		clientManager.OnWatchForBannedAddressesResult(status);
		break;

	default:
		log.Warn("Unrecognized status code for u144: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U145(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_WATCHING_FOR_BANNED_ADDRESSES */
{
}
void
UnionBridge::U146(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_WATCHING_FOR_BANNED_ADDRESSES_RESULT */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	UPCStatus status = UPC::Status::GetStatusCode(args[0].c_str());
	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::NOT_WATCHING:
	clientManager.OnStopWatchingForBannedAddressesResult(status);
	break;

	default:
		log.Warn("Unrecognized status code for u146: [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U147(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* BANNED_ADDRESS_ADDED */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	Address address(args[0]);
	clientManager.AddWatchedBannedAddress(address);
}
void
UnionBridge::U148(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* BANNED_ADDRESS_REMOVED */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	Address address(args[0]);
	clientManager.RemoveWatchedBannedAddress(address);
}
void
UnionBridge::U149(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* KICK_CLIENT */
{
}
void
UnionBridge::U150(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* KICK_CLIENT_RESULT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

// XXX there is a conflict between what orbiter and reactor seem to do, what the server says, and what information should be coming back ... there should be a clientID on the return packet???
	ClientID clientID(args[0]);
	UPCStatus status = UPC::Status::GetStatusCode(args[1].c_str());

	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::CLIENT_NOT_FOUND:
	case UPC::Status::PERMISSION_DENIED:
		clientManager.OnKickClientResult(clientID, status);
	break;

	default:
		log.Warn("Unrecognized status code for u150:  [" + UPC::Status::GetStatusString(status) + "].");
	}
}
void
UnionBridge::U151(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_SERVERMODULELIST_SNAPSHOT */
{
}
void
UnionBridge::U152(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* SERVERMODULELIST_SNAPSHOT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

#ifdef SNAPSHOT_MANAGER
	std::string requestID(args[0]);
	std::string serverModuleListSource(args[1]);

	var moduleListArray = serverModuleListSource == "" ? [] : serverModuleListSource.split(Token::RS);
	var moduleList = [];
	for (var i = 0; i < moduleListArray.length; i+= 3) {
		moduleList.push(new ModuleDefinition(moduleListArray[i],
				moduleListArray[i+1],
				moduleListArray[i+2]));
	}

	if (requestID == "") {
		log.Warn("Incoming SERVERMODULELIST_SNAPSHOT UPC missing required requestID. Ignoring message.");
	} else {
		// Snapshot
		snapshotManager.RecieveServerModuleListSnapshot(requestID, moduleList);
	}
#endif
}
void
UnionBridge::U153(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CLEAR_MODULE_CACHE */
{
}
void
UnionBridge::U154(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_UPC_STATS_SNAPSHOT */
{
}
void
UnionBridge::U155(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_UPC_STATS_SNAPSHOT_RESULT */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string requestID(args[0]);
#ifdef SNAPSHOT_MANAGER
	snapshotManager.RecieveSnapshotResult(requestID, status);
#endif
}
void
UnionBridge::U156(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* UPC_STATS_SNAPSHOT */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

#ifdef SNAPSHOT_MANAGER
	std::string requestID(args[0]);

	float totalUPCsProcessed = atof(args[1].c_str());
	float numUPCsInQueue = atof(args[2].c_str());
	float lastQueueWaitTime = atof(args[3].c_str());

	var longestUPCProcesses = Array.prototype.slice.call(arguments).slice(5);
	var upcProcessingRecord;
	for (var i = 0; i < longestUPCProcesses.length; i++) {
		upcProcessingRecord = new net.user1.orbiter.UPCProcessingRecord();
		upcProcessingRecord.deserialize(longestUPCProcesses[i]);
		longestUPCProcesses[i] = upcProcessingRecord;
	}

	snapshotManager.RecieveUPCStatsSnapshot(requestID,
			totalUPCsProcessed,
			numUPCsInQueue,
			lastQueueWaitTime,
			longestUPCProcesses);
#endif
}
void
UnionBridge::U157(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* RESET_UPC_STATS */
{
}
void
UnionBridge::U158(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* RESET_UPC_STATS_RESULT */
{
#ifdef PROCESSING_RECORDS
	switch (status) {
	case UPC::Status::SUCCESS:
	case UPC::Status::UPC_ERROR:
	case UPC::Status::PERMISSION_DENIED:
		GetServer().OnResetUPCStatsResult(status);
	break;

	default:
		log.Warn("Unrecognized status code for u158. Status: [" + UPC::Status::GetStatusString(status) + "].");
	}
#endif
}
void
UnionBridge::U159(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* WATCH_FOR_PROCESSED_UPCS */
{
}
void
UnionBridge::U160(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* WATCH_FOR_PROCESSED_UPCS_RESULT */
{
#ifdef PROCESSING_RECORDS
	switch (status) {
	case UPC::Status::SUCCESS:
		GetServer().SetIsWatchingForProcessedUPCs(true);
	case UPC::Status::UPC_ERROR:
	case UPC::Status::ALREADY_WATCHING:
	case UPC::Status::PERMISSION_DENIED:
		GetServer().OnWatchForProcessedUPCsResult(status);
	break;

	default:
		log.Warn("Unrecognized status code for u160. Status: [" + UPC::Status::GetStatusString(status) + "].");
	}
#endif
}
void
UnionBridge::U161(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* PROCESSED_UPC_ADDED */
{
	if (args.size() < 7)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	ClientID fromClientID(args[0]);
	ClientID fromUserID(args[1]);
	Address fromClientAddress(args[2]);
	std::string queuedAt(args[3]);
	std::string processingStartedAt(args[4]);
	std::string processingFinishedAt(args[5]);
	std::string source(args[6]);

#ifdef PROCESSING_RECORDS
	UPCProcessingRecord upcProcessingRecord = new UPCProcessingRecord();
	upcProcessingRecord.deserializeParts(fromClientID,
			fromUserID,
			fromClientAddress,
			queuedAt,
			processingStartedAt,
			processingFinishedAt,
			source);
	GetServer().DispatchUPCProcessed(upcProcessingRecord);
#endif
}
void
UnionBridge::U162(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_WATCHING_FOR_PROCESSED_UPCS */
{
}
void
UnionBridge::U163(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* STOP_WATCHING_FOR_PROCESSED_UPCS_RESULT */
{
	if (args.size() < 1)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

#ifdef PROCESSING_RECORDS
	switch (status) {
	case UPC::Status::SUCCESS:
		GetServer().SetIsWatchingForProcessedUPCs(false);
	case UPC::Status::UPC_ERROR:
	case UPC::Status::NOT_WATCHING:
	case UPC::Status::PERMISSION_DENIED:
		GetServer().DispatchStopWatchingForProcessedUPCsResult(status);
		break;

	default:
		log.Warn("Unrecognized status code for u163. Status: [" + UPC::Status::GetStatusString(status) + "].");
	}
#endif
}
void
UnionBridge::U164(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* CONNECTION_REFUSED */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string reason(args[0]);
	std::string description(args[1]);

	NxConnectRefused(reason, description);
}
void
UnionBridge::U165(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_NODELIST_SNAPSHOT */
{
}

void
UnionBridge::U166(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* NODELIST_SNAPSHOT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

	std::string request(args[0]);
	std::string nodeListSource(args[1]);

#ifdef SNAPSHOT_MANAGER
	var nodeIDs = nodeListSource == "" ? [] : nodeListSource.split(Token::RS);

	if (requestID == "") {
		log.Warn("Incoming NODELIST_SNAPSHOT UPC missing required requestID. Ignoring message.");
	} else {
		snapshotManager.RecieveNodeListSnapshot(requestID, nodeIDs);
	}
#endif
}
void
UnionBridge::U167(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GET_GATEWAYS_SNAPSHOT */
{
}
void
UnionBridge::U168(EventType t, std::vector<std::string> args, UPCStatus ioStatus) /* GATEWAYS_SNAPSHOT */
{
	if (args.size() < 2)  {
		log.Error("[UNION BRIDGE] malformed packet, argument mismatch, ignoring");
		return;
	}

#ifdef SNAPSHOT_MANAGER
	std::string request(args[0]);
	std::string gatewayListSource(args[1]);

	var gateways = [];

	var gateway;
	var gatewayBandwidth;
	var gatewayBandwidthSource;
	var gatewayIntervalSource;
	for (var i = 0; i < gatewayListSource.length; i+=8) {
		gateway = new net.user1.orbiter.Gateway();
		gateway.id = gatewayListSource[i];
		gateway.type = gatewayListSource[i+1];

		gateway.lifetimeConnectionsByCategory = gatewayListSource[i+2] === "" ? {} : this.createHashFromArg(gatewayListSource[i+2]);
		for (var p in gateway.lifetimeConnectionsByCategory) {
			gateway.lifetimeConnectionsByCategory[p] = parseFloat(gateway.lifetimeConnectionsByCategory[p]);
		}
		gateway.lifetimeClientsByType = gatewayListSource[i+3] === "" ? {} : this.createHashFromArg(gatewayListSource[i+3]);
		for (p in gateway.lifetimeClientsByType) {
			gateway.lifetimeClientsByType[p] = parseFloat(gateway.lifetimeClientsByType[p]);
		}
		gateway.lifetimeClientsByUPCVersion = gatewayListSource[i+4] === "" ? {} : this.createHashFromArg(gatewayListSource[i+4]);
		for (p in gateway.lifetimeClientsByUPCVersion) {
			gateway.lifetimeClientsByUPCVersion[p] = parseFloat(gateway.lifetimeClientsByUPCVersion[p]);
		}
		gateway.attributes = gatewayListSource[i+5] === "" ? {} : this.createHashFromArg(gatewayListSource[i+5]);

		gatewayIntervalSource = gatewayListSource[i+6].split(Token::RS);
		gateway.connectionsPerSecond = parseFloat(gatewayIntervalSource[0]);
		gateway.maxConnectionsPerSecond = parseFloat(gatewayIntervalSource[1]);
		gateway.clientsPerSecond = parseFloat(gatewayIntervalSource[2]);
		gateway.maxClientsPerSecond = parseFloat(gatewayIntervalSource[3]);

		gatewayBandwidth = new net.user1.orbiter.GatewayBandwidth();
		gatewayBandwidthSource = gatewayListSource[i+7].split(Token::RS);
		gatewayBandwidth.lifetimeRead = gatewayBandwidthSource[0] === "" ? 0 : parseFloat(gatewayBandwidthSource[0]);
		gatewayBandwidth.lifetimeWritten = gatewayBandwidthSource[1] === "" ? 0 : parseFloat(gatewayBandwidthSource[1]);
		gatewayBandwidth.averageRead = gatewayBandwidthSource[2] === "" ? 0 : parseFloat(gatewayBandwidthSource[2]);
		gatewayBandwidth.averageWritten = gatewayBandwidthSource[3] === "" ? 0 : parseFloat(gatewayBandwidthSource[3]);
		gatewayBandwidth.intervalRead = gatewayBandwidthSource[4] === "" ? 0 : parseFloat(gatewayBandwidthSource[4]);
		gatewayBandwidth.intervalWritten = gatewayBandwidthSource[5] === "" ? 0 : parseFloat(gatewayBandwidthSource[5]);
		gatewayBandwidth.maxIntervalRead = gatewayBandwidthSource[6] === "" ? 0 : parseFloat(gatewayBandwidthSource[6]);
		gatewayBandwidth.maxIntervalWritten = gatewayBandwidthSource[7] === "" ? 0 : parseFloat(gatewayBandwidthSource[7]);
		gatewayBandwidth.scheduledWrite = gatewayBandwidthSource[8] === "" ? 0 : parseFloat(gatewayBandwidthSource[8]);
		gateway.bandwidth = gatewayBandwidth;
		gateways.push(gateway);
	}

	if (requestID == "") {
		log.Warn("Incoming GATEWAYS_SNAPSHOT UPC missing required requestID. Ignoring message.");
	} else {
		snapshotManager.RecieveGatewaysSnapshot(requestID, gateways);
	}
#endif
}

