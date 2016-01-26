/*
 * AccountManager.cpp
 *
 *  Created on: Apr 9, 2014
 *      Author: dak
 */

#include "UCUpperHeaders.h"


AccountManager::AccountManager(RoomManager& roomManager, ClientManager& clientManager, UnionBridge& unionBridge, ILogger& log)
	: Manager(AcctRefId, ValidAcctRef, log)
	, watchedAccounts(AcctRefId, ValidAcctRef)
	, observedAccounts(AcctRefId, ValidAcctRef)
	, roomManager(roomManager)
	, clientManager(clientManager)
	, unionBridge(unionBridge) {
	DEBUG_OUT("AccountManager::AccountManager();");
}

AccountManager::~AccountManager() {
	DEBUG_OUT("AccountManager::~AccountManager();");
}



/**
 * override for Manager base find an object ... ideally in the cache, but it will search thoroughly if not
 * initially found. This is filling the function of GetInternal... of the original js union client
 */
AccountRef
AccountManager::Get(const UserID userID) const
{
	if (!Valid(userID)) {
		return AccountRef();
	}
	AccountRef theUser = GetCached(userID);
	if (theUser) return theUser;

	theUser = observedAccounts.Get(userID);
	if (theUser) return theUser;
	theUser = watchedAccounts.Get(userID);
	if (theUser) return theUser;

	/* TODO
	// Look in connected accounts
	roomManager.GetAllClients,
	clientManager..observedClients.GetAll,
	clientManager.watchedClients.GetAll;

	for (var clientID in clients) {
		account = clients[clientID].GetAccount();
		if (account != null && account.GetUserID() == userID) {
			return account;
		}
	}*/
	return AccountRef();
}

AccountRef
AccountManager::GetAccount(const UserID userID) const
{
	return GetCached(userID);
}

/**
 * override for Manager base to make a basic object
 */
UserAccount*
AccountManager::Make(const UserID userID)
{
	/*
		      account = new net.user1.orbiter.UserAccount(userID, log, this, clientManager, roomManager);
		      account.setAttributeManager(new net.user1.orbiter.AttributeManager(account, unionBridge, log));
		      accountCache.put(userID, account);
		  }
	*/
	return new UserAccount(userID, roomManager, clientManager, *this, unionBridge, log);
}


/**
 * override for Manager base give the name of this manager
 */
std::string
AccountManager::GetName() const
{
	return "[ACCOUNT MANAGER]";
}

/**
 * override for Manager base give the name of this manager
 */
bool
AccountManager::Valid(const UserID userID) const
{
	return userID != "";
}


std::vector<ClientRef>
AccountManager::GetClientsForObservedAccounts() const
{
	std::vector<ClientRef> clients;
	observedAccounts.ApplyToAll([this,&clients](AccountRef r) {
		ClientRef client = r->GetInternalClient();
		if (client) {
			clients.push_back(client);
		}
	});
	return clients;
}


ClientRef
AccountManager::GetObservedAccountsClient(const ClientID clientID) const
{
	ClientRef client;
	observedAccounts.ApplyBool([this,clientID,&client](AccountRef r)->bool {
		ClientRef c = r->GetInternalClient();
		if (c && c->GetClientID() == clientID) {
			client = c;
			return false;
		}
		return true;
	});
	return client;
}


/**
 * using a map here to eliminate duplicates, and also give us useful Apply functionality
 */
Map<UserID, AccountRef>
AccountManager::GetAccounts() const
{
	Map<UserID, AccountRef> connectedAccounts(AcctRefId, ValidAcctRef);
	Map<ClientID,ClientRef> clients = clientManager.GetClients();
	clients.ApplyToAll([this,&connectedAccounts](ClientRef c) {
		AccountRef a=c->GetAccount();
		if (a) connectedAccounts.Add(a);
	});
	observedAccounts.AppendTo(connectedAccounts);
	watchedAccounts.AppendTo(connectedAccounts);
	return connectedAccounts;
}

void
AccountManager::ApplyToAccounts(std::function<void(AccountRef)> f) const
{
	Map<ClientID,ClientRef> clients = clientManager.GetClients();
	clients.ApplyToAll([this,f](ClientRef c) {
		AccountRef a=c->GetAccount();
		if (a) f(a);
	});
	observedAccounts.ApplyToAll(f);
	watchedAccounts.ApplyToAll(f);
}


