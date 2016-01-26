/*
 * RoomManager.cpp
 *
 *  Created on: Apr 7, 2014
 *      Author: dak
 */

#include "UCUpperHeaders.h"

/**
 * @class RoomManager RoomManager.h
 * manager for our cache of room objects, which represent the information we have received from the server regarding rooms. also holds lists of watched rooms etc. and is our main
 * interface for building, and modifying rooms
 */
RoomManager::RoomManager(ClientManager& clientManager, AccountManager& accountManager, UnionBridge& unionBridge, ILogger& log)
	: Manager(RoomRefId, ValidRoomRef, log)
	, clientManager(clientManager)
	, accountManager(accountManager)
	, unionBridge(unionBridge)
	, occupiedRooms(RoomRefId, ValidRoomRef)
	, observedRooms(RoomRefId, ValidRoomRef)
	, watchedRooms(RoomRefId, ValidRoomRef) {

	DEBUG_OUT("RoomManager::RoomManager();");
// TODO do we really need these collection listeners and events? afics we are managing these objects from private methods
/*
	  cachedRooms.addListener(CollectionEvent::REMOVE_ITEM, removeRoomListener);
	  occupiedRooms.addListener(CollectionEvent::ADD_ITEM, addRoomListener);
	  occupiedRooms.addListener(CollectionEvent::REMOVE_ITEM, removeRoomListener);
	  observedRooms.addListener(CollectionEvent::ADD_ITEM, addRoomListener);
	  observedRooms.addListener(CollectionEvent::REMOVE_ITEM, removeRoomListener);
	  watchedRooms.addListener(CollectionEvent::ADD_ITEM, addRoomListener);
	  watchedRooms.addListener(CollectionEvent::REMOVE_ITEM, removeRoomListener);
*/

	watchForRoomsResultListener = std::make_shared<CBRoomInfo>([this](EventType t, RoomQualifier q, RoomID id, RoomRef ref, UPCStatus status) {
		if (status == UPC::Status::SUCCESS) {
			if (q != "") {
				watchedQualifiers.push_back(q);
			} else if (ref) {
				q = ref->GetQualifier();
				if (q != "") {
					watchedQualifiers.push_back(q);
				}

			}
		}
	});
	stopWatchingForRoomsResultListener = std::make_shared<CBRoomInfo>([this](EventType t, RoomQualifier q, RoomID id, RoomRef ref, UPCStatus status) {
		if (status == UPC::Status::SUCCESS) {
			if (q != "") {
				auto it=watchedQualifiers.begin();
				while (it!=watchedQualifiers.end()) {
					RoomQualifier qq = *it;
					if (q == qq) {
						it = watchedQualifiers.erase(it);
					} else {
						++it;
					}
				}
			}
		}
	});

	NXRoomInfo::AddListener(Event::WATCH_FOR_ROOMS_RESULT, watchForRoomsResultListener);
	NXRoomInfo::AddListener(Event::STOP_WATCHING_FOR_ROOMS_RESULT, stopWatchingForRoomsResultListener);

}

RoomManager::~RoomManager() {
	DEBUG_OUT("RoomManager::~RoomManager()");

}

/////////////////////////////
// UPC server calls
/////////////////////////////

/**
 * request server to create a room ... precreates a room entry in the cache
 *  @return a shared pointer, empty if not possible to create
 */
