/*
 * Room.cpp
 *
 *  Created on: Apr 8, 2014
 *      Author: dak
 */

#include "UCUpperHeaders.h"

/**
 * @class Room Room.h
 * @brief Interface, model, and operations on a Union Room
 *
 * Events spawned (in response to Union server events: standard UPC semantics)
 *    - UPDATE_CLIENT_ATTRIBUTE (NXAttrInfo)
 *    - DELETE_CLIENT_ATTRIBUTE (NXAttrInfo)
 *    - JOIN (NXStatusInfo)
 *    - JOIN_RESULT (NXStatusInfo)
 *    - OBSERVE (NXStatusInfo)
 *    - OBSERVE_RESULT (NXStatusInfo)
 *    - STOP_OBSERVING (NXStatusInfo)
 *    - STOP_OBSERVING_RESULT (NXStatusInfo)
 *    - LEAVE (NXStatusInfo)
 *    - LEAVE_RESULT (NXStatusInfo)
 *
 */
Room::Room(RoomID id, RoomManager &roomManager, ClientManager& clientManager, AccountManager& accountManager,
		UnionBridge &unionBridge, ILogger& log)
	: AttributeManager()
	, id("room")
	, occupantList(ClientRefId, ValidClientRef)
	, observerList(ClientRefId, ValidClientRef)
	, roomManager(roomManager)
	, clientManager(clientManager)
	, accountManager(accountManager)
	, unionBridge(unionBridge)
	, log(log) {

	disposed = false;
	clientIsInRoom = false;
	clientIsObservingRoom = false;
	numOccupants = 0;
	numObservers = 0;
	SetSyncState(kSynchronized);
	SetRoomID(id);

	updateClientAttributeListener = std::make_shared<CBAttrInfo>(
		[this](EventType t, AttrName n, AttrVal v, AttrScope s, AttrVal ov, ClientRef c, UPCStatus status) {
		NXAttrInfo::NotifyListeners(Event::UPDATE_CLIENT_ATTRIBUTE, n, v, s, ov, c, status);
	});
	deleteClientAttributeListener = std::make_shared<CBAttrInfo>(
		[this](EventType t, AttrName n, AttrVal v, AttrScope s, AttrVal ov, ClientRef c, UPCStatus status) {
		NXAttrInfo::NotifyListeners(Event::DELETE_CLIENT_ATTRIBUTE, n, v, s, ov, c, status);
	});

}

Room::~Room() {
	log.Debug("~Room() " + id);
}


///////////////////////////////////
// Attribute owner overrides
//////////////////////////////////
std::string Room::GetName() const { return id; };
UPCMessageID Room::DeleteAttribCommand() const { return UPC::ID::REMOVE_ROOM_ATTR; };
UPCMessageID Room::SetAttribCommand() const { return UPC::ID::SET_ROOM_ATTR; };
StringArgs Room::DeleteAttribRequest(AttrName attrName, AttrScope attrScope) const { return { id, attrName }; };
StringArgs Room::SetAttribRequest(AttrName attrName, AttrVal attrValue, AttrScope scope, unsigned int options) const {
	return {id, attrName, attrValue, std_to_string(options)};};
bool Room::SetLocalAttribs() const { return false; }
void Room::SendUPC(UPCMessageID c, StringArgs a) { unionBridge.SendUPC(c, a); }
/////////////////////////////////////
// getters, setters, manipulators
/////////////////////////////////////

std::string
Room::ToString() const
{
	return id;
}

/**
 * @return true if the client is in this room as an occupant
 */
bool
Room::ClientIsInRoom(const ClientID clientID) const {
	if (disposed) return false;

	if (clientID == "") {
		return clientIsInRoom;
	}
	return occupantList.Contains(clientID);
}

/**
 * @return true if the client is observing this room
 */
bool
Room::ClientIsObservingRoom(const ClientID clientID) const {
	if (disposed) return false;

	if (clientID == "") {
		return clientIsInRoom;
	}
	return observerList.Contains(clientID);
}

/**
 * @return reference to the given client from either the observer or occupant list of this room
 */
ClientRef
Room::GetClient(const ClientID client) const {
	if (disposed) return ClientRef();
	ClientRef cref = occupantList.Get(client);
	if (cref) {
		cref = observerList.Get(client);
	}
	return cref;
}

