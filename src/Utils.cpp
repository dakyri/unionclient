/*
 * Utils.cpp
 *
 *  Created on: Apr 10, 2014
 *      Author: dak
 */

#include "UCUpperHeaders.h"

/**
 * @class UPCUtils Utils.h
 * wrapper class around useful functions for UPC room and client IDs
 */

/**
 * @return true if valid room id
 */
bool
UPCUtils::IsValidRoomID(RoomID value) {
	// Can't be null, nor the empty string
	if (value == "") {
		return false;
	}
	// Can't contain "."
	if (value.find_first_of(".") != std::string::npos){
		return false;
	}
	// Can't contain RS
	if (value.find_first_of(Token::RS) != std::string::npos){
		return false;
	}
	// Can't contain WILDCARD
	if (value.find_first_of(Token::WILDCARD) != std::string::npos){
		return false;
	}

	return true;
};

/**
 * @return true if valid room qualifier
 */
bool
UPCUtils::IsValidRoomQualifier(RoomQualifier value) {
	if (value == "") {
		return false;
	}
	// "*" is valid (it means the unnamed qualifier)
	if (value == "*") {
		return true;
	}

	// Can't contain RS
	if (value.find_first_of(Token::RS) != std::string::npos){
		return false;
	}
	// Can't contain WILDCARD
	if (value.find_first_of(Token::WILDCARD) != std::string::npos){
		return false;
	}

	return true;
};

/**
 * @return true if valid resolved room id
 */
bool
UPCUtils::IsValidResolvedRoomID(RoomID value) {
	// Can't be null, nor the empty string
	if (value == "") {
		return false;
	}

	// Can't contain RS
	if (value.find_first_of(Token::RS) != std::string::npos){
		return false;
	}
	// Can't contain WILDCARD
	if (value.find_first_of(Token::WILDCARD) != std::string::npos){
		return false;
	}

	return true;
};

/**
 * @return true if valid attribute name
 */
bool
UPCUtils::IsValidAttributeName(AttrName value) {
	// Can't be empty
	if (value == "") {
		return false;
	}

	// Can't contain RS
	if (value.find_first_of(Token::RS) != std::string::npos){
		return false;
	}

	return true;
}

/**
 * @return true if valid attribute value
 */
bool
UPCUtils::IsValidAttributeValue(AttrVal value) {
	// Can't contain RS
	if (value.find_first_of(Token::RS) == std::string::npos){
		return true;
	} else {
		return false;
	}
}

/**
 * @return true if valid attribute scope
 */
bool
UPCUtils::IsValidAttributeScope(AttrScope value) {
	// Can't contain RS
	// TODO can be null, but a string won't be .. change to a char *?
//	if (value == nullptr) {
//		return true;
//	}
	return IsValidResolvedRoomID(value);
};

/**
 * @return true if valid module name
 */
bool
UPCUtils::IsValidModuleName(ModuleID value) {
	// Can't be empty (can be null)
	// TODO can be null, but a string won't be .. change to a char *?
//	if (value == nullptr) {
//		return true;
//	}
	if (value == "") {
		return false;
	}

	// Can't contain RS
	if (value.find_first_of(Token::RS) != std::string::npos){
		return false;
	}

	return true;
};

/**
 * @return true if valid password
 */
bool
UPCUtils::IsValidPassword(std::string value) {
	// Can't contain RS
	if (value.find_first_of(Token::RS) != std::string::npos){
		return false;
	}

	return true;
};


/**
 * @return the simple room id part of a qualified room id
 */
RoomID
UPCUtils::GetSimpleRoomID(RoomID id)
{
	size_t pos=id.find_last_of('.');
	return (pos == std::string::npos)?id:id.substr(pos+1);
}

/**
 * @return the RoomQualifier part of a given room id
 */
RoomQualifier
UPCUtils::GetRoomQualifier(RoomID id)
{
	size_t pos=id.find_last_of('.');
	return (pos == std::string::npos)?id:id.substr(0, pos);
}

/**
 * true if the string is in the given vector
 */
bool
UPCUtils::InVector(std::vector<std::string> l, std::string s)
{
	for (auto it=l.begin(); it!=l.end(); ++it) {
		if (!it->compare(s)) {
			return true;
		}
	}
	return false;
}
