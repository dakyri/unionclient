/*
 * ClientManager.h
 *
 *  Created on: Apr 8, 2014
 *      Author: dak
 */

#ifndef CLIENTMANAGER_H_
#define CLIENTMANAGER_H_

#include "CommonTypes.h"

class ClientManager: public Manager<Client, ClientID>, public NotifyClientInfo, public NotifyAddressInfo, public NotifyStatus {
	friend class UnionBridge;
public:
	ClientManager(
			RoomManager& roomManager, AccountManager& accountManager,
			UnionBridge& unionBridge, ILogger& log);
	virtual ~ClientManager();

	bool ClientIsKnown(const ClientID clientID) const;

	std::unordered_map<ClientID, AttrVal> GetAttributeForClients(const std::vector<ClientID> clientIDs, const AttrName attrName, const AttrScope attrScope) const;

	// overrides for Manager base
	ClientRef Get(const ClientID clientID) const override;
	std::string GetName() const override;
	bool Valid(const ClientID) const override;

	const Map<ClientID, ClientRef> GetClients() const;
	int GetLifetimeNumClientsKnown() const;
	int GetNumClients() const;
	int GetNumClientsOnServer() const;

	bool HasWatchedClient(const ClientID clientID) const;
	bool IsObservingClient(const ClientID clientID) const;
	bool IsWatchingForBannedAddresses() const;
	bool IsWatchingForClients() const;

	void SendMessage(const UPCMessageID messageName, const std::vector<std::string> msg,
			const std::vector<ClientID> clientIDs = std::vector<ClientID>(), const IFilterRef filters = nullptr) const;

	void KickClient(const ClientID clientID) const;
	void ObserveClient(const ClientID clientID) const;
	void StopWatchingForBannedAddresses() const;
	void StopWatchingForClients() const;
	void WatchForBannedAddresses() const;
	void WatchForClients() const;

	void Ban(const Address address, const int duration, const std::string reason = "") const;
	void Unban(const Address address) const;
	const Set<Address>& GetBannedAddresses() const;
	void SetIsWatchingForBannedAddresses(const bool value) const;
	void AddWatchedBannedAddress(Address address) const;
	void SetWatchedBannedAddresses(const Set<Address> bannedList) const;

	ClientRef Self() const;
	void SetSelf(const ClientRef) const;
	int GetSelfConnectionState();
	AccountRef SelfAccount() const;
	void SelfLogoff();

protected:
	void OnObserveClient(ClientRef client) const;
	void OnStopObservingClient(ClientRef client) const;
	void OnClientConnected(ClientRef client) const;
	void OnClientDisconnected(ClientRef client) const;
	void OnStopWatchingForClientsResult(UPCStatus status) const;
	void OnWatchForClientsResult(UPCStatus status) const;
	void OnObserveClientResult(ClientID clientID, UPCStatus status) const;
	void OnStopObservingClientResult(ClientID clientID, UPCStatus status) const;
	void OnKickClientResult(ClientID clientID, UPCStatus status) const;
	void OnBanClientResult(Address address, ClientID clientID, UPCStatus status) const;
	void OnUnbanClientResult(Address address, UPCStatus status) const;
	void OnWatchForBannedAddressesResult(UPCStatus status) const;
	void OnStopWatchingForBannedAddressesResult(UPCStatus status) const;
	void OnAddressBanned(Address address) const;
	void OnAddressUnbanned(Address address) const;
	void OnSynchronizeBanlist() const;
	void OnSynchronize() const;

	void SetIsWatchingForClients(const bool value);
	void AddWatchedClient(const ClientRef client);
	void AddObservedClient(const ClientRef client);
	void RemoveWatchedClient(ClientID clientID);
	void RemoveObservedClient(const ClientID clientID);
	void RemoveAllWatchedClients();
	void RemoveAllObservedClients();
	void RemoveWatchedBannedAddress(const Address a);
	void DeserializeWatchedClients(const std::string ids);

	// overrides for Manager base
	virtual Client* Make(const ClientID clientID) override;

	unsigned int mutable lifetimeClientsRequested=0;
	bool mutable isWatchingForClients = false;
	bool mutable isWatchingForBannedAddresses = false;

	Map<ClientID, ClientRef> watchedClients;
	Map<ClientID, ClientRef> observedClients;

	Set<Address> mutable bannedAddresses;

	ClientRef mutable selfReference;

	RoomManager& roomManager;
	AccountManager& accountManager;

	UnionBridge& unionBridge;
};

#endif /* CLIENTMANAGER_H_ */
