/*
 * ClientManager.cpp
 *
 *  Created on: Apr 8, 2014
 *      Author: dak
 */

#include "UCUpperHeaders.h"

/**
 * @class ClientManager ClientManager.h
 * @brief manager for client data
 * manages the cache of client info we have from the server and store in room objects, and also the lists of observed, banned, watched or otherwise scrutinized clients. it's the
 * main point of call for doing UPC commands on clients
 */
ClientManager::ClientManager(RoomManager& roomManager, AccountManager& accountManager, UnionBridge& unionBridge, ILogger& log)
	: Manager(ClientRefId, ValidClientRef, log)
	, watchedClients(ClientRefId, ValidClientRef)
	, observedClients(ClientRefId, ValidClientRef)
	, roomManager(roomManager)
	, accountManager(accountManager)
	, unionBridge(unionBridge) {
	DEBUG_OUT("ClientManager::ClientManager()");
}

ClientManager::~ClientManager() {
	DEBUG_OUT("ClientManager::~ClientManager()");
}

/**
 * @return a reference to the self client of the library itself
 */
ClientRef
ClientManager::Self() const {
	return selfReference;
}

/**
 * sets the self client
 */
void
ClientManager::SetSelf(const ClientRef client) const {
	selfReference = client;
	if (client) {
		client->SetIsSelf();
	}
}

/**
 * gets the connection state of the self client
 */
int
ClientManager::GetSelfConnectionState()
{
	if (selfReference) {
		AccountRef r=SelfAccount();
		if (r) return r->GetConnectionState();
	}
	return ConnectionState::UNKNOWN;
}

/**
 * gets the account of the self client
 */
AccountRef
ClientManager::SelfAccount() const
{
	if (selfReference) {
		return selfReference->GetAccount();
	}
	return AccountRef();
}

/**
 * logs off the self client
 */
void
ClientManager::SelfLogoff()
{
	if (selfReference) {
		AccountRef r=SelfAccount();
		if (r) r->Logoff();
	}
}

/**
 * Gets a Map<ClientID,ClientRef> of all available clients from every possible source
 *
 *  hopefully we don't need to do this too often and ideally, everyone is in the cach. note that this method is thorough
 *  and looks in places other than the cache ... originally (js Orbiter) GetInternalClients and for some reason the only
 *  place it didn't look was its own cache ... marking as FIXME because maybe they had a good reason for doing something
 *  that to me looks like a bug
 */
const Map<ClientID, ClientRef>
ClientManager::GetClients() const {
	Map<ClientID, ClientRef>  clients(ClientRefId, ValidClientRef);
	AppendCached(clients);
	clients.Append(roomManager.GetAllClients());
	clients.Append(accountManager.GetClientsForObservedAccounts());
	clients.Append(observedClients.GetAll());
	clients.Append(watchedClients.GetAll());
	if (selfReference) {
		clients.Add(selfReference);
	}
	return clients;
};


/**
 * override for Manager base find an object ... ideally in the cache, but it will search thoroughly if not
 * initially found. This is filling the function of GetInternal... of the original js union client
 */
ClientRef
ClientManager::Get(const ClientID clientID) const
{
	if (!Valid(clientID)) {
		return ClientRef();
	}
	ClientRef theClient = GetCached(clientID); if (theClient) return theClient;

	theClient = roomManager.GetClient(clientID);
	if (theClient) {
		AddCached(theClient);
		return theClient;
	}

	theClient = accountManager.GetObservedAccountsClient(clientID);
	if (theClient) {
//		AddCached(theClient);
		return theClient;
	}

	theClient = observedClients.Get(clientID);
	if (theClient) {
//		AddCached(theClient);
		return theClient;
	}

	theClient = watchedClients.Get(clientID);
	if (theClient) {
// TODO this is a good idea but breaks out const
//		AddCached(theClient);
		return theClient;
	}

	return ClientRef();
}

/**
 * override for Manager base to make a basic object
 * if we need to override client, hook into alternate factories here ....
 */
Client *
ClientManager::Make(const ClientID clientID)
{
	log.Debug("making a new client\n");
	Client *c = new Client(clientID, roomManager, *this, accountManager, unionBridge, log);
	return c;
}


/**
 * override for Manager base give the name of this manager
 */
std::string
ClientManager::GetName() const
{
	return "[CLIENT MANAGER]";
}

/**
 * override for Manager base give the name of this manager
 */
bool
ClientManager::Valid(const ClientID clientID) const
{
	return clientID != "";
}

/**
 * generate a UPC message to the given clients
 */
