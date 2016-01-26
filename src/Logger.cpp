/*
 * Logger.cpp
 *
 *  Created on: Apr 9, 2014
 *      Author: dak
 */

#include "Logger.h"
#include <iostream>

/**
 * @class DefaultLogger Logger.h
 * default logger which does no frills, suppression, or history, and dumps everything to stderr
 */
DefaultLogger::DefaultLogger(unsigned historyLength)
	: ILogger()
	, logStream(&std::cerr)
	, historyLength(historyLength)
{
	SetLevel(kLogDebug);
}

DefaultLogger::~DefaultLogger() {
}
using namespace std;
void
DefaultLogger::Error(std::string msg)
{
	if (logLevel >= kLogError) {
		lock.lock();
		*logStream << msg << endl;
		lock.unlock();
	}
}

void
DefaultLogger::Warn(std::string msg)
{
	if (logLevel >= kLogWarn) {
		lock.lock();
		*logStream << msg << endl;
		lock.unlock();
	}
}

void
DefaultLogger::Debug(std::string msg)
{
	if (logLevel >= kLogDebug) {
		lock.lock();
		*logStream << msg << endl;
		lock.unlock();
	}
}

void
DefaultLogger::Info(std::string msg)
{
	if (logLevel >= kLogInfo) {
		lock.lock();
		*logStream << msg << endl;
		lock.unlock();
	}
}

void
DefaultLogger::SetLevel(LogLevel level)
{
	logLevel = level;
}

void
DefaultLogger::SetLogStream(std::ostream* stream)
{
	if (stream != nullptr) logStream = stream;
}