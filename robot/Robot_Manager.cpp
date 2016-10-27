/*
* Robot_Manager.cpp
*
*  Created on: Jan 4, 2016
*      Author: zhangyalei
*/

#include <sstream>
#include "Struct_Manager.h"
#include "Robot_Config.h"
#include "Robot_Timer.h"
#include "Robot_Manager.h"

Robot_Manager::Robot_Manager(void) :
	center_connector_(nullptr),
	gate_connector_(nullptr),
	center_port_(0),
	robot_count_(0),
	login_interval_(1),
	send_msg_interval_(1),
	robot_lifetime_(1),
	robot_mode_(0),
	robot_index_(0),
	server_tick_(Time_Value::zero),
	first_login_tick_(Time_Value::zero),
  last_login_tick_(Time_Value::zero)
{ }

Robot_Manager::~Robot_Manager(void) { }

Robot_Manager *Robot_Manager::instance_ = 0;

Robot_Manager *Robot_Manager::instance(void) {
	if (! instance_)
		instance_ = new Robot_Manager;
	return instance_;
}

void Robot_Manager::destroy(void) {
	if (instance_) {
		delete instance_;
		instance_ = 0;
	}
}

void Robot_Manager::run_handler(void) {
	process_list();
}

int Robot_Manager::init(void) {
	ROBOT_CONFIG->load_robot_config();
	const Json::Value &robot_config = ROBOT_CONFIG->robot_config();
	center_ip_ = robot_config["center_ip"].asString();
	center_port_ = robot_config["center_port"].asInt();
	robot_count_ = robot_config["robot_count"].asInt();
	login_interval_ = robot_config["login_interval"].asInt();
	send_msg_interval_ = robot_config["send_msg_interval_"].asInt();
	robot_lifetime_ = robot_config["robot_lifetime"].asInt();
	robot_mode_ = robot_config["robot_mode"].asInt();
	robot_index_ = 0;

	//加载robot_struct
	STRUCT_MANAGER->init_struct("config/client_msg.xml", ROBOT_STRUCT);
	STRUCT_MANAGER->init_struct("config/public_struct.xml", ROBOT_STRUCT);

	//连接center_server
	Endpoint_Info endpoint_info;
	endpoint_info.endpoint_type = CONNECTOR;
	endpoint_info.endpoint_id = 1001;
	endpoint_info.endpoint_name = "center_connector";
	endpoint_info.server_ip = center_ip_;
	endpoint_info.server_port = center_port_;
	endpoint_info.protocol_type = TCP;
	center_connector_ = connector_pool_.pop();
	center_connector_->init(endpoint_info);
	center_connector_->start();

	ROBOT_TIMER->thr_create();
	return 0;
}

int Robot_Manager::process_list(void) {
	Block_Buffer *buffer = 0;

	while (1) {
		bool all_empty = true;
		if (center_connector_ && (buffer = center_connector_->block_list().pop_front()) != 0) {
			all_empty = false;
			process_block(*buffer);
		}
		if (gate_connector_ && (buffer = gate_connector_->block_list().pop_front()) != 0) {
			all_empty = false;
			process_block(*buffer);
		}
		if (!tick_list_.empty()) {
			all_empty = false;
			tick_list_.pop_front();
			tick();
		}
		if (all_empty) {
			Time_Value::sleep(Time_Value(0, 100));
		}
	}

	return 0;
}

int Robot_Manager::process_block(Block_Buffer &buf) {
	int endpoint_id = 0;
	int32_t cid = 0;
	uint8_t compress = 0;
	uint8_t client_msg = 0;
	uint8_t msg_id = 0;
	buf.read_int32(endpoint_id);
	buf.read_int32(cid);
	buf.read_uint8(compress);
	buf.read_uint8(client_msg);
	buf.read_uint8(msg_id);

	Robot *robot = get_robot(cid);
	switch (msg_id) {
	case RES_SELECT_GATE:{
		robot->res_select_gate(buf);
		break;
	}
	case RES_CONNECT_GATE: {
		robot->res_connect_gate(buf);
		break;
	}
	case RES_FETCH_ROLE: {
		robot->res_role_info(buf);
		break;
	}
	case RES_ERROR_CODE:{
		robot->res_error_code(msg_id, buf);
		break;
	}
	default: {
		robot->recv_server_msg(msg_id, buf);
		break;
	}
	}

	return 0;
}

int Robot_Manager::push_tick(int tick_v) {
	tick_list_.push_back(tick_v);
	return 0;
}

