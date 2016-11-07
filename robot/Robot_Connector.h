/*
 * Robot_Connector.h
 *
 *  Created on: Nov 7, 2016
 *      Author: zhangyalei
 */

#ifndef ROBOT_CONNECTOR_H_
#define ROBOT_CONNECTOR_H_

#include "Endpoint.h"

class Connector: public Endpoint {
public:
	Connector(void);
	virtual ~Connector(void);

	virtual int init(Endpoint_Info &endpoint_info);
	virtual int start(void);
	int connect_server(std::string ip = "", int port = 0);

	virtual void post_buffer(Byte_Buffer* buffer);
	virtual int get_cid(void) { return cid_; }

private:
	int cid_;
};

#endif /* ROBOT_CONNECTOR_H_ */
