/*
 * Version.cpp
 *
 *  Created on: Apr 9, 2014
 *      Author: dak
 */

#include "CommonTypes.h"
#include "Version.h"

/**
 * @class Version Version.h
 * simple helper class to represent a software, protocol or api version
 */
Version::Version(short b, short mj, short mi, short re) {
	build = b;
	major = mj;
	minor = mi;
	revision = re;
}

Version
Version::FromVersionString(std::string s)
{
	Version v;
	std::stringstream idss(s);
	std::string item;
	if (std::getline(idss, item, '.')) {
		v.major = atoi(item.c_str());
	}
	if (std::getline(idss, item, '.')) {
		v.minor = atoi(item.c_str());
	}
	if (std::getline(idss, item, '.')) {
		v.revision = atoi(item.c_str());
	}
	if (std::getline(idss, item, '.')) {
		v.build = atoi(item.c_str());
	} else {
		v.build = -1;
	}
	return v;
}

std::string
Version::ToString()
{
	char buf[100];
	if (build >= 0) {
#ifdef _MSC_VER
		sprintf_s(buf, 100, "%d.%d.%d.%d", major, minor, revision, build);
#else
		snprintf(buf, 100, "%d.%d.%d.%d", major, minor, revision, build);
#endif
	} else {
#ifdef _MSC_VER
		sprintf_s(buf, 100, "%d.%d.%d", major, minor, revision);
#else
		snprintf(buf, 100, "%d.%d.%d", major, minor, revision);
#endif
	}
	return std::string(buf);
}

std::string
Version::ToVerboseString()
{
	char buf[100];
	if (build >= 0) {
#ifdef _MSC_VER
		sprintf_s(buf, 100, "%d.%d.%d (Build %d)", major, minor, revision, build);
#else
		snprintf(buf, 100, "%d.%d.%d (Build %d)", major, minor, revision, build);
#endif
	} else {
#ifdef _MSC_VER
		sprintf_s(buf, 100, "%d.%d.%d", major, minor, revision);
#else
		snprintf(buf, 100, "%d.%d.%d", major, minor, revision);
#endif
	}
	return std::string(buf);
}
