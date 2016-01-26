/*
 * TimeoutManager.cpp
 *
 *  Created on: May 13, 2014
 *      Author: dak
 */

#include "uv.h"
#include "UVEventLoop.h"
#include <stdint.h>
#include <iostream>
#include <cstring>
#ifndef _MSC_VER
#include <unistd.h>
#endif

/**
 * @class UVLock UVEventLoop.h
 * basic mutex
 */
UVLock::UVLock() {
	if (uv_mutex_init(&uvMutex) < 0) { // oops
		;
	}
}
UVLock::~UVLock() {
	uv_mutex_destroy(&uvMutex);
}

/**
 * lock it
 */
void
UVLock::Lock()
{
	uv_mutex_lock(&uvMutex);
}

/**
 * unlock it
 */
void
UVLock::Unlock()
{
	uv_mutex_unlock(&uvMutex);
}

/**
 * @class UVEventLoop UVEventLoop.h
 * @brief wrapper around a threaded uv event loop for providing basic async facilities
 *
 * the main thread for timing and all network io
 * does not lock the uv_run call, ie the timers and workers and io routines can modify the uv_loop (many of the timers in particular either schedule or
 * events or close scheduled events)
 * TODO at the moment, we should be a bit cautious about removing callbacks ... it would be safer to use the shared-weak-pointer patter as in the NXR class
 * and descendants. though? perhaps passing around shared pointers will make it a pain to reuse this in some places
 */

/**
 * create loop, mutex, and start the thread
 */
UVEventLoop::UVEventLoop() {
	runUV = false;
	loop = uv_loop_new();
	if (uv_mutex_init(&mutex) < 0) { // oops
		;
	}
	StartUVRunner();
}

/**
 * shut down uv and cleanup ... and should be the first thing we do when exitting
 * this closes all current connections to uv. if we have multiple instances of the client, that will close their io
 */
void
UVEventLoop::ForceStopAndClose()
{
	DEBUG_OUT("UVEventLoop::~UVEventLoop()");
	StopUVRunner(true, true); // wait till we are definitely out of harms way
	for (auto it : resolvers) {
		it->disposed = true;
	}
	DEBUG_OUT(readers.size() << " readers");
	for (auto it : readers) {
		uv_handle_t *h = (uv_handle_t*)it->client->socket;
		if (uv_is_active(h)) uv_close(h, nullptr);
		it->disposed = true;
	}
	DEBUG_OUT(readers.size() << " writers");
	for (auto it : writers) {
		uv_handle_t *h = (uv_handle_t*)it->client->socket;
		if (uv_is_active(h)) uv_close(h, nullptr);
		it->disposed = true;
	}
	DEBUG_OUT(readers.size() << " closers ");
	for (auto it : closers) {
		uv_handle_t *h = (uv_handle_t*)it->client->socket;
		if (uv_is_active(h)) uv_close(h, nullptr);
		it->disposed = true;
	}
	for (auto it : scheduledTimers) { // won't have been called
		it->disposed = true;
	}
	DEBUG_OUT(readers.size() << " timers ");
	for (auto it : activeTimers) {
		uv_handle_t *h = (uv_handle_t*)&it->timer;
		if (uv_is_active(h)) uv_close(h, nullptr);
	}
	for (auto it : activeWorkers) { // doesn't get closed
		it->disposed = true;
	}
	uv_run(loop, UV_RUN_DEFAULT);
	HandleRunnerQueues(); // do the actual cleanup
}
/**
 * stop the thread, get rid of any active timeouts, and clean up the mutexes
 * really really want the thread to be shut down before we get anywhere near doing this
 */
UVEventLoop::~UVEventLoop() {
	DEBUG_OUT("UVEventLoop::~UVEventLoop()");
	// wait till we are definitely out of harms way  ...
	StopUVRunner(true, true);
	// and double check that we've got everything
	for (auto it : resolvers) {
		it->disposed = true;
	}
	for (auto it: readers) {
		it->disposed = true;
	}
	for (auto it: writers) {
		it->disposed = true;
	}
	for (auto it: closers) {
		it->disposed = true;
	}
	for (auto it: activeTimers) {
		it->disposed = true;
	}
	for (auto it: scheduledTimers) {
		it->disposed = true;
	}
	for (auto it: activeWorkers) {
		it->disposed = true;
	}
	HandleRunnerQueues(); // do the actual cleanup
	uv_mutex_destroy(&mutex);
	DEBUG_OUT("UVEventLoop::~UVEventLoop() done");
}

