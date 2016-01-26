/*
 * RoomManifest.cpp
 *
 *  Created on: Apr 17, 2014
 *      Author: dak
 */

#include "UCUpperHeaders.h"

/**
 * @class RoomManifest
 * @brief representation of server info passed back by, among others, the u54 command
 */
RoomManifest::RoomManifest()
	: observerCount(0)
	, occupantCount(0) 	{
}

/**
 * @private
 */
RoomManifest::~RoomManifest() {
}

/**
 * @private
 * builds a manifest from the passed server side string info
 */
void
RoomManifest::Deserialize(const std::string _roomID, const std::string _serializedAttributes, const std::vector<ClientID> _clientList, const int _occupantCount, const int _observerCount) {
	roomID = _roomID;
	occupantCount = _occupantCount;
	observerCount = _observerCount;
	DeserializeAttributes(_serializedAttributes);
	DeserializeClientList(_clientList);
};

/**
 * @private
 * builds the attributes passed
 */
void
RoomManifest::DeserializeAttributes(const std::string serializedAttributes) {
	std::vector<std::string> attrList;
	std::stringstream idss(serializedAttributes);
	std::string item;
	char c = *Token::RS;
	while (std::getline(idss, item, c)) {
		attrList.push_back(item);
	}
	for (int i = (int)attrList.size()-2; i >= 0; i -=2) {
		attributes.SetAttribute(attrList[i], attrList[i+1], Token::GLOBAL_ATTR);
	}
};

/**
 * @private
 * builds the client list passed
 */
void
RoomManifest::DeserializeClientList(const std::vector<ClientID> clientList) {
	for (int i = (int)clientList.size()-5; i >= 0; i -=5) {

		ClientManifest clientManifest;
		clientManifest.Deserialize(clientList[i], clientList[i+1], "", "", clientList[i+3], {roomID, clientList[i+4]});
		if (clientList[i+2] == "0") {
			occupants.push_back(clientManifest);
		} else {
			observers.push_back(clientManifest);
		}
	}
}
