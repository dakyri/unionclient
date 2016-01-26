/*
 * Client.cpp
 *
 *  Created on: Apr 8, 2014
 *      Author: dak
 */

#include "UCUpperHeaders.h"

/**
 * @class Client Client.h
 * @brief Interface, model, and operations on a Union Client
 */
Client::Client(ClientID clientID, RoomManager& roomManager, ClientManager& clientManager, AccountManager& accountManager,
		UnionBridge& unionBridge, ILogger& log)
	: AttributeManager()
	, id(clientID)
	, roomManager(roomManager)
	, clientManager(clientManager)
	, accountManager(accountManager)
	, unionBridge(unionBridge)
	, log(log) {
}

Client::~Client() {
	log.Debug("~Client()");
}

///////////////////////////////////
// Attribute owner overrides
//////////////////////////////////

std::string Client::GetName() const  { return id; };
UPCMessageID Client::DeleteAttribCommand() const { return UPC::ID::REMOVE_CLIENT_ATTR; };
UPCMessageID Client::SetAttribCommand() const { return UPC::ID::SET_CLIENT_ATTR; };
StringArgs Client::DeleteAttribRequest(AttrName attrName, AttrScope attrScope) const { return { id, "", attrName, attrScope }; };
StringArgs Client::SetAttribRequest(AttrName attrName, AttrVal attrValue, AttrScope scope, unsigned int options) const {
	return {id, "", attrName, attrValue, scope, std_to_string(options)};};
bool Client::SetLocalAttribs() const { return IsSelf(); }
void Client::SendUPC(const UPCMessageID c, const StringArgs a) { unionBridge.SendUPC(c, a); }

/**
 * @return true if this client is an admin, according to current attributes
 */
bool
Client::IsAdmin() const {
	AttrVal rolesVal = GetAttribute(Token::ROLES_ATTR);
	if (rolesVal != "") {
		return (atoi(rolesVal.c_str()) & kFLAG_ADMIN) != 0; // FIXME std::stoi broken under cygwin
	} else {
		log.Warn("[" + id + "] Could not determine admin status because the client is not synchronized.");
		return false;
	}
	return false;
}

/**
 * @return true if this room is in the clients observe list
 */
bool
Client::IsObservingRoom(const RoomID roomID) const {
	return observedRooms.Contains(roomID);
}

/**
 * @return true if this is the self client
 */
bool
Client::IsSelf() const {
	return isSelf;
}

/**
 * sets the isself flag to true
 */
void
Client::SetIsSelf() const {
	isSelf = true;
}

/**
 * @return true if this client is in the given room
 */
bool
Client::IsInRoom(const RoomID roomID) const {
	return occupiedRooms.Contains(roomID);
}

/**
 * @return the UpdateLevels map from this clients "_UL" attribute
 */
UpdateLevels
Client::GetUpdateLevels(const RoomID roomID) const
{
	AttrVal levelsAttr = GetAttribute("_UL", roomID);
	if (levelsAttr != "") {
		return atoi(levelsAttr.c_str());
	} else {
		return 0;
	}
}

/**
 * sets a reference to this clients account
 */
void
Client::SetAccount(AccountRef r) const
{
	if (!r) {
		account.reset();
	} else if (account != r) {
		account = r;
		account->SetClient(shared_from_this());
	}
}

/**
 * @return the client id
 */
const ClientID
Client::GetClientID() const {
	return id;
}

/**
 * @return the associated account
 */
const AccountRef
Client::GetAccount() const {
	return account;
}

/**
 * @return the UserID from this clients account
 */
const UserID
Client::GetUserID() const {
	if (account) {
		return account->GetUserID();
	}
	return UserID();
}

/**
 * @return a vector of room ids that this client occupies
 */
const std::vector<RoomID>
Client::GetOccupiedRoomIDs() const {
	std::vector<RoomID> rooms;
	if (clientManager.IsObservingClient(id)) {
		// This client is under observation, so its occupiedRoomIDs array is 100% accurate.
		return occupiedRooms.Elements();
	} else {
		// This client is not under observation, so the current client can only
		// deduce this client's occupied room list based on its current sphere of awareness.
		std::vector<RoomRef> knownRooms = roomManager.GetRooms();

		for (auto it: knownRooms) {
			if (it) {
				if (it->ClientIsInRoom(id)) {
					rooms.push_back(it->GetRoomID());
				}
			}
		}
	}
	return rooms;
}

/**
 * @return a vector of RoomID's this client is observing
 */
