/*
 * Room.h
 *
 *  Created on: Apr 8, 2014
 *      Author: dak
 */

#ifndef ROOM_H_
#define ROOM_H_

#include "CommonTypes.h"

#include "MessageNotifier.h"

class Room: public std::enable_shared_from_this<Room>, public AttributeManager, public NotifyInt, public NotifyClientInfo {
public:
	Room(RoomID id, RoomManager& roomManager, ClientManager& clientManager, AccountManager& accountManager,
			UnionBridge &unionBridge, ILogger& log);
	virtual ~Room();

	std::string ToString() const;

	bool ClientIsInRoom(const ClientID clientID="") const;
	bool ClientIsObservingRoom(const ClientID clientID="") const;

	ClientRef GetClient(const ClientID client) const;
	int GetNumObservers() const;
	int GetNumOccupants() const;
	void SetNumOccupants(const int n) const;
	void SetNumObservers(const int n) const;

	const RoomID &GetRoomID() const;
	const RoomID GetSimpleRoomID() const;
	void SetRoomID(const RoomID id);

	std::vector<ClientID> GetObserverIDs() const;
	std::vector<ClientRef> GetObservers() const;
	std::vector<ClientID> GetOccupantIDs() const;
	std::vector<ClientRef> GetOccupants() const;
	const RoomQualifier GetQualifier() const;

	void AddOccupant(const ClientRef) const;
	void RemoveOccupant(const ClientID) const;
	void AddObserver(const ClientRef) const;
	void RemoveObserver(const ClientID) const;

	const Map<ClientID, ClientRef>& GetObserverList() const;
	const Map<ClientID, ClientRef>& GetOccupantList() const;
	const ClientRef GetObserver(const ClientID) const;
	const ClientRef GetOccupant(const ClientID) const;

	SyncState GetSyncState() const;
	void SetSyncState(const SyncState) const;
	void UpdateSyncState() const;
	void Synchronize(const RoomManifest& manifest) const;

	RoomSettings GetRoomSettings() const;
	void SetRoomSettings(const RoomSettings &settings);

	void Join(const std::string password = "", const UpdateLevels updateLevels = 0) const;
	void Leave() const;
	void Observe(const std::string password = "", const UpdateLevels updateLevels = 0) const;
	void StopObserving() const;
	void Remove(const std::string password = "");
	void SendUPCMessage(const UPCMessageID messageName, const std::vector<std::string> msg,
			const bool includeSelf = false, const IFilterRef filters = nullptr) const;

	void SendModuleMessage(const UserMessageID messageName, const StringMap messageArguments) const;
	CBModMsgRef AddMessageListener(const UserMessageID message, const CBModMsg listener) const ;
	void RemoveMessageListener(const UserMessageID message, const CBModMsgRef listener) const;
	bool HasMessageListener(const UserMessageID message, const CBModMsgRef listener) const;

	void SetUpdateLevels(const UpdateLevels updateLevels) const;

	virtual std::string GetName() const override;
	virtual UPCMessageID DeleteAttribCommand() const override;
	virtual UPCMessageID SetAttribCommand() const override;
	virtual StringArgs DeleteAttribRequest(const AttrName attrName, const AttrScope scope) const override;
	virtual StringArgs SetAttribRequest(const AttrName attrName, const AttrVal attrValue, const AttrScope scope, const unsigned int options) const override;
	virtual bool SetLocalAttribs() const  override;
	virtual void SendUPC(const UPCMessageID messageID, const StringArgs args) override;

	void OnJoin() const;
	void OnSynchronized() const;
	void OnJoinResult(UPCStatus status) const;
	void OnLeave() const;
	void OnLeaveResult(UPCStatus status) const;
	void OnObserve() const;
	void OnObserveResult(UPCStatus status) const;
	void OnStopObserving() const;
	void OnStopObservingResult(UPCStatus status) const;

	static const int kSyncUnknown = 0;
	static const int kSynchronized = 1;
	static const int kUnsynchronized = 2;
	static const int kSynchronizing = 3;

private:
	void PurgeRoomData() const;

	void AddClientAttributeListeners(const ClientRef c) const;
	void RemoveClientAttributeListeners(const ClientRef c) const;

	RoomID mutable id;

	bool mutable disposed = false;
	bool mutable clientIsInRoom = false;
	bool mutable clientIsObservingRoom = false;
	int mutable numOccupants = 0;
	int mutable numObservers = 0;

	SyncState mutable syncState;
	Map<ClientID,ClientRef> mutable occupantList;
	Map<ClientID,ClientRef> mutable observerList;

	RoomManager& roomManager;
	ClientManager& clientManager;
	AccountManager& accountManager;

	UnionBridge& unionBridge;
	ILogger& log;

	CBAttrInfoRef mutable updateClientAttributeListener;
	CBAttrInfoRef mutable deleteClientAttributeListener;
};

inline bool ValidRoomRef(RoomRef v) { return v.use_count()>0; }
inline RoomID RoomRefId(RoomRef v) { return v? v->GetRoomID():RoomID(); }

#endif /* ROOM_H_ */
