/*
 * Robot.h
 *
 *  Created on: Jan 4, 2016
 *      Author: zhangyalei
 */

#ifndef ROBOT_H_
#define ROBOT_H_

#include "Event_Handler.h"
#include "Robot_Define.h"
#include "Bit_Buffer.h"

class Robot : public Event_Handler {
public:
	Robot(void);
	virtual ~Robot(void);

	void reset(void);
	int tick(Time_Value &now);

	Robot_Info &robot_info(void) { return robot_info_; }
	void set_center_cid(int cid) { center_cid_ = cid; };
	void set_gate_cid(int cid) { gate_cid_ = cid; };

	//client-->server
	int auto_send_msg();
	int req_heartbeat(Time_Value &now);
	int req_select_gate();
	int req_connect_gate(std::string& account, std::string& token);
	int req_fetch_role(void);
	int req_create_role(void);

	//server-->client
	int recv_server_msg(int msg_id, Bit_Buffer &buffer);
	int res_select_gate(Bit_Buffer &buffer);
	int res_connect_gate(Bit_Buffer &buffer);
	int res_role_info(Bit_Buffer &buffer);
	int res_error_code(int msg_id, Bit_Buffer &buffer);

	inline int32_t get_cost_time() { return cost_time_; }
	inline int32_t get_msg_count() { return msg_count_; }

	void set_msg_time(int msg_id);
	Time_Value get_msg_time(int msg_id);

private:
	bool login_success_;
	int center_cid_;
	int gate_cid_;

	Time_Value login_tick_;
	Time_Value heart_tick_;
	Time_Value send_tick_;
	Robot_Info robot_info_;

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