RoomRef
RoomManager::CreateRoom(RoomID roomID, std::shared_ptr<RoomSettings> roomSettings, std::shared_ptr<AttributeInitializer> attributes, std::shared_ptr<RoomModules> modules) const
{
	if (!roomSettings) {
		roomSettings = shared_ptr<RoomSettings>(new RoomSettings());
	}

	if (!attributes) {
		attributes = shared_ptr<AttributeInitializer>(new AttributeInitializer());
	}

	if (!modules) {
		modules = shared_ptr<RoomModules>(new RoomModules());
	}
	std::vector<ModuleID> moduleIDs = modules->GetIdentifiers();
	for (ModuleID moduleID: moduleIDs) {
		if (!UPCUtils::IsValidModuleName(moduleID)) {
			log.Error(GetName()+" createRoom() failed. Illegal room module name: ["
						  + moduleID + "]. See Utils::IsValidModuleName().");
			return RoomRef();
		}
	}

	if (roomID.compare("")) {
		if (!UPCUtils::IsValidResolvedRoomID(roomID)) {
			log.Error(GetName()+" createRoom() failed. Illegal room id: ["
					+ roomID + "]. See Utils::isValidResolvedRoomID().");
			return RoomRef();
		}
	}

	// Send "" as the roomID if no roomID is specified. When the server
	// receives a request to create a roomID of "", it auto-generates
	// the id, and returns it via RoomManagerEvent.CREATE_ROOM_RESULT.
	RoomRef theRoom;
	if (roomID.compare("")) {
		theRoom = const_cast<RoomManager*>(this)->CreateCached(roomID);
	}

	unionBridge.SendUPC(UPC::ID::CREATE_ROOM,
				 {roomID.c_str(),
				 roomSettings->Serialize().c_str(),
				 attributes->Serialize().c_str(),
				 modules->Serialize().c_str()});

	return theRoom;
}

/**
 * request server to remove a room
 */
void
RoomManager::RemoveRoom(RoomID roomID, std::string password) const{
  if (roomID == "" || !UPCUtils::IsValidResolvedRoomID(roomID)) {
	  log.Error("Invalid room id supplied to removeRoom(): [" + roomID + "]. Request not sent.");
	  return;
  }

  unionBridge.SendUPC(UPC::ID::REMOVE_ROOM, { roomID, password });
};

/**
 * request server to observe a room
 *  @return a shared pointer, empty if not possible to find
 */
RoomRef
RoomManager::ObserveRoom(RoomID roomID, std::string password, UpdateLevels updateLevels) const {
	RoomRef theRoom;
	if (!UPCUtils::IsValidResolvedRoomID(roomID)) {
		log.Error("Invalid room id supplied to observeRoom(): [" + roomID + "]. Request not sent."
					+ " See Utils::IsValidResolvedRoomID().");
		return RoomRef();
	}

	// Try to get a reference to the room
	theRoom = Get(roomID);

	// If the room exists
	if (theRoom) {
		if (theRoom->ClientIsObservingRoom()) {
			log.Warn(GetName()+" Room observe attempt ignored. Already observing room: '" + roomID + "'.");
			return RoomRef();
		}
	} else {
		// Make the local room
		theRoom = const_cast<RoomManager*>(this)->CreateCached(roomID);
	}

	// Validate the password
	if (!UPCUtils::IsValidPassword(password)) {
		log.Error("Invalid room password supplied to observeRoom(). Room ID: [" + roomID + "], password: [" + password + "]. See Utils::IisValidPassword().");
		return RoomRef();
	}

	// If update levels were specified for this room, send them now.
	if (updateLevels != 0) {
		theRoom->SetUpdateLevels(updateLevels);
	}

	unionBridge.SendUPC(UPC::ID::OBSERVE_ROOM, {roomID, password});

	return theRoom;
};

/**
 * request server to watch for rooms according to given qualifier
 */
void
RoomManager::WatchForRooms(const RoomQualifier roomQualifier) const {
	bool recursive = false;

	// null means watch the whole server
	if (roomQualifier == "") {
		recursive = true;
	}

	unionBridge.SendUPC(UPC::ID::WATCH_FOR_ROOMS, { roomQualifier, bool_to_string(recursive) });
};

/**
 * request server to stop watching for rooms according to given qualifier
 */
void
RoomManager::StopWatchingForRooms(const RoomQualifier roomQualifier) const {
	bool recursive = false;
	// "" means watch the whole server
	if (roomQualifier == "") {
		recursive = true;
	}


	unionBridge.SendUPC(UPC::ID::STOP_WATCHING_FOR_ROOMS, {roomQualifier, bool_to_string(recursive)} );
};


