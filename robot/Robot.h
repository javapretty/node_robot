/*
 * Robot.h
 *
 *  Created on: Jan 4, 2016
 *      Author: zhangyalei
 */

#ifndef ROBOT_H_
#define ROBOT_H_

#include <unordered_map>
#include <vector>
#include "Event_Handler.h"
#include "Bit_Buffer.h"
#include "Time_Value.h"

typedef std::unordered_map<int32_t, Time_Value> Msg_Cost_Time_Map;

enum Message {
	REQ_HEARTBEAT 		= 1,//发送心跳到gate_server
	RES_HEARTBEAT 		= 1,

	REQ_SELECT_GATE 	= 2,//选择gate_server
	RES_SELECT_GATE		= 2,

	REQ_CONNECT_GATE	= 3,//登录gate_server
	RES_CONNECT_GATE	= 3,

	REQ_ROLE_LIST		= 4,//获取列表
	RES_ROLE_LIST		= 4,

	REQ_ENTER_GAME 		= 5,//进入游戏
	RES_ENTER_GAME		= 5,

	REQ_CREATE_ROLE 	= 6,//创建角色
};

struct Role_Info {
	int64_t role_id;
	std::string role_name;
	uint8_t gender;
	uint8_t career;
	uint8_t level;
	uint32_t combat;

	Role_Info(void) { reset(); }
	void reset(void) {
		role_id = 0;
		role_name.clear();
		gender = 0;
		career = 0;
		level = 0;
		combat = 0;
	}
};

class Robot : public Event_Handler {
public:
	Robot(void);
	virtual ~Robot(void);

	void reset(void);
	int tick(Time_Value &now);

	void set_account(std::string account) { account_ = account; }
	void set_center_cid(int cid) { center_cid_ = cid; };
	void set_gate_cid(int cid) { gate_cid_ = cid; };

	//client-->server
	int auto_send_msg();
	int req_heartbeat(Time_Value &now);
	int req_select_gate();
	int req_connect_gate(std::string& account, std::string& token);
	int req_role_list(void);
	int req_enter_game(int64_t role_id);
	int req_create_role(void);

	//server-->client
	int recv_server_msg(int msg_id, Bit_Buffer &buffer);
	int res_select_gate(Bit_Buffer &buffer);
	int res_connect_gate(Bit_Buffer &buffer);
	int res_role_list(Bit_Buffer &buffer);
	int res_enter_game(Bit_Buffer &buffer);

	inline int32_t get_cost_time() { return cost_time_; }
	inline int32_t get_msg_count() { return msg_count_; }

	void set_msg_time(int msg_id);
	Time_Value get_msg_time(int msg_id);

private:
	bool login_success_;
	int center_cid_;
	int gate_cid_;

	Time_Value login_tick_;
	Time_Value heartbeat_tick_;
	Time_Value send_msg_tick_;

	std::string account_;
	std::vector<Role_Info> role_list_;

	Msg_Cost_Time_Map msg_cost_time_map_;
	int32_t cost_time_;
	int32_t msg_count_;
};

inline void Robot::set_msg_time(int msg_id) {
	msg_cost_time_map_[msg_id] = Time_Value::gettimeofday();
}

inline Time_Value Robot::get_msg_time(int msg_id){
	Msg_Cost_Time_Map::iterator iter = msg_cost_time_map_.find(msg_id);
	if(iter != msg_cost_time_map_.end()){
		return iter->second;
	}
	return Time_Value::zero;
}

#endif /* ROBOT_H_ */
