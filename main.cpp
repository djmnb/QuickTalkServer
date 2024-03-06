#include<WinSock2.h>
#include<iostream>
#include "include/mysql.h"
#include<vector>
#include<string>
#include<string.h>
#include"DataTran.h"



#include<thread>
#include<mutex>
#include<WinSock2.h>
#include<unordered_map>
#include<condition_variable>
#include<ctime>
#include"cJSON.h"

using namespace std;

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"libmysql.lib")

mutex Lockmp;
unordered_map<string, SOCKET> mp;
enum
{
	REGIST,
	LOGIN,
	CHAT,
	BYE
};

MYSQL* sql = nullptr;

//连接数据库函数
int connectsql();

//获取查询数据库信息函数
vector<vector<string>> GetResult(string info);

//初始化网络连接
int initnet();

//创建套接字
SOCKET creatsocket();

//绑定
int bindnet(SOCKET fd);

//监听
int Listen(SOCKET fd);

//用户登录处理函数
bool LoginHandle(SOCKET fd, SOCKADDR_IN* addr);

//用户聊天处理函数
bool ChatHandel(SOCKET fd, SOCKADDR_IN* addr);

void RegistHandel(SOCKET fd, SOCKADDR_IN* addr);

int main() {


	cJSON* root;
	root = cJSON_CreateString("你好");

	char* str = cJSON_Print(root);
	connectsql();

	initnet();

	SOCKET fd = creatsocket();
	

	if (fd == SOCKET_ERROR) {
		cout << "创建套接字失败" << endl;
		return 0;
	}

	if (!bindnet(fd)) {
		cout << "绑定失败" << endl;
		return 0;
	}

	if (!Listen(fd)) {
		cout << "监听失败" << endl;
		return 0;
	}

	while (1) {

		SOCKADDR_IN addr;
		int len = sizeof(addr);
		SOCKET cfd = accept(fd, (sockaddr*)&addr, &len);
		cout << cfd << endl;

		char info[1024] = "0";
		inet_ntop(AF_INET, &addr.sin_addr, info,len);

		cout << info << " 连入服务器 " << endl;

		int choose;

		char* tt;
		recvMsg(cfd, &tt);

		choose = atoi(tt);
		delete[]tt;

		switch (choose)
		{
		case REGIST:
			new thread(RegistHandel, cfd, &addr);
			break;
		case LOGIN:
			new thread(LoginHandle, cfd, &addr);
			break;
		default:
			break;
		}
	}

	closesocket(fd);
	WSACleanup();
}

int connectsql() {

	const char* host = "localhost";
	const char* user = "root";
	const char* passwd = "mysql666.";
	const char* db = "userDB";
	const int port = 3306;

	

	sql = mysql_init(sql);

	if (sql == nullptr) {
		cout << "初始化失败" << endl;
		return 0;
	}
	if (!mysql_real_connect(sql, host, user, passwd, db, port, NULL, 0)) {
		cout << "连接数据库失败" << endl;
		return 0;
	}
	cout << "连接数据库成功" << endl;
	return 1;

	
}

vector<vector<string>> GetResult(string info) {

	

	vector<vector<string>> ans;

	//result = mysql_store_result(sql);

	MYSQL_RES* result = nullptr;
	MYSQL_ROW row;

	
	mysql_query(sql, info.c_str());


	result = mysql_store_result(sql);

	if (result == nullptr) {
		cout << "转化结果集失败" << endl;
		return ans;
	}

	int num = mysql_num_fields(result);
	while (row = mysql_fetch_row(result)) {
		vector<string> temp;
		for (int i = 0; i < num; i++) {

			if (row[i] == nullptr || strlen(row[i]) == 0) {
				temp.push_back("NULL");
				continue;
			}

			temp.push_back(row[i]);
		}

		ans.push_back(temp);

	}
	return ans;
}

int initnet() {
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
	return 1;
}

SOCKET creatsocket() {

	SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	return fd;
}

