/*
 * Version.h
 *
 *  Created on: Apr 9, 2014
 *      Author: dak
 */

#ifndef VERSION_H_
#define VERSION_H_

#include <string>

class Version {
public:
	Version(short mj=0, short mi=0, short re=0, short b=0);

	Version& operator()(short mj=0, short mi=0, short re=0, short b=0) {
		major=mj;
		minor=mi;
		revision=re;
		build=b;
		return *this;
	}
	static Version FromVersionString(std::string s);
	std::string ToString();
	std::string ToVerboseString();
	bool operator==(Version v) {
		return major == v.major && minor == v.minor && revision == v.revision && build == v.build;
	}
	bool operator!=(Version v) {
		return major != v.major || minor != v.minor || revision != v.revision || build != v.build;
	}
	bool operator<(Version v) {
		if (major < v.major) return true; else if (major > v.major) return false;
		if (minor < v.minor) return true; else if (minor > v.minor) return false;
		if (revision < v.revision) return true; else if (revision > v.revision) return false;
		return  build < v.build;
	}
	bool operator>(Version v) {
		if (major > v.major) return true; else if (major < v.major) return false;
		if (minor > v.minor) return true; else if (minor < v.minor) return false;
		if (revision > v.revision) return true; else if (revision < v.revision) return false;
		return  build > v.build;
	}
	bool operator<=(Version v) {
		if (major < v.major) return true; else if (major > v.major) return false;
		if (minor < v.minor) return true; else if (minor > v.minor) return false;
		if (revision < v.revision) return true; else if (revision > v.revision) return false;
		return  build <= v.build;
	}
	bool operator>=(Version v) {
		if (major > v.major) return true; else if (major < v.major) return false;
		if (minor > v.minor) return true; else if (minor < v.minor) return false;
		if (revision > v.revision) return true; else if (revision < v.revision) return false;
		return  build >= v.build;
	}
	Version operator=(Version v) {
		major = v.major;
		minor = v.minor;
		revision = v.revision;
		build = v.build;
		return v;
	}

	short major;
	short minor;
	short revision;
	short build;
};

#endif /* VERSION_H_ */
