/*
 * RoomModules.cpp
 *
 *  Created on: Apr 10, 2014
 *      Author: dak
 */

#include "RoomModules.h"
#include "Token.h"

/**
 * @class RoomModules RoomModules.h
 * a serializeable helper object for passing module info to a create room request
 */
RoomModules::RoomModules() {
}

RoomModules::~RoomModules() {
}

/**
 * a vector of identifiers from this list
 */
std::vector<ModuleID>
RoomModules::GetIdentifiers() const {
	std::vector<ModuleID> v;
	for (auto it=begin(); it!=end(); ++it) {
		v.push_back(it->first);
	}
	return v;
}

/**
 * adds a module to this object
 */
void
RoomModules::AddModule(std::string identifier, std::string type) {
	if (find(identifier) != end()) at(identifier) = type;
};

/**
 * serialize all this info
 */
std::string
RoomModules::Serialize() const
{
	std::string modulesString("");
	auto it=begin();
	for (; it!=end(); ) {
		modulesString += it->second + std::string(Token::RS) + it->first;
		++it;
		if (it != end()) {
			modulesString += std::string(Token::RS);
		}
	}

	return modulesString;

}
