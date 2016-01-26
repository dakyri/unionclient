/*
 * AccountManager.h
 *
 *  Created on: Apr 9, 2014
 *      Author: dak
 */

#ifndef ACCOUNTMANAGER_H_
#define ACCOUNTMANAGER_H_

#include "CommonTypes.h"

class AccountManager: public Manager<UserAccount, UserID>, public NXStatus, public NXAcctInfo {
friend class UnionBridge;
public:
	AccountManager(RoomManager& roomManager, ClientManager& clientManager, UnionBridge& unionBridge, ILogger& log);
	virtual ~AccountManager();

	// overrides for Manager base
	AccountRef Get(const UserID userID) const override;
	std::string GetName() const override;
	bool Valid(const UserID) const override;

	bool AccountIsKnown(const UserID userID) const;
	void ChangePassword(const UserID userID, const std::string newPassword, const std::string oldPassword = "");
	AccountRef GetAccount(const UserID userID) const;

	Map<UserID, AccountRef> GetAccounts() const;
	int GetNumAccounts() const;
	int GetNumAccountsOnServer() const;
	int GetNumLoggedInAccounts() const;
	void ApplyToAccounts(std::function<void(AccountRef)> f) const;
	bool BoolToAccounts(std::function<bool(AccountRef)> f) const;

	bool HasWatchedAccount(const UserID userID) const;
	bool IsObservingAccount(const UserID userID) const;
	bool IsWatchingForAccounts() const;

	void WatchForAccounts() const;
	void StopWatchingForAccounts() const;
	void StopObservingAccount(UserID userID);
	AccountRef SelfAccount() const;

	void DeserializeWatchedAccounts(std::string ids);

	void AddRole(const UserID userID, const std::string role);
	void RemoveRole(const UserID userID, const std::string role);

	void RemoveAccount(const UserID userID, const std::string password = "");
	void CreateAccount(const UserID userID, const std::string password);
	void Login(const UserID userID, const std::string password);
	void Logoff(const UserID userID = "", const std::string password = "");

	std::vector<ClientRef> GetClientsForObservedAccounts() const;
	ClientRef GetObservedAccountsClient(const ClientID clientID) const;

protected:
	void SetIsWatchingForAccounts(bool value);

	void OnCreateAccountResult(UserID userID, UPCStatus status);
	void OnRemoveAccountResult(UserID userID, UPCStatus status);
	void OnChangePasswordResult(UserID userID, UPCStatus status);
	void OnAccountAdded(UserID userID, AccountRef account);
	void OnAccountRemoved(UserID userID, AccountRef account);
	void OnLogoffResult(UserID userID, UPCStatus status);
	void OnLogoff(AccountRef account, ClientID clientID);
	void OnLoginResult(UserID userID, UPCStatus status);
	void OnLogin(AccountRef account, ClientID clientID);
	void OnChangePassword(UserID userID);
	void OnObserveAccount(UserID userID);
	void OnStopObservingAccount(UserID userID);
	void OnStopWatchingForAccountsResult(UPCStatus status);
	void OnWatchForAccountsResult(UPCStatus status);
	void OnObserveAccountResult(UserID userID, UPCStatus status);
	void OnStopObservingAccountResult(UserID userID, UPCStatus status);
	void OnAddRoleResult(UserID userID, Role role, UPCStatus status);
	void OnRemoveRoleResult (UserID userID, Role role, UPCStatus status);
	void OnSynchronize();

	void AddObservedAccount(AccountRef account);
	void RemoveAllObservedAccounts();
	AccountRef RemoveObservedAccount(UserID userID);
	void AddWatchedAccount(AccountRef account);
	void RemoveAllWatchedAccounts();
	AccountRef RemoveWatchedAccount(UserID userID);
	// overrides for Manager base
	virtual UserAccount* Make(const UserID userID) override;

	bool isWatchingForAccounts = false;

	Map<UserID,AccountRef> watchedAccounts;
	Map<UserID,AccountRef> observedAccounts;

	RoomManager& roomManager;
	ClientManager& clientManager;
	UnionBridge& unionBridge;
};

#endif /* ACCOUNTMANAGER_H_ */
