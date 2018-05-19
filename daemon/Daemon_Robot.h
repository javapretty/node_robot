/*
 *		Daemon_Robot.h
 *
 *  Created on: Nov 25, 2016
 *      Author: zhangyalei
 */

#ifndef DAEMON_ROBOT_H_
#define DAEMON_ROBOT_H_

#include <getopt.h>
#include <string>
#include <unordered_map>
#include <vector>

class Daemon_Robot {
	//pid--robot_index
	typedef std::unordered_map<int, int> Pid_Index_Map;
	//robot_index--core_num
	typedef std::unordered_map<int, int> Robot_Core_Map;
public:
	static Daemon_Robot *instance(void);

	int init(int argc, char *argv[]);
	int start(int argc, char *argv[]);

	int fork_process(int robot_index);
	static void sigcld_handle(int signo);
	void restart_process(int pid);

private:
	Daemon_Robot(void);
	~Daemon_Robot(void);
	Daemon_Robot(const Daemon_Robot &);
	const Daemon_Robot &operator=(const Daemon_Robot &);

private:
	static Daemon_Robot *instance_;

	int robot_count_;
	std::string exec_name_;

	Pid_Index_Map pid_index_map_;
	Robot_Core_Map robot_core_map_;
};

#define DAEMON_ROBOT Daemon_Robot::instance()

#endif /* DAEMON_ROBOT_H_ */
