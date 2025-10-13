#ifndef STOP_WAIT_RDT_RECEIVER_H
#define STOP_WAIT_RDT_RECEIVER_H
#include "RdtReceiver.h"
class SRReceiver :public RdtReceiver
{
private:
	const int seqSize;		//序号空间大小
	const int windowSize;	//接收窗口大小
	Packet lastAckPkt;//上次发送的确认报文
	Packet* const recvBuf;//分组缓存区s
	bool* const bufStatus;//分组的状态
	int base;				 //接收窗口的基序号

private:
	void initWindow(); // 初始化接收窗口
	void print();	  // 打印接收窗口状态
	bool isInWindow(int seqNum); // 判断vo序号是否在接收窗口内

public:
	SRReceiver();
	SRReceiver(int seqSize, int winSize);
	virtual ~SRReceiver();
	void receive(const Packet &packet);	//接收报文，将被NetworkService调用
};

#endif

