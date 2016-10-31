机器人自动化测试系统
	连接到服务后，可以自动收发消息，测试服务器性能

配置说明
robot_config.json
   "center_ip" : "127.0.0.1",	//login ip
   "center_port" : 7502,		//login port
   "robot_count" : 1,			//登录的机器人总数
   "login_interval" : 200,		//登录间隔,单位是毫秒
   "send_msg_interval" : 2000,	//发送消息间隔,单位是毫秒
   "robot_lifetime" : 120,		//机器人生命周期,单位是秒
   "robot_mode" : 0				//机器人模式，0是自动化模式，1是交互模式