/**
 * request server to join a room
 *  @return a shared pointer, empty if not possible to find
 */
RoomRef
RoomManager::JoinRoom(RoomID roomID, std::string password, UpdateLevels updateLevels) const
{
	if (!UPCUtils::IsValidResolvedRoomID(roomID)) {
		log.Error(GetName()+" Invalid room id supplied to joinRoom(): [" + roomID + "]. Join request not sent.");
		return RoomRef();
	}

	RoomRef theRoom = Get(roomID);

	if (theRoom) {
		if (theRoom->ClientIsInRoom()) {
			log.Warn(GetName()+" Room join attempt aborted. Already in room: [" + theRoom->GetRoomID() + "].");
			return theRoom;
		}
	} else {
		theRoom = const_cast<RoomManager*>(this)->CreateCached(roomID);
	}

	if (!UPCUtils::IsValidPassword(password)) {
		log.Error(GetName()+" Invalid room password supplied to joinRoom(): ["+ roomID + "]. Join request not sent.");
		return theRoom;
	}

	if (updateLevels != 0) {
		theRoom->SetUpdateLevels(updateLevels);
	}

	unionBridge.SendUPC(UPC::ID::JOIN_ROOM, {roomID, password});
	return theRoom;
}

/**
 * sends a UPC message to send a message to the given room
 */
void
RoomManager::SendMessage(UPCMessageID messageName, std::vector<RoomID> rooms, StringArgs msg,
		bool includeSelf, IFilterRef filters) const {
	if (std::string(messageName) == "") {
		log.Warn(GetName()+"  sendMessage() failed. No messageName supplied.");
		return;
	}

	std::string roomStr;
	for (auto it=rooms.begin(); it!=rooms.end(); ++it) {
		if (it > rooms.begin()) {
			roomStr.append(Token::RS);
		}
		roomStr.append(*it);
	}

	StringArgs args = {
		messageName,
		roomStr,
		bool_to_string(includeSelf),
		filters ? filters->ToXMLString() : ""};

	for (auto it=msg.begin(); it!=msg.end(); it++) {
		args.push_back(*it);
	}
	unionBridge.SendUPC(UPC::ID::SEND_MESSAGE_TO_ROOMS, args);
}

/////////////////////////////
// Utils, Construction, Info
/////////////////////////////

/**
 * @return true if we are watching this qualifier
 */
bool
RoomManager::IsWatchingQualifier(const RoomQualifier qualifier) const {
	for (auto it=watchedQualifiers.begin(); it!=watchedQualifiers.end(); it++) {
		if (!qualifier.compare(*it)) {
			return true;
		}
	}
	return false;
};

/**
 * @return an exhaustive list of rooms
 */
std::vector<RoomRef>
RoomManager::GetRooms() const {
	std::vector<RoomRef> roomlist;
	occupiedRooms.AppendTo(roomlist);
	observedRooms.AppendTo(roomlist);
	watchedRooms.AppendTo(roomlist);
	AppendCached(roomlist);
	return roomlist;
};

/**
 * @return an exhaustive list of rooms matching the given qualifier
 */
std::vector<RoomRef>
RoomManager::GetRoomsWithQualifier(const RoomQualifier qualifier) const {
	if (qualifier == "")  {
		return GetRooms();
	}

	std::vector<RoomRef> roomlist;
	std::vector<RoomRef> rooms = GetRooms();
	for (auto it=rooms.begin(); it!=rooms.end(); ++it) {
		RoomRef room = *it;
		RoomID rid = room->GetRoomID();
		if (qualifier == UPCUtils::GetRoomQualifier(rid)) {
			roomlist.push_back(room);
		}
	}

	return roomlist;
};

/**
 * @return the number of rooms matching the qualifier
 */
int
RoomManager::GetNumRooms(const RoomQualifier qualifier) const {
	return (int) GetRoomsWithQualifier(qualifier).size();
}

/**
 * buids an exhaustive list from all possible sources of known clients
 */
