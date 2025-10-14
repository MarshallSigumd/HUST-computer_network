
#include "Global.h"
#include "StopWaitRdtSender.h"

TCPSender::TCPSender() : base(1), nextSeqNum(1), waitingState(false), numOfPacInWin(0)
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
	if (nextSeqNum < base + Configuration::WINDOW_SIZE)
	{
		waitingState = false;
		sw[numOfPacInWin].seqnum = nextSeqNum;
		sw[numOfPacInWin].acknum = -1; // 忽略该字段
		sw[numOfPacInWin].checksum = 0;
		memcpy(sw[numOfPacInWin].payload, message.data, sizeof(message.data));
		sw[numOfPacInWin].checksum = pUtils->calculateCheckSum(sw[numOfPacInWin]);

		pUtils->printPacket("发送方发送报文", sw[numOfPacInWin]);
		if (base == nextSeqNum) // 发送窗口为空，启动定时器
		{
			pns->startTimer(SENDER, Configuration::TIME_OUT, sw[numOfPacInWin].seqnum);
		}
		pns->sendToNetworkLayer(RECEIVER, sw[numOfPacInWin]);
		numOfPacInWin++;

		if (numOfPacInWin > Configuration::WINDOW_SIZE)
		{
			waitingState = true;
		}

		nextSeqNum++;
		return true;
	}
	else
	{
		waitingState = true;
		// 发送窗口已满，不能发送数据报
		return false;
	}
}

void TCPSender::receive(const Packet &ackPkt)
{
	if (numOfPacInWin > 0)
	{
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		cout << "curAck: " << curAck << " lastAck: " << lastAck << endl;
		if (checkSum == ackPkt.checksum && ackPkt.acknum >= base)
		{
			if (ackPkt.acknum == sw[0].seqnum)
			{
				countdown++;
				if (countdown == 4)
				{
					pns->stopTimer(SENDER, sw[0].seqnum);
					pns->sendToNetworkLayer(RECEIVER, sw[0]);
					pUtils->printPacket("发送方快速重传报文", sw[0]);
					pns->startTimer(SENDER, Configuration::TIME_OUT, sw[0].seqnum);
					cout << "冗余ACK为acknum: " << ackPkt.acknum << endl;
					countdown = 0;
					return;
				}
			}
			else
			{
				countdown = 1;
			}

			if (countdown != 1)
				return;

			else
			{
				int num = ackPkt.acknum - base;
				base = ackPkt.acknum;
				pUtils->printPacket("发送方收到确认报文", ackPkt);

				if (this->base == this->nextSeqNum) // 如果确认的是发送窗口最后一个报文的ACK，那么代表sw此时无报文
				{
					pns->stopTimer(SENDER, sw[0].seqnum);
				}
				else // 如果确认的不是发送窗口最后一个报文的ACK，那么代表sw此时还有报文
				{
					pns->stopTimer(SENDER, sw[0].seqnum);
					pns->startTimer(SENDER, Configuration::TIME_OUT, sw[num].seqnum);
				}
				for (int i = 0; i < numOfPacInWin - num; i++) // 窗口报文前移
				{
					sw[i] = sw[i + num];
					cout << "now sw[i].seqnum: " << sw[i].seqnum << endl;
				}
				numOfPacInWin -= num;
			}
		}
	}
}

void TCPSender::timeoutHandler(int seqNum)
{
	pUtils->printPacket("发送方定时器时间到，重发上次发送的报文", sw[0]);
	pns->stopTimer(SENDER, sw[0].seqnum);
	pns->startTimer(SENDER, Configuration::TIME_OUT, sw[0].seqnum);
	pns->sendToNetworkLayer(RECEIVER, sw[0]);
}
