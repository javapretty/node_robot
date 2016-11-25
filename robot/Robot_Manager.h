/*
* Robot_Manager.h
*
 *  Created on: Jan 4, 2016
 *      Author: zhangyalei
*/

#ifndef ROBOT_MANAGER_H_
#define ROBOT_MANAGER_H_

#include "Buffer_List.h"
#include "Robot_Connector.h"
#include "Robot.h"

class Robot_Manager: public Thread {
	typedef Object_Pool<Connector, Mutex_Lock> Connector_Pool;
	typedef Object_Pool<Robot> Robot_Pool;
	typedef Buffer_List<Mutex_Lock> Data_List;
	typedef List<int, Mutex_Lock> Int_List;
	typedef std::unordered_map<int, Robot *> Cid_Robot_Map;
public:
	static Robot_Manager *instance(void);
	static void destroy(void);
	void run_handler(void);

	int init(void);

	int process_list();
	int process_buffer(Byte_Buffer &buffer);

	inline void push_buffer(Byte_Buffer *buffer) {
		notify_lock_.lock();
		buffer_list_.push_back(buffer);
		notify_lock_.signal();
		notify_lock_.unlock();
	}
	inline void push_tick(int tick) {
		notify_lock_.lock();
		tick_list_.push_back(tick);
		notify_lock_.signal();
		notify_lock_.unlock();
	}

	int tick(void);
	int login_tick(Time_Value &now);
	int robot_tick(Time_Value &now);

	Robot *connect_center(const char *account = nullptr);
	int connect_gate(int center_cid, const char* gate_ip, int gate_port, std::string& token, std::string& account);

	int send_to_center(int cid, int msg_id, Bit_Buffer &buffer);
	int send_to_gate(int cid, int msg_id, Bit_Buffer &buffer);

	Robot* get_center_robot(int cid);
	Robot* get_gate_robot(int cid);

	inline Time_Value &server_tick() { return server_tick_; };
	inline int send_msg_interval() { return send_msg_interval_; }

private:
	Robot_Manager(void);
	virtual ~Robot_Manager(void);
	int print_report(void);

private:
	static Robot_Manager *instance_;

	Connector_Pool connector_pool_;
	Robot_Pool robot_pool_;

	Connector *center_connector_;
	Connector *gate_connector_;

	std::string center_ip_;		//center服务器ip
	int center_port_;			//center服务器端口
	int robot_count_;			//机器人数量
	int login_interval_;		//登录间隔(毫秒)
	int send_msg_interval_;		//发送消息间隔(毫秒)
	int robot_index_;

	Time_Value server_tick_;
	Time_Value first_login_tick_;
	Time_Value last_login_tick_;

	Data_List buffer_list_;
	Int_List tick_list_;

	Cid_Robot_Map center_robot_map_;
	Cid_Robot_Map gate_robot_map_;
};

#define ROBOT_MANAGER Robot_Manager::instance()

#endif /* ROBOT_MANAGER_H_ */
