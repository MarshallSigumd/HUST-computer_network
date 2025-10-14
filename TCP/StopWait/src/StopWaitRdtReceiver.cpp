
#include "Global.h"
#include "StopWaitRdtReceiver.h"

TCPReceiver::TCPReceiver() : expectSequenceNumberRcvd(1)
{
	lastAckPkt.acknum = 0;
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1; // 忽略该字段
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++)
	{
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}

TCPReceiver::~TCPReceiver()
{
}

void TCPReceiver::receive(const Packet &packet)
{
	// 检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);

	// 如果校验和正确，同时收到报文的序号等于接收方期待收到的报文序号一致
	if (checkSum == packet.checksum && expectSequenceNumberRcvd == packet.seqnum)
	{
		pUtils->printPacket("接收方正确收到发送方的报文", packet);

		// 取出Message，向上递交给应用层
		Message msg;
		memcpy(msg.data, packet.payload, sizeof(packet.payload));
		pns->delivertoAppLayer(RECEIVER, msg);

		lastAckPkt.acknum = expectSequenceNumberRcvd + 1;
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("接收方发送确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt); // 调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方

		this->expectSequenceNumberRcvd++;
	}
	else
	{
		if (packet.acknum != expectSequenceNumberRcvd)
		{
			pUtils->printPacket("报文序列号错误", packet);
		}
		else
		{
			pUtils->printPacket("检验和错误", packet);
		}
		pUtils->printPacket("ERROR：接收方发送冗余ACK", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt); // 调用模拟网络环境的sendToNetworkLayer，通过网络层发送上次的确认报文
	}
}