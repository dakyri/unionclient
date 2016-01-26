/*
 * CoutConnector.cpp
 *
 *  Created on: Apr 28, 2014
 *      Author: dak
 */

#include "UCLowerHeaders.h"
#include "connector/CoutConnector.h"
#include <iostream>
using namespace std;

CoutConnector::CoutConnector() {
}

CoutConnector::~CoutConnector() {
}

bool
CoutConnector::IsReady() const
{
	return true;
}
int
CoutConnector::Connect()
{
	return 0;
}
int
CoutConnector::Disconnect()
{
	return 0;
}
int
CoutConnector::Send(std::string msg)
{
	cout << msg << endl;
	return 0;
}

void CoutConnector::SetActiveConnectionSessionID(std::string) { }
void CoutConnector::SetConnectionAffinity(std::string host, int durationSec) { }