const std::vector<RoomID>
Client::GetObservedRoomIDs() const {
	std::vector<RoomID> rooms;
	if (clientManager.IsObservingClient(id)) {
		// This client is under observation, so its occupiedRoomIDs array is 100% accurate.
		return observedRooms.Elements();
	} else {
		// This client is not under observation, so the current client can only
		// deduce this client's occupied room list based on its current sphere of awareness.
		std::vector<RoomRef> knownRooms = roomManager.GetRooms();

		for (auto it: knownRooms) {
			if (it) {
				if (it->ClientIsObservingRoom(id)) {
					rooms.push_back(it->GetRoomID());
				}
			}
		}
	}
	return rooms;
}

/**
 * Normally, this client's connection state is not assigned directly; it
 * it is deduced within getConnectionState(). But when Union
 * sends a u103, we know that this client has definitely disconnected from
 * the server, and this client object will never be reused, so CoreMessageListener
 * permanently assigns its connection state to NOT_CONNECTED.
 *
 * @return the current connection state
 */
void
Client::SetConnectionState(int newState) const {
	connectionState = newState;
}

/**
 * adds a room id to this clients list of occupied rooms
 */
void
Client::AddOccupiedRoomID(const RoomID r) const
{
	occupiedRooms.Add(r);
}

/**
 * removes a room id to this clients list of occupied rooms
 */
void
Client::RemoveOccupiedRoomID(const RoomID r) const
{
	occupiedRooms.Remove(r);

}

/**
 * adds a room id to this clients list of observed rooms
 */
void
Client::AddObservedRoomID(const RoomID r) const
{
	observedRooms.Add(r);
}

/**
 * removes a room id to this clients list of observed rooms
 */
void
Client::RemoveObservedRoomID(const RoomID r) const
{
	observedRooms.Add(r);
}


/**
 * synchronizes this client to the server-given client manifest
 */
void
Client::Synchronize(const ClientManifest &clientManifest) const
{
	occupiedRooms.Synchronize(clientManifest.occupiedRoomIDs);
	observedRooms.Synchronize(clientManifest.observedRoomIDs);

	// Synchronize Client attributes
	std::vector<AttrScope> scopes = clientManifest.transientAttributes.GetScopes();
	for (auto it=scopes.begin(); it!=scopes.end(); ++it) {
		attributes.SynchronizeScope(*it, clientManifest.transientAttributes);
	}
	// Synchronize UserAccount attributes
	if (account) {
		scopes = clientManifest.persistentAttributes.GetScopes();
		for (auto it=scopes.begin(); it!=scopes.end(); ++it) {
//			const AttributeManager& am =account->GetAttributeManager();
			/*const_cast<AttributeManager&>(am)*/account->GetAttributeCollection().SynchronizeScope(*it, clientManifest.persistentAttributes);
		}
	}
}

/**
 * adds a room specific message listener to the UnionBridge notifiers
 */
CBModMsgRef
Client::AddMessageListener(const UserMessageID message, const CBModMsg listener) const {
	return unionBridge.AddMessageListener(message, {GetClientID()}, {}, listener);
};

/**
 * removes a room specific message listener to the UnionBridge notifiers
 */
void
Client::RemoveMessageListener(const UserMessageID message, const CBModMsgRef listener) const {
	unionBridge.RemoveMessageListener(message, listener);
};

/**
 * @return true if there is a room specific message listener in  the UnionBridge notifiers
 */
bool
Client::HasMessageListener(const UserMessageID message, const CBModMsgRef listener) const {
	return unionBridge.HasMessageListener(message, listener);
};

/**
 * sends appropriate UPC to ban this client
 */
void
Client::Ban(int duration, std::string reason) const{
	if (id == "") {
		log.Warn(GetName() + " Ban attempt failed. Client id is null.");
	}
	unionBridge.SendUPC(UPC::ID::BAN, {"", GetClientID(), std_to_string(duration), reason});
}

/**
 * sends appropriate UPC to kick this client
 */
void
Client::Kick() const {
	if (id == "") {
		log.Warn(GetName() + " Kick attempt failed. Client id is null.");
		return;
	}
	unionBridge.SendUPC(UPC::ID::KICK_CLIENT, { GetClientID() });
}

/**
 * sends appropriate UPC to observe this client
 */
void
Client::Observe() const {
	if (id == "") {
		log.Warn(GetName() + " Observe attempt failed. Client id is null.");
		return;
	}
	unionBridge.SendUPC(UPC::ID::OBSERVE_CLIENT, { GetClientID() });
}

