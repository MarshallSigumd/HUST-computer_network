
#include "Global.h"
#include "StopWaitRdtSender.h"

TCPSender::TCPSender() : WINDOW_SIZE(4), base(0), nextSeqNum(0), numOfPacInWin(0), countdown(0), curAck(-1), lastAck(-1), waitingState(false)
{
}

TCPSender::~TCPSender()
{
}

bool TCPSender::getWaitingState()
{
	return waitingState;
}

bool TCPSender::send(const Message &message)
{
	if (nextSeqNum >= base + WINDOW_SIZE)
	{
		this->waitingState = true;
		// 发送窗口已满，不能发送数据报
		return false;
	}
	else
	{
		this->waitingState = false;
		this->sw[nextSeqNum % WINDOW_SIZE].seqnum = nextSeqNum;
		this->sw[nextSeqNum % WINDOW_SIZE].acknum = -1; // 忽略该字段
		this->sw[nextSeqNum % WINDOW_SIZE].checksum = 0;
		memcpy(this->sw[nextSeqNum % WINDOW_SIZE].payload, message.data, sizeof(message.data));
		this->sw[nextSeqNum % WINDOW_SIZE].checksum = pUtils->calculateCheckSum(this->sw[nextSeqNum % WINDOW_SIZE]);

		pUtils->printPacket("发送方发送报文", this->sw[nextSeqNum % WINDOW_SIZE]);
		if (base == nextSeqNum) // 发送窗口为空，启动定时器
		{
			pns->startTimer(SENDER, Configuration::TIME_OUT, nextSeqNum);
		}

		this->nextSeqNum++;
		this->numOfPacInWin++;
		return true;
	}
}

void TCPSender::receive(const Packet &ackPkt)
{
	if (this->numOfPacInWin == 0)
	{
		return;
	}
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	cout << "curAck: " << curAck << " lastAck: " << lastAck << endl;
	if (checkSum == ackPkt.checksum && ackPkt.acknum >= base)
	{
		pUtils->printPacket("发送方正确收到确认报文", ackPkt);
		this->curAck = ackPkt.acknum;
		if (this->curAck == this->lastAck)
		{
			this->countdown++;
		}
		else
		{
			this->countdown = 1;
			this->lastAck = this->curAck;
		}
		if (this->countdown == 3)
		{ // 收到3个重复ACK，快速重传
			pUtils->printPacket("发送方收到3个重复ACK，快速重传上次发送的报文", this->sw[base % WINDOW_SIZE]);
			pns->stopTimer(SENDER, this->sw[base % WINDOW_SIZE].seqnum);
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->sw[base % WINDOW_SIZE].seqnum);
			pns->sendToNetworkLayer(SENDER, this->sw[base % WINDOW_SIZE]);
			this->countdown = 0;
		}
		if (this->curAck >= this->base)
		{
			this->base = this->curAck + 1;
			this->numOfPacInWin = this->nextSeqNum - this->base;
			if (this->base == this->nextSeqNum)
			{ // 发送窗口为空，停止定时器
				pns->stopTimer(SENDER, ackPkt.acknum);
				this->waitingState = false;
			}
			else
			{ // 发送窗口不为空，重启定时器
				pns->stopTimer(SENDER, this->sw[base % WINDOW_SIZE].seqnum);
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->sw[base % WINDOW_SIZE].seqnum);
				this->waitingState = false;
			}
		}
	}
}

void TCPSender::timeoutHandler(int seqNum)
{
	pUtils->printPacket("发送方定时器时间到，重发上次发送的报文", this->sw[base % WINDOW_SIZE]);
	pns->stopTimer(SENDER, seqNum);
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);
	pns->sendToNetworkLayer(SENDER, this->sw[base % WINDOW_SIZE]);
}
