#ifndef STOP_WAIT_RDT_RECEIVER_H
#define STOP_WAIT_RDT_RECEIVER_H
#include "RdtReceiver.h"
class SRReceiver :public RdtReceiver
{
private:
	const int seqSize = 8;		//序号空间大小
	const int windowSize = 4;	//接收窗口大小
	pair<bool, Packet> *recvBuf; //接收窗口
	Packet lastAckPkt;			 //上次发送的确认报文
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