/**
 * handle queues. this handles all deletions of callbacks as well as setting up the uv requests on each cycle. In normal course of affairs, it will be locked
 * for operation, as should all the queue vectors for all the various requests
 */
void
UVEventLoop::HandleRunnerQueues()
{
	for (auto bit = resolvers.begin(); bit != resolvers.end();) {
		auto qp = *bit;
		if (qp->disposed) {
			if (qp->bindCB) delete qp->bindCB;
			delete qp;
			bit = resolvers.erase(bit);
		} else if (!qp->resolving) {
			qp->resolving = true;
			qp->dnsHints.ai_family = PF_INET;
			qp->dnsHints.ai_socktype = SOCK_STREAM;
			qp->dnsHints.ai_protocol = IPPROTO_TCP;
			qp->dnsHints.ai_flags = 0;
			qp->resolver.data = qp;
			UVTCPClient* client = const_cast<UVTCPClient*>(qp->client);
			if (client->address == nullptr || client->socket == nullptr) {

			}
			int r = uv_getaddrinfo(
					loop, &qp->resolver, OnResolved,
					qp->host.c_str(), qp->service.c_str(), &qp->dnsHints);
			if (r<0) {
				if (qp->bindCB) {
					(*qp->bindCB)(client->address, kUVBindCallError);
				}
				qp->disposed = true;
			}
			++bit;
		} else {
			++bit;
		}
	}
	for (auto rit = readers.begin(); rit != readers.end();) {
		auto qp = *rit;
		if (qp->disposed) {
//! in the main connection queue, readers, the socket i.e. client has ownership of the read callback (*rit)->cb that should be erased by the close request, which will
//! supplant the read functional call back with a close functional
			if (qp->connectCB) delete qp->connectCB;
			if (qp->readerCB) delete qp->readerCB;
			if (qp->closerCB) delete qp->closerCB;
			delete qp;
			rit = readers.erase(rit);
		} else if (qp->closing) {
			UVTCPClient* client = const_cast<UVTCPClient*>(qp->client);
			int r = uv_read_stop((uv_stream_t*)client->socket);
			if (r >= 0) {
				uv_close((uv_handle_t*)client->socket, OnClose);
			}
			qp->closing = false; // only try to close once!
			++rit;
		} else if (!qp->connected) {
			qp->connected = true;
			UVTCPClient *client = const_cast<UVTCPClient*>(qp->client);
			if (client->socket) {
				if (client->socket->data) { // should already be deleted
				}
				uv_tcp_init(loop, client->socket);
				client->socket->data = qp;
				qp->request.data = qp;
				int r=uv_tcp_connect(&qp->request, client->socket, client->address, OnConnect);
				if (r<0) {
					if (qp->connectCB) {
						(*qp->connectCB)(&qp->request, kUVCnxCallError);
					}
				}
			}
			++rit;
		} else {
			++rit;
		}
	}
	for (auto writ = writers.begin(); writ != writers.end();) {
		auto qp=*writ;
		if (qp->disposed) {
			if (qp->buf.base) {
				delete []qp->buf.base;
			}
			delete *writ;
			writ = writers.erase(writ);
		} else {
			UVTCPClient* client = const_cast<UVTCPClient*>((*writ)->client);
			int r=uv_write(&qp->request, (uv_stream_t*)client->socket, &qp->buf, 1, OnWrite);
			if (r < 0) {
				qp->disposed = true;
			}
			++writ;
		}
	}
	;
	for (auto cit = closers.begin(); cit != closers.end();) {
		auto qp = *cit;
		if (qp->disposed) {
			if (qp->cb) delete qp->cb;
			delete qp;
			cit=closers.erase(cit);
		} else {
			UVTCPClient* client = const_cast<UVTCPClient*>(qp->client);
			uv_read_stop((uv_stream_t*)client->socket);
			if (client->socket->data) {
				delete static_cast<ReaderCB*>(client->socket->data);
			}
			client->socket->data = qp;
			uv_close((uv_handle_t*)client->socket, OnClose);
			++cit;
		}
	}
//	workerLock.lock();
	for (auto wit = activeWorkers.begin(); wit != activeWorkers.end();) {
		auto qp=*wit;
		if (qp->disposed) {
			if (qp->queuedCB) delete qp->queuedCB;
			if (qp->apresCB) delete qp->apresCB;
			delete qp;
			wit = activeWorkers.erase(wit);
		} else {
			int r = uv_queue_work(loop, &qp->work, OnWork, OnAfterWork);
			if (r < 0) {
				OnAfterWork(&qp->work, r);
			}
			++wit;
		}
	}
//	workerLock.unlock();
	for (auto tit = scheduledTimers.begin(); tit != scheduledTimers.end();) {
		auto qp = *tit;
		if (qp->disposed) {
			if (qp->tikCB) delete qp->tikCB;
			delete qp;
		} else {
			DEBUG_OUT("UVEventLoop::HandleRunnerQueues() timer started");
			uv_timer_init(loop, &qp->timer);
			uv_timer_start(&qp->timer, OnTick, qp->delayMS, qp->repeatMs);
			activeTimers.push_back(qp);
		}
		tit = scheduledTimers.erase(tit);
	}
	for (auto tit = activeTimers.begin(); tit != activeTimers.end();) {
		auto qp = *tit;
		if (qp->disposed) {
			DEBUG_OUT("UVEventLoop::HandleRunnerQueues() timer cleaned up");
			if (uv_is_active((uv_handle_t*)&qp->timer)) {
				DEBUG_OUT("UVEventLoop::HandleRunnerQueues() stopping an active timer");
				uv_timer_stop(&qp->timer);
				++tit;
			} else {
				if (qp->tikCB) delete qp->tikCB;
				delete qp;
				tit = activeTimers.erase(tit);
			}
		} else {
			++tit;
		}
	}
}