std::vector<ClientRef>
RoomManager::GetAllClients() const {
	std::vector<ClientRef> clientSet;
	std::vector<RoomRef> rooms = GetRooms();
	for (auto it=rooms.begin(); it!=rooms.end(); ++it) {
		(*it)->GetOccupantList().AppendTo(clientSet);
		(*it)->GetObserverList().AppendTo(clientSet);
	}
	return clientSet;
}

/**
 * finds a particular client locally with an exhaustive search through all possible sources of client info
 */
ClientRef
RoomManager::GetClient(ClientID clientID) const {
	ClientRef theClient;
	std::function<bool(RoomRef)> seeker=
			[this,&theClient,clientID](RoomRef r) {
				theClient = r->GetOccupant(clientID);
				if (theClient) return false;
				theClient = r->GetObserver(clientID);
				if (theClient) return false;
				return true;
			};

	occupiedRooms.ApplyBool(seeker); if (theClient) return theClient;
	observedRooms.ApplyBool(seeker); if (theClient) return theClient;
	watchedRooms.ApplyBool(seeker); if (theClient) return theClient;
	ApplyBoolCached(seeker);
	return theClient;
}

/**
 * @return true if the given client is present in any imaginable local search
 */
bool
RoomManager::ClientIsKnown(const ClientID clientID) const {
	std::vector<RoomRef> rooms = GetRooms();
	for (auto it=rooms.begin(); it!=rooms.end(); ++it) {
		if (it->use_count() > 0) {
			const Room* r=it->get();
			if(r->GetOccupantList().Contains(clientID)) {
				return true;
			}
			if(r->GetObserverList().Contains(clientID)) {
				return true;
			}
		}
	}
	return false;
}



/**
 * override for Manager base find an object ... ideally in the cache, but it will search thoroughly if not
 * initially found
 */
RoomRef
RoomManager::Get(const RoomID roomID) const
{
	RoomRef theRoom = GetCached(roomID); if (theRoom) return theRoom;
	theRoom = occupiedRooms.Get(roomID); if (theRoom) return theRoom;
	theRoom = observedRooms.Get(roomID); if (theRoom) return theRoom;
	theRoom = watchedRooms.Get(roomID); if (theRoom) return theRoom;

	return theRoom;
}

/**
 * override for Manager base to make a basic object
 */
Room *
RoomManager::Make(const RoomID roomID)
{
	return new Room(roomID, *this, clientManager, accountManager, unionBridge, log);
}


/**
 * override for Manager base give the name of this manager
 */
std::string
RoomManager::GetName() const
{
	return "[ROOM MANAGER]";
}

/**
 * override for Manager base give the name of this manager
 */
bool
RoomManager::Valid(RoomID roomID) const
{
	return roomID != "";
}


/////////////////////////////
// Room list management
/////////////////////////////

/**
 * disposes of a room, removing all references to the room in any of our lists
 */
void
RoomManager::Dispose(RoomID roomID) {
	RoomRef room = Get(roomID);
	if (room) {
		log.Debug("[ROOM_MANAGER] Disposing room: " + roomID);
		RemoveCached(roomID);
		RemoveWatchedRoom(roomID);
		RemoveOccupiedRoom(roomID);
		RemoveObservedRoom(roomID);
	} else {
		log.Debug("[ROOM_MANAGER] disposeRoom() called for unknown room: [" + roomID + "]");
	}
}
/**
 * add a room to the watched list, and synchronize it
 */
RoomRef
RoomManager::AddWatchedRoom(const RoomID roomID) {
	log.Debug(GetName()+" adding watched room: [" + roomID + "]");

	RoomRef room = Request(roomID);
	if (room && !watchedRooms.Contains(room)) {
		watchedRooms.Add(room);
		room->UpdateSyncState();
	}
	return room;
}

/**
 * remove a room from our watched list, and update its server synchronization
 */
