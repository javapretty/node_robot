/*
* Robot_Manager.cpp
*
*  Created on: Jan 4, 2016
*      Author: zhangyalei
*/

#include <sstream>
#include "Struct_Manager.h"
#include "Robot_Timer.h"
#include "Robot_Manager.h"

Robot_Manager::Robot_Manager(void) :
	center_connector_(nullptr),
	gate_connector_(nullptr),
	center_port_(0),
	robot_count_(0),
	login_interval_(1),
	send_msg_interval_(1),
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
	Xml xml;
	bool ret = xml.load_xml("./config/robot_conf.xml");
	if(ret < 0) {
		LOG_FATAL("load config:robot_conf.xml abort");
		return -1;
	}

	TiXmlNode* log_node = xml.get_root_child_node("log");
	if(log_node) {
		bool log_file = xml.get_attr_int(log_node, "file");
		int log_level = xml.get_attr_int(log_node, "level");
		std::string folder_name = "robot";
		Log::instance()->set_log_file(log_file);
		Log::instance()->set_log_level(log_level);
		Log::instance()->set_folder_name(folder_name);
	}

	TiXmlNode* center_node = xml.get_root_child_node("center");
	if(center_node) {
		center_ip_ = xml.get_attr_str(center_node, "ip");
		center_port_ = xml.get_attr_int(center_node, "port");
	}

	TiXmlNode* robot_node = xml.get_root_child_node("robot");
	if(robot_node) {
		robot_count_ = xml.get_attr_int(robot_node, "count");
		login_interval_ = xml.get_attr_int(robot_node, "login_interval");
		send_msg_interval_ = xml.get_attr_int(robot_node, "send_msg_interval_");
	}
	robot_index_ = 0;

	//加载robot_struct
	STRUCT_MANAGER->init_struct("config/client_msg.xml", ROBOT_STRUCT);
	STRUCT_MANAGER->init_struct("config/public_struct.xml", ROBOT_STRUCT);

	//初始化center_connector
	Endpoint_Info endpoint_info;
	endpoint_info.endpoint_type = CONNECTOR;
	endpoint_info.endpoint_gid = 1;
	endpoint_info.endpoint_id = 1001;
	endpoint_info.endpoint_name = "center_connector";
	endpoint_info.server_ip = center_ip_;
	endpoint_info.server_port = center_port_;
	endpoint_info.protocol_type = TCP;
	endpoint_info.receive_timeout = 0;
	center_connector_ = connector_pool_.pop();
	center_connector_->init(endpoint_info);
	center_connector_->start();

	//初始化gate_connector
	endpoint_info.reset();
	endpoint_info.endpoint_type = CONNECTOR;
	endpoint_info.endpoint_gid = 1;
	endpoint_info.endpoint_id = 1002;
	endpoint_info.endpoint_name = "gate_connector";
	endpoint_info.server_ip = center_ip_;
	endpoint_info.server_port = center_port_;
	endpoint_info.protocol_type = TCP;
	endpoint_info.receive_timeout = 0;
	gate_connector_ = connector_pool_.pop();
	gate_connector_->init(endpoint_info);
	gate_connector_->start();

	ROBOT_TIMER->thr_create();
	return 0;
}

int Robot_Manager::process_list(void) {
	Byte_Buffer *buffer = nullptr;

	while (true) {
		notify_lock_.lock();

		//put wait in while cause there can be spurious wake up (due to signal/ENITR)
		while (buffer_list_.empty() && tick_list_.empty()) {
			notify_lock_.wait();
		}

		buffer = buffer_list_.pop_front();
		if (buffer != nullptr) {
			process_buffer(*buffer);
		}
		if (!tick_list_.empty()) {
			tick_list_.pop_front();
			tick();
		}

		notify_lock_.unlock();
	}

	return 0;
}

