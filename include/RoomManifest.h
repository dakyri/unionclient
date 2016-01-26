/*
 * RoomManifest.h
 *
 *  Created on: Apr 17, 2014
 *      Author: dak
 */

#ifndef ROOMMANIFEST_H_
#define ROOMMANIFEST_H_

#include "CommonTypes.h"

class RoomManifest {
public:
	RoomManifest();
	virtual ~RoomManifest();

	RoomID roomID;
	int observerCount;
	int occupantCount;
	std::vector<ClientManifest> occupants;
	std::vector<ClientManifest> observers;
	AttributeCollection attributes;

	void DeserializeClientList(const std::vector<ClientID> clientList);
	void DeserializeAttributes(const std::string serializedAttributes);
	void Deserialize(const std::string roomID, const std::string serializedAttributes, const std::vector<ClientID> clientList, const int occupantCount, const int observerCount);
};

#endif /* ROOMMANIFEST_H_ */
