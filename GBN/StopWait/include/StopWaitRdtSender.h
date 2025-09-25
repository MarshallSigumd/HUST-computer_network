#ifndef STOP_WAIT_RDT_SENDER_H
#define STOP_WAIT_RDT_SENDER_H
#include "RdtSender.h"
class GBNSender : public RdtSender
{
private:
	int base;					  // 发送窗口的基序号
	int nextSeqNum;				  // 下一个待发送的报文序号
	int expectSequenceNumberSend; // 下一个发送序号
	bool waitingState;			  // 是否处于等待Ack的状态

	Packet sw[Configuration::WINDOW_SIZE]; // 发送窗口
	int numberOfPacketsInWindow;		   // 当前在发送窗口中的报文数

public:
	bool getWaitingState();
	bool send(const Message &message);	// 发送应用层下来的Message，由NetworkServiceSimulator调用,如果发送方成功地将Message发送到网络层，返回true;如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	void receive(const Packet &ackPkt); // 接受确认Ack，将被NetworkServiceSimulator调用
	void timeoutHandler(int seqNum);	// Timeout handler，将被NetworkServiceSimulator调用

public:
	GBNSender();
	virtual ~GBNSender();
};

#endif