/**
 * the core of the event loop
 */
void
UVEventLoop::Runner(void *up)
{
	int status=0;
	UVEventLoop *l = (UVEventLoop*)up;
	l->uvIsRunning = true;
	while (l->runUV) {
		l->Lock();
		l->HandleRunnerQueues();
		l->Unlock();

		if ((status = uv_run(l->loop, UV_RUN_NOWAIT)) < 0) { // error ... == 0 means all ok ... > 0 all ok but need run again
		}
		if (!uv_loop_alive(l->loop)) {
#ifdef _MSC_VER
			Sleep(1);
#else
			usleep(1);
#endif
		}
	}
	l->uvIsRunning = false;
	DEBUG_OUT("UVEventLoop::UVWorker() closing");
}

/**
 * start UVRunner()
 */
bool
UVEventLoop::StartUVRunner()
{
	if (runUV) return true;
	runUV = true;
	uv_thread_create(&runner, Runner, this);
	return true;
}

/**
 * stop UVRunner()
 */
bool
UVEventLoop::StopUVRunner(const bool force, const bool andWait)
{
	if (!runUV) return true;
	if (!force) {
		if (uv_loop_alive(loop)) {
			DEBUG_OUT("UVRun::Stop() loop is still active ...");
			return false;
		}
	}
	runUV = false;
	if (force) {
// afics if i  mutex this, the run thread will see that it should exit anyway, and it should be mutexed as it isn't otherwise safe
//		uv_stop(loop);
	}

	if (andWait) {
		DEBUG_OUT("waiting ...");
		uv_thread_join(&runner); // wait for the worker to finish
//		while (uvIsRunning); // a fallback, hopefully never necessary to do it this way
		DEBUG_OUT("we waited and we got there");
	}
	return true;
}

/**
 * put a worker on the worker queue which will be processed and uv_queue'd on the next cycle
 * xxxx seems to be doing something thread unsave inside libuv
 */