RoomRef
RoomManager::RemoveWatchedRoom(const RoomID roomID) {
	RoomRef room = watchedRooms.Remove(roomID);
	if (room) {
		room->UpdateSyncState();
	} else {
		log.Debug(GetName()+" Request to remove watched room ["  + roomID + "] ignored; room not in watched list.");
	}
	return RoomRef();
};

/**
 * clear the watch list
 */
void
RoomManager::DisposeWatchedRooms() {
	watchedRooms.RemoveAllApplying([this](RoomRef room) {
		if (room) {
			room->UpdateSyncState();
		}
	});
};

/**
 * synchronize our watch list with the given roomIDs as the new list
 */
void
RoomManager::SetWatchedRooms(const RoomQualifier qualifier, const std::vector<RoomID> newRoomIDs) {
	std::vector<RoomRef> rooms = GetRoomsWithQualifier(qualifier);
	for (auto it=rooms.begin(); it!=rooms.end(); ++it) {
		RoomRef room = *it;
		bool hasInNew = false;
		RoomID sid = room.get()->GetSimpleRoomID();
		for (auto jt=newRoomIDs.begin(); jt!=newRoomIDs.end(); ++jt) {
			if (!sid.compare(*jt)) {
				hasInNew = true;
				break;
			}
		}
		if (!hasInNew) {
			RemoveWatchedRoom(room.get()->GetRoomID());
		}
	}
	for (auto it=newRoomIDs.begin(); it!=newRoomIDs.end(); ++it) {
		RoomID roomID = *it;
		RoomID fullRoomID = qualifier + (qualifier != "" ? "." : "") + roomID;
		if (!watchedRooms.Contains(fullRoomID)) {
			AddWatchedRoom(fullRoomID);
		}
	}
};

/**
 * @return true if we're watching this room
 */
bool
RoomManager::HasWatchedRoom(const RoomID roomID) const {
	return watchedRooms.Contains(roomID);
}

// =============================================================================
// OCCUPIED ROOMS
// =============================================================================

/**
 * adds a room to the occupied list
 */
RoomRef
RoomManager::AddOccupiedRoom(const RoomID roomID) {
	log.Debug(GetName()+" Adding occupied room: [" + roomID + "]");
	RoomRef room = Request(roomID);
	if (room && !occupiedRooms.Contains(room)) {
		occupiedRooms.Add(room);
	}
	room->UpdateSyncState();
	return room;
};

/**
 * remove a room from the occupied list
 */
RoomRef
RoomManager::RemoveOccupiedRoom(const RoomID roomID) {
	RoomRef room = occupiedRooms.Remove(roomID);
	if (room) {
		room->UpdateSyncState();
	} else {
		log.Debug(GetName()+" Request to remove occupied room [" + roomID + "] ignored; room is not in occupied list.");
	}
	return room;
};

/**
 * @return true if this is in our occupy list
 */
bool
RoomManager::HasOccupiedRoom(const RoomID roomID) const {
	return occupiedRooms.Contains(roomID);
};

// =============================================================================
// OBSERVED ROOMS
// =============================================================================

/**
 * adds the room to our observed list
 */
RoomRef
RoomManager::AddObservedRoom(const RoomID roomID) {
  log.Debug(GetName()+" Adding observed room: [" + roomID + "]");
  RoomRef room = Request(roomID);
  if (room && !observedRooms.Contains(room)) {
	  observedRooms.Add(room);
  }
  room->UpdateSyncState();
  return room;
}

/**
 * removes a room from our observe list
 */
RoomRef
RoomManager::RemoveObservedRoom(const RoomID roomID) {
	RoomRef room = observedRooms.Remove(roomID);
	if (room) {
		room->UpdateSyncState();
	} else {
		log.Debug(GetName()+" Request to remove observed room [" + roomID + "] ignored; client is not observing room.");
	}
	return room;
}

/**
 * @return true if we have this room in our observe list
 */
bool
RoomManager::HasObservedRoom(const RoomID roomID) const {
	return observedRooms.Contains(roomID);
}



////////////////////////////////////
// notify and event hooks
///////////////////////////////////

