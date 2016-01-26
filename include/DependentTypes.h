/*
 * DependentTypes.h
 *
 *  Created on: Apr 22, 2014
 *      Author: dak
 */

#ifndef DEPENDENTTYPES_H_
#define DEPENDENTTYPES_H_

typedef std::shared_ptr<const UserAccount> AccountRef;
typedef Manager<Client,ClientID>::Ref ClientRef;
typedef Manager<Room,RoomID>::Ref RoomRef;
typedef std::unordered_map<std::string, std::string> StringMap;

typedef std::function<void(EventType, int)> ICB;
typedef std::function<void(EventType, std::string)> SCB;
typedef std::function<void(EventType, RoomRef)> RCB;

typedef Notifier<RoomQualifier, RoomID, RoomRef, UPCStatus> NotifyRoomInfo;
typedef NXR<RoomQualifier, RoomID, RoomRef, UPCStatus> NXRoomInfo;
typedef NXRoomInfo::CB CBRoomInfo;
typedef std::shared_ptr<CBRoomInfo> CBRoomInfoRef;

typedef Notifier<int> NotifyInt;
typedef NXR<int> NXInt;
typedef NXInt::CB CBInt;
typedef std::shared_ptr<CBInt> CBIntRef;

typedef Notifier<UPCStatus> NotifyStatus;
typedef NXR<UPCStatus> NXStatus;
typedef NXStatus::CB CBStatus;
typedef std::shared_ptr<CBStatus> CBStatusRef;

typedef Notifier<UserID, AccountRef, UPCStatus> NotifyUserAcct;
typedef NXR<UserID, AccountRef, UPCStatus> NXUserAcct;
typedef NXUserAcct::CB CBUserAcct;
typedef std::shared_ptr<CBUserAcct> CBUserAcctRef;

typedef Notifier<RoomRef> NotifyRoom;
typedef NXR<RoomRef> NXRoom;
typedef NXRoom::CB CBRoom;
typedef std::shared_ptr<CBRoom> CBRoomRef;

typedef Notifier<ClientID, ClientRef, int, RoomID, RoomRef, UPCStatus> NotifyClientInfo;
typedef NXR<ClientID, ClientRef, int, RoomID, RoomRef, UPCStatus> NXClientInfo;
typedef NXClientInfo::CB CBClientInfo;
typedef std::shared_ptr<CBClientInfo> CBClientInfoRef;

typedef Notifier<ClientID, ClientRef, Address, UPCStatus> NotifyAddressInfo;
typedef NXR<ClientID, ClientRef, Address, UPCStatus> NXAddressInfo;
typedef NXAddressInfo::CB CBAddressInfo;
typedef std::shared_ptr<CBAddressInfo> CBAddressInfoRef;

typedef Notifier<AttrName, AttrVal, AttrScope, AttrVal, ClientRef, UPCStatus> NotifyAttrInfo;
typedef NXR<AttrName, AttrVal, AttrScope, AttrVal, ClientRef, UPCStatus> NXAttrInfo;
typedef NXAttrInfo::CB CBAttrInfo;
typedef std::shared_ptr<CBAttrInfo> CBAttrInfoRef;

typedef Notifier<UserID, ClientID, Role, AccountRef, UPCStatus> NotifyAcctInfo;
typedef NXR<UserID, ClientID, Role, AccountRef, UPCStatus> NXAcctInfo;
typedef NXAcctInfo::CB CBAcctInfo;
typedef std::shared_ptr<CBAcctInfo> CBAcctInfoRef;

typedef Dispatcher<std::vector<std::string>, UPCStatus> DispatchUPC;
typedef Notifier<std::vector<std::string>, UPCStatus> NotifyUPC;
typedef NXR<std::vector<std::string>, UPCStatus> NXUPC;
typedef NXUPC::CB CBUPC;
typedef std::shared_ptr<CBUPC> CBUPCRef;

typedef Notifier<std::string, UPCStatus> NotifyStatusMessage;
typedef NXR<std::string, UPCStatus> NXStatusMessage;
typedef NXStatusMessage::CB CBStatusMessage;
typedef std::shared_ptr<CBStatusMessage> CBStatusMessageRef;

#endif /* DEPENDENTTYPES_H_ */