/**
 * sets the cached observer count
 */
void
Room::SetNumObservers(int n) const
{
	numObservers = n;
}

/**
 * @return either the cached observer count for the self client, or for other clients, attempts to update the observeer count
 */
int
Room::GetNumObservers() const {
	if (disposed) return 0;

	ClientRef self = clientManager.Self();
	if (!self) return numObservers;
	UpdateLevels levels = self->GetUpdateLevels(GetRoomID());

	if (RecievesUpdates(levels)) {
		if (UpdateObserverCount(levels) || UpdateObserverList(levels)) {
			return numObservers;
		} else {
			log.Warn(ToString() + " GetNumObservers() called, but no observer count is " +
				   "available. To enable observer count, turn on observer list" +
				   " updates or observer count updates via the Room's SetUpdateLevels()" +
				   " method.");
			return 0;
		}
	} else {
		log.Warn(ToString() + " GetNumObservers() called, but the current client's update "
					+ " levels for the room are unknown. Please report this issue to union@user1.net.");
		return 0;
	}
}

/**
 * sets the cached occupant count
 */
void
Room::SetNumOccupants(int n) const
{
	numOccupants = n;
}

/**
 * @return the cached occupant count for the self client, else it will get an updated figure
 */
int
Room::GetNumOccupants() const {
	if (disposed) return 0;
	ClientRef self = clientManager.Self();
	if (!self) return numOccupants;
	UpdateLevels levels = self.get()->GetUpdateLevels(GetRoomID());
	if (RecievesUpdates(levels)) {
		if (UpdateOccupantCount(levels) || UpdateOccupantList(levels)) {
			return numOccupants;
		} else {
			log.Warn(ToString() + " GetNumOccupants() called, but no occupant count is " +
				"available. To enable occupant count, turn on occupant list" +
				" updates or occupant count updates via the Room's SetUpdateLevels()" +
				" method.");
			return 0;
		}
	} else {
		log.Debug(ToString() + " GetNumOccupants() called, but the current client's update"
		   + " levels for the room are unknown. To determine the room's"
		   + " occupant count, first join or observe the room.");
		return 0;
	}
}

/**
 * setter for the room id, check that this is a valid room id first
 */
void
Room::SetRoomID(RoomID rid)
{
	if (!UPCUtils::IsValidResolvedRoomID(rid)) {
		log.Error("Invalid room ID specified during room creation. Offending ID: " + rid);
		return;
	}
	id = rid;
}

/**
 * @return the current room id
 */
const RoomID &
Room::GetRoomID() const
{
	return id;
}

/**
 * @return the current room id, stripped of any qualifiers
 */
const RoomID
Room::GetSimpleRoomID() const {
	return UPCUtils::GetSimpleRoomID(id);
}

/**
 * @return the qualifier part of the current room id
 */
const RoomQualifier
Room::GetQualifier() const {
	return UPCUtils::GetRoomQualifier(id);
}

/**
 * @return a room settings structure based on the current actual settings from attributes
 */
RoomSettings
Room::GetRoomSettings() const {
	  if (disposed) return RoomSettings();

	  RoomSettings settings;
	  AttrVal maxClients = GetAttribute(Token::MAX_CLIENTS_ATTR);
	  AttrVal removeOnEmpty = GetAttribute(Token::REMOVE_ON_EMPTY_ATTR);

	  if (maxClients != "") {
		  settings.maxClients = atoi(maxClients.c_str());
	  }
	  if (removeOnEmpty != "") {
		  settings.removeOnEmpty = removeOnEmpty.compare("true");
	  }
	  return settings;
}

/**
 * sets the current room settings attributes according to the passed structure
 */
void
Room::SetRoomSettings(const RoomSettings &settings){
	if (disposed) return;

	if (settings.maxClients > 0) {
		SetAttribute(Token::MAX_CLIENTS_ATTR, std_to_string(settings.maxClients));
	}
	if (settings.password != "") {
		SetAttribute(Token::PASSWORD_ATTR, settings.password);
	}
	if (settings.removeOnEmpty >= 0) {
		SetAttribute(Token::REMOVE_ON_EMPTY_ATTR, bool_to_string(settings.removeOnEmpty!=0));
	}
}

///////////////////////////////
// list maintenance
//////////////////////////////

