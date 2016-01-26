/*
 * UserAccount.cpp
 *
 *  Created on: Apr 8, 2014
 *      Author: dak
 */

#include "UCUpperHeaders.h"

/**
 * @class UserAccount UserAccount.h
 *
 * redundant in the prezi scheme of things ... they have their own system
 */
UserAccount::UserAccount(UserID userID, RoomManager& roomManager, ClientManager& clientManager, AccountManager& accountManager, UnionBridge& bridge, ILogger& log)
	: AttributeManager()
	, roomManager(roomManager)
	, clientManager(clientManager)
	, accountManager(accountManager)
	, log(log) {
}

UserAccount::~UserAccount() {
}


///////////////////////////////////
// Attribute owner overrides
//////////////////////////////////

std::string UserAccount::GetName() const  { return id; };
UPCMessageID UserAccount::DeleteAttribCommand()  const { return UPC::ID::REMOVE_CLIENT_ATTR; };
UPCMessageID UserAccount::SetAttribCommand() const { return UPC::ID::SET_CLIENT_ATTR; };
StringArgs UserAccount::DeleteAttribRequest(AttrName attrName, AttrScope attrScope) const  { return { "", id, attrName, attrScope }; };
StringArgs UserAccount::SetAttribRequest(AttrName attrName, AttrVal attrValue, AttrScope scope, unsigned int options) const {
	return { "", id, attrName, attrValue, scope, std_to_string(options)};};
bool UserAccount::SetLocalAttribs() const { return IsSelf(); }
void UserAccount::SendUPC(UPCMessageID c, StringArgs a) { /*accountManager.SendUPC(c, a);*/ }

/////////////////////////////////////
// getters, setters, manipulators
/////////////////////////////////////

void UserAccount::SetClient(const ClientRef c) const {
	client = c;
	if (!c) {
		client = nullptr;
	} else if (client != c) {
		client = c;
		client->SetAccount(shared_from_this());
	}
}

bool UserAccount::IsSelf() const {
	if (client) return client->IsSelf();
	return false;
}

const UserID
UserAccount::GetUserID() const
{
	return id;
}

void
UserAccount::SetUserID(const UserID uid) const {
	if (id != uid) {
		id = uid;
	}
};

const ClientRef
UserAccount::GetClient()
{
	ValidateClientReference();
	return client;
}

const ClientRef
UserAccount::GetInternalClient() const{
	ValidateClientReference();
	return client;
}

/**
 * @private
 */
void
UserAccount::ValidateClientReference() const{
	if (client) {
		if (!client->IsSelf()
				&& !clientManager.IsWatchingForClients()
				&& !accountManager.IsObservingAccount(GetUserID())
				&& !clientManager.IsObservingClient(client->GetClientID())
				&& !roomManager.ClientIsKnown(client->GetClientID())) {
			SetClient(ClientRef());
		}
	}
}

int
UserAccount::GetConnectionState() const {
	if (GetInternalClient()) {
		return ConnectionState::LOGGED_IN;
	} else if (!accountManager.IsObservingAccount(GetUserID())) {
		return ConnectionState::NOT_CONNECTED;
	} else if (clientManager.IsWatchingForClients()) {
		return ConnectionState::NOT_CONNECTED;
	} else {
		// Not observing this user, not watching for clients, and no client means
		// this account's state is unknown. (This happens when watching for user
		// accounts).
		return ConnectionState::UNKNOWN;
	}
}

void
UserAccount::AddRole(const std::string role) {
	  accountManager.AddRole(GetUserID(), role);
}

bool
UserAccount::IsLoggedIn() const {
	 return GetConnectionState() == ConnectionState::LOGGED_IN;
}

bool
UserAccount::IsModerator() const {
	AttrVal rolesAttr = GetAttribute(Token::ROLES_ATTR);
	if (rolesAttr != "") {
		return (atoi(rolesAttr.c_str()) & kFlagModerator) > 0;
	} else {
		log.Warn("Could not determine moderator status because the account is not synchronized.");
		return false;
	}
}