void
ClientManager::SendMessage(const UPCMessageID messageName, const StringArgs msg,
		const std::vector<ClientID> clientIDs, const IFilterRef filters) const {
	if (std::string(messageName) == "") {
		log.Warn(GetName()+"  sendMessage() failed. No messageName supplied.");
		return;
	}

	std::string clientStr;
	for (auto it=clientIDs.begin(); it!=clientIDs.end(); ++it) {
		if (it > clientIDs.begin()) {
			clientStr.append(Token::RS);
		}
		clientStr.append(*it);
	}

	StringArgs args = {
		messageName,
		clientStr,
		filters ? filters->ToXMLString() : ""};

	for (auto it=msg.begin(); it!=msg.end(); it++) {
		args.push_back(*it);
	}
	unionBridge.SendUPC(UPC::ID::SEND_MESSAGE_TO_CLIENTS, args);
}


/**
 * generate a UPC message to start watching for clients
 */
void
ClientManager::WatchForClients() const{
	unionBridge.SendUPC(UPC::ID::WATCH_FOR_CLIENTS, {});
}

/**
 * generate a UPC message to stop watching for clients
 */
void
ClientManager::StopWatchingForClients() const {
	unionBridge.SendUPC(UPC::ID::STOP_WATCHING_FOR_CLIENTS, {});
}

/**
 * @return true if is watching clients
 */
bool
ClientManager::IsWatchingForClients() const {
	return isWatchingForClients;
}

/**
 * @return true if the given client is in the watched list
 */
bool
ClientManager::HasWatchedClient(const ClientID clientID) const {
	return watchedClients.Contains(clientID);
}

/**
 * @true set the watching-for-clients state
 */
void
ClientManager::SetIsWatchingForClients(const bool value) {
	isWatchingForClients = value;
}

/**
 * adds a watched client
 */
void
ClientManager::AddWatchedClient(const ClientRef client) {
 	 watchedClients.Add(client);
 	 OnClientConnected(client);
};

/**
 * removes a watched client
 */
void
ClientManager::RemoveWatchedClient(const ClientID clientID) {
	watchedClients.Remove(clientID);
};

/**
 * removes all watched clients
 */
void
ClientManager::RemoveAllWatchedClients() {
	watchedClients.RemoveAll();
};

/**
 * deserialize the given watched list
 */
void
ClientManager::DeserializeWatchedClients(const std::string ids) {
	std::unordered_set<ClientID> idList;
	std::stringstream idss(ids);
	std::string item;
	char c = *Token::RS;
	while (std::getline(idss, item, c)) {
		idList.insert(item);
	}
	std::vector<ClientID> localClients = watchedClients.GetKeys();// copy assign
	ClientRef theClient;
	ClientID clientID;
	UserID accountID;

	// Client list received, so set isWatchingForClients now, otherwise, code
	// with side-effects may take action against the clients being added
	SetIsWatchingForClients(true);
	std::unordered_map<ClientID,UserID> idHash;
	// Remove all local clients that are not in the new list from the server
	for (auto it=localClients.begin(); it!= localClients.end(); ++it) {
		if (idList.find(*it) == idList.end()) {
			watchedClients.Remove(*it);
		}
	}
	// Add all new clients that are not in the local set
	for (auto jt=idList.begin(); jt!=idList.end(); jt++) {
		if (jt->compare("")) {
			if (!watchedClients.Contains(*jt)) {
				theClient = Request(*jt);
				jt++; // should be account id
				UserID accountID = *jt;
				if (accountID != "") {
					theClient->SetAccount(accountManager.Request(accountID));
				}
				AddWatchedClient(theClient);
			}
		} else {
			log.Error("[CLIENT_MANAGER] Received empty client id in client list (u101).");
			return;
		}
	}

	OnSynchronize();
};

/**
 * sends upc to observe a particulatr client
 */
void
ClientManager::ObserveClient(const ClientID clientID) const {
	unionBridge.SendUPC(UPC::ID::OBSERVE_CLIENT, {clientID});
};

/**
 * @return true if we are observing this client
 */
bool
ClientManager::IsObservingClient(const ClientID clientID) const {
	return observedClients.Contains(clientID);
}

/**
 * add this observed client to the list
 */
void
ClientManager::AddObservedClient(const ClientRef client) {
	observedClients.Add(client);
	OnObserveClient(client);
};

/**
 * remove this observed client from the list
 */
void
ClientManager::RemoveObservedClient(const ClientID clientID) {
	ClientRef client = observedClients.Remove(clientID);
	if (client) {
		OnStopObservingClient(client);
	}
};

/**
 * remove everyone from the observed list
 */
void
ClientManager::RemoveAllObservedClients() {
	observedClients.RemoveAll();
};


