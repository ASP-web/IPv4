#include "IP.h"
#include <iostream>
#include <vector>

using namespace std;

int main(char argc, char** argv) {
	
	/*TEST FRAGMENTATION WITH (MTU = 280) FROM RFC 791 EXAMPLE*/
	ISocketSender SENDER;

	IDatagram* MainSendDatagram = new IDatagram;
	MainSendDatagram->Header = new IDatagramHeader;

	//452 SEND DATA BYTES 
	MainSendDatagram->Data = new vector<uint8_t>(100, 0x0);
	MainSendDatagram->Data->resize(200); memset(MainSendDatagram->Data->data() + 100, 0x1, 100);
	MainSendDatagram->Data->resize(300); memset(MainSendDatagram->Data->data() + 200, 0x2, 100);
	MainSendDatagram->Data->resize(400); memset(MainSendDatagram->Data->data() + 300, 0x3, 100);
	MainSendDatagram->Data->resize(452); memset(MainSendDatagram->Data->data() + 400, 0x4, 52);

	MainSendDatagram->Header->Version = 0x4;
	MainSendDatagram->Header->IHL = 0x5;
	//TOTAL LENGTH = HEADER LENGTH + DATA LENGTH
	MainSendDatagram->Header->TotalLength = 
		(uint16_t)(MainSendDatagram->Data->size()) +
		MainSendDatagram->Header->IHL * 4;
	MainSendDatagram->Header->Identification = 0x0;
	MainSendDatagram->Header->Flags = 0x0;
	MainSendDatagram->Header->Offset = 0x0;
	MainSendDatagram->Header->TimeToLive = 123;
	MainSendDatagram->Header->Protocol = 6;

	SENDER.proc_fragmentation(MainSendDatagram);

	/*TEST REASSEBLY*/
	ISocketReceiver RECEIVER;
	for (size_t i = 0; i < SENDER.SendingDatagrams.size(); i++) { RECEIVER.proc_reassembly(SENDER.SendingDatagrams[i]); }

	return 0;
}