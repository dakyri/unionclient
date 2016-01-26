/*
 * UserAccount.h
 *
 *  Created on: Apr 8, 2014
 *      Author: dak
 */

#ifndef USERACCOUNT_H_
#define USERACCOUNT_H_

#include "CommonTypes.h"

class UserAccount: public AttributeManager, public NotifyAcctInfo, public std::enable_shared_from_this<UserAccount> {
public:
	static const unsigned int kFlagModerator = 1 << 1;

	UserAccount();
	virtual ~UserAccount();

	UserAccount(UserID userID, RoomManager& roomManager, ClientManager& clientManager, AccountManager& accountManager, UnionBridge& bridge, ILogger& log);

	virtual std::string GetName() const override;
	virtual UPCMessageID DeleteAttribCommand() const override;
	virtual UPCMessageID SetAttribCommand() const override;
	virtual StringArgs DeleteAttribRequest(const AttrName attrName, const AttrScope scope) const override;
	virtual StringArgs SetAttribRequest(const AttrName attrName, const AttrVal attrValue, const AttrScope scope, const unsigned int options) const override;
	virtual bool SetLocalAttribs() const override;
	virtual void SendUPC(const UPCMessageID messageID, const StringArgs args) override;

	const UserID GetUserID() const; // 	Returns this account's userID.
	void SetUserID(const UserID uid) const;
	const ClientRef GetClient();  //Returns a reference to this account's Client object, which is available if this account is logged in only.
	void SetClient(const ClientRef c) const;
	const ClientRef GetInternalClient() const; // 	Returns the internal Client object for this account's corresponding client.
	int GetConnectionState() const; // Indicates whether this UserAccount is currently logged in.
	void AddRole(const std::string role); //	Adds a new security role to the account.
	bool IsLoggedIn() const; // 	Returns true if this user account's connection state is ConnectionState.LOGGED_IN; false otherwise.
	bool IsModerator() const; // 	Returns a bool indicating whether the account has moderator privileges.
	bool IsSelf() const; // 	Returns true if the current client is logged in under this UserAccount.

	void ChangePassword(const UserPasswd newPassword, const UserPasswd oldPassword) const; //	Changes the account's password.
	void Logoff(const UserPasswd password = "") const; //	Logs off this user account.
	void Observe() const; // 	Asks the server to notify the current client any time this UserAccount's state changes.
	void RemoveRole(UserID userID, std::string role) const; // 	Removes a security role from the account.
	void StopObserving() const; // 	Asks the server to stop observing this UserAccount.

	void OnRemoveRoleResult(Role role, UPCStatus status) const;
	void OnAddRoleResult(Role role, UPCStatus status) const;
	void OnStopObservingResult(UPCStatus status) const;
	void OnStopObserving() const;
	void OnObserveResult(UPCStatus status) const;
	void OnObserve() const;
	void OnSynchronize() const;
	void OnChangePassword() const;
	void OnChangePasswordResult(UPCStatus status) const;
	void OnLogoff() const;
	void OnLogoffResult(UPCStatus status) const;
	void OnLogin() const;

protected:
	void ValidateClientReference() const;

	UserID mutable id;
	std::string mutable password;
	std::string mutable lastAttemptedPassword;
	ClientRef mutable client;

	RoomManager& roomManager;
	ClientManager& clientManager;
	AccountManager& accountManager;

	ILogger& log;
};

inline bool ValidAcctRef(const AccountRef v) { return v.use_count() > 0; }
inline UserID AcctRefId(const AccountRef v) { return v? v->GetUserID():UserID(); }

#endif /* USERACCOUNT_H_ */
