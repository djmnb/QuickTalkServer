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

//�������ݿ⺯��
int connectsql();

//��ȡ��ѯ���ݿ���Ϣ����
vector<vector<string>> GetResult(string info);

//��ʼ����������
int initnet();

//�����׽���
SOCKET creatsocket();

//��
int bindnet(SOCKET fd);

//����
int Listen(SOCKET fd);

//�û���¼������
bool LoginHandle(SOCKET fd, SOCKADDR_IN* addr);

//�û����촦����
bool ChatHandel(SOCKET fd, SOCKADDR_IN* addr);

void RegistHandel(SOCKET fd, SOCKADDR_IN* addr);

int main() {


	cJSON* root;
	root = cJSON_CreateString("���");

	char* str = cJSON_Print(root);
	connectsql();

	initnet();

	SOCKET fd = creatsocket();
	

	if (fd == SOCKET_ERROR) {
		cout << "�����׽���ʧ��" << endl;
		return 0;
	}

	if (!bindnet(fd)) {
		cout << "��ʧ��" << endl;
		return 0;
	}

	if (!Listen(fd)) {
		cout << "����ʧ��" << endl;
		return 0;
	}

	while (1) {

		SOCKADDR_IN addr;
		int len = sizeof(addr);
		SOCKET cfd = accept(fd, (sockaddr*)&addr, &len);
		cout << cfd << endl;

		char info[1024] = "0";
		inet_ntop(AF_INET, &addr.sin_addr, info,len);

		cout << info << " ��������� " << endl;

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
		cout << "��ʼ��ʧ��" << endl;
		return 0;
	}
	if (!mysql_real_connect(sql, host, user, passwd, db, port, NULL, 0)) {
		cout << "�������ݿ�ʧ��" << endl;
		return 0;
	}
	cout << "�������ݿ�ɹ�" << endl;
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
		cout << "ת�������ʧ��" << endl;
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

	cout << addr->sin_addr.S_un.S_addr << "������֤��¼��Ϣ" << endl;

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

		//Ҫ��������
		char buff[] = "0";
		sendMsg(fd, buff, strlen(buff));

		cout << "�û���Ϣ��ƥ��" << endl;
		closesocket(fd);
		delete[]name;
		delete[]passwd;

		return false;
	}

	cout << "�û���Ϣƥ��" << endl;

	//Ҫ��������
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
	cout << addr->sin_addr.S_un.S_addr << "����ע���˺�" << endl;

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

		//Ҫ��������
		char buff[] = "1";
		sendMsg(fd, buff, strlen(buff));

		info = string("insert into login_info(username,passwd) values(") + string(name) + "," + string(passwd) + ")";
		mysql_query(sql, info.c_str());
		cout << "ע��ɹ�" << endl;
		
		
	}
	else {
		//Ҫ��������
		char buff[] = "0";
		sendMsg(fd, buff, strlen(buff));
	}

	delete[]name;
	delete[]passwd;
	closesocket(fd);
}