/**
 * @return all the client ids in the observer list
 */
std::vector<ClientID>
Room::GetObserverIDs() const{
	return observerList.GetKeys();
}

/**
 * @return all the client refs in the observer list
 */
std::vector<ClientRef>
Room::GetObservers() const{
	return observerList.GetValues();
}

/**
 * @return all the client ids in the occupant list
 */
std::vector<ClientID>
Room::GetOccupantIDs() const{
	return occupantList.GetKeys();
}

/**
 * @return all the client refs in the occupant list
 */
std::vector<ClientRef>
Room::GetOccupants() const{
	return occupantList.GetValues();
}

/**
 * adds the given client to the occupant list, generating a notification to listeners
 */
void
Room::AddOccupant(ClientRef client) const
{
	if (client) {
//		DEBUG_OUT("add client " << client->GetName() << " to " << id << " currently has " << occupantList.size());
		if (occupantList.Add(client)) {
			SetNumOccupants(occupantList.Length());
			if (!observerList.Contains(client)) {
				AddClientAttributeListeners(client);
			}
//			DEBUG_OUT(const_cast<Room*>(this)->NXClientInfo::listeners.size() << " client info listeners for " << id);
			const_cast<Room*>(this)->NXClientInfo::NotifyListeners(Event::ADD_OCCUPANT, client->GetClientID(), client, numOccupants, id, shared_from_this(), UPC::Status::SUCCESS);
		} else {
			log.Info(id + " ignored AddOccupant() request. Occupant list" + " already contains client:" + client->GetClientID() + ".");
		}
	}
}

/**
 * removes the given occupant from this room
 */
void
Room::RemoveOccupant(ClientID clientID) const
{
	log.Debug("remove occupant "+id);
	ClientRef client = occupantList.Remove(clientID);
	if (client) {
		SetNumOccupants(occupantList.Length());
		if (!observerList.Contains(client)) {
			RemoveClientAttributeListeners(client);
		}
		const_cast<Room*>(this)->NXClientInfo::NotifyListeners(Event::REMOVE_OCCUPANT, clientID, client, numOccupants, id, shared_from_this(), UPC::Status::SUCCESS);
	} else {
		log.Debug(id + " could not remove occupant: " + clientID + ". No such client in the room's occupant list.");
	}
}

/**
 * adds the given client to the observer list, generating a notification to listeners
 */
void
Room::AddObserver(ClientRef client) const
{
	if (client) {
		if (observerList.Add(client)) {
			SetNumObservers(observerList.Length());
			if (!observerList.Contains(client)) {
				AddClientAttributeListeners(client);
			}
			const_cast<Room*>(this)->NXClientInfo::NotifyListeners(Event::ADD_OBSERVER, client->GetClientID(), client, numOccupants, id, shared_from_this(), UPC::Status::SUCCESS);
		} else {
			log.Info(id + " ignored AddObserver() request. Occupant list" + "already contains client:" + client->GetClientID() + ".");
		}
	}
}

/**
 * removes the given observer from this room
 */
void
Room::RemoveObserver(ClientID clientID) const
{
	ClientRef client = occupantList.Remove(clientID);
	if (client) {
		SetNumOccupants(occupantList.Length());
		if (!observerList.Contains(client)) {
			RemoveClientAttributeListeners(client);
		}
		const_cast<Room*>(this)->NXClientInfo::NotifyListeners(Event::REMOVE_OBSERVER, clientID, client, numOccupants, id, shared_from_this(), UPC::Status::SUCCESS);

	} else {
		log.Debug(id + " could not remove observer: " + clientID + ". No such client in the room's occupant list.");
	}
}

/**
 * adds appropriate client attribute listeners for this client
 */
void
Room::AddClientAttributeListeners(ClientRef c) const
{
	if (c) {
		c->NXAttrInfo::AddListener(Event::UPDATE_ATTR, updateClientAttributeListener);
		c->NXAttrInfo::AddListener(Event::REMOVE_ATTR, deleteClientAttributeListener);
	}

}

/**
 * removes appropriate client attribute listeners for this client
 */
void
Room::RemoveClientAttributeListeners(ClientRef c) const
{
	if (c) {
		c->NXAttrInfo::RemoveListener(Event::UPDATE_ATTR, updateClientAttributeListener);
		c->NXAttrInfo::RemoveListener(Event::REMOVE_ATTR, deleteClientAttributeListener);
	}
}