std::unordered_map<ClientID, AttrVal>
ClientManager::GetAttributeForClients(const std::vector<ClientID>clientIDs, const AttrName attrName, const AttrScope attrScope) const {
	std::unordered_map<ClientID, AttrVal> vals;
	for (auto it=clientIDs.begin(); it!=clientIDs.end(); ++it) {
		ClientRef thisClient = Get(*it);
		if (thisClient) {
			vals[*it] = thisClient->GetAttribute(attrName, attrScope);
		} else {
			log.Debug("[CLIENT_MANAGER] Attribute retrieval failed during GetAttributeForClients(). Unknown client ID [" + *it + "]");
		}
	}
	return vals;
};

/**
 * send UPC to ban a particular address
 */
void
ClientManager::Ban(const Address address, const int duration, const std::string reason) const {
	unionBridge.SendUPC(UPC::ID::BAN, {address, "", std_to_string(duration), reason});
};

/**
 * send UPC to unban a particular address
 */
void
ClientManager::Unban(const Address address) const {
	unionBridge.SendUPC(UPC::ID::UNBAN, {address});
};

/**
 * send UPC to kick a particular address
 */
void
ClientManager::KickClient(const ClientID clientID) const{
	if (clientID == "") {
		log.Warn(GetName()+" Kick attempt failed. No clientID supplied.");
	}
	unionBridge.SendUPC(UPC::ID::KICK_CLIENT, {clientID});
}

/**
 * send UPC to watch for any banned addresses
 */
void
ClientManager::WatchForBannedAddresses() const {
	unionBridge.SendUPC(UPC::ID::WATCH_FOR_BANNED_ADDRESSES, {});
};

/**
 * send UPC to stop watching for any banned addresses
 */
void
ClientManager::StopWatchingForBannedAddresses() const {
	unionBridge.SendUPC(UPC::ID::STOP_WATCHING_FOR_BANNED_ADDRESSES, {});
};

/**
 * syncrhonises the banned list
 */
void
ClientManager::SetWatchedBannedAddresses(const Set<Address> bannedList) const {
	bannedAddresses = bannedList;
	OnSynchronizeBanlist();
}

/**
 * add to the banned list
 */
void
ClientManager::AddWatchedBannedAddress(Address address) const {
	bannedAddresses.Add(address);
	OnAddressBanned(address);
};

/**
 * remove from banned list
 */
void
ClientManager::RemoveWatchedBannedAddress(const Address address) {
	if (bannedAddresses.Contains(address)) {
		bannedAddresses.Remove(address);
		OnAddressUnbanned(address);
	}
}

/**
 * setter for banned address watching
 */
void
ClientManager::SetIsWatchingForBannedAddresses(const bool value) const {
	isWatchingForBannedAddresses = value;
};

/**
 * @return true if watching for banned ...
 */
bool
ClientManager::IsWatchingForBannedAddresses() const {
	return isWatchingForBannedAddresses;
};

/**
 * @return Set<Address> of banned addresses
 */
const Set<Address>&
ClientManager::GetBannedAddresses() const {
	return bannedAddresses;
};

/**
 * @return total number of clients known
 */
int
ClientManager::GetLifetimeNumClientsKnown() const{
  // -1 for each 'ready' state the connection has achieved because we don't
  // count the current client ("self")
	return lifetimeClientsRequested-unionBridge.GetReadyCount();
};

/**
 * @return the cached size of the client lists
 */
int
ClientManager::GetNumClients() const{
	return (int) GetClients().Length(); // ok thiss is brutal but it's the easiest way to catch all and avoid doubles FIXME
};

/**
 * @return the cached size of the client lists
 */
int
ClientManager::GetNumClientsOnServer() const {
	return (int) watchedClients.Length();
}


////////////////////////////////////
// notify and event hooks
///////////////////////////////////

/**
 * called back on a client observation
 */
void
ClientManager::OnObserveClient(ClientRef client) const{
	const_cast<ClientManager*>(this)->NXClientInfo::NotifyListeners(Event::OBSERVE, ClientRefId(client), client, 0, RoomID(), RoomRef(), UPC::Status::SUCCESS);
}

/**
 * called back when observation ends
 */
void
ClientManager::OnStopObservingClient(ClientRef client) const {
	const_cast<ClientManager*>(this)->NXClientInfo::NotifyListeners(Event::STOP_OBSERVING, ClientRefId(client), client, 0, RoomID(), RoomRef(), UPC::Status::SUCCESS);
}

/**
 * called back when the client connects
 */
void
ClientManager::OnClientConnected(ClientRef client) const {
	const_cast<ClientManager*>(this)->NXClientInfo::NotifyListeners(Event::CLIENT_CONNECTED, ClientRefId(client), client, 0, RoomID(), RoomRef(), UPC::Status::SUCCESS);
}

/**
 * called back when the client disconnects
 */
