/*
 * CnxLayer.cpp
 *
 *  Created on: May 11, 2014
 *      Author: dak
 */


#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <random>

#include "UCLowerHeaders.h"

#include <ctype.h>


/**
 * @class CnxLayer CnxLayer.h
 * @brief provides bass class for protocol layers for network io
 *
 * base protocol layer ... passes output to a lower layer, and passes notifications back to an upper layer. the upper layer isn't necessarily a CnxLayer. if this is a fundamental io layer,
 * the lower layer may be null
 */
CnxLayer::CnxLayer(CnxLayerUpper* upper, CnxLayer* lower)
	: upper(upper)
	, lower(lower)
	, connectState(ConnectionState::UNKNOWN) {
}

CnxLayer::~CnxLayer()
{
}

/**
 * format an io error and pass to an upper layer
 */
void
CnxLayer::DoIOError(int status, const std::string s, ...) const
{
	char buf[100];

	va_list args;
	va_start (args, s);


#ifdef _MSC_VER
	vsnprintf_s(buf, 100, s.c_str(), args);
#else
	vsnprintf(buf, 100, s.c_str(), args);
#endif
	va_end(args);
	std::string e(buf);
	if (upper) {
		upper->OnIOError(e, status);
	} else {
		std::clog << "Error " << e << std::endl;
	}
}

/**
 * format an open failure error and pass to an upper layer
 */
void
CnxLayer::DoOpenFailure(int status, const std::string s, ...) const
{
	char buf[100];

	va_list args;
	va_start (args, s);

#ifdef _MSC_VER
	vsnprintf_s(buf, 100, s.c_str(), args);
#else
	vsnprintf(buf, 100, s.c_str(), args);
#endif
	va_end(args);
	std::string e(buf);
	if (upper) {
		upper->OnOpenFailure(e, status);
	} else {
		std::clog << "Error " << e << std::endl;
	}
}

/**
 * format a server disconnect message and pass to an upper layer
 */
void
CnxLayer::DoServerDisconnect(int status, const std::string s, ...) const
{
	char buf[100];

	va_list args;
	va_start (args, s);

#ifdef _MSC_VER
	vsnprintf_s(buf, 100, s.c_str(), args);
#else
	vsnprintf(buf, 100, s.c_str(), args);
#endif

	va_end(args);
	std::string e(buf);
	if (upper) {
		upper->OnServerDisconnect(e, status);
	} else {
		std::clog << "Error " << e << std::endl;
	}
}
