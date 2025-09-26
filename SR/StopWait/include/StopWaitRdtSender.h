#pragma once
#include "RdtSender.h"

class SRSender : public RdtSender
{
private:
	const int seqSize;	  // 序号空间大小
	const int windowSize; // 发送窗口大小
	int base;					  // 发送窗口的基序号
	int nextSeqNum;				  // 下一个待发送的报文序号
	pair<bool, Packet> *sendBuf;   // 发送窗口
	bool waitingState;			  // 是否处于等待Ack的状态


private:
	void initWindow(); // 初始化发送窗口
	void print();	  // 打印发送窗口状态
	bool isInWindow(int seqNum); // 判断序号是否在发送窗口内

public:
	bool getWaitingState();
	bool send(const Message &message);	// 发送应用层下来的Message，由NetworkServiceSimulator调用,如果发送方成功地将Message发送到网络层，返回true;如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	void receive(const Packet &ackPkt); // 接受确认Ack，将被NetworkServiceSimulator调用
	void timeoutHandler(int seqNum);	// Timeout handler，将被NetworkServiceSimulator调用

public:
	SRSender();
	SRSender(int seqSize, int winSize);
	virtual ~SRSender();
};
