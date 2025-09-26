
#include "Global.h"
#include "StopWaitRdtReceiver.h"

SRReceiver ::SRReceiver():seqSize(8),windowSize(4),recvBuf(new pair<bool,Packet>),lastAckPkt(),base(0)
{
	initWindow();
}

SRReceiver::SRReceiver(int seqSize, int winSize):seqSize(seqSize),windowSize(winSize),recvBuf(new pair<bool,Packet>),lastAckPkt(),base(0)
{
	initWindow();
}

SRReceiver::~SRReceiver()
{
	delete[] recvBuf;
}

void SRReceiver::initWindow() {
	base = 0;
	for (int i = 0; i < windowSize; i++) {
		recvBuf[i].first = false; //窗口初始时均为空
	}
	lastAckPkt.acknum = -1;
	lastAckPkt.checksum = 0;
	memset(lastAckPkt.payload, 0, sizeof(lastAckPkt.payload));
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}

bool SRReceiver::isInWindow(int seqNum)//判断序号是否在接收窗口内
{
	if (base <= (base + windowSize - 1) % seqSize) { //窗口未环绕
		if (seqNum >= base && seqNum <= (base + windowSize - 1) % seqSize) {
			return true;
		}
		else {
			return false;
		}
	}
	else { //窗口环绕
		if (seqNum >= base || seqNum <= (base + windowSize - 1) % seqSize) {
			return true;
		}
		else {
			return false;
		}
	}
}

void SRReceiver::print() {
	printf("SRReceiver: [base=%d] ", base);
	cout<<"窗口从0到seqSize-1: ";
	for (int i = 0; i < seqSize;i++)
	{
		cout<<i;
		if(i==base)
			cout << "[ ";
		if(i==(base + windowSize) % seqSize)
			cout << "] ";
		if(isInWindow(i)==false)
			cout<<"不可用 ";
		else if(isInWindow(i)&&recvBuf[i].first==false)
			cout<<"可用未收到 ";
		else if(isInWindow(i)&&recvBuf[i].first==true)
			cout<<"收到未交付 ";
	}
	cout << endl;
}

void SRReceiver::receive(const Packet &ackPkt)
{
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	if(checkSum!=ackPkt.checksum) //校验失败
	{
		pUtils->printPacket("SRReceiver: 收到损坏的报文", ackPkt);
		return;
	}
	else
	{
		if(isInWindow(ackPkt.seqnum)==false) //不在接收窗口内
		{
			pUtils->printPacket("SRReceiver: 收到不在接收窗口内的报文", ackPkt);
			lastAckPkt.seqnum=-1;//USELESS ,just for distinguish
			lastAckPkt.acknum=ackPkt.seqnum;
			lastAckPkt.checksum=pUtils->calculateCheckSum(lastAckPkt);
			memset(lastAckPkt.payload, 0, sizeof(lastAckPkt.payload));//USELESS ,just for distinguish
			pns->sendToNetworkLayer(RECEIVER, lastAckPkt); //向发送方发送上次的确认报文
			return;
		}

		else{
			recvBuf[ackPkt.seqnum].first = true;
			recvBuf[ackPkt.seqnum].second = ackPkt;
			lastAckPkt.acknum = ackPkt.seqnum;
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			lastAckPkt.seqnum = -1;//USELESS ,just for distinguish
			memset(lastAckPkt.payload, 0, sizeof(lastAckPkt.payload));//USELESS ,just for distinguish

			pUtils->printPacket("SRReceiver: 收到正确的报文", ackPkt);
			pns->sendToNetworkLayer(RECEIVER, ackPkt); //向上递交给应用层
			while(recvBuf[base].first==true) //移动窗口
			{
				Message msg;
				memcpy(msg.data, recvBuf[base].second.payload, sizeof(recvBuf[base].second.payload));
				pns->delivertoAppLayer(RECEIVER, msg); //向上递交给应用层
				pUtils->printPacket("SRReceiver: 向上递交给应用层的报文", recvBuf[base].second);
				recvBuf[base].first = false; //清空该缓冲区
				base = (base + 1) % seqSize;
			}

			cout<<"接收后窗口状态: ";
			print();
			cout << endl;
		}
	}
}