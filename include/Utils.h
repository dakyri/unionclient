/*
 * Utils.h
 *
 *  Created on: Apr 10, 2014
 *      Author: dak
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "CommonTypes.h"

class UPCUtils {
public:
	static bool IsValidAttributeName(AttrName value);
	static bool IsValidAttributeScope(AttrScope value);
	static bool IsValidAttributeValue(AttrVal value);
	static bool IsValidModuleName(ModuleID value);
	static bool IsValidPassword(std::string value);
	static bool IsValidResolvedRoomID(RoomID value);
	static bool IsValidRoomID(RoomID value);
	static bool IsValidRoomQualifier(RoomQualifier value);
	static RoomID GetSimpleRoomID(RoomID id);
	static RoomQualifier GetRoomQualifier(RoomID id);
	static bool InVector(std::vector<std::string> l, std::string s);
};
#endif /* UTILS_H_ */
