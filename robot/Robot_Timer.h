/*
 * Robot_Timer.cpp
 *
 *  Created on: Jan 4, 2016
 *      Author: zhangyalei
 */

#ifndef Robot_TIMER_H_
#define Robot_TIMER_H_

#include "Thread.h"
#include "Epoll_Watcher.h"

class Robot_Timer_Handler: public Event_Handler {
public:
	Robot_Timer_Handler(void);
	virtual ~Robot_Timer_Handler(void);

	virtual int handle_timeout(const Time_Value &tv);
};

class Robot_Timer: public Thread {
public:
	static Robot_Timer *instance(void);
	virtual void run_handler(void);

	Epoll_Watcher &watcher(void);

private:
	Robot_Timer(void);
	virtual ~Robot_Timer(void);

	void register_handler(void);

private:
	static Robot_Timer *instance_;
	Epoll_Watcher watcher_;
	Robot_Timer_Handler timer_handler_;
};

#define ROBOT_TIMER Robot_Timer::instance()

#endif /* Robot_TIMER_H_ */