/**
 * sends appropriate UPC to stop observing this client
 */
void
Client::StopObserving() const {
	if (id == "") {
		log.Warn(GetName() + " Observe attempt failed. Client not currently connected.");
		return;
	}
	unionBridge.SendUPC(UPC::ID::STOP_OBSERVING_CLIENT, { GetClientID() });
}

/**
 * sends appropriate UPC to send the given message to this client
 */
void
Client::SendMessage(UPCMessageID messageName, std::vector<string> msgs) const {
	msgs.insert(msgs.begin(), {id, UserID()});
	clientManager.SendMessage(messageName, msgs);
}

/**
 * callback for when this client joins a room
 */
void
Client::OnJoinRoom(RoomRef room, RoomID roomID) const {
	log.Debug(GetName() + " triggering Event.JOIN_ROOM event. ");
	const_cast<Client*>(this)->NXClientInfo::NotifyListeners(Event::JOIN_ROOM, id, shared_from_this(), -1, roomID, room, UPC::Status::SUCCESS);
};

/**
 * callback for when this client leaves a room
 */
void
Client::OnLeaveRoom(RoomRef room, RoomID roomID) const {
	log.Debug(GetName() + " triggering Event.LEAVE_ROOM event.");
	const_cast<Client*>(this)->NXClientInfo::NotifyListeners(Event::LEAVE_ROOM, id, shared_from_this(), -1, roomID, room, UPC::Status::SUCCESS);
};

/**
 * call back for when this client observes a given room
 */
void
Client::OnObserveRoom(RoomRef room, RoomID roomID) const {
	log.Debug(GetName() + " triggering Event.OBSERVE_ROOM event.");
	const_cast<Client*>(this)->NXClientInfo::NotifyListeners(Event::OBSERVE_ROOM, id, shared_from_this(), -1, roomID, room, UPC::Status::SUCCESS);
};

/**
 * called back when this client stops observing a particular room
 */
void
Client::OnStopObservingRoom(RoomRef room, RoomID roomID) const {
	log.Debug(GetName() + " triggering Event.STOP_OBSERVING_ROOM event.");
	const_cast<Client*>(this)->NXClientInfo::NotifyListeners(Event::STOP_OBSERVING_RESULT, id, shared_from_this(), -1, roomID, room, UPC::Status::SUCCESS);
};

/**
 * called back when this client begins observing
 */
void
Client::OnObserve() const {
	const_cast<Client*>(this)->NXClientInfo::NotifyListeners(Event::OBSERVE, id, shared_from_this(), -1, RoomID(), RoomRef(), UPC::Status::SUCCESS);
};

/**
 * called back when this client stops observing
 */
void
Client::OnStopObserving() const {
	const_cast<Client*>(this)->NXClientInfo::NotifyListeners(Event::STOP_OBSERVING, id, shared_from_this(), -1, RoomID(), RoomRef(), UPC::Status::SUCCESS);
};

/**
 * called back when this client gets a result from observing
 */
void
Client::OnObserveResult(UPCStatus status) const {
	const_cast<Client*>(this)->NXClientInfo::NotifyListeners(Event::OBSERVE_RESULT, id, shared_from_this(), -1, RoomID(), RoomRef(), status);
};

/**
 * called back when this client gets a stop observing result from server
 */
void
Client::OnStopObservingResult(UPCStatus status) const {
	const_cast<Client*>(this)->NXClientInfo::NotifyListeners(Event::STOP_OBSERVING_RESULT, id, shared_from_this(), -1, RoomID(), RoomRef(), status);
};

/**
 * called back when this client logs on
 */
void
Client::OnLogin() const {
	const_cast<Client*>(this)->NXAcctInfo::NotifyListeners(Event::LOGOFF, GetUserID(), GetClientID(), Role(), AccountRef(), UPC::Status::SUCCESS);
};

/**
 * called back when this client is logs off
 */
void
Client::OnLogoff(UserID userID) const {
	const_cast<Client*>(this)->NXAcctInfo::NotifyListeners(Event::LOGOFF, userID, GetClientID(), Role(), AccountRef(), UPC::Status::SUCCESS);
};

/**
 * called back when this client is synchronized from the server
 */
void
Client::OnSynchronize() const {
	const_cast<Client*>(this)->NXStatus::NotifyListeners(Event::SYNCHRONIZED, UPC::Status::SUCCESS);
};

