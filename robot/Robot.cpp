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

Robot::Robot(void):login_success_(false), center_cid_(0), gate_cid_(0), cost_time_(0), msg_count_(0) { }

Robot::~Robot(void) { }

void Robot::reset(void) { 
	login_success_ = false;
	center_cid_ = 0;
	gate_cid_ = 0;
	login_tick_ = Time_Value::gettimeofday();
	heart_tick_ = Time_Value::gettimeofday();
	send_tick_ = Time_Value::gettimeofday();
	robot_info_.reset();
};

int Robot::tick(Time_Value &now) {
	if (login_success_) {
		// 心跳
		if (heart_tick_ < now) {
			req_heartbeat(now);
			heart_tick_ = now + Time_Value(30, 0);
		}

		if(now - send_tick_ >= Time_Value(0, ROBOT_MANAGER->send_msg_interval() * 1000)){
			//发送消息到服务器
			//auto_send_msg();
			send_tick_ = now;
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
	buffer.write_str(robot_info_.account.c_str());
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

int Robot::req_fetch_role(void) {
	Bit_Buffer buffer;
	buffer.write_str(robot_info_.account.c_str());
	ROBOT_MANAGER->send_to_gate(gate_cid_, REQ_FETCH_ROLE, buffer);
	return 0;
}

int Robot::req_create_role(void) {
	Bit_Buffer buffer;
	buffer.write_str(robot_info_.account.c_str());
	buffer.write_str(robot_info_.role_name.c_str());
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
	std::string gate_ip = "";
	uint16_t gate_port = 0;
	std::string token = "";

	buffer.read_str(gate_ip);
	gate_port = buffer.read_uint(16);
	buffer.read_str(token);
	LOG_INFO("select gate success, gate_ip:%s, gate_port:%d, token:%s, account:%s, login_msec:%d",
			gate_ip.c_str(), gate_port, token.c_str(), robot_info_.account.c_str(), login_msec);
	ROBOT_MANAGER->connect_gate(center_cid_, gate_ip.c_str(), gate_port, token, robot_info_.account) ;

	return 0;
}

int Robot::res_connect_gate(Bit_Buffer &buffer) {
	std::string account;
	int login_msec = Time_Value::gettimeofday().msec() - login_tick_.msec();
	LOG_INFO("connect gate success, gate_cid = %d, account = %s, login_msec = %d", gate_cid_, robot_info_.account.c_str(), login_msec);

	req_fetch_role();
	return 0;
}

int Robot::res_role_info(Bit_Buffer &buffer) {
	robot_info_.role_id = buffer.read_int(64);
	buffer.read_str(robot_info_.role_name);
	buffer.read_str(robot_info_.account);
	robot_info_.level = buffer.read_uint(8);
	robot_info_.exp = buffer.read_uint(32);
	robot_info_.gender = buffer.read_uint(1);
	robot_info_.career = buffer.read_uint(2);

	Time_Value now = Time_Value::gettimeofday();
	int login_msec = now.msec() - login_tick_.msec();
	Date_Time date(now);
	LOG_INFO("%ld:%ld:%ld login game success account:%s gate_cid = %d login_sec = %d", date.hour(), date.minute(), date.second()
			, robot_info_.account.c_str(), gate_cid_, login_msec);
	login_success_ = true;

	auto_send_msg();
	return 0;
}

int Robot::res_error_code(int msg_id, Bit_Buffer &buffer) {
	uint16_t error_code = buffer.read_uint(16);
	LOG_ERROR("res_error_code, msg_id:%d, error_code:%d", msg_id, error_code);
	if (error_code == 1) {
		robot_info_.role_name = robot_info_.account;
		req_create_role();
	}
	return 0;
}