bool
AccountManager::BoolToAccounts(std::function<bool(AccountRef)> f) const
{
	Map<ClientID,ClientRef> clients = clientManager.GetClients();
	return clients.ApplyBool([f](ClientRef c)->bool {
			AccountRef a=c->GetAccount();
			if (a) if (!f(a)) return false;
			return true;
		})
			&& observedAccounts.ApplyBool(f)
			&& watchedAccounts.ApplyBool(f);
}

bool
AccountManager::AccountIsKnown(const UserID userID) const
{
	bool found=false;
	BoolToAccounts([userID,&found](AccountRef c)->bool {
		if (c->GetUserID() == userID) {
			found = true;
			return false;
		}
		return true;
	});
	return found;
}

int
AccountManager::GetNumAccounts() const
{
// messy to form the whole vector and pass around, but most reliable way to avoid duplicates
// applying lambdas will miss the duplicates
	return (int) GetAccounts().size();
}

int
AccountManager::GetNumAccountsOnServer() const
{
	return (int) watchedAccounts.Length();
}

int
AccountManager::GetNumLoggedInAccounts() const
{
	Map<UserID, AccountRef> accounts = GetAccounts(); // do it messily to avoid duplicates
	int count=0;
	accounts.ApplyToAll([&count] (AccountRef c){
		if (c->IsLoggedIn()) count++;
	});
	return count;
}


bool
AccountManager::HasWatchedAccount(const UserID userID) const
{
	return watchedAccounts.Contains(userID);
}

bool
AccountManager::IsObservingAccount(const UserID userID) const
{
	return observedAccounts.Contains(userID);
}

bool
AccountManager::IsWatchingForAccounts() const
{
	return isWatchingForAccounts;
}
/**
 * @private
 */
void
AccountManager::SetIsWatchingForAccounts(bool value) {
	isWatchingForAccounts = value;
}

///////////////////////////
// server actions
///////////////////////////
void
AccountManager::ChangePassword(const UserID userID, const std::string newPassword, const std::string oldPassword)
{
	if (userID == "") {
		log.Warn("[ACCOUNT_MANAGER] Change password failed. No userID supplied.");
	} else if (newPassword == "") {
		log.Warn("[ACCOUNT_MANAGER] Change password failed for account ["
			+ userID + "]. No new password supplied.");
	} else {
		if (oldPassword == "") {
			log.Warn("[ACCOUNT_MANAGER] Change account password for account ["
				+ userID + "]: no old password supplied."
				+ " Operation will fail unless sender is an administrator.");
		}
		unionBridge.SendUPC(UPC::ID::CHANGE_ACCOUNT_PASSWORD, {userID, oldPassword, newPassword});
	}
}

void
AccountManager::CreateAccount(const UserID userID, const std::string password)
{
	if (userID == "") {
		log.Warn("[ACCOUNT_MANAGER] Create account failed. No userID supplied.");
	} else if (password == "") {
		log.Warn("[ACCOUNT_MANAGER] Create account failed. No password supplied.");
	} else {
		unionBridge.SendUPC(UPC::ID::CREATE_ACCOUNT, {userID, password});
	}
}


void
AccountManager::Login(const UserID userID, const std::string password)
{
	if (clientManager.GetSelfConnectionState() == ConnectionState::LOGGED_IN) {
		log.Warn("[ACCOUNT_MANAGER] User [" + userID + "]: Login attempt"
				+ " ignored. Already logged in. Current client must logoff before"
				+ " logging in again.");
		OnLoginResult(userID, UPC::Status::UPC_ERROR);
	} else if (userID == "") {
		log.Warn("[ACCOUNT_MANAGER] Login attempt  failed. No userID supplied.");
	} else if (password == "") {
		log.Warn("[ACCOUNT_MANAGER] Login attempt failed for user  [" + userID + "] failed. No password supplied.");
	} else {
		unionBridge.SendUPC(UPC::ID::LOGIN, {userID, password});
	}
}

void
AccountManager::Logoff(const UserID userID, const std::string password)
{
	if (userID == "") {
		// Current client
		if (clientManager.GetSelfConnectionState() != ConnectionState::LOGGED_IN) {
			log.Warn("[ACCOUNT_MANAGER] Logoff failed. The current user is not logged in.");
		} else {
			clientManager.SelfLogoff();
		}
	} else if (userID == "") {
		// Invalid client
		log.Warn("[ACCOUNT_MANAGER] Logoff failed. Supplied userID must not be the empty string.");
	} else {
		// UserID supplied
		if (password == "") {
			if (clientManager.GetSelfConnectionState() != ConnectionState::LOGGED_IN) {
				log.Warn("[ACCOUNT_MANAGER] Logoff: no password supplied. Operation will fail unless sender is an administrator.");
			}
		}
		unionBridge.SendUPC(UPC::ID::LOGOFF, {userID, password});
	}
}

void
AccountManager::WatchForAccounts() const
{
	 unionBridge.SendUPC(UPC::ID::WATCH_FOR_ACCOUNTS, {});
}

void
AccountManager::StopWatchingForAccounts() const
{
	  unionBridge.SendUPC(UPC::ID::STOP_WATCHING_FOR_ACCOUNTS_RESULT, {});
}

AccountRef
AccountManager::SelfAccount() const
{
	  return clientManager.SelfAccount();
}


void
AccountManager::AddRole(const UserID userID, const std::string role)
{
	if (userID == "") {
		log.Warn("[ACCOUNT_MANAGER] Add role failed. No userID supplied.");
	} else if (role == "") {
		log.Warn("[ACCOUNT_MANAGER] Add role failed for account [" + userID + "]. No role supplied.");
	} else {
		unionBridge.SendUPC(UPC::ID::ADD_ROLE, {userID, role});
	}
}
void
AccountManager::RemoveRole(const UserID userID, const std::string role)
{
	if (userID == "") {
		log.Warn("[ACCOUNT_MANAGER] Remove role failed. No userID supplied.");
	} else if (role == "") {
		log.Warn("[ACCOUNT_MANAGER] Remove role failed for account [" + userID + "]. No role supplied.");
	} else {
		unionBridge.SendUPC(UPC::ID::REMOVE_ROLE, {userID, role});
	}
}


void
AccountManager::RemoveAccount(const UserID userID, const std::string password)
{
	if (userID == "") {
		log.Warn("[ACCOUNT_MANAGER] Remove account failed. No userID supplied.");
	} else {
		if (password == "") {
			log.Warn("[ACCOUNT_MANAGER] Remove account: no password supplied.  Removal will fail unless sender is an administrator.");
		}
		unionBridge.SendUPC(UPC::ID::REMOVE_ACCOUNT, {userID, password});
	}
}

void
AccountManager::StopObservingAccount(UserID userID) {
	unionBridge.SendUPC(UPC::ID::STOP_OBSERVING_ACCOUNT, {userID});
}
/**
 * @private
 */
void
AccountManager::DeserializeWatchedAccounts(std::string ids) {
	std::unordered_set<ClientID> idList;
	std::stringstream idss(ids);
	std::string item;
	char c = *Token::RS;
	while (std::getline(idss, item, c)) {
		idList.insert(item);
	}

	std::vector<UserID> localAccounts = watchedAccounts.GetKeys();// copy assign

	for (auto it=localAccounts.begin(); it!= localAccounts.end(); ++it) {
		if (idList.find(*it) == idList.end()) {
			watchedAccounts.Remove(*it);
		}
	}
	for (auto jt=idList.begin(); jt!=idList.end(); jt++) {
		if (*jt != "") {
			if (!watchedAccounts.Contains(*jt)) {
				AddWatchedAccount(Request(*jt));
			}
		} else {
			log.Error(GetName()+" Received empty user id in user list (u101).");
			return;
		}
	}

	OnSynchronize();
};


/**
 * @private
 */
void
AccountManager::AddObservedAccount(AccountRef account) {
	observedAccounts.Add(account);
	OnObserveAccount(account->GetUserID());
}

/**
 * @private
 */
AccountRef
AccountManager::RemoveObservedAccount(UserID userID) {
	OnStopObservingAccount(userID);
	return observedAccounts.Remove(userID);
}

/**
 * @private
 */
void
AccountManager::RemoveAllObservedAccounts() {
	observedAccounts.RemoveAll();
}


/**
 * @private
 */
void
AccountManager::AddWatchedAccount(AccountRef account) {
	watchedAccounts.Add(account);
	OnAccountAdded(account->GetUserID(), account);
}

/**
 * @private
 */
AccountRef
AccountManager::RemoveWatchedAccount(UserID userID) {
	return watchedAccounts.Remove(userID);
}

/**
 * @private
 */
void
AccountManager::RemoveAllWatchedAccounts() {
	watchedAccounts.RemoveAll();
}

////////////////////////////////////
// notify and event hooks
///////////////////////////////////

/**
 * @private
 */