int Robot_Manager::tick(void) {
	Time_Value now(Time_Value::gettimeofday());
	server_tick_ = now;

	if(!robot_mode_){
		login_tick(now);
	}
	robot_tick(now);

	//if(now - first_login_tick_ >= Time_Value(run_time_, 0)){
	//print_report();
	//exit(0);
	//}

	return 0;
}

int Robot_Manager::login_tick(Time_Value &now) {
	if (robot_index_ >= robot_count_) {
		return 0;
	}

	if (now - last_login_tick_ > Time_Value(0, login_interval_ * 1000)) {
		last_login_tick_ = now;
		if (robot_index_ == 0) {
			first_login_tick_ = now;
		}

		connect_center();
	}

	return 0;
}

int Robot_Manager::robot_tick(Time_Value &now) {
	for (Cid_Robot_Map::iterator it = robot_map_.begin(); it != robot_map_.end(); ++it) {
		it->second->tick(now);
	}

	return 0;
}

Robot *Robot_Manager::connect_center(const char *account) {
	int center_cid = center_connector_->connect_server();
	if (center_cid < 2) {
		LOG_ERROR("center_cid = %d", center_cid);
		return nullptr;
	}
	center_connector_->set_cid(center_cid);

	Robot *robot = robot_pool_.pop();
	if (!robot) {
		LOG_FATAL("robot_pool_ pop return 0");
	}
	robot->reset();
	robot->set_center_cid(center_cid);
	if(account == nullptr){
		std::stringstream account_stream;
		int rand_num = random() % 1000;
		account_stream << "robot" << robot_index_++ << "_" << rand_num;
		robot->robot_info().account = account_stream.str();
	}
	else{
		robot->robot_info().account = account;
	}
	robot_map_.insert(std::make_pair(center_cid, robot));
	robot->req_select_gate();
	return robot;
}

int Robot_Manager::connect_gate(int center_cid, std::string& gate_ip, int gate_port, std::string& session, std::string& account) {
	//连接gate_server
	Endpoint_Info endpoint_info;
	endpoint_info.endpoint_type = CONNECTOR;
	endpoint_info.endpoint_id = 1002;
	endpoint_info.endpoint_name = "gate_connector";
	endpoint_info.server_ip = gate_ip;
	endpoint_info.server_port = gate_port;
	endpoint_info.protocol_type = TCP;
	gate_connector_ = connector_pool_.pop();
	gate_connector_->init(endpoint_info);
	gate_connector_->start();
	int gate_cid = gate_connector_->connect_server(gate_ip.c_str(), gate_port);
	if (gate_cid < 2) {
		LOG_ERROR("gate_cid = %d", gate_cid);
		return -1;
	}
	gate_connector_->set_cid(gate_cid);

	Cid_Robot_Map::iterator robot_iter = robot_map_.find(center_cid);
	if (robot_iter == robot_map_.end()) {
		LOG_ERROR("cannot find center_cid = %d robot", center_cid);
		return -1;
	}

	Robot *robot = robot_iter->second;
	robot->set_gate_cid(gate_cid);
	robot->robot_info().account = account;
	robot->req_connect_gate(account, session);
	return 0;
}

int Robot_Manager::send_to_center(int cid, Block_Buffer &buf)  {
	if (cid < 2) {
		LOG_ERROR("cid = %d", cid);
		return -1;
	}
	return center_connector_->send_block(cid, buf);
}

int Robot_Manager::send_to_gate(int cid, Block_Buffer &buf)  {
	if (cid < 2) {
		LOG_ERROR("cid = %d", cid);
		return -1;
	}
	return gate_connector_->send_block(cid, buf);
}

Robot* Robot_Manager::get_robot(int cid) {
	Cid_Robot_Map::iterator iter = robot_map_.find(cid);
	if (iter == robot_map_.end()) {
		LOG_ERROR("cannot find cid = %d robot", cid);
		return nullptr;
	}

	return iter->second;
}

int Robot_Manager::print_report(void) {
	uint64_t cost_time = 0;
	uint64_t msg_count = 0;
	FILE *fp = fopen("./report.txt", "ab+");
	for(Cid_Robot_Map::iterator iter = robot_map_.begin();
			iter != robot_map_.end(); iter++){
		cost_time += (iter->second)->get_cost_time_total();
		msg_count += (iter->second)->get_msg_count();
	}
	char temp[512] = {};
	sprintf(temp, "%ld player login, %lu msg recv, average cost time: %lu\n",
			robot_map_.size(), msg_count, cost_time / msg_count);
	fputs(temp, fp);

	return 0;
}
