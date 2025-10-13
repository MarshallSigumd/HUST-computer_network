
#include "Global.h"
#include "StopWaitRdtReceiver.h"

SRReceiver ::SRReceiver():seqSize(8),windowSize(4),recvBuf(new Packet[seqSize]),bufStatus(new bool[seqSize])
{
	initWindow();
}

SRReceiver::SRReceiver(int seqSize, int winSize):seqSize(seqSize),windowSize(winSize),recvBuf(new Packet[seqSize]),bufStatus(new bool[seqSize])
{
	initWindow();
}

SRReceiver::~SRReceiver()
{
	delete[] recvBuf;
}

void SRReceiver::initWindow() {
	base = 0;
	for (int i = 0; i < seqSize; i++) {
		bufStatus[i] = false; //窗口初始时均为空
	}
	lastAckPkt.acknum = -1;
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
	memset(lastAckPkt.payload, '.', Configuration::PAYLOAD_SIZE);
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}

bool SRReceiver::isInWindow(int seqNum)//判断序号是否在接收窗口内
{
	if (base < (base + windowSize) % seqSize)//窗口未环绕
	{
		return seqNum >= base && seqNum < (base + windowSize) % seqSize;
	}
	else
	{
		return seqNum >= base || seqNum < (base + windowSize) % seqSize;
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
		else if(isInWindow(i)&&bufStatus[i]==false)
			cout<<"可用未收到 ";
		else if(isInWindow(i)&&bufStatus[i]==true)
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
			pUtils->printPacket("ERROR: 收到不在接收窗口内的报文", ackPkt);
			lastAckPkt.seqnum=-1;//USELESS ,just for distinguish
			lastAckPkt.acknum=ackPkt.seqnum;
			lastAckPkt.checksum=pUtils->calculateCheckSum(lastAckPkt);
			memset(lastAckPkt.payload, '.', Configuration::PAYLOAD_SIZE);//USELESS ,just for distinguish
			pns->sendToNetworkLayer(SENDER, lastAckPkt); //向发送方发送上次的确认报文
			return;
		}

		else{
			bufStatus[ackPkt.seqnum] = true;
			recvBuf[ackPkt.seqnum] = ackPkt;
			lastAckPkt.acknum = ackPkt.seqnum;
			lastAckPkt.seqnum = 0;//USELESS ,just for distinguish
			memset(lastAckPkt.payload, '.',sizeof(lastAckPkt.payload));//USELESS ,just for distinguish

			pUtils->printPacket("接收方发送ack", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt); //向上递交给应用层
			while(bufStatus[base]==true) //移动窗口
			{
				Message msg;
				memcpy(msg.data, recvBuf[base].payload, sizeof(recvBuf[base].payload));
				pns->delivertoAppLayer(RECEIVER, msg); //向上递交给应用层
				pUtils->printPacket("SRReceiver: 向上递交给应用层的报文", recvBuf[base]);
				bufStatus[base] = false; //清空该缓冲区
				base = (base + 1) % seqSize;
			}

			cout<<"接收后窗口状态: ";
			print();
			cout << endl;
		}
	}
}