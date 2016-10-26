机器人自动化测试系统
	连接到服务后，可以自动收发消息，测试服务器性能

配置说明
robot_config.json
   "login_ip" : "127.0.0.1",		//login ip
   "login_port" : 7502,				//login port
	 "gate_ip" : "127.0.0.1",		//gate ip
	 "gate_port" : 7507,   			//gate port
   "robot_count" : 1,					//登录的机器人总数
   "login_interval" : 200,			//登录间隔,单位是毫秒
   "send_msg_interval" : 2000,	//发送消息间隔,单位是毫秒
   "robot_lifetime" : 120,			//机器人生命周期,单位是秒
   "robot_mode" : 0						//机器人模式，0是自动化模式，1是交互模式

robot_msg.json
	该文件配置的是当机器人连接到服务器后，自动发的消息
	//msg_param:该字段是一个数组，按照变量定义顺序赋值，第一个元素是类型，第二个元素是值，没有参数时候数组为空
	//类型说明：0(int8_t) 1(int16_t) 2(int32_t) 3(int64_t) 4(bool) 5(double) 6(string)
	{
		"msg_id"	:	120003,
		"msg_param" : []
	}
