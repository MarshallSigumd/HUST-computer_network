
#include "Global.h"
#include "StopWaitRdtSender.h"

GBNSender::GBNSender() : base(1), nextSeqNum(1), expectSequenceNumberSend(0), waitingState(false), numberOfPacketsInWindow(0)
{
}

GBNSender::~GBNSender()
{
}

bool GBNSender::getWaitingState()
{
	return waitingState;
}

bool GBNSender::send(const Message &message)
{
	if (nextSeqNum >= base + Configuration::WINDOW_SIZE)
	{
		this->waitingState = true;
		return false;
	}
	else // 发送窗口未满，可以发送数据报
	{
		this->waitingState = false;
		this->sw[numberOfPacketsInWindow].seqnum = nextSeqNum;
		this->sw[numberOfPacketsInWindow].acknum = this->nextSeqNum;
		this->sw[numberOfPacketsInWindow].checksum = 0;
		memcpy(this->sw[numberOfPacketsInWindow].payload, message.data, sizeof(message.data));
		this->sw[numberOfPacketsInWindow].checksum = pUtils->calculateCheckSum(this->sw[numberOfPacketsInWindow]);

		pUtils->printPacket("发送方发送报文:", this->sw[numberOfPacketsInWindow]);
		if (base == nextSeqNum) // 发送窗口中第一个报文，启动定时器
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->sw[numberOfPacketsInWindow].seqnum);
		pns->sendToNetworkLayer(SENDER, this->sw[numberOfPacketsInWindow]);
		this - numberOfPacketsInWindow++;
		this->nextSeqNum++;

		if (numberOfPacketsInWindow >= Configuration::WINDOW_SIZE)
			this->waitingState = true;

		return true;
	}
}

void GBNSender::receive(const Packet &ackPkt)
{
	if (this->numberOfPacketsInWindow == 0)
		return;										  // 没有数据报在发送窗口中，直接返回
	int checkSum = pUtils->calculateCheckSum(ackPkt); // 计算校验和
	if (checkSum == ackPkt.checksum && ackPkt.acknum >= base)
	{
		int num = ackPkt.acknum - base + 1; // 确认号表示已经收到的最大序号
		base = ackPkt.acknum + 1;
		this->numberOfPacketsInWindow -= num; // 发送窗口中剩余的报文数
		pUtils->printPacket("发送方收到确认报文:", ackPkt);

		if (base = nextSeqNum) // 发送窗口中没有报文，停止定时器
			pns->stopTimer(SENDER, ackPkt.acknum);
		else // 发送窗口中还有报文，重启定时器
		{
			pns->stopTimer(SENDER, ackPkt.acknum);
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->sw[0].seqnum);
		}

		for (int i = 0; i < this->numberOfPacketsInWindow; i++)
			this->sw[i] = this->sw[i + num]; // 将发送窗口中未确认的报文前移
	}
}

void GBNSender::timeoutHandler(int seqNum)
{
	pUtils->printPacket("发送方定时器时间到，重发窗口中所有报文:", this->sw[0]);
	pns->stopTimer(SENDER, this->sw[0].seqnum);
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->sw[0].seqnum);
	for (int i = 0; i < this->numberOfPacketsInWindow; i++)
	{
		pUtils->printPacket("发送方重发报文:", this->sw[i]);
		pns->sendToNetworkLayer(RECEIVER, this->sw[i]);
	}
}