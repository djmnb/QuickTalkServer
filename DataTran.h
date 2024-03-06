#pragma once

#include<WinSock2.h>
#include<WS2tcpip.h>
#include<fstream>
#include<io.h>
#include<vector>
#include<fcntl.h>
#include<string>
using namespace std;



/// <summary>
/// 传输信息函数
/// </summary>
/// <param name="fd">接收端套接字</param>
/// <param name="buff">传输的信息</param>
/// <param name="len">传输信息长度</param>
/// <returns>是否传输成功</returns>
bool sendData(const int fd, const char* buff, const int len) {

	//当前传输到的位置
	char* str = new char[len+4];
	int bignum = htonl(len);

	memcpy(str, &bignum, 4);
	memcpy(str + 4, buff, len);


	//当前剩余传输长度
	int nums = len+4;

	//一直循环传输，直到要发的信息发完
	while (nums>0) {
		int length = send(fd, str, nums, 0);

		if (length < 0) {
			return false;
		}
	    
		nums -= length;
		str += length;
	}
	delete[]str;
	return true;
}

/// <summary>
/// 发送一个int类型数据
/// </summary>
/// <param name="fd"></param>
/// <param name="n"></param>
/// <returns></returns>
/// 
//bool sendInt(const int fd,int n) {
//
//	n = htonl(n);
//	int flag = sendData(fd, (const char*)&n, 4);
//	return flag;
//}

/// <summary>
/// 接收数据
/// </summary>
/// <param name="fd">发送端的套接字</param>
/// <param name="buff">存储接收内容的地方，给一个空指针变量,记得delete</param>
/// <param name="length">计划要接收数据的长度</param>
/// <returns>是否接收成功</returns>
bool recvData(const int fd,char*& buff,const int len) {

	if (buff == NULL) {
		buff = new char[len+1];
		memset(buff, '\0', sizeof(char)*(len+1));
	}

	//写入到buff中的位置
	char* str = buff;

	//剩余待写入buff的
	int nums = len;

	while (nums > 0) {

		int length = recv(fd, buff, nums,0);
		if (length < 0) {
			return false;
		}
		if (length == 0)continue;

		nums -= length;
		str += length;
	}
	buff[len] = '\0';
	return true;
}


/// <summary>
/// 接收一个int类型数据
/// </summary>
/// <param name="fd"></param>
/// <param name="n"></param>
/// <returns>是否接收成功</returns>
bool recvInt(const int fd, int* n) {

	bool flag = recvData(fd, (char*&)n, 4);
	*n = ntohl(*n);

	return flag;
}


/*
函数描述: 发送指定的字节数
函数参数:
	- fd: 通信的文件描述符(套接字)
	- msg: 待发送的原始数据
	- size: 待发送的原始数据的总字节数
函数返回值: 函数调用成功返回发送的字节数, 发送失败返回-1
*/
int writen(int fd, const char* msg, int size)
{
	const char* buf = msg;
	int count = size;
	while (count > 0)
	{
		int len = send(fd, buf, count, 0);
		if (len == -1)
		{
			closesocket(fd);
			return -1;
		}
		else if (len == 0)
		{
			continue;
		}
		buf += len;
		count -= len;
	}
	return size;
}

/*
函数描述: 发送带有数据头的数据包
函数参数:
	- cfd: 通信的文件描述符(套接字)
	- msg: 待发送的原始数据
	- len: 待发送的原始数据的总字节数
函数返回值: 函数调用成功返回发送的字节数, 发送失败返回-1
*/
int sendMsg(int cfd, char* msg, int len)
{
	if (msg == NULL || len <= 0 || cfd <= 0)
	{
		return -1;
	}
	// 申请内存空间: 数据长度 + 包头4字节(存储数据长度)
	char* data = (char*)malloc(len + 4);
	int bigLen = htonl(len);
	memcpy(data, &bigLen, 4);
	memcpy(data + 4, msg, len);
	// 发送数据
	int ret = writen(cfd, data, len + 4);
	// 释放内存
	free(data);
	return ret;
}

/*
函数描述: 接收指定的字节数
函数参数:
	- fd: 通信的文件描述符(套接字)
	- buf: 存储待接收数据的内存的起始地址
	- size: 指定要接收的字节数
函数返回值: 函数调用成功返回发送的字节数, 发送失败返回-1
*/
int readn(int fd, char* buf, int size)
{
	char* pt = buf;
	int count = size;
	while (count > 0)
	{
		int len = recv(fd, pt, count, 0);
		if (len == -1)
		{
			return -1;
		}
		else if (len == 0)
		{
			return size - count;
		}
		pt += len;
		count -= len;
	}
	return size;
}

/*
函数描述: 接收带数据头的数据包
函数参数:
	- cfd: 通信的文件描述符(套接字)
	- msg: 一级指针的地址，函数内部会给这个指针分配内存，用于存储待接收的数据，这块内存需要使用者释放
函数返回值: 函数调用成功返回接收的字节数, 发送失败返回-1
*/
int recvMsg(int cfd, char** msg)
{
	// 接收数据
	// 1. 读数据头
	int len = 0;
	if (readn(cfd, (char*)&len, 4) != 4) {
		return -1;
	}
	len = ntohl(len);
	if (len == 0)return 0;
	// 根据读出的长度分配内存，+1 -> 这个字节存储\0
	char* buf = (char*)malloc(len + 1);
	int ret = readn(cfd, buf, len);
	if (ret != len)
	{
		closesocket(cfd);
		free(buf);
		return -1;
	}
	buf[len] = '\0';
	*msg = buf;

	return ret;
}


void sendFileStrings(int fd) {


}

vector<string> getFileStrings(int fd) {
	vector<string> ans;

	return ans;
}





