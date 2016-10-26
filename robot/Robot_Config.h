/*
 * Robot_Config.h
 *
 *  Created on: Jan 4, 2016
 *      Author: zhangyalei
 */

#ifndef ROBOT_CONFIG_H_
#define ROBOT_CONFIG_H_

#include "Config.h"

class Robot_Config : public Config {
public:
	struct Robot_Config_Entry {
		Config_Entry robot_config;
		Config_Entry robot_msg;

		Config_Lock lock;
	};

	static Robot_Config *instance();

	void load_robot_config(void);
	const Json::Value &robot_config(void);
	const Json::Value &robot_msg(void);

private:
	Robot_Config(void);
	virtual ~Robot_Config(void);
	Robot_Config(const Robot_Config &);
	const Robot_Config &operator=(const Robot_Config &);

private:
	static Robot_Config *instance_;
	Robot_Config_Entry robot_config_;
};

#define ROBOT_CONFIG Robot_Config::instance()

#endif /* ROBOT_CONFIG_H_ */