void
UserAccount::ChangePassword(const UserPasswd newPassword, const UserPasswd oldPassword) const {
	  accountManager.ChangePassword(GetUserID(), newPassword, oldPassword);
}

void
UserAccount::Logoff(const UserPasswd password) const {
	  accountManager.Logoff(GetUserID(), password);
}

void
UserAccount::Observe() const{

}

void
UserAccount::RemoveRole(UserID userID, std::string role) const {
	 accountManager.RemoveRole(GetUserID(), role);
}

void
UserAccount::StopObserving() const {

}

////////////////////////////////////
// notify and event hooks
///////////////////////////////////
/**
 * @private
 */
void
UserAccount::OnLogin() const {
	const_cast<UserAccount*>(this)->NXAcctInfo::NotifyListeners(Event::LOGIN, GetUserID(), client? client->GetClientID():"", Role(), AccountRef(), UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
UserAccount::OnLogoffResult(UPCStatus status) const {
	const_cast<UserAccount*>(this)->NXAcctInfo::NotifyListeners(Event::LOGOFF_RESULT, GetUserID(), client? client->GetClientID():"", Role(), AccountRef(),  status);
}

/**
 * @private
 */
void
UserAccount::OnLogoff() const {
	SetClient(ClientRef());
	const_cast<UserAccount*>(this)->NXAcctInfo::NotifyListeners(Event::LOGOFF, GetUserID(), client? client->GetClientID():"", Role(), AccountRef(), UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
UserAccount::OnChangePasswordResult(UPCStatus status) const {
	const_cast<UserAccount*>(this)->NXAcctInfo::NotifyListeners(Event::CHANGE_PASSWORD_RESULT, GetUserID(), client? client->GetClientID():"", Role(), AccountRef(), status);
};

/**
 * @private
 */
void
UserAccount::OnChangePassword() const {
	const_cast<UserAccount*>(this)->NXAcctInfo::NotifyListeners(Event::CHANGE_PASSWORD, GetUserID(), client? client->GetClientID():"", Role(), AccountRef(), UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
UserAccount::OnSynchronize() const {
	const_cast<UserAccount*>(this)->NXAcctInfo::NotifyListeners(Event::SYNCHRONIZED, GetUserID(), client? client->GetClientID():"", Role(), AccountRef(), UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
UserAccount::OnObserve() const {
	const_cast<UserAccount*>(this)->NXAcctInfo::NotifyListeners(Event::OBSERVE, GetUserID(), client? client->GetClientID():"", Role(), AccountRef(), UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
UserAccount::OnObserveResult(UPCStatus status) const {
	const_cast<UserAccount*>(this)->NXAcctInfo::NotifyListeners(Event::OBSERVE_RESULT, GetUserID(), client? client->GetClientID():"", Role(), AccountRef(), status);
};

/**
 * @private
 */
void
UserAccount::OnStopObserving() const {
	const_cast<UserAccount*>(this)->NXAcctInfo::NotifyListeners(Event::STOP_OBSERVING, GetUserID(), client? client->GetClientID():"", Role(), AccountRef(), UPC::Status::SUCCESS);
};

/**
 * @private
 */
void
UserAccount::OnStopObservingResult(UPCStatus status) const {
	const_cast<UserAccount*>(this)->NXAcctInfo::NotifyListeners(Event::STOP_OBSERVING_RESULT, GetUserID(), client? client->GetClientID():"", Role(), AccountRef(), status);
};

/**
 * @private
 */
void
UserAccount::OnAddRoleResult(Role role, UPCStatus status) const {
	const_cast<UserAccount*>(this)->NXAcctInfo::NotifyListeners(Event::ADD_ROLE_RESULT, GetUserID(), client? client->GetClientID():"", role, AccountRef(), status);
}

/**
 * @private
 */
void
UserAccount::OnRemoveRoleResult(Role role, UPCStatus status) const {
	const_cast<UserAccount*>(this)->NXAcctInfo::NotifyListeners(Event::REMOVE_ROLE_RESULT, GetUserID(), client? client->GetClientID():"", role, AccountRef(), status);
}