/**
 * adds a room specific message listener to the UnionBridge notifiers
 */
CBModMsgRef
Room::AddMessageListener(const UserMessageID message, const CBModMsg listener) const {
	return unionBridge.AddMessageListener(message, {}, {GetRoomID()}, listener);
};

/**
 * removes a room specific message listener to the UnionBridge notifiers
 */
void
Room::RemoveMessageListener(const UserMessageID message, const CBModMsgRef listener) const {
	unionBridge.RemoveMessageListener(message, listener);
};

/**
 * @return true if there is a room specific message listener in  the UnionBridge notifiers
 */
bool
Room::HasMessageListener(const UserMessageID message, const CBModMsgRef listener) const {
	return unionBridge.HasMessageListener(message, listener);
};

/////////////////////////////////////
// synchronization
/////////////////////////////////////

/**
 * @return the actual occupant list
 */
const Map<ClientID, ClientRef>&
Room::GetOccupantList() const
{
	return occupantList;
}

/**
 * @return a reference for the given ClientID
 */
const ClientRef
Room::GetOccupant(const ClientID clientID) const
{
	return occupantList.Get(clientID);
}

/**
 * @return the actual observer list
 */
const Map<ClientID, ClientRef>&
Room::GetObserverList() const
{
	return observerList;
}

/**
 * @return a specific observer from the occupant list
 */
const ClientRef
Room::GetObserver(const ClientID clientID) const
{
	return occupantList.Get(clientID);
}

/////////////////////////////////////
// synchronization
/////////////////////////////////////

/**
 * setter for syncState
 */
void
Room::SetSyncState(SyncState s) const {
	syncState = s;
}

/**
 * getter for syncState
 */
SyncState
Room::GetSyncState() const {
	return syncState;
}

/**
 * refresh sync state if occupied observed or satched
 */
void
Room::UpdateSyncState() const
{
	if (disposed) {
		SetSyncState(kUnsynchronized);
	} else if (roomManager.HasObservedRoom(GetRoomID())
			|| roomManager.HasOccupiedRoom(GetRoomID())
			|| roomManager.HasWatchedRoom(GetRoomID())) {
		SetSyncState(kSynchronized);
	} else {
		SetSyncState(kUnsynchronized);
	}
}

/**
 * call synchronize for all connected clients
 */
