/*
 * UVEventLoop.h
 *
 *  Created on: May 13, 2014
 *      Author: dak
 */

#ifndef UVEVENTLOOP_H_
#define UVEVENTLOOP_H_

#include "CommonTypes.h"
#include "UVForwards.h"
#include "EventLoop.h"

typedef std::function<void(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)> ReaderCB;
typedef std::function<void(uv_connect_t*req, int status)> ConnectCB;
typedef std::function<void(sockaddr* res, int status)> ResolverCB;
typedef std::function<void(uv_handle_t *res)> CloserCB;

struct UVTimer: public Timer {
	UVTimer(TimerCB* _cb, uint64_t _delayMS, uint64_t _repeatMs)
	{
		tikCB = _cb;
		delayMS = _delayMS;
		repeatMs = _repeatMs;
		disposed = false;
	}
	mutable bool disposed;
	uv_timer_t timer;
};

struct UVWorker: public Worker {
	UVWorker(WorkerCB* _cb, WorkerCB* _acb) {
		queuedCB = _cb;
		apresCB = _acb;
		disposed = false;
	}
	mutable bool disposed;
	uv_work_t work;
};

struct UVWriter {
	UVWriter(const UVTCPClient *client, const char *msg, const size_t n)
		: disposed(false)
		, client(client)
		, msg(msg)
		, n(n) { }
	mutable bool disposed;
	const UVTCPClient *client;
	uv_buf_t buf;
	uv_write_t request;
	const char *msg;
	const size_t n;
};

struct UVReader {
	UVReader(const UVTCPClient *client, ConnectCB*_ocb, ReaderCB *_cb)
		: disposed(false)
		, connected(false)
		, closing(false)
		, client(client) {
		connectCB = _ocb;
		readerCB = _cb;
		closerCB = nullptr;
	}
	mutable bool disposed;
	mutable bool connected;
	mutable bool closing;
	const UVTCPClient *client;
	ConnectCB *connectCB;
	ReaderCB *readerCB;
	CloserCB* closerCB;
	uv_connect_t request;
};

struct UVResolver {
	UVResolver(const UVTCPClient *client, std::string host, std::string service, ResolverCB*_cb)
		: disposed(false)
		, resolving(false)
		, client(client)
		, host(host)
		, service(service) {
		bindCB = _cb;
	};
	mutable bool disposed;
	mutable bool resolving;

	const UVTCPClient* client;
	ResolverCB* bindCB;
	addrinfo dnsHints;
	uv_getaddrinfo_t resolver;
	std::string host;
	std::string service;
};

struct UVCloser {
	UVCloser(const UVTCPClient *client, CloserCB* _ocb)
		: disposed(false)
		, client(client) {
		cb = _ocb;
	}
	mutable bool disposed;
	const UVTCPClient *client;
	CloserCB* cb;
};

typedef UVWriter* WriterRef;
typedef UVReader* ReaderRef;

class UVLock: public Lockable
{
public:
	UVLock();
	virtual ~UVLock();

	virtual void Lock() override;
	virtual void Unlock() override;
	uv_mutex_t uvMutex;
};

class UVEventLoop: public EventLoop {
public:
	UVEventLoop();
	virtual ~UVEventLoop();

	virtual TimerRef Schedule(uint64_t delayMS, uint64_t repeatMs, TimerCB cb) override;
	virtual void CancelTimer(TimerRef) override;
	virtual void Lock() override;
	virtual void Unlock() override;
	virtual WorkerRef Worker(WorkerCB, WorkerCB acb=WorkerCB()) override;
	virtual void CancelWorker(WorkerRef) override;

	void Write(const UVTCPClient *client, const char *msg, const size_t n);
	void Connect(const UVTCPClient *client, ConnectCB ocb, ReaderCB cb);
	void Resolve(const UVTCPClient *client, const std::string host, const std::string service, ResolverCB ocb);
	void Close(const UVTCPClient *client, CloserCB cb);

	static const int kUVCnxCallError = -1;
	static const int kUVBindCallError = -2;

	void ForceStopAndClose();
	bool StartUVRunner();
	bool StopUVRunner(const bool force, const bool andWait);

protected:
	static void Runner(void *up);
	void HandleRunnerQueues();
	static void OnTick(uv_timer_t* handle);
	static void OnWork(uv_work_t *req);
	static void OnAfterWork(uv_work_t *req, int status);

	static void OnResolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res);
	static void OnConnect(uv_connect_t *req, int status);
	static void OnRead(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
	static void OnWrite(uv_write_t* req, int status);
	static void OnClose(uv_handle_t* handle);
	static void AllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t* buf);

	uv_loop_t* loop;
	bool runUV;
	bool uvIsRunning;

	uv_thread_t runner; // trying to avoid a uv.h dependency in header file or hack in a platform specific reference
	uv_mutex_t mutex;

//	std::mutex workerLock;

	std::vector<UVTimer*> scheduledTimers;
	std::vector<UVTimer*> activeTimers;
	std::vector<UVWorker*> activeWorkers;

	std::vector<UVWriter*> writers;
	std::vector<UVReader*> readers;
	std::vector<UVResolver*> resolvers;
	std::vector<UVCloser*> closers;
};

#endif /* UVEVENTLOOP_H_ */