void
AccountManager::OnCreateAccountResult(UserID userID, UPCStatus status) {
	NXAcctInfo::NotifyListeners(Event::CREATE_ACCOUNT_RESULT, userID, ClientID(), Role(), GetAccount(userID), status);
};

/**
 * @private
 */
void
AccountManager::OnRemoveAccountResult(UserID userID, UPCStatus status) {
	NXAcctInfo::NotifyListeners(Event::REMOVE_ACCOUNT_RESULT, userID, ClientID(), Role(), GetAccount(userID), status);
};

/**
 * @private
 */
void
AccountManager::OnChangePasswordResult(UserID userID, UPCStatus status) {
	NXAcctInfo::NotifyListeners(Event::CHANGE_PASSWORD_RESULT, userID, ClientID(), Role(), GetAccount(userID), status);
};

/**
 * @private
 */
void
AccountManager::OnAccountAdded(UserID userID, AccountRef account) {
	NXAcctInfo::NotifyListeners(Event::ACCOUNT_ADDED, userID, ClientID(), Role(), account, UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
AccountManager::OnAccountRemoved(UserID userID, AccountRef account) {
	NXAcctInfo::NotifyListeners(Event::ACCOUNT_REMOVED, userID, ClientID(), Role(), account, UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
AccountManager::OnLogoffResult(UserID userID, UPCStatus status) {
	NXAcctInfo::NotifyListeners(Event::LOGOFF_RESULT, userID, ClientID(), Role(), GetAccount(userID), status);
};

/**
 * @private
 */
void
AccountManager::OnLogoff(AccountRef account, ClientID clientID) {
	NXAcctInfo::NotifyListeners(Event::LOGOFF, (account?account->GetUserID():UserID()), ClientID(), Role(), account, UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
AccountManager::OnLoginResult(UserID userID, UPCStatus status) {
	NXAcctInfo::NotifyListeners(Event::LOGIN_RESULT, userID, ClientID(), Role(), GetAccount(userID), status);
};

/**
 * @private
 */
void
AccountManager::OnLogin(AccountRef account, ClientID clientID) {
	NXAcctInfo::NotifyListeners(Event::LOGIN, (account?account->GetUserID():UserID()), ClientID(), Role(), account, UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
AccountManager::OnChangePassword(UserID userID) {
	NXAcctInfo::NotifyListeners(Event::LOGIN, userID, ClientID(), Role(), GetAccount(userID), UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
AccountManager::OnObserveAccount(UserID userID) {
	NXAcctInfo::NotifyListeners(Event::OBSERVE, userID, ClientID(), Role(), GetAccount(userID), UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
AccountManager::OnStopObservingAccount(UserID userID) {
	NXAcctInfo::NotifyListeners(Event::LOGIN, userID, ClientID(), Role(), GetAccount(userID), UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
AccountManager::OnStopWatchingForAccountsResult(UPCStatus status) {
	NXStatus::NotifyListeners(Event::STOP_WATCHING_FOR_ACCOUNTS_RESULT, status);
}

/**
 * @private
 */
void
AccountManager::OnWatchForAccountsResult(UPCStatus status) {
	NXStatus::NotifyListeners(Event::WATCH_FOR_ACCOUNTS_RESULT, status);
};

/**
 * @private
 */
void
AccountManager::OnObserveAccountResult(UserID userID, UPCStatus status) {
	NXAcctInfo::NotifyListeners(Event::OBSERVE_RESULT, userID, ClientID(), Role(), GetAccount(userID), status);
};

/**
 * @private
 */
void
AccountManager::OnStopObservingAccountResult(UserID userID, UPCStatus status) {
	NXAcctInfo::NotifyListeners(Event::STOP_OBSERVING_RESULT, userID, ClientID(), Role(), GetAccount(userID), status);
};

/**
 * @private
 */
void
AccountManager::OnAddRoleResult(UserID userID, Role role, UPCStatus status) {
	NXAcctInfo::NotifyListeners(Event::ADD_ROLE_RESULT, userID, ClientID(), role, GetAccount(userID), status);
};

/**
 * @private
 */
void
AccountManager::OnRemoveRoleResult (UserID userID, Role role, UPCStatus status) {
	NXAcctInfo::NotifyListeners(Event::REMOVE_ROLE_RESULT, userID, ClientID(), role, GetAccount(userID), status);
};

/**
 * @private
 */
void
AccountManager::OnSynchronize() {
	NXStatus::NotifyListeners(Event::WATCH_FOR_ACCOUNTS_RESULT, UPC::Status::SUCCESS);
}