void
Room::Synchronize(const RoomManifest& manifest) const {
	SyncState oldSyncState = GetSyncState();
	SetSyncState(kSynchronizing);

	// SYNC ROOM ATTRIBUTES
	/*attributeManager.GetAttributeCollection()*/attributes.SynchronizeScope(Token::GLOBAL_ATTR, manifest.attributes);
	if (disposed) {
		return;
	}

	// SYNC OCCUPANT LIST
	std::vector<ClientID> oldOccupantList = GetOccupantIDs();
	std::vector<ClientID> newOccupantList;

	// Add all unknown occupants to the room's occupant list, and
	// synchronize all existing occupants.
	for (auto it=manifest.occupants.begin(); it != manifest.occupants.end(); ++it) {
		ClientID thisOccupantClientID = it->clientID;
		UserID thisOccupantUserID = it->userID;
		ClientRef thisOccupant;
		AccountRef thisOccupantAccount;

		newOccupantList.push_back(thisOccupantClientID);

		thisOccupant = clientManager.Request(thisOccupantClientID);
		if (thisOccupant) {
			thisOccupantAccount = accountManager.Request(thisOccupantUserID);
			if (thisOccupantAccount) {
				thisOccupant->SetAccount(thisOccupantAccount);
			}
			if (!thisOccupant->IsSelf()) {
				// If it's not the current client, update it.
				// The current client obtains its attributes through separate u8s.
				thisOccupant->Synchronize(*it);
			}

			AddOccupant(thisOccupant);
			if (disposed) {
				return;
			}
		}
	}

	// Remove occupants that are now gone...
	for (auto it=oldOccupantList.begin(); it!=oldOccupantList.end(); ++it) {
		if (!UPCUtils::InVector(newOccupantList, *it)) {
			RemoveOccupant(*it);
			if (disposed) {
				return;
			}
		}
	}

	// SYNC OBSERVER LIST
	std::vector<ClientID> oldObserverList = GetObserverIDs();
	std::vector<ClientID> newObserverList;

	// Add all unknown observers to the room's observer list, and
	// synchronize all existing observers.
	for (auto it=manifest.observers.begin(); it != manifest.observers.end(); ++it) {
		ClientID thisObserverClientID = it->clientID;
		UserID thisObserverUserID = it->userID;
		ClientRef thisObserver;
		AccountRef thisObserverAccount;

		newObserverList.push_back(thisObserverClientID);

		thisObserver = clientManager.Request(thisObserverClientID);
		// Init user account, if any
		thisObserverAccount = accountManager.Request(thisObserverUserID);
		if (thisObserver) {
			if (thisObserverAccount) {
				thisObserver->SetAccount(thisObserverAccount);
				// If it's not the current client, update it.
				// The current client obtains its attributes through separate u8s.
				if (!thisObserver->IsSelf()) {
					thisObserver->Synchronize(*it);
				}

				AddObserver(thisObserver);
			}

			if (disposed) {
				return;
			}
		}
	}

	// Remove observers that are now gone...
	// Remove occupants that are now gone...
	for (auto it=oldObserverList.begin(); it!=oldObserverList.end(); ++it) {
		if (!UPCUtils::InVector(newObserverList, *it)) {
			RemoveOccupant(*it);
			if (disposed) {
				return;
			}
		}
	}

	// UPDATE CLIENT COUNTS
	//   If a client list is available, use its length to calculate the
	//   client count. That way, the list length and the "get count" method
	//   return values will be the same (e.g., getOccupants().length and
	//   getNumOccupants()). Otherwise, rely on the server's reported count.
	if (clientManager.Self()) {
		UpdateLevels levels = clientManager.Self()->GetUpdateLevels(GetRoomID());
		if (UpdateOccupantList(levels)) {
			SetNumOccupants(occupantList.Length());
		} else if (UpdateOccupantCount(levels)) {
			SetNumOccupants(manifest.occupantCount);
		}
		if (UpdateObserverList(levels)) {
			SetNumObservers(observerList.Length());
		} else if (UpdateObserverCount(levels)) {
			SetNumObservers(manifest.observerCount);
		}
	}

	SetSyncState(oldSyncState);
	OnSynchronized();
}
/////////////////////////////////////
// server actions
/////////////////////////////////////

/**
 * send upc to join a room
 */
void
Room::Join(const std::string password, const UpdateLevels updateLevels) const{
	if (disposed) return;

	// Client can't join a room the its already in.
	if (ClientIsInRoom()) {
		log.Warn(ToString() + "Room join attempt aborted. Already in room.");
		return;
	}
	if (!UPCUtils::IsValidPassword(password)) {
		log.Error(ToString() + ": Invalid room password supplied to join(). " + " Join request not sent. See Utils::IsValidPassword().");
		return;
	}

	if (updateLevels != 0) {
		SetUpdateLevels(updateLevels);
	}

	unionBridge.SendUPC(UPC::ID::JOIN_ROOM, {GetRoomID(), password});
}

/**
 * send upc to leave a room
 */
void
Room::Leave() const {
	if (disposed) return;

	if (ClientIsInRoom()) {
		unionBridge.SendUPC(UPC::ID::LEAVE_ROOM, {GetRoomID()});
	} else {
		log.Debug(ToString() + ": Leave room request ignored. Not in room.");
	}
}

/**
 * send upc to observe this room
 */
void
Room::Observe(const std::string password, const UpdateLevels updateLevels) const {
	if (disposed) return;
	roomManager.ObserveRoom(GetRoomID(), password, updateLevels);
}

/**
 * send upc to stop observing this room
 */
void
Room::StopObserving() const {
	if (disposed) return;

	if (ClientIsObservingRoom()) {
		unionBridge.SendUPC(UPC::ID::STOP_OBSERVING_ROOM, {GetRoomID()});
	} else {
		log.Debug(ToString() + " Stop-observing-room request ignored. Not observing room.");
	}
}

/**
 * request that this room be removed
 */
void
Room::Remove(std::string password){
	// todo
}

/**
 * main wrapper for sending UPCs
 */
