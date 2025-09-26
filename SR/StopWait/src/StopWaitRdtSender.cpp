
#include "Global.h"
#include "StopWaitRdtSender.h"


SRSender::SRSender():seqSize(8),windowSize(4),base(0),nextSeqNum(0),sendBuf(new pair<bool,Packet>),waitingState(false)
{
	initWindow();
}

SRSender::SRSender(int seqSize, int winSize):seqSize(seqSize),windowSize(winSize),base(0),nextSeqNum(0),sendBuf(new pair<bool,Packet>),waitingState(false)
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
		sendBuf[i].first = false; //窗口初始时均为空
	}
}

bool SRSender::isInWindow(int seqNum)//判断序号是否在发送窗口内
{
	if (base <= nextSeqNum) { //窗口未环绕
		if (seqNum >= base && seqNum < base + windowSize) {
			return true;
		}
		else {
			return false;
		}
	}
	else { //窗口环绕
		if (seqNum >= base || seqNum < (base + windowSize) % seqSize) {
			return true;
		}
		else {
			return false;
		}
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
		else if(isInWindow(i)&&sendBuf[i].first==true)
			cout<<"发送并确认 ";		
	}
	cout << endl;
}

bool SRSender::getWaitingState() {
	return (base+windowSize)%seqSize==nextSeqNum%seqSize; //发送窗口已满
}

bool SRSender::send(const Message &message)
{
	if(getWaitingState())
	{
		cout << "SRSender: 发送窗口已满，拒绝发送报文" << endl;
		return false;
	}

	sendBuf[nextSeqNum].second.acknum = -1;
	sendBuf[nextSeqNum].second.seqnum = nextSeqNum;
	memcpy(sendBuf[nextSeqNum].second.payload, message.data, sizeof(message.data));
	sendBuf[nextSeqNum].second.checksum = pUtils->calculateCheckSum(sendBuf[nextSeqNum].second);
	
	pUtils->printPacket("SRSender: 发送报文", sendBuf[nextSeqNum].second);
	cout<<"发送前窗口状态: ";
	print();
	pns->sendToNetworkLayer(RECEIVER, sendBuf[nextSeqNum].second); //发送报文到网络层

	pns->stopTimer(SENDER, nextSeqNum); //先停止定时器，以防定时器已经启动
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
		pUtils->printPacket("SRSender: 收到确认报文", ackPct);
		if (isInWindow(ackPct.acknum) && sendBuf[ackPct.acknum].first == false) { //确认号在发送窗口内，且未被确认
			sendBuf[ackPct.acknum].first = true; //标记该报文已被确认
			cout<<"收到确认前窗口状态: ";
			print();
			while (sendBuf[base].first == true) { //移动发送窗口
				sendBuf[base].first = false; //清除该位置
				base = (base + 1) % seqSize;
				pns->stopTimer(SENDER, base); //停止该报文的定时器
			}
			cout<<"收到确认后窗口状态: ";
			print();
			cout << endl;
		}
		else {
			cout << "SRSender: 确认号不在发送窗口内，或已被确认，忽略该确认报文" << endl;
		}
	}
	else {
		pUtils->printPacket("SRSender: 收到损坏的确认报文", ackPct);
		cout << "SRSender: 确认报文校验错误，忽略该确认报文" << endl;
	}
}

void SRSender::timeoutHandler(int seqNum)
{
	cout << "SRSender: 定时器超时，重传序号为 " << seqNum << " 的报文" << endl;
	pUtils->printPacket("SRSender: 重传报文", sendBuf[seqNum].second);
	pns->sendToNetworkLayer(RECEIVER, sendBuf[seqNum].second); //重传报文到网络层
	pns->stopTimer(SENDER, seqNum); //先停止定时器，以防定时器已经启动
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum); //重新启动定时器
	pUtils->printPacket("SRSender: 重传后报文", sendBuf[seqNum].second);
	cout << endl;
}