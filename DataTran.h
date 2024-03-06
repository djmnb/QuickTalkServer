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
/// ������Ϣ����
/// </summary>
/// <param name="fd">���ն��׽���</param>
/// <param name="buff">�������Ϣ</param>
/// <param name="len">������Ϣ����</param>
/// <returns>�Ƿ���ɹ�</returns>
bool sendData(const int fd, const char* buff, const int len) {

	//��ǰ���䵽��λ��
	char* str = new char[len+4];
	int bignum = htonl(len);

	memcpy(str, &bignum, 4);
	memcpy(str + 4, buff, len);


	//��ǰʣ�ഫ�䳤��
	int nums = len+4;

	//һֱѭ�����䣬ֱ��Ҫ������Ϣ����
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
/// ����һ��int��������
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
/// ��������
/// </summary>
/// <param name="fd">���Ͷ˵��׽���</param>
/// <param name="buff">�洢�������ݵĵط�����һ����ָ�����,�ǵ�delete</param>
/// <param name="length">�ƻ�Ҫ�������ݵĳ���</param>
/// <returns>�Ƿ���ճɹ�</returns>
bool recvData(const int fd,char*& buff,const int len) {

	if (buff == NULL) {
		buff = new char[len+1];
		memset(buff, '\0', sizeof(char)*(len+1));
	}

	//д�뵽buff�е�λ��
	char* str = buff;

	//ʣ���д��buff��
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
/// ����һ��int��������
/// </summary>
/// <param name="fd"></param>
/// <param name="n"></param>
/// <returns>�Ƿ���ճɹ�</returns>
bool recvInt(const int fd, int* n) {

	bool flag = recvData(fd, (char*&)n, 4);
	*n = ntohl(*n);

	return flag;
}


/*
��������: ����ָ�����ֽ���
��������:
	- fd: ͨ�ŵ��ļ�������(�׽���)
	- msg: �����͵�ԭʼ����
	- size: �����͵�ԭʼ���ݵ����ֽ���
��������ֵ: �������óɹ����ط��͵��ֽ���, ����ʧ�ܷ���-1
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
��������: ���ʹ�������ͷ�����ݰ�
��������:
	- cfd: ͨ�ŵ��ļ�������(�׽���)
	- msg: �����͵�ԭʼ����
	- len: �����͵�ԭʼ���ݵ����ֽ���
��������ֵ: �������óɹ����ط��͵��ֽ���, ����ʧ�ܷ���-1
*/
int sendMsg(int cfd, char* msg, int len)
{
	if (msg == NULL || len <= 0 || cfd <= 0)
	{
		return -1;
	}
	// �����ڴ�ռ�: ���ݳ��� + ��ͷ4�ֽ�(�洢���ݳ���)
	char* data = (char*)malloc(len + 4);
	int bigLen = htonl(len);
	memcpy(data, &bigLen, 4);
	memcpy(data + 4, msg, len);
	// ��������
	int ret = writen(cfd, data, len + 4);
	// �ͷ��ڴ�
	free(data);
	return ret;
}

/*
��������: ����ָ�����ֽ���
��������:
	- fd: ͨ�ŵ��ļ�������(�׽���)
	- buf: �洢���������ݵ��ڴ����ʼ��ַ
	- size: ָ��Ҫ���յ��ֽ���
��������ֵ: �������óɹ����ط��͵��ֽ���, ����ʧ�ܷ���-1
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
��������: ���մ�����ͷ�����ݰ�
��������:
	- cfd: ͨ�ŵ��ļ�������(�׽���)
	- msg: һ��ָ��ĵ�ַ�������ڲ�������ָ������ڴ棬���ڴ洢�����յ����ݣ�����ڴ���Ҫʹ�����ͷ�
��������ֵ: �������óɹ����ؽ��յ��ֽ���, ����ʧ�ܷ���-1
*/
int recvMsg(int cfd, char** msg)
{
	// ��������
	// 1. ������ͷ
	int len = 0;
	if (readn(cfd, (char*)&len, 4) != 4) {
		return -1;
	}
	len = ntohl(len);
	if (len == 0)return 0;
	// ���ݶ����ĳ��ȷ����ڴ棬+1 -> ����ֽڴ洢\0
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





