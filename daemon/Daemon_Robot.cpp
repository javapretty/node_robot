/*
 *		Daemon_Robot.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: zhangyalei
 */

#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <iterator>
#include <sstream>
#include "Base_Function.h"
#include "Byte_Buffer.h"
#include "Log.h"
#include "Xml.h"
#include "Daemon_Robot.h"

Daemon_Robot::Daemon_Robot(void) : robot_count_(0), pid_index_map_(get_hash_table_size(512)), robot_core_map_(get_hash_table_size(512)) { }

Daemon_Robot::~Daemon_Robot(void) { }

Daemon_Robot *Daemon_Robot::instance_;

Daemon_Robot *Daemon_Robot::instance(void) {
    if (! instance_)
        instance_ = new Daemon_Robot();
    return instance_;
}

int Daemon_Robot::init(int argc, char *argv[]) {
	Xml xml;
	bool ret = xml.load_xml("./config/daemon_conf.xml");
	if(ret < 0) {
		LOG_FATAL("load config:daemon_conf.xml abort");
		return -1;
	}

	TiXmlNode* exec_node = xml.get_root_node("exec");
	if(exec_node) {
		exec_name_ = xml.get_attr_str(exec_node, "name");
		robot_count_ = xml.get_attr_int(exec_node, "count");
	}
	
	TiXmlNode* log_node = xml.get_root_node("log");
	if(log_node) {
		std::string folder_name = "daemon_robot";
		LOG_INSTACNE->set_log_file(xml.get_attr_int(log_node, "file"));
		LOG_INSTACNE->set_log_level(xml.get_attr_int(log_node, "level"));
		LOG_INSTACNE->set_folder_name(folder_name);
	}
	LOG_INSTACNE->thr_create();

	return 0;
}

int Daemon_Robot::start(int argc, char *argv[]) {
	//注册SIGCHLD信号处理函数
	signal(SIGCHLD, sigcld_handle);

	//创建进程
	for(int i = 0; i < robot_count_; ++i) {
		fork_process(i + 1);
		Time_Value::sleep(Time_Value(0, 500 * 1000));
	}

	while(true) {
		Time_Value::sleep(Time_Value(0, 500 * 1000));
	}
	return 0;
}

int Daemon_Robot::fork_process(int robot_index) {
	std::stringstream execname_stream;
	execname_stream << exec_name_ << " --index=" << robot_index;

	std::vector<std::string> exec_str_tok;
	std::istringstream exec_str_stream(execname_stream.str().c_str());
	std::copy(std::istream_iterator<std::string>(exec_str_stream), std::istream_iterator<std::string>(), std::back_inserter(exec_str_tok));
	if (exec_str_tok.empty()) {
		printf("exec_str = %s", execname_stream.str().c_str());
	}

	const char *pathname = (*exec_str_tok.begin()).c_str();
	std::vector<const char *> vargv;
	for (std::vector<std::string>::iterator tok_it = exec_str_tok.begin(); tok_it != exec_str_tok.end(); ++tok_it) {
		vargv.push_back((*tok_it).c_str());
	}
	vargv.push_back((const char *)0);

	pid_t pid = fork();
	if (pid < 0) {
		LOG_FATAL("fork fatal, pid:%d", pid);
	} else if (pid == 0) { //child
		if (execvp(pathname, (char* const*)&*vargv.begin()) < 0) {
			LOG_FATAL("execvp %s fatal", pathname);
		}
	} else { //parent
		pid_index_map_.insert(std::make_pair(pid, robot_index));
		LOG_INFO("fork new process, pid:%d, robot_index:%d", pid, robot_index);
	}

	return pid;
}

void Daemon_Robot::sigcld_handle(int signo) {
	LOG_INFO("daemon_server receive singal:%d", signo);
	//注册SIGCHLD信号处理函数
	signal(SIGCHLD, sigcld_handle);

	int status;
	pid_t pid = wait(&status);
	if (WIFEXITED(status)) {
		//子进程正常终止
		if (WEXITSTATUS(status) != 0) {
			int error_code = WEXITSTATUS(status);
			LOG_WARN("child:%d exit with status code:%d", pid, error_code);
		}
	} 
	else if (WIFSIGNALED(status)) {
		//子进程异常终止
		int signal = WTERMSIG(status);
		LOG_WARN("child:%d killed by singal:%d", pid, signal);
	}

	if (pid < 0) {
		LOG_ERROR("wait error, pid:%d", pid);
		return;
	}
	Time_Value::sleep(Time_Value(0, 500 * 1000));
	DAEMON_ROBOT->restart_process(pid);
}

void Daemon_Robot::restart_process(int pid) {
	Pid_Index_Map::iterator iter = pid_index_map_.find(pid);
	if (iter == pid_index_map_.end()) {
		LOG_ERROR("cannot find process, pid = %d.", pid);
		return;
	}

	Robot_Core_Map::iterator core_iter = robot_core_map_.find(iter->second);
	if (core_iter != robot_core_map_.end()) {
		if (core_iter->second++ > 2000) {
			LOG_ERROR("core dump too much, robot_index = %d, core_num=%d", iter->second, core_iter->second);
			return;
		}
	} else {
		robot_core_map_.insert(std::make_pair(iter->second, 0));
	}

	fork_process(iter->second);
	pid_index_map_.erase(pid);
}
