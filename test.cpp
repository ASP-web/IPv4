#include "IP.h"
#include <iostream>

using namespace std;

int main(char argc, char** argv) {
	IDatagram* MainSendDatagram = new IDatagram;
	MainSendDatagram->Header = new IDatagramHeader;

	MainSendDatagram->DataLength = 472;
	MainSendDatagram->Data = new uint8_t[MainSendDatagram->DataLength];
	memset(MainSendDatagram->Data, 0x01, MainSendDatagram->DataLength);

	MainSendDatagram->Header->Version = 0x4;
	MainSendDatagram->Header->IHL = 0x5;
	MainSendDatagram->Header->TotalLength = MainSendDatagram->DataLength;
	MainSendDatagram->Header->Identification = 111;
	MainSendDatagram->Header->Flags = 0x0;
	MainSendDatagram->Header->Offset = 0x0;
	MainSendDatagram->Header->TimeToLive = 123;
	MainSendDatagram->Header->Protocol = 6;

	proc_fragmentation(MainSendDatagram);
	
	return 0;
}