/*
 * Robot.cpp
 *
 *  Created on: Jan 4, 2016
 *      Author: zhangyalei
 */

#include <sstream>
#include "Base_Function.h"
#include "Robot_Struct.h"
#include "Struct_Manager.h"
#include "Robot_Manager.h"
#include "Robot.h"

Robot::Robot(void) : login_success_(false), center_cid_(0), gate_cid_(0), account_(""), cost_time_(0), msg_count_(0) { }

Robot::~Robot(void) { }

void Robot::reset(void) { 
	login_success_ = false;
	center_cid_ = 0;
	gate_cid_ = 0;
	login_tick_ = Time_Value::gettimeofday();
	heartbeat_tick_ = Time_Value::gettimeofday();
	send_msg_tick_ = Time_Value::gettimeofday();
	account_.clear();
	role_list_.clear();
};

int Robot::tick(Time_Value &now) {
	if (login_success_) {
		// 心跳
		if (heartbeat_tick_ < now) {
			heartbeat_tick_ = now + Time_Value(5, 0);
			req_heartbeat(now);
		}

		//发送消息间隔
		if(send_msg_tick_ < now){
			send_msg_tick_ = now + Time_Value(0, ROBOT_MANAGER->send_msg_interval() * 1000);
		}
	}
	return 0;
}

int Robot::auto_send_msg() {
	for (int msg_id = REQ_CREATE_ROLE + 1; msg_id < 256; ++msg_id) {
		std::stringstream stream;
		stream << "c2s_" << msg_id;
		Robot_Struct *robot_struct = STRUCT_MANAGER->get_robot_struct(stream.str());
		if (robot_struct) {
			Bit_Buffer buffer;
			robot_struct->write_bit_buffer(robot_struct->field_vec(), buffer);
			ROBOT_MANAGER->send_to_gate(gate_cid_, msg_id, buffer);
		}
	}
	return 0;
}

int Robot::req_heartbeat(Time_Value &now) {
	Bit_Buffer buffer;
	buffer.write_int(now.sec(), 32);
	ROBOT_MANAGER->send_to_gate(gate_cid_, REQ_HEARTBEAT, buffer);
	return 0;
}

int Robot::req_select_gate() {
	Bit_Buffer buffer;
	buffer.write_str(account_.c_str());
	ROBOT_MANAGER->send_to_center(center_cid_, REQ_SELECT_GATE, buffer);
	return 0;
}

int Robot::req_connect_gate(std::string& account, std::string& token) {
	Bit_Buffer buffer;
	buffer.write_str(account.c_str());
	buffer.write_str(token.c_str());
	ROBOT_MANAGER->send_to_gate(gate_cid_, REQ_CONNECT_GATE, buffer);
	return 0;
}

int Robot::req_role_list(void) {
	Bit_Buffer buffer;
	buffer.write_str(account_.c_str());
	ROBOT_MANAGER->send_to_gate(gate_cid_, REQ_ROLE_LIST, buffer);
	return 0;
}

int Robot::req_enter_game(int64_t role_id) {
	Bit_Buffer buffer;
	buffer.write_int64(role_id);
	ROBOT_MANAGER->send_to_gate(gate_cid_, REQ_ENTER_GAME, buffer);
	return 0;
}

int Robot::req_create_role(void) {
	Bit_Buffer buffer;
	buffer.write_str(account_.c_str());
	buffer.write_str(account_.c_str());
	buffer.write_uint(rand() % 2, 1);
	buffer.write_uint(rand() % 3, 2);
	ROBOT_MANAGER->send_to_gate(gate_cid_, REQ_CREATE_ROLE, buffer);
	return 0;
}

int Robot::recv_server_msg(int msg_id, Bit_Buffer &buffer) {
	std::stringstream stream;
	stream << "s2c_";
	stream << msg_id;
	Robot_Struct *robot_struct = STRUCT_MANAGER->get_robot_struct(stream.str());
	if (robot_struct) {
		robot_struct->read_bit_buffer(robot_struct->field_vec(), buffer);
	}
	return 0;
}

int Robot::res_select_gate(Bit_Buffer &buffer) {
	int login_msec = Time_Value::gettimeofday().msec() - login_tick_.msec();
	std::string server_ip = "";
	uint16_t server_port = 0;
	std::string token = "";

	buffer.read_str(server_ip);
	server_port = buffer.read_uint(16);
	buffer.read_str(token);
	LOG_INFO("select server success, server_ip:%s, server_port:%d, token:%s, account:%s, login_msec:%d",
			server_ip.c_str(), server_port, token.c_str(), account_.c_str(), login_msec);
	ROBOT_MANAGER->connect_gate(center_cid_, server_ip.c_str(), server_port, token, account_);
	return 0;
}

int Robot::res_connect_gate(Bit_Buffer &buffer) {
	std::string account;
	int login_msec = Time_Value::gettimeofday().msec() - login_tick_.msec();
	LOG_INFO("connect server success, cid = %d, account = %s, login_msec = %d", gate_cid_, account_.c_str(), login_msec);
	req_role_list();
	return 0;
}

int Robot::res_role_list(Bit_Buffer &buffer) {
	uint length = buffer.read_uint(3);
	for(uint i = 0; i < length; ++i) {
		Role_Info role_info;
		role_info.role_id = buffer.read_int(64);
		buffer.read_str(role_info.role_name);	
		role_info.gender = buffer.read_uint(1);
		role_info.career = buffer.read_uint(2);
		role_info.level = buffer.read_uint(8);
		role_info.combat = buffer.read_uint(32);
		role_list_.push_back(role_info);
	}

	if(length > 0) {
		uint index = rand() % length;
		req_enter_game(role_list_[index].role_id);
	}
	else {
		req_create_role();
	}
	return 0;
}

int Robot::res_enter_game(Bit_Buffer &buffer) {
	Time_Value now = Time_Value::gettimeofday();
	int login_msec = now.msec() - login_tick_.msec();
	Date_Time date(now);
	LOG_INFO("%ld:%ld:%ld login success account:%s cid:%d login_sec:%d", date.hour(), date.minute(), date.second() ,account_.c_str(), gate_cid_, login_msec);

	login_success_ = true;
	auto_send_msg();
	return 0;
}