/**
 * called back when we get a watch result from server
 */
void
RoomManager::OnWatchForRoomsResult(RoomQualifier roomIDQualifier, UPCStatus status) {
	NXRoomInfo::NotifyListeners( Event::WATCH_FOR_ROOMS_RESULT, roomIDQualifier, RoomID(), RoomRef(), status);
};

/**
 * called back when we get a stop watching result from server
 */
void
RoomManager::OnStopWatchingForRoomsResult(RoomQualifier roomIDQualifier, UPCStatus status) {
	NXRoomInfo::NotifyListeners( Event::STOP_WATCHING_FOR_ROOMS_RESULT, roomIDQualifier, RoomID(), RoomRef(), status);
};

/**
 * called back when we get a create result from server
 */
void
RoomManager::OnCreateRoomResult(RoomQualifier roomIDQualifier, RoomID roomID, UPCStatus status) {
	if (status == UPC::Status::PERMISSION_DENIED) {
		// todo clean up this room
	}
	NXRoomInfo::NotifyListeners( Event::CREATE_ROOM_RESULT, roomIDQualifier, roomID, RoomRef(), status);
};

/**
 * called back by a remove result from server
 */
void
RoomManager::OnRemoveRoomResult(RoomQualifier roomIDQualifier, RoomID roomID, UPCStatus status) {
	NXRoomInfo::NotifyListeners( Event::REMOVE_ROOM_RESULT, roomIDQualifier, roomID, RoomRef(), status);
}

/**
 * called back when a room is attempted to be added
 */
void
RoomManager::OnRoomAdded(RoomQualifier roomIDQualifier, RoomID roomID, RoomRef theRoom) {
	NXRoomInfo::NotifyListeners( Event::ROOM_ADDED, roomIDQualifier, roomID, theRoom, UPC::Status::SUCCESS);
}

/**
 * called back when a room is attempted to be removed
 */
void
RoomManager::OnRoomRemoved(RoomQualifier roomIDQualifier, RoomID roomID, RoomRef theRoom) {
	NXRoomInfo::NotifyListeners(Event::ROOM_REMOVED, roomIDQualifier, roomID, theRoom, UPC::Status::SUCCESS);
}

/**
 * called when we get a changed on room count
 */
void
RoomManager::OnRoomCount(int numRooms) {
	NXInt::NotifyListeners(Event::ROOM_COUNT, numRooms);
};

/**
 * called back when we get a server result from a join
 */
void
RoomManager::OnJoinRoomResult(RoomID roomID, UPCStatus status) {
	RoomRef r;
	if (status == UPC::Status::SUCCESS) {
		r = Get(roomID);
	}
	NXRoomInfo::NotifyListeners(Event::JOIN_RESULT, RoomQualifier(), roomID, r, status);
}

/**
 * called back when we get a server result from a leave request
 */
void
RoomManager::OnLeaveRoomResult(RoomID roomID, UPCStatus status) {
	RoomRef r;
	if (status == UPC::Status::SUCCESS) {
		r = Get(roomID);
	}
	NXRoomInfo::NotifyListeners(Event::LEAVE_RESULT, RoomQualifier(), roomID, r, status);
}

/**
 * called back when we get a server result from an observation request
 */
void
RoomManager::OnObserveRoomResult(RoomID roomID, UPCStatus status) {
	RoomRef r;
	if (status == UPC::Status::SUCCESS) {
		r = Get(roomID);
	}
	NXRoomInfo::NotifyListeners(Event::OBSERVE_RESULT, RoomQualifier(), roomID, r, status);
}

/**
 * called back when we get a server result from a stop observing request
 */
void
RoomManager::OnStopObservingRoomResult(RoomID roomID, UPCStatus status) {
	RoomRef r;
	if (status == UPC::Status::SUCCESS) {
		r = Get(roomID);
	}
	NXRoomInfo::NotifyListeners(Event::STOP_OBSERVING_RESULT, RoomQualifier(), roomID, r, status);
}
