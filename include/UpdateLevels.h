/*
 * UpdateLevel.h
 *
 *  Created on: Apr 9, 2014
 *      Author: dak
 */

#ifndef UPDATELEVEL_H_
#define UPDATELEVEL_H_

// TODO rewrite as fields perhaps?

typedef unsigned int UpdateLevels;

const unsigned kRoomMessages = 1;
const unsigned kSharedRoomAttributes = 1 << 1;
const unsigned kOccupantCount = 1 << 2;
const unsigned kObserverCount = 1 << 3;
const unsigned kOccupantList = 1 << 4;
const unsigned kObserverList = 1 << 5;
const unsigned kSharedOccupantAttributesRoom = 1 << 6;
const unsigned kSharedObserverAttributesRoom = 1 << 7;
const unsigned kSharedOccupantAttributesGlobal = 1 << 8;
const unsigned kSharedObserverAttributesGlobal = 1 << 9;
const unsigned kOccupantLoginLogoff = 1 << 10;
const unsigned kObserverLoginLogoff = 1 << 11;
const unsigned kAllRoomAttributes = 1 << 12;

inline bool RecievesUpdates(UpdateLevels l) { return l!=0; }
inline bool UpdateRoomMessage(UpdateLevels l) { return (l&kRoomMessages) != 0; }
inline bool UpdateSharedRoomAttributes(UpdateLevels l) { return (l&kSharedRoomAttributes) != 0; }
inline bool UpdateOccupantCount(UpdateLevels l) { return (l&kOccupantCount) != 0; }
inline bool UpdateObserverCount(UpdateLevels l) { return (l&kObserverCount) != 0; }
inline bool UpdateOccupantList(UpdateLevels l) { return (l&kOccupantList) != 0; }
inline bool UpdateObserverList(UpdateLevels l) { return (l&kObserverList) != 0; }
inline bool UpdateSharedOccupantAttributesRoom(UpdateLevels l) { return (l&kSharedOccupantAttributesRoom) != 0; }
inline bool UpdateSharedObserverAttributesRoom(UpdateLevels l) { return (l&kSharedObserverAttributesRoom) != 0; }
inline bool UpdateSharedOccupantAttributesGlobal(UpdateLevels l) { return (l&kSharedOccupantAttributesGlobal) != 0; }
inline bool UpdateSharedObserverAttributesGlobal(UpdateLevels l) { return (l&kSharedObserverAttributesGlobal) != 0; }
inline bool UpdateOccupantLoginLogoff(UpdateLevels l) { return (l&kOccupantLoginLogoff) != 0; }
inline bool UpdateObserverLoginLogoff(UpdateLevels l) { return (l&kObserverLoginLogoff) != 0; }
inline bool UpdateAllRoomAttributes(UpdateLevels l) { return (l&kAllRoomAttributes) != 0; }

#endif /* UPDATELEVEL_H_ */