WorkerRef
UVEventLoop::Worker(WorkerCB _cb, WorkerCB _acb) {
	WorkerCB *cb = nullptr;
	WorkerCB *acb = nullptr;
	if (_cb) cb = new WorkerCB(_cb);
	if (_acb) acb = new WorkerCB(_acb);
	UVWorker* r = new UVWorker(cb, acb);
	r->work.data = r;
	Lock();
//	workerLock.lock();
	activeWorkers.push_back(r);
//	workerLock.unlock();
	Unlock();
	return r;
}

/**
 * cancel a queued worker by removing from the queue
 */
void
UVEventLoop::CancelWorker(WorkerRef cb) {
	Lock();
	for (auto it=activeWorkers.begin(); it!=activeWorkers.end(); ++it) {
		if (*it == cb) {
			(*it)->disposed = true;
			break;
		}
	}
	Unlock();
}

/**
 * schedule an event to trigger the given callback ... repeatMS > 0 -> it's repeating
 * @return a handle to refer to and remove this event
 */
TimerRef
UVEventLoop::Schedule(const uint64_t delayMs, const uint64_t repeatMs, TimerCB _cb)
{
	DEBUG_OUT("UVEventLoop::schedule()!!");
	TimerCB *cb = nullptr;
	if (_cb) cb = new TimerCB(_cb);
	UVTimer *timer = new UVTimer(cb, delayMs, repeatMs);
	DEBUG_OUT("UVEventLoop::schedule() allocating " << (uint64_t)timer);
	timer->timer.data = timer;
	Lock();
	scheduledTimers.push_back(timer);
	Unlock();

	return timer;
}

/**
 * cancel the given event
 */
void
UVEventLoop::CancelTimer(TimerRef cb)
{
	DEBUG_OUT("UVEventLoop::CancelTimer()!!");
	Lock();
	for (auto it=scheduledTimers.begin(); it!=scheduledTimers.end(); ++it) {
		if ((*it) == cb) {
			(*it)->disposed = true;
			break;
		}
	}
	for (auto it=activeTimers.begin(); it!=activeTimers.end(); ++it) {
		if ((*it) == cb) {
			(*it)->disposed = true;
			break;
		}
	}
	Unlock();
}

/**
 * write data to the given UVTCPClient's socket
 */
void
UVEventLoop::Write(const UVTCPClient *client, const char *msg, const size_t n)
{
	DEBUG_OUT("UVEventLoop::Write()!!");
	UVWriter *writer = new UVWriter(client, msg, n);
	writer->buf = uv_buf_init(new char[writer->n], (unsigned int)writer->n);
	memcpy(writer->buf.base, msg, n);
	writer->request.data = writer;
	Lock();
	writers.push_back(writer);
	Unlock();
	DEBUG_OUT("UVEventLoop::Write() done");
}

/**
 * make a connection on the given UVTCP clients socket to the address given in this hadlers sockddr
 */
void
UVEventLoop::Connect(const UVTCPClient *client, ConnectCB _ocb, ReaderCB _cb)
{
	DEBUG_OUT("UVEventLoop::Connect()!!");

	ConnectCB *ocb = nullptr;
	if (_ocb) ocb = new ConnectCB(_ocb);
	ReaderCB *cb = nullptr;
	if (_cb) cb = new ReaderCB(_cb);
	UVReader *reader = new UVReader(client, ocb, cb);

	DEBUG_OUT("UVEventLoop::Connect() trying " << reader->client->IP4Addr() << " family " << reader->client->address->sa_family
			<<" port "<<(((struct sockaddr_in*) reader->client->address)->sin_port)<<" ...");

	Lock();
	readers.push_back(reader);
	Unlock();
}

/**
 * resolve host:service, and set the UVTCPClient's sockaddr structure
 */
void
UVEventLoop::Resolve(const UVTCPClient *client, const std::string host, const std::string service, ResolverCB _cb)
{
	ResolverCB *cb = nullptr;
	if (_cb) cb = new ResolverCB(_cb);
	UVResolver* resolver = new UVResolver(client, host, service, cb);
	Lock();
	resolvers.push_back(resolver);
	Unlock();
}

/**
 * close the socket of the given UVTCPClient
 */