void
Room::SendUPCMessage(const UPCMessageID messageName, const std::vector<std::string> msg, const bool includeSelf, const IFilterRef filters) const {
	  if (disposed) return;
	  roomManager.SendMessage(messageName, {GetRoomID()}, msg, includeSelf, filters);

}

/**
 * send a module specific message
 */
void
Room::SendModuleMessage(const UserMessageID messageName, const StringMap msg) const{
	if (disposed) return;

	StringArgs sendupcArgs = {GetRoomID(), messageName};
	for (auto it: msg) {
		sendupcArgs.push_back(it.first + "|" + it.second);
	}
	unionBridge.SendUPC(UPC::ID::SEND_ROOMMODULE_MESSAGE, sendupcArgs);
}

/**
 * set the update levels for this request
 */
void
Room::SetUpdateLevels(const UpdateLevels updateLevels) const {
	unionBridge.SendUPC(UPC::ID::SET_ROOM_UPDATE_LEVELS, {GetRoomID(), std_to_string(updateLevels)});
}

////////////////////////////////////
// notify and event hooks
///////////////////////////////////

/**
 * called on joining a room ... simple wrapper around upc onjoin
 */
void
Room::OnJoin() const {
	clientIsInRoom = true;
	const_cast<Room*>(this)->NXStatus::NotifyListeners(Event::JOIN, UPC::Status::SUCCESS);
};

/**
 * called on synchronizing a room ...
 */
void
Room::OnSynchronized() const {
	const_cast<Room*>(this)->NXStatus::NotifyListeners(Event::SYNCHRONIZED, UPC::Status::SUCCESS);
};


/**
 * called back when we have notifications regarding previous join attempts
 */
void
Room::OnJoinResult(UPCStatus status) const {
	const_cast<Room*>(this)->NXStatus::NotifyListeners(Event::JOIN_RESULT, status);
};

/**
 * called back when we have attempted to leave a room
 */
void
Room::OnLeave() const {
	if (!ClientIsObservingRoom()) {
		PurgeRoomData();
	}
	clientIsInRoom = false;
	const_cast<Room*>(this)->NXStatus::NotifyListeners(Event::LEAVE, UPC::Status::SUCCESS);
}

/**
 * called back when we are sure our last leave was successful
 */
void
Room::OnLeaveResult(UPCStatus status) const {
	const_cast<Room*>(this)->NXStatus::NotifyListeners(Event::LEAVE_RESULT, status);
};

/**
 * called back when room is observed
 */
void
Room::OnObserve() const {
	clientIsObservingRoom = true;
	const_cast<Room*>(this)->NXStatus::NotifyListeners(Event::OBSERVE, UPC::Status::SUCCESS);
};

/**
 * called back when status for last observe request is known
 */
void
Room::OnObserveResult(UPCStatus status) const {
	const_cast<Room*>(this)->NXStatus::NotifyListeners(Event::OBSERVE_RESULT, status);
};

/**
 * called  when we request the end of observing
 */
void
Room::OnStopObserving() const {
	if (!ClientIsInRoom()) {
		PurgeRoomData();
	}
	clientIsObservingRoom = false;
	const_cast<Room*>(this)->NXStatus::NotifyListeners(Event::STOP_OBSERVING, UPC::Status::SUCCESS);
}


/**
 * called to stop observing this room
 */
void
Room::OnStopObservingResult(UPCStatus status) const {
	const_cast<Room*>(this)->NXStatus::NotifyListeners(Event::STOP_OBSERVING_RESULT, status);
}

///////////////////////////////////////
// cleanup
//////////////////////////////////////

/**
 * cleanup
 */
void
Room::PurgeRoomData() const {
	if (disposed) return;

	log.Debug(ToString() + " Clearing occupant list.");
// TODO FIXME I think that this is unnecessary now ... removing the shared client should remove the shared pointer to any listeners from the
// client ... the notify routine should delete these automatically. needs to be tested thoroughly esp wrt multithreaded situation
// oth maybe it's still better to be explicit
	occupantList.ApplyToAll([this](ClientRef c) {
		RemoveClientAttributeListeners(c);
	});
	occupantList.RemoveAll();

	observerList.ApplyToAll([this](ClientRef c) {
		RemoveClientAttributeListeners(c);
	});
	observerList.RemoveAll();

	log.Debug(ToString() + " Clearing room attributes.");
	const_cast<Room*>(this)->RemoveAllAttributes();
}

