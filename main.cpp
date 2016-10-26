/*
 * main.cpp
 *
 *  Created on: Jan 4, 2016
 *      Author: zhangyalei
 */

#include "Robot_Manual.h"
#include "Robot_Manager.h"

int main(int argc, char *argv[]) {
	srand(Time_Value::gettimeofday().sec() + Time_Value::gettimeofday().usec());

	ROBOT_MANAGER->init();
	ROBOT_MANAGER->thr_create();

	if(ROBOT_MANAGER->robot_mode()) {
		ROBOT_MANUAL->init();
		ROBOT_MANUAL->run();
	}
	else {
		Epoll_Watcher watcher;
		watcher.loop();
	}

	return 0;
}

