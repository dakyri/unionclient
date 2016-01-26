/*
 * RoomManager.h
 *
 *  Created on: Apr 7, 2014
 *	  Author: dak
 */

#ifndef ROOMMANAGER_H_
#define ROOMMANAGER_H_

class RoomManager;

using namespace std;

class RoomManager: public Manager<Room, RoomID>, public NotifyInt, public NotifyRoomInfo {
	friend class UnionBridge;
public:
	RoomManager(ClientManager& clientManager, AccountManager& accountManager, UnionBridge& UnionBridge, ILogger &log);
	virtual ~RoomManager();

	// overrides for Manager base
	RoomRef Get(const RoomID roomID) const override;
	std::string GetName() const override;
	bool Valid(const RoomID) const override;

	bool ClientIsKnown(const ClientID clientID) const;
	bool RoomIsKnown(const RoomID clientID) const;
	bool HasObservedRoom(const RoomID roomID) const;
	bool HasOccupiedRoom(const RoomID roomID) const;
	bool HasWatchedRoom(const RoomID roomID) const;
	bool IsWatchingQualifier(const RoomQualifier qualifier) const;

	std::vector<ClientRef> GetAllClients() const;
	ClientRef GetClient(const ClientID clientID) const;

	int GetNumRooms(const RoomQualifier qualifier = "") const;

	void StopWatchingForRooms(const RoomQualifier qualifier = "") const;
	void WatchForRooms(const RoomQualifier qualifier = "") const;

	RoomRef CreateRoom(const RoomID roomID = "",
			shared_ptr<RoomSettings> roomSettings=nullptr,
			shared_ptr<AttributeInitializer> attributes = nullptr,
			shared_ptr<RoomModules> modules = nullptr) const;
	RoomRef JoinRoom(const RoomID roomID, const std::string password = "", const UpdateLevels updateLevels = 0) const;
	RoomRef ObserveRoom(const RoomID roomID, const std::string password = "", const UpdateLevels updateLevels = 0) const;
	void RemoveRoom(const RoomID roomID, std::string password = "") const;
	void SendMessage(const UPCMessageID messageName, const std::vector<RoomID> rooms, const StringArgs msg,
			const bool includeSelf = false, const IFilterRef filters = nullptr) const;

	std::vector<RoomID> GetRoomIDs() const;
	std::vector<RoomRef> GetRooms() const ;
	std::vector<RoomRef> GetRoomsWithQualifier(const RoomQualifier qualifier = "") const;

protected:
	void OnCreateRoomResult(RoomQualifier roomIDQualifier, RoomID roomID, UPCStatus status);
	void OnRemoveRoomResult(RoomQualifier roomIDQualifier, RoomID roomID, UPCStatus status);
	void OnRoomAdded(RoomQualifier roomIDQualifier, RoomID roomID, RoomRef theRoom);
	void OnRoomRemoved(RoomQualifier roomIDQualifier, RoomID roomID, RoomRef theRoom);
	void OnRoomCount(int numRooms);
	void OnJoinRoomResult(RoomID roomID, UPCStatus status);
	void OnLeaveRoomResult(RoomID roomID, UPCStatus status);
	void OnObserveRoomResult(RoomID roomID, UPCStatus status);
	void OnStopObservingRoomResult(RoomID roomID, UPCStatus status);
	void OnWatchForRoomsResult(RoomQualifier roomIDQualifier, UPCStatus status);
	void OnStopWatchingForRoomsResult(RoomQualifier roomIDQualifier, UPCStatus status);

	RoomRef AddOccupiedRoom(const RoomID roomID);
	RoomRef RemoveOccupiedRoom(const RoomID roomID);

	RoomRef AddObservedRoom(const RoomID roomID);
	RoomRef RemoveObservedRoom(const RoomID roomID);

	RoomRef AddWatchedRoom(const RoomID roomID);
	RoomRef RemoveWatchedRoom(const RoomID roomID);
	void SetWatchedRooms(const RoomQualifier qualifier, const std::vector<RoomID> newRoomIDs);
	void DisposeWatchedRooms();
	void Dispose(const RoomID roomID);

	// overrides for Manager base
	virtual Room* Make(const RoomID roomID) override;



	ClientManager& clientManager;
	AccountManager& accountManager;

	UnionBridge& unionBridge;

	std::vector<std::string> watchedQualifiers;

	Map<RoomID,RoomRef> occupiedRooms;
	Map<RoomID,RoomRef> observedRooms;
	Map<RoomID,RoomRef> watchedRooms;

	CBRoomInfoRef watchForRoomsResultListener;
	CBRoomInfoRef stopWatchingForRoomsResultListener;

};

#endif /* ROOMMANAGER_H_ */