void
UVEventLoop::Close(const UVTCPClient *client, CloserCB _cb)
{
	DEBUG_OUT("UVEventLoop::Close()!! ");
	CloserCB *cb = nullptr;
	if (_cb) cb = new CloserCB(_cb);
//	UVCloser* closer = new UVCloser(client, cb);
//	Lock();
//	closers.push_back(closer);
//	Unlock();
	UVReader *qp = nullptr;
	if (client) {
		Lock();
		if (client->socket) {
			for (auto it: readers) {
				if (it == static_cast<UVReader*>(client->socket->data)) {
					qp = it;
					break;
				}
			}
		}
		if (qp) {
			qp->closerCB = cb;
			qp->closing = true;
		}
		Unlock();
		if (!qp) {
			// if not found in the list of active readers, we should assume it's open perhaps ??? xxx not sure how to tell if still valid
			DEBUG_OUT("UVEventLoop::Close() ... reader to close not found");
			if (cb) (*cb)((uv_handle_t *)client->socket); // todo ?? change this callback to take a status
			delete cb;
		}
	}
}

/**
 * lock this queues for this thread
 */
void
UVEventLoop::Lock()
{
	uv_mutex_lock(&mutex);
}

void
UVEventLoop::Unlock()
{
	uv_mutex_unlock(&mutex);
}

/**
 * static wrapper around the user callback function (typedef void (*uv_timer_cb)(uv_timer_t* handle))
 */
void
UVEventLoop::OnTick(uv_timer_t* timer)
{
//	DEBUG_OUT("tick!!");
	UVTimer* otCBp=nullptr;
	if (timer) {
		otCBp=static_cast<UVTimer*>(timer->data);
		if (otCBp) {
			(*otCBp->tikCB)();
			if (!uv_is_active((uv_handle_t*)timer)) { // it's a 1 shot timer!
				otCBp->disposed = true;
			}
		}
	}
}

void
UVEventLoop::OnWork(uv_work_t *req)
{
	UVWorker* wCBp=nullptr;
	if (req) {
		wCBp=static_cast<UVWorker*>(req->data);
		if (wCBp && wCBp->queuedCB != nullptr) {
			(*wCBp->queuedCB)();
		}
	}
}

void
UVEventLoop::OnAfterWork(uv_work_t *req, int status)
{
	UVWorker* wCBp=nullptr;
	if (req) {
		wCBp=static_cast<UVWorker*>(req->data);
		if (wCBp && wCBp->apresCB != nullptr) {
			(*wCBp->apresCB)();
		}
	}
}


/**
 * the static callback called by the C-level routines in uv. just does a little house keeping
 *
 * from uv.h
 * typedef void (*uv_write_cb)(uv_write_t* req, int status);
 */
void
UVEventLoop::OnWrite(uv_write_t* req, int status)
{
	if (req && req->data) {
		UVWriter *w = (UVWriter*)req->data;
		if (w) {
			w->disposed = true;
		}
	}
}

/**
 * the static callback called by the C-level routines in uv. makes a standard buffer in the standard way
 *
 * from uv.h
 * typedef void (*uv_alloc_cb)(uv_handle_t* handle,
 *							size_t suggested_size,
 *							uv_buf_t* buf);
 */
void
UVEventLoop::AllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t* buf) {
//	DEBUG_OUT("UVCnxLayer::AllocBuffer() " << suggested_size << " handle " << (unsigned)handle << std::endl);
	*buf = uv_buf_init(new char[suggested_size], (unsigned int)suggested_size);
}


/**
 * the static callback called by the C-level routines in uv. it's main job is to call a C++ level callback to do the work with higher layers
 *
 * from uv.h
 * typedef void (*uv_read_cb)(uv_stream_t* stream,
 *						   ssize_t nread,
 *						   const uv_buf_t* buf);
 */
