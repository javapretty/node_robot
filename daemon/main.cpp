/*
 * main.cpp
 *  Created on: Nov 11, 2016
 *      Author: zhangyalei
 */

#include <signal.h>
#include "Daemon_Robot.h"

static void sighandler(int sig_no) { exit(0); } /// for gprof need normal exit

int main(int argc, char *argv[]) {
	signal(SIGPIPE, SIG_IGN);
	signal(SIGUSR1, sighandler);

	DAEMON_ROBOT->init(argc, argv);
	DAEMON_ROBOT->start(argc, argv);
	return 0;
}
