/*
 * Robot_Define.h
 *
 *  Created on: Aug 6, 2016
 *      Author: zhangyalei
 */

#ifndef ROBOT_DEFINE_H_
#define ROBOT_DEFINE_H_

#include "boost/unordered_map.hpp"
#include <vector>
#include "Time_Value.h"
#include "Byte_Buffer.h"

typedef boost::unordered_map<int32_t, Time_Value> Msg_Cost_Time_Map;

enum Message {
	REQ_HEARTBEAT 			=	1,	//发送心跳到gate_server
	RES_HEARTBEAT 			= 1,

	REQ_SELECT_GATE 		= 2,	//选择gate_server
	RES_SELECT_GATE		= 2,

	REQ_CONNECT_GATE		=	3,	//登录gate_server
	RES_CONNECT_GATE		= 3,

	REQ_FETCH_ROLE			=	4,	//获取角色
	RES_FETCH_ROLE			=	4,

	REQ_CREATE_ROLE 		= 5,	//创建角色

	RES_ERROR_CODE			= 5,	//返回错误号
};

struct Args_Info {
	int msg_id;
	std::vector<int> args_list;
	int cursor[7];
	 //类型说明：1(int8_t) 2(int16_t) 3(int32_t) 4(int64_t) 5(bool) 6(double) 7(string)
	std::vector<int8_t> int8_args;
	std::vector<int16_t> int16_args;
	std::vector<int32_t> int32_args;
	std::vector<int64_t> int64_args;
	std::vector<bool> bool_args;
	std::vector<double> double_args;
	std::vector<std::string> string_args;
};

struct Robot_Info {
	int64_t role_id;
	std::string role_name;
	std::string account;
	int8_t level;
	int32_t exp;
	int8_t gender;
	int8_t career;

	void reset(void) {
		role_id = 0;
		role_name.clear();
		account.clear();
		level = 0;
		exp = 0;
		gender = 0;
		career = 0;
	}

	int deserialize(Byte_Buffer &buffer) {
		buffer.read_int64(role_id);
		buffer.read_string(role_name);
		buffer.read_string(account);
		buffer.read_int8(level);
		buffer.read_int32(exp);
		buffer.read_int8(gender);
		buffer.read_int8(career);
		return 0;
	}
};

#endif /* ROBOT_DEFINE_H_ */