int Robot_Manager::process_buffer(Byte_Buffer &buffer) {
	int endpoint_id = 0;
	int32_t cid = 0;
	uint8_t compress = 0;
	uint8_t client_msg = 0;
	uint8_t msg_id = 0;
	buffer.read_int32(endpoint_id);
	buffer.read_int32(cid);
	buffer.read_uint8(compress);
	buffer.read_uint8(client_msg);
	buffer.read_uint8(msg_id);

	Robot *robot = nullptr;
	if(msg_id == RES_SELECT_GATE) {
		robot = get_center_robot(cid);
	}
	else {
		robot = get_gate_robot(cid);
	}
	Bit_Buffer buf;
	buf.set_ary(buffer.get_read_ptr(), buffer.readable_bytes());
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

int Robot_Manager::tick(void) {
	Time_Value now(Time_Value::gettimeofday());
	server_tick_ = now;
	login_tick(now);
	robot_tick(now);

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
	for (Cid_Robot_Map::iterator it = center_robot_map_.begin(); it != center_robot_map_.end(); ++it) {
		it->second->tick(now);
	}

	return 0;
}

Robot *Robot_Manager::connect_center(const char *account) {
	//连接center_server
	int center_cid = center_connector_->connect_server();
	if (center_cid < 2) {
		LOG_ERROR("center_cid = %d", center_cid);
		return nullptr;
	}

	Robot *robot = robot_pool_.pop();
	if (!robot) {
		LOG_FATAL("robot_pool_ pop return 0");
	}
	robot->reset();
	robot->set_center_cid(center_cid);
	if(account == nullptr) {
		std::stringstream account_stream;
		int rand_num = random() % 1000000;
		account_stream << "robot" << robot_index_++ << "_" << rand_num;
		robot->robot_info().account = account_stream.str();
	}
	else{
		robot->robot_info().account = account;
	}
	center_robot_map_.insert(std::make_pair(center_cid, robot));

	robot->req_select_gate();
	return robot;
}

int Robot_Manager::connect_gate(int center_cid, const char* gate_ip, int gate_port, std::string& token, std::string& account) {
	//连接gate_server
	int gate_cid = gate_connector_->connect_server(gate_ip, gate_port);
	if (gate_cid < 2) {
		LOG_ERROR("gate_cid = %d", gate_cid);
		return -1;
	}

	Cid_Robot_Map::iterator robot_iter = center_robot_map_.find(center_cid);
	if (robot_iter == center_robot_map_.end()) {
		LOG_ERROR("cannot find center_cid = %d robot", center_cid);
		return -1;
	}

	Robot *robot = robot_iter->second;
	robot->set_gate_cid(gate_cid);
	gate_robot_map_.insert(std::make_pair(gate_cid, robot));

	robot->req_connect_gate(account, token);
	return 0;
}

int Robot_Manager::send_to_center(int cid, int msg_id, Bit_Buffer &buffer)  {
	if (cid < 2) {
		LOG_ERROR("cid = %d", cid);
		return -1;
	}

	Byte_Buffer buf;
	buf.write_uint16(0);
	buf.write_uint8(msg_id);
	buf.copy(buffer.data(), buffer.get_byte_size());
	buf.write_len(RPC_PKG);
	return center_connector_->send_buffer(cid, buf);
}

int Robot_Manager::send_to_gate(int cid, int msg_id, Bit_Buffer &buffer)  {
	if (cid < 2) {
		LOG_ERROR("cid = %d", cid);
		return -1;
	}

	Byte_Buffer buf;
	buf.write_uint16(0);
	buf.write_uint8(msg_id);
	buf.copy(buffer.data(), buffer.get_byte_size());
	buf.write_len(RPC_PKG);
	return gate_connector_->send_buffer(cid, buf);
}

Robot* Robot_Manager::get_center_robot(int cid) {
	Cid_Robot_Map::iterator iter = center_robot_map_.find(cid);
	if (iter == center_robot_map_.end()) {
		LOG_ERROR("cannot find center_robot, cid = %d", cid);
		return nullptr;
	}

	return iter->second;
}

Robot* Robot_Manager::get_gate_robot(int cid) {
	Cid_Robot_Map::iterator iter = gate_robot_map_.find(cid);
	if (iter == gate_robot_map_.end()) {
		LOG_ERROR("cannot find gate_robot, cid = %d", cid);
		return nullptr;
	}

	return iter->second;
}

int Robot_Manager::print_report(void) {
	uint64_t cost_time = 0;
	uint64_t msg_count = 0;
	FILE *fp = fopen("./report.txt", "ab+");
	for(Cid_Robot_Map::iterator iter = center_robot_map_.begin();
			iter != center_robot_map_.end(); iter++){
		cost_time += (iter->second)->get_cost_time();
		msg_count += (iter->second)->get_msg_count();
	}
	char temp[512] = {};
	sprintf(temp, "%ld player login, %lu msg recv, average cost time: %lu\n",
			center_robot_map_.size(), msg_count, cost_time / msg_count);
	fputs(temp, fp);

	return 0;
}
