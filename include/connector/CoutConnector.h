/*
 * CoutConnector.h
 *
 *  Created on: Apr 28, 2014
 *      Author: dak
 */

#ifndef COUTCONNECTOR_H_
#define COUTCONNECTOR_H_

#include "AbstractConnector.h"

class CoutConnector: public AbstractConnector {
public:
	CoutConnector();
	virtual ~CoutConnector();

	virtual bool IsReady() const override;
	virtual int Connect() override;
	virtual int Disconnect() override;
	virtual int Send(const std::string msg) override;

	virtual void SetActiveConnectionSessionID(std::string) override;
	virtual void SetConnectionAffinity(std::string affinityAddress, int durationSec) override;
};

#endif /* COUTCONNECTOR_H_ */
