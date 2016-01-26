/*
 * Logger.h
 *
 *  Created on: Apr 9, 2014
 *      Author: dak
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "CommonTypes.h"

typedef std::string LogEntry;
typedef std::string LogEntryRef;
typedef std::string LogTimestamp;
typedef int LogLevel;
typedef std::string LogMessage;

class ILogger {
public:
	ILogger() {};
	virtual ~ILogger() {};

	virtual void Debug(LogMessage msg) =0;
	virtual void Warn(LogMessage msg) =0;
	virtual void Info(std::string msg) =0;
	virtual void Error(LogMessage msg) =0;

	virtual void AddSuppressionTerm(std::string term) {}
	virtual void RemoveSuppressionTerm(std::string term) {}
};

class DefaultLogger: public ILogger {
public:
	DefaultLogger(unsigned historyLength = 100);
	virtual ~DefaultLogger();

	virtual void Debug(LogMessage msg) override;
	virtual void Warn(LogMessage msg) override;
	virtual void Info(std::string msg) override;
	virtual void Error(LogMessage msg) override;

	virtual void AddSuppressionTerm(std::string term) {}
	virtual void RemoveSuppressionTerm(std::string term) {}

	void SetLevel(LogLevel logLevel);
	void SetHistoryLength(unsigned int newHistoryLength);
	void DisableTimeStamp();
	LogLevel GetLevel();
	void EnableTimeStamp();
	void SetLogStream(std::ostream* stream);

	unsigned int GetHistoryLength();
	std::vector<LogEntryRef> GetHistory();

	static const int kLogNothing = 0;
	static const int kLogError = 1;
	static const int kLogInfo = 2;
	static const int kLogWarn = 3;
	static const int kLogDebug = 4;

protected:
	LogLevel logLevel;
	std::mutex lock;
	std::ostream* logStream;
	void AddEntry(LogLevel lev, LogMessage msg, LogTimestamp ts);
	void AddToHistory(LogLevel lev, LogMessage msg, LogTimestamp ts);
	unsigned historyLength;
};

#endif /* LOGGER_H_ */
