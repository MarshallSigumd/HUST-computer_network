#ifndef STOP_WAIT_RDT_SENDER_H
#define STOP_WAIT_RDT_SENDER_H
#include "RdtSender.h"
class TCPSender : public RdtSender
{
private:
	const int WINDOW_SIZE; // 发送窗口大小
	int base;			   // 发送窗口的左边界
	int nextSeqNum;		   // 下一个待使用的序号
	Packet *sw;			   // 发送窗口
	int numOfPacInWin;	   // 发送窗口中数据包的数
	int countdown;		   // 用于计算当前收到的重复ACK数量
	int curAck;			   // 当前收到的ACK编号
	int lastAck;		   // 上次收到的ACK编号
	bool waitingState;	   // 是否处于等待Ack的状态

public:
	bool getWaitingState();
	bool send(const Message &message);	// 发送应用层下来的Message，由NetworkServiceSimulator调用,如果发送方成功地将Message发送到网络层，返回true;如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	void receive(const Packet &ackPkt); // 接受确认Ack，将被NetworkServiceSimulator调用
	void timeoutHandler(int seqNum);	// Timeout handler，将被NetworkServiceSimulator调用

public:
	TCPSender();
	virtual ~TCPSender();
};

#endif