void
UVEventLoop::OnRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
//	DEBUG_OUT(UVCnxLayer::OnRead() handle " << (unsigned)client<< " n " << nread);
//	ReaderCB* ocCBp=nullptr;
	UVReader* ocCBp=nullptr;
	if (stream) {
//		ocCBp=static_cast<ReaderCB*>(client->data);
		ocCBp=static_cast<UVReader*>(stream->data);
		if (ocCBp) {
			if (nread > 0) {
//				std::string test(buf->base, nread);
//				DEBUG_OUT("UVCnxLayer::OnRead() handle " << (unsigned)ocCBp<< " n " << test );
			} else {
//				DEBUG_OUT("UVCnxLayer::OnRead() status " << n );
			}
			if (ocCBp->readerCB) {
				(*ocCBp->readerCB)(stream, nread, buf);
			}
		}
	}
	if (buf && buf->base) {
		delete [] buf->base;
	}
}

/**
 * the static callback called by the C-level routines in uv. it's main job is to call a C++ level callback to do the work with higher layers
 *
 * from uv.h
 * typedef void (*uv_connect_cb)(uv_connect_t* req, int status);
 */
void
UVEventLoop::OnConnect(uv_connect_t *req, int status)
{
	DEBUG_OUT("OnConnect() ... " << status);
	UVReader* ocCBp=nullptr;
	if (req) {
		ocCBp=static_cast<UVReader*>(req->data);
	}
	if (ocCBp) {
		ocCBp->client->socket->data = ocCBp;
		if (status >= 0) {
			status = uv_read_start((uv_stream_t*)ocCBp->client->socket, AllocBuffer, OnRead);
		}
		if (ocCBp->connectCB) {
			(*ocCBp->connectCB)(req, status);
		}
//		ocCBp->client->socket->data = ocCBp->readerCB;
//		ocCBp->disposed = true;
	}
}

/**
 * the static callback called by the C-level routines in uv.. it's main job is to call a C++ level callback to do the work with higher layers
 *
 * from uv.h
 * typedef void (*uv_close_cb)(uv_handle_t* handle);
 */
void
UVEventLoop::OnClose(uv_handle_t* handle)
{
	DEBUG_OUT("UVEventLoop::OnClose() ");
//	UVCloser* ocCBp=nullptr;
	UVReader* ocCBp=nullptr;
	if (handle) {
//		ocCBp=static_cast<UVCloser*>(handle->data);
		ocCBp=static_cast<UVReader*>(handle->data);
		if (ocCBp) {
			if (ocCBp->closerCB) {
				(*ocCBp->closerCB)(handle);
			}
			ocCBp->disposed = true;
			handle->data = nullptr;
		}
	}
}

/**
 * the static callback called by the C-level routines in uv. it's main job is to call a C++ level callback to do the work with higher layers
 *
 * from uv.h
 * typedef void (*uv_getaddrinfo_cb)(uv_getaddrinfo_t* req,
 *								  int status,
 *								  struct addrinfo* res);
 */
void
UVEventLoop::OnResolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
	UVResolver* ocCBp=nullptr;
	if (resolver) {
		ocCBp=static_cast<UVResolver*>(resolver->data);
	}
	sockaddr adr;
	if (status >= 0 && res->ai_addr) {
		adr = *res->ai_addr;
		if (ocCBp && ocCBp->client && ocCBp->client->address) {
			*ocCBp->client->address = adr;
		}
		uv_freeaddrinfo(res);
	}
	if (ocCBp) {
		if (ocCBp->bindCB) {
			(*ocCBp->bindCB)(&adr, status);
		}
		ocCBp->disposed = true;
	}
}

/**
 * UVTCPClient UVForwards.h
 *   simple class for carrying around the basic permanent structures for maintaining a tcp connection ... base class of most things that would use UVEventLoop for tcp
 */
UVTCPClient::UVTCPClient()
{
	socket = new uv_tcp_t();
	address = new sockaddr();
	DEBUG_OUT("UVTCPClient::UVTCPClient() " << sizeof(uv_tcp_t) << ", " << sizeof(sockaddr));
}

UVTCPClient::~UVTCPClient()
{
	DEBUG_OUT("UVTCPClient::~UVTCPClient() " << sizeof(uv_tcp_t) << ", " << sizeof(sockaddr));
	delete socket;
	delete address;
}

std::string
UVTCPClient::IP4Addr() const
{
	std::string s;
	char addr[17] = {'\0'};
	uv_ip4_name((struct sockaddr_in*) address, addr, 16);
	s.assign(addr, strlen(addr));
	return s;
}
