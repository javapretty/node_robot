/*
 * Robot_Connector.cpp
 *
 *  Created on: Nov 7, 2016
 *      Author: zhangyalei
 */

#include "Robot_Manager.h"
#include "Robot_Connector.h"

Connector::Connector(void) : cid_(-1) { }

Connector::~Connector(void) { }

int Connector::init(Endpoint_Info &endpoint_info) {
	Endpoint::init(endpoint_info);

	connect().init(this);
	network().init(this, endpoint_info.heartbeat_timeout);
	return 0;
}

int Connector::start(void) {
	network().thr_create();
	return 0;
}

int Connector::connect_server(std::string ip, int port) {
	if (ip == "" && port == 0) {
		cid_ = connect().connect(endpoint_info().server_ip.c_str(), endpoint_info().server_port);
	} else {
		cid_ = connect().connect(ip.c_str(), port);
	}
	return cid_;
}

void Connector::post_buffer(Byte_Buffer* buffer) {
	ROBOT_MANAGER->push_buffer(buffer);
}
