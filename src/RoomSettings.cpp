/*
 * RoomSettings.cpp
 *
 *  Created on: Apr 10, 2014
 *      Author: dak
 */

#include "CommonTypes.h"
#include "RoomSettings.h"
#include "Token.h"

/**
 * @class RoomSettings RoomSettings.h
 * wrapper for data sent on room create upc
 */
RoomSettings::RoomSettings(std::string password, int maxClients, int removeOnEmpty)
	: password(password)
	, maxClients(maxClients)
	, removeOnEmpty(removeOnEmpty) {

}

RoomSettings::~RoomSettings() {

}

/**
 * generate a serialized string from the fields
 */
std::string
RoomSettings::Serialize()
{
	std::string rs(Token::RS);
	std::string rae(Token::REMOVE_ON_EMPTY_ATTR);
	std::string pa(Token::RS);

	return rae + rs +
		(removeOnEmpty >= 0 ? "true" : "false")	+ rs + Token::MAX_CLIENTS_ATTR + rs +
		std_to_string(maxClients < 0 ? -1 : maxClients)	+ rs + pa + rs + password;
}


