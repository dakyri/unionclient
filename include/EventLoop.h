/*
 * EventLoop.h
 *
 *  Created on: May 25, 2014
 *      Author: dak
 */

#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

typedef std::function<void()> TimerCB;
typedef std::function<void()> WorkerCB;

struct Timer {
	TimerCB* tikCB;
	uint64_t delayMS;
	uint64_t repeatMs;
};

struct Worker {
	WorkerCB* queuedCB;
	WorkerCB* apresCB;
};


typedef Timer* TimerRef;
typedef Worker* WorkerRef;

class Lockable {
public:
	Lockable() {}
	virtual ~Lockable() {}

	virtual void Lock()=0;
	virtual void Unlock()=0;
};

class EventLoop: public Lockable {
public:
	EventLoop() {}
	virtual ~EventLoop() {}

	virtual TimerRef Schedule(uint64_t delayMS, uint64_t repeatMs, TimerCB cb) = 0;
	virtual void CancelTimer(TimerRef) = 0;
	virtual WorkerRef Worker(WorkerCB w, WorkerCB aw=WorkerCB())=0;
	virtual void CancelWorker(WorkerRef)=0;
};


#endif /* EVENTLOOP_H_ */
