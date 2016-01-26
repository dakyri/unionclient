/*
 * RoomModules.h
 *
 *  Created on: Apr 10, 2014
 *      Author: dak
 */

#ifndef ROOMMODULES_H_
#define ROOMMODULES_H_

#include "CommonTypes.h"

class RoomModules: public std::unordered_map<std::string, std::string> {
public:
	RoomModules();
	virtual ~RoomModules();

	std::vector<ModuleID> GetIdentifiers() const;
	std::string Serialize() const;
	void AddModule(const std::string identifier, const std::string type);
};

#endif /* ROOMMODULES_H_ */
