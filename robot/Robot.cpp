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
#include "Robot_Config.h"
#include "Robot_Manager.h"
#include "Robot.h"

Robot::Robot(void):login_success_(false), center_cid_(0), gate_cid_(0), cost_time_total_(0), msg_count_(0) { }

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

		if(ROBOT_MANAGER->robot_mode() == 0){
			if(now - send_tick_ >= Time_Value(0, ROBOT_MANAGER->send_msg_interval() * 1000)){
				//发送机器人消息到服务器
				//send_robot_msg();
				send_tick_ = now;
			}
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
			Block_Buffer buffer;
			make_buffer(buffer, msg_id);
			robot_struct->write_buffer(buffer);
			buffer.write_len(RPC_PKG);
			ROBOT_MANAGER->send_to_gate(gate_cid_, buffer);
		}
	}
	return 0;
}

int Robot::manual_send_msg(Args_Info &args) {
	int msg_id = args.msg_id;
	set_msg_time(msg_id);
	Block_Buffer buf;
	make_buffer(buf, msg_id);
	for (std::vector<int>::iterator iter = args.args_list.begin(); iter != args.args_list.end(); ++iter) {
		switch(*iter)
		{
		case 1:
			buf.write_int8(args.int8_args[args.cursor[0]++]);
			break;
		case 2:
			buf.write_int16(args.int16_args[args.cursor[1]++]);
			break;
		case 3:
			buf.write_int32(args.int32_args[args.cursor[2]++]);
			break;
		case 4:
			buf.write_int64(args.int64_args[args.cursor[3]++]);
			break;
		case 5:
			buf.write_bool(args.bool_args[args.cursor[4]++]);
			break;
		case 6:
			buf.write_double(args.double_args[args.cursor[5]++]);
			break;
		case 7:
			buf.write_string(args.string_args[args.cursor[6]++]);
			break;
		default:
			break;
		}
	}
	buf.write_len(RPC_PKG);
	ROBOT_MANAGER->send_to_gate(gate_cid_, buf);
	return 0;
}

int Robot::req_heartbeat(Time_Value &now) {
	Block_Buffer buf;
	make_buffer(buf, REQ_HEARTBEAT);
	buf.write_int32(now.sec());
	buf.write_len(RPC_PKG);
	ROBOT_MANAGER->send_to_gate(gate_cid_, buf);
	return 0;
}

int Robot::req_select_gate() {
	Block_Buffer buf;
	make_buffer(buf, REQ_SELECT_GATE);
	buf.write_string(robot_info_.account);
	buf.write_len(RPC_PKG);
	ROBOT_MANAGER->send_to_center(center_cid_, buf);
	return 0;
}

int Robot::req_connect_gate(std::string& account, std::string& token) {
	Block_Buffer buf;
	make_buffer(buf, REQ_CONNECT_GATE);
	buf.write_string(account);
	buf.write_string(token);
	buf.write_len(RPC_PKG);
	ROBOT_MANAGER->send_to_gate(gate_cid_, buf);
	return 0;
}

int Robot::req_fetch_role(void) {
	Block_Buffer buf;
	make_buffer(buf, REQ_FETCH_ROLE);
	buf.write_string(robot_info_.account);
	buf.write_len(RPC_PKG);
	ROBOT_MANAGER->send_to_gate(gate_cid_, buf);
	return 0;
}

int Robot::req_create_role(void) {
	Block_Buffer buf;
	make_buffer(buf, REQ_CREATE_ROLE);
	buf.write_string(robot_info_.account);
	buf.write_string(robot_info_.role_name);
	buf.write_int8(rand() % 2);
	buf.write_int8(rand() % 3);
	buf.write_len(RPC_PKG);
	ROBOT_MANAGER->send_to_gate(gate_cid_, buf);
	return 0;
}

int Robot::recv_server_msg(int msg_id, Block_Buffer &buf) {
	std::stringstream stream;
	stream << "s2c_";
	stream << msg_id;
	Robot_Struct *robot_struct = STRUCT_MANAGER->get_robot_struct(stream.str());
	if (robot_struct) {
		robot_struct->read_buffer(buf);
	}
	return 0;
}

int Robot::res_select_gate(Block_Buffer &buf) {
	int login_msec = Time_Value::gettimeofday().msec() - login_tick_.msec();
	std::string gate_ip = "";
	int16_t gate_port = 0;
	std::string token = "";
	buf.read_string(gate_ip);
	buf.read_int16(gate_port);
	buf.read_string(token);
	LOG_INFO("select gate success, gate_ip:%s, gate_port:%d, token:%s, account:%s, login_msec:%d",
			gate_ip.c_str(), gate_port, token.c_str(), robot_info_.account.c_str(), login_msec);
	ROBOT_MANAGER->connect_gate(center_cid_, gate_ip, gate_port, token, robot_info_.account) ;

	return 0;
}

int Robot::res_connect_gate(Block_Buffer &buf) {
	std::string account;
	int login_msec = Time_Value::gettimeofday().msec() - login_tick_.msec();
	LOG_INFO("connect gate success, gate_cid = %d, account = %s, login_msec = %d", gate_cid_, robot_info_.account.c_str(), login_msec);

	req_fetch_role();
	return 0;
}

int Robot::res_role_info(Block_Buffer &buf) {
	robot_info_.deserialize(buf);
	Time_Value now = Time_Value::gettimeofday();
	int login_msec = now.msec() - login_tick_.msec();
	Date_Time date(now);
	LOG_INFO("%ld:%ld:%ld login game success account:%s gate_cid = %d login_sec = %d", date.hour(), date.minute(), date.second()
			, robot_info_.account.c_str(), gate_cid_, login_msec);
	login_success_ = true;

	auto_send_msg();
	return 0;
}

int Robot::res_error_code(int msg_id, Block_Buffer &buf) {
	int16_t error_code = 0;
	buf.read_int16(error_code);
	LOG_ERROR("res_error_code, msg_id:%d, error_code:%d", msg_id, error_code);
	if (error_code == 1) {
		robot_info_.role_name = robot_info_.account;
		req_create_role();
	}
	return 0;
}

void Robot::make_buffer(Block_Buffer &buf, int msg_id) {
	buf.write_uint16(0);
	buf.write_uint8(msg_id);
}
