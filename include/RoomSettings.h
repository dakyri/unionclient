/*
 * RoomSettings.h
 *
 *  Created on: Apr 8, 2014
 *      Author: dak
 */

#ifndef ROOMSETTINGS_H_
#define ROOMSETTINGS_H_

#include <string>

class RoomSettings {
public:
	RoomSettings(std::string password="", int maxClients=-1, int removeOnEmpty=-1);
	virtual ~RoomSettings();

	std::string Serialize();

	std::string	password; // "" if not set
	int			maxClients;	// < 0 if not to be set
	int			removeOnEmpty; // < 0 if not to be set
};

#endif /* ROOMSETTINGS_H_ */
