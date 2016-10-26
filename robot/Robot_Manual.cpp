/*
 * Robot_Manual.cpp
 *
 *  Created on: Jan 6, 2016
 *      Author: zhangyalei
 */

#include "Robot_Manual.h"
#include "Robot_Manager.h"

Robot_Manual *Robot_Manual::instance_ = nullptr;

Robot_Manual *Robot_Manual::instance(){
	if(instance_ == nullptr)
		instance_ = new Robot_Manual;
	return instance_;
}

Robot_Manual::Robot_Manual():
		args_(),
		robot_()
{

}

Robot_Manual::~Robot_Manual()
{

}

int Robot_Manual::init() {
	sleep(1);
	char account[64] = {};
	printf("\nPlease Input the Account:");
	scanf("%s", account);
	robot_ = ROBOT_MANAGER->connect_center(account);
	return 0;
}

void Robot_Manual::run(){
	sleep(2);
	while(get_cmd() != 0){
		robot_->manual_send_msg(args_);
	}
}

int Robot_Manual::get_cmd(){
	sleep(1);
	printf("\nPlease Input the Msg Id: ");
	clear();
	scanf("%d", &args_.msg_id);
	get_args();
	return args_.msg_id;
}

int Robot_Manual::get_args(){
	int type = 0;
	printf("Input the Arg Type, To the End of the Number 0\n");
	printf("类型说明：1(int8_t) 2(int16_t) 3(int32_t) 4(int64_t) 5(bool) 6(double) 7(string)\n");
	scanf("%d", &type);
	while(type != 0){
		if(type > 7 || type < 0){
			printf("Wrong type, Please Input Another type");
			scanf("%d", &type);
			continue;
		}
		args_.args_list.push_back(type);
		printf("Input the Arg Value\n");
		fill_args(type);
		printf("Input the Arg Type, To the End of the Number 0\n");
		scanf("%d", &type);
	}
	return 0;
}

int Robot_Manual::fill_args(int type){
	int64_t num;
	char str[64] = {};
	switch(type){
	case 1:
		scanf("%ld", &num);
		args_.int8_args.push_back(num);
		break;
	case 2:
		scanf("%ld", &num);
		args_.int16_args.push_back(num);
		break;
	case 3:
		scanf("%ld", &num);
		args_.int32_args.push_back(num);
		break;
	case 4:
		scanf("%ld", &num);
		args_.int64_args.push_back(num);
		break;
	case 5:
		scanf("%ld", &num);
		args_.bool_args.push_back(num);
		break;
	case 6:
		scanf("%ld", &num);
		args_.double_args.push_back(num);
		break;
	case 7:
		scanf("%s", str);
		args_.string_args.push_back(str);
		break;
	default:
		break;
	}
	return 0;
}

void Robot_Manual::clear(){
	args_.msg_id = 0;
	args_.args_list.clear();
	for(int i = 0; i < 7; i++)
	{
		args_.cursor[i] = 0;
	}
	args_.int8_args.clear();
	args_.int16_args.clear();
	args_.int32_args.clear();
	args_.int64_args.clear();
	args_.bool_args.clear();
	args_.double_args.clear();
	args_.string_args.clear();
}
