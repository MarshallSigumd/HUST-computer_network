
#include "Global.h"
#include "StopWaitRdtSender.h"


SRSender::SRSender():seqSize(8),windowSize(4),sendBuf(new Packet[8]),status(new bool[8])
{
	initWindow();
}

SRSender::SRSender(int seqSize, int winSize):seqSize(seqSize),windowSize(winSize),sendBuf(new Packet[seqSize]),status(new bool[seqSize])
{
	initWindow();
}

SRSender::~SRSender()
{
	delete[] sendBuf;
}

void SRSender::initWindow() {
	base = 0;
	nextSeqNum = 0;
	for (int i = 0; i < windowSize; i++) {
		status[i] = false; //窗口初始时均为空
	}
}

bool SRSender::isInWindow(int seqNum)//判断序号是否在接收窗口内
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

void SRSender::print() {
	printf("SRSender: [base=%d, nextSeqNum=%d] ", base, nextSeqNum);
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

		if(isInWindow(i)&&i>=nextSeqNum)
			cout<<"可用未发送 ";
		else if(isInWindow(i)&&i<nextSeqNum)
			cout<<"发送未确认 ";
		else if(isInWindow(i)&&status[i]==true)
			cout<<"发送并确认 ";		
	}
	cout << endl;
}

bool SRSender::getWaitingState() {
	waitingState=(base+windowSize)%seqSize==nextSeqNum%seqSize;
	return waitingState;
}

bool SRSender::send(const Message &message)
{
	if(getWaitingState())
	{
		cout << "ERROR: 发送窗口已满，拒绝发送报文" << endl;
		return false;
	}

	sendBuf[nextSeqNum].acknum = -1;
	sendBuf[nextSeqNum].seqnum = nextSeqNum;
	memcpy(sendBuf[nextSeqNum].payload, message.data, sizeof(message.data));
	sendBuf[nextSeqNum].checksum = pUtils->calculateCheckSum(sendBuf[nextSeqNum]);
	
	pUtils->printPacket("SRSender: 发送报文", sendBuf[nextSeqNum]);
	cout<<"发送前窗口状态: ";
	print();
	pns->sendToNetworkLayer(RECEIVER, sendBuf[nextSeqNum]); //发送报文到网络层

	pns->startTimer(SENDER, Configuration::TIME_OUT, nextSeqNum); //启动定时器

	nextSeqNum = (nextSeqNum + 1) % seqSize; //更新下一个待发送的报文序号
	cout<<"发送后窗口状态: ";
	print();
	cout << endl;
	return true;
}

void SRSender::receive(const Packet &ackPct)
{
	int checkSum = pUtils->calculateCheckSum(ackPct);
	if (checkSum == ackPct.checksum) { //校验成功
		pns->stopTimer(SENDER, ackPct.acknum); //停止该报文的定时器
		if (isInWindow(ackPct.acknum)) { //确认号在发送窗口内，且未被确认
			status[ackPct.acknum] = true; //标记该报文已被确认
			cout<<"收到确认前窗口状态: ";
			print();
			while (status[ackPct.acknum] == true) { //移动发送窗口
				status[ackPct.acknum] = false; //清除该位置
				base = (base + 1) % seqSize;
			}
			cout<<"收到确认后窗口状态: ";
			print();
			cout << endl;
		}
	}
	else {
		pUtils->printPacket("ERROR: 确认报文校验错误，忽略该确认报文", ackPct);
	}
}

void SRSender::timeoutHandler(int seqNum)
{
	pUtils->printPacket("SRSender: 重传报文", sendBuf[seqNum]);
	pns->sendToNetworkLayer(RECEIVER, sendBuf[seqNum]); //重传报文到网络层
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum); //重新启动定时器
	pUtils->printPacket("SRSender: 重传后报文", sendBuf[seqNum]);
}