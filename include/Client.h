/*
 * Client.h
 *
 *  Created on: Apr 8, 2014
 *	  Author: dak
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include "CommonTypes.h"


class Client: public std::enable_shared_from_this<Client>, public AttributeManager, public NotifyClientInfo, public NotifyStatus, public NotifyRoomInfo, public NotifyAcctInfo  {
	friend class UnionBridge;
public:
	Client(ClientID userID, RoomManager& roomManager, ClientManager& clientManager, AccountManager& accountManager,
				UnionBridge& unionBridge, ILogger& log);
	virtual ~Client();

	std::string ToString() const;

	bool IsAdmin() const;
	bool IsObservingRoom(const RoomID roomID) const;
	bool IsSelf() const;
	bool IsInRoom(const RoomID roomID) const;

	void Ban(const int duration, const std::string reason = "") const;
	void Kick() const;
	void Observe() const;
	void StopObserving() const;
	void SendMessage(const UPCMessageID messageName, const std::vector<string> msgs) const;

	virtual std::string GetName() const override;
	virtual UPCMessageID DeleteAttribCommand() const override;
	virtual UPCMessageID SetAttribCommand() const override;
	virtual StringArgs DeleteAttribRequest(const AttrName attrName, const AttrScope scope) const override;
	virtual StringArgs SetAttribRequest(const AttrName attrName, const AttrVal attrValue,const  AttrScope scope, const unsigned int options) const override;
	virtual bool SetLocalAttribs() const override;
	virtual void SendUPC(const UPCMessageID messageID, const StringArgs args) override;

	const ClientID GetClientID() const;
	const AccountRef GetAccount() const;
	const UserID GetUserID() const;

	const std::vector<RoomID> GetOccupiedRoomIDs() const;
	const std::vector<RoomID> GetObservedRoomIDs() const;

	void SetIsSelf() const;
	UpdateLevels GetUpdateLevels(const RoomID rid) const;
	void Synchronize(const ClientManifest &) const;
	void SetAccount(const AccountRef r) const;

	static const unsigned int kFLAG_ADMIN=1<<2;

	CBModMsgRef AddMessageListener(const UserMessageID message, const CBModMsg listener) const ;
	void RemoveMessageListener(const UserMessageID message, const CBModMsgRef listener) const;
	bool HasMessageListener(const UserMessageID message, const CBModMsgRef listener) const;

	void OnJoinRoom(const RoomRef room, const RoomID roomID) const;
	void OnLeaveRoom(const RoomRef room, const RoomID roomID) const;
	void OnObserveRoom(const RoomRef room, const RoomID roomID) const;
	void OnStopObservingRoom(const RoomRef room, const RoomID roomID) const;
	void OnObserve() const;
	void OnStopObserving() const;
	void OnObserveResult(const UPCStatus) const;
	void OnStopObservingResult(const UPCStatus) const;
	void OnSynchronize() const;
	void OnLogoff(const UserID id) const;
	void OnLogin() const;

protected:
	void SynchronizeOccupiedRoomIDs(const std::vector<RoomID> newOccupiedRoomIDs);
	void SynchronizeObservedRoomIDs(const std::vector<RoomID> newObservedRoomIDs);

	void AddOccupiedRoomID(const RoomID) const;
	void RemoveOccupiedRoomID(const RoomID) const;
	void AddObservedRoomID(const RoomID) const;
	void RemoveObservedRoomID(const RoomID) const;

	void SetConnectionState(int newState) const;
	int mutable connectionState = ConnectionState::UNKNOWN;

	ClientID id;

	Set<RoomID> mutable occupiedRooms;
	Set<RoomID> mutable observedRooms;

	bool mutable isSelf = false;
	AccountRef mutable account;
	bool mutable disposed = false;

	RoomManager& roomManager;
	ClientManager& clientManager;
	AccountManager& accountManager;

	UnionBridge& unionBridge;
	ILogger& log;
};

inline bool ValidClientRef(const ClientRef v) { return v.use_count() > 0; }
inline ClientID ClientRefId(const ClientRef v) { return v? v->GetClientID():ClientID(); }

#endif /* CLIENT_H_ */