void
ClientManager::OnClientDisconnected(ClientRef client) const {
	const_cast<ClientManager*>(this)->NXClientInfo::NotifyListeners(Event::CLIENT_DISCONNECTED, ClientRefId(client), client, 0, RoomID(), RoomRef(), UPC::Status::SUCCESS);
}

/**
 * called back when there is a successful observation termination
 */
void
ClientManager::OnStopWatchingForClientsResult(UPCStatus status) const {
	const_cast<ClientManager*>(this)->NXClientInfo::NotifyListeners(Event::STOP_WATCHING_FOR_CLIENTS_RESULT, ClientID(), ClientRef(), 0, RoomID(), RoomRef(), status);
}

/**
 * called back when there is a successful watch initiated
 */
void
ClientManager::OnWatchForClientsResult(UPCStatus status) const {
	const_cast<ClientManager*>(this)->NXClientInfo::NotifyListeners(Event::WATCH_FOR_CLIENTS_RESULT, ClientID(), ClientRef(), 0, RoomID(), RoomRef(), status);
}

/**
 * called back on an observe result
 */
void
ClientManager::OnObserveClientResult(ClientID clientID, UPCStatus status) const {
	const_cast<ClientManager*>(this)->NXClientInfo::NotifyListeners(Event::OBSERVE_RESULT, clientID, ClientRef(), 0, RoomID(), RoomRef(), status);
}

/**
 * called back when there is a result from the cessation of observation
 */
void
ClientManager::OnStopObservingClientResult(ClientID clientID, UPCStatus status) const {
	const_cast<ClientManager*>(this)->NXClientInfo::NotifyListeners(Event::STOP_OBSERVING_RESULT, clientID, ClientRef(), 0, RoomID(), RoomRef(), status);
}

/**
 * called back when the server has processed our kick request
 */
void
ClientManager::OnKickClientResult(ClientID clientID, UPCStatus status) const {
	const_cast<ClientManager*>(this)->NXClientInfo::NotifyListeners(Event::KICK_RESULT, clientID, ClientRef(), 0, RoomID(), RoomRef(), status);
}

/**
 * called back when the server has processed our ban request
 */
void
ClientManager::OnBanClientResult(Address address, ClientID clientID, UPCStatus status) const {
	const_cast<ClientManager*>(this)->NXAddressInfo::NotifyListeners(Event::BAN_RESULT, clientID, ClientRef(), address, status);
}

/**
 * called back when the server has processed our unban request
 */
void
ClientManager::OnUnbanClientResult(Address address, UPCStatus status) const {
	const_cast<ClientManager*>(this)->NXAddressInfo::NotifyListeners(Event::UNBAN_RESULT, ClientID(), ClientRef(), address, status);
}

/**
 * called back when the srver acknowledges our watch on banned addresses
 */
void
ClientManager::OnWatchForBannedAddressesResult(UPCStatus status) const{
	const_cast<ClientManager*>(this)->NXAddressInfo::NotifyListeners(Event::WATCH_FOR_BANNED_ADDRESSES_RESULT, ClientID(), ClientRef(), Address(), status);
}

/**
 * called back when the server acknowledges stopping a watch on banned addresses
 */
void
ClientManager::OnStopWatchingForBannedAddressesResult(UPCStatus status) const{
	const_cast<ClientManager*>(this)->NXAddressInfo::NotifyListeners(Event::STOP_WATCHING_FOR_BANNED_ADDRESSES_RESULT, ClientID(), ClientRef(), Address(), status);
}

/**
 * called back when the server has acknowledged our ban on an address
 */
void
ClientManager::OnAddressBanned(Address address) const {
	const_cast<ClientManager*>(this)->NXAddressInfo::NotifyListeners(Event::ADDRESS_BANNED, ClientID(), ClientRef(), address, UPC::Status::SUCCESS);
}

/**
 * called back when the server has acknowledged our unban on an address
 */
void
ClientManager::OnAddressUnbanned(Address address) const {
	const_cast<ClientManager*>(this)->NXAddressInfo::NotifyListeners(Event::ADDRESS_UNBANNED, ClientID(), ClientRef(), address, UPC::Status::SUCCESS);
}

/**
 * called back when the banlist is synchronized
 */
void
ClientManager::OnSynchronizeBanlist() const {
	const_cast<ClientManager*>(this)->NXStatus::NotifyListeners(Event::SYNCHRONIZE_BANLIST, UPC::Status::SUCCESS);
}

/**
 * called back when we havve processed a synchronize
 */
void
ClientManager::OnSynchronize() const {
	const_cast<ClientManager*>(this)->NXStatus::NotifyListeners(Event::SYNCHRONIZED, UPC::Status::SUCCESS);
}