int bindnet(SOCKET fd) {

	SOCKADDR_IN addr;

	addr.sin_port = htons(10007);
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = INADDR_ANY;

	int flag = bind(fd, (sockaddr*)&addr, sizeof(addr));
	
	return flag == SOCKET_ERROR ? 0 : 1;
}

int Listen(SOCKET fd) {

	int ll = listen(fd, 128);

	return ll != SOCKET_ERROR ? 1 : 0;
}


bool LoginHandle(SOCKET fd, SOCKADDR_IN* addr) {

	cout << addr->sin_addr.S_un.S_addr << "正在验证登录信息" << endl;

	char* name = nullptr;
	char* passwd = nullptr;

	recvMsg(fd, &name);
	recvMsg(fd, &passwd);

	cout << "name:" << name << endl;
	cout << "passwd:" << passwd << endl;
	string info = string("select * from login_info where username = ") + string(name)+string(" and passwd = ")+string(passwd);
	cout << info << endl;
	

	auto ans = GetResult(info);

	if (ans.size() == 0) {

		//要发送数据
		char buff[] = "0";
		sendMsg(fd, buff, strlen(buff));

		cout << "用户信息不匹配" << endl;
		closesocket(fd);
		delete[]name;
		delete[]passwd;

		return false;
	}

	cout << "用户信息匹配" << endl;

	//要发送数据
	char buff[] = "1";
	sendMsg(fd, buff, strlen(buff));

	mp[name] = fd;

	delete[]name;
	delete[]passwd;

	ChatHandel(fd, addr);

	closesocket(fd);
	return true;
}

bool ChatHandel(SOCKET fd, SOCKADDR_IN* addr)
{

	string Sp = to_string(BYE);

	while (1) {
		char* buff;
		if (recvMsg(fd, &buff) != -1) {
			

			if (string(buff) == Sp) {

				sendMsg(fd, buff, Sp.length());

				delete[]buff;
				
				break;
			}

			cout <<fd<<"  "<< buff << endl;

			cJSON* root = cJSON_Parse(buff);
			cJSON* sendname = cJSON_GetObjectItem(root, "sendname");
			cJSON* recvname = cJSON_GetObjectItem(root, "recvname");
			cJSON* Msg = cJSON_GetObjectItem(root, "msg");

			cJSON* msg = cJSON_CreateObject();

			cJSON_AddStringToObject(msg, "sendname",sendname->valuestring);
			cJSON_AddStringToObject(msg, "msg", Msg->valuestring);

			char timet[40];

			time_t t = time(NULL);

			strftime(timet, sizeof(timet), "%H:%M:%S", localtime(&t));

			cJSON_AddStringToObject(msg, "date", timet);

			char* p = cJSON_Print(msg);

			cout << p << endl;
			if (mp.find(recvname->valuestring) != mp.end());
			sendMsg(mp[recvname->valuestring], p, strlen(p));

			delete p;
			cJSON_free(root);
			cJSON_free(msg);
		}
		else {
			break;
		}
	}
	
	return false;
}

void RegistHandel(SOCKET fd, SOCKADDR_IN* addr)
{
	cout << addr->sin_addr.S_un.S_addr << "正在注册账号" << endl;

	char* name = nullptr;
	char* passwd = nullptr;

	recvMsg(fd, &name);
	recvMsg(fd, &passwd);

	cout << "name:" << name << endl;
	cout << "passwd:" << passwd << endl;
	string info = string("select * from login_info where username = ") + string(name);
	cout << info << endl;
	

	auto ans = GetResult(info);

	if (ans.size() == 0) {

		//要发送数据
		char buff[] = "1";
		sendMsg(fd, buff, strlen(buff));

		info = string("insert into login_info(username,passwd) values(") + string(name) + "," + string(passwd) + ")";
		mysql_query(sql, info.c_str());
		cout << "注册成功" << endl;
		
		
	}
	else {
		//要发送数据
		char buff[] = "0";
		sendMsg(fd, buff, strlen(buff));
	}

	delete[]name;
	delete[]passwd;
	closesocket(fd);
}

