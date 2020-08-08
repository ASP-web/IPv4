#pragma once
//RFC 791

#include <iostream>
using namespace std;

#define MTU 280

struct IDatagramHeader {
	uint8_t Version{ 0x4 };		//(4 bits) IP Version
	uint8_t IHL{};				//(4 bits) Internet Header Length (in 32 bit words) [! min value is 5]	
	uint8_t TypeOfService{};	//(8 bits) Service precedence: 
								// 1) bits 0-2: 
								//		111 - Network Control;
								//		110 - Internetwork Control;
								//		101 - CRITIC/ECP;
								//		100 - Flash Override;
								//		011 - Flash;
								//		010 - Immediate;
								//		001 - Priority;
								//		000 - Routine
								// 2) bit 3: 0 = Normal Delay; 1 = Low Delay
								// 3) bit 4: 0 = Normal Throughput; 1 = High Troughput
								// 4) bit 5: 0 = Normal Reliability; 1 = High Reliability
								// 5) bit 6-7: Reserved for Future use
	uint16_t TotalLength{};		//(16 bits) Length of the Datagram Maximum 65535 byte, Middle value is 576 bytes
	uint16_t Identification{};	//(16 bits) Aid in assembling the fragments of a datagram
	uint8_t Flags{};			//(3 bits) Control Flags
								// 1) bit 0: reserved, must be zero
								// 2) bit 1: (DF) 0 = May Fragment, 1 = Don't Fragment
								// 3) bit 2: (MF) 0 = Last Fragment, 1 = More Fragments
	uint16_t Offset{};			//(13 bits) Indicates where in the datagram this fragment belongs
								//The fragment is measured in units of 8 octets (64 bits). The firs fragment has offset zero.
	uint8_t TimeToLive{};		//(8 bits) Indicates the maximum time the datagram is allowed to remain in the internet system.
								//Time in seconds
	uint8_t Protocol{};			//(8 bits) Indicates the next level protocol
	uint16_t HeaderChecksum{ 0x0 };	//(16 bits) Checksum on header only
								//Algorithm: sum all 16 bit words in the header
	uint32_t SourceAddress{};	//(32 bits) Source Address IPv4
	uint32_t DestinationAddress{};	//(32 bits) Destination Address
	uint8_t Option{ 0x0 };			//(8 bits) IP Option
									//0x0 a single octet of option-type (0x0 end of option list)
	uint32_t Padding{ 0x0 };	//(in our case is 24 bit) Padding variable use, because the internet header ends on a 32 bit boundary 
};

struct IDatagram {
	IDatagramHeader* Header{ nullptr };
	uint8_t* Data{ nullptr };
	uint32_t DataLength{ 0 };
};


void proc_calc_checksum(IDatagram* SendDatagram) {

}

void proc_send_datagram(IDatagram* SendDatagram) {
	//Transport time is 4 seconds (4 rout steps)
	SendDatagram->Header->TimeToLive -= 4;
	
	cout << "Datagram is sent!" << endl;
	cout << "Datagram with Version: " << (int)SendDatagram->Header->Version << endl;
	cout << "Datagram with IHL: " << (int) SendDatagram->Header->IHL << endl;
	cout << "Datagram with Total length: " << (int)SendDatagram->Header->TotalLength << endl;
	cout << "Datagram with Identification: " << (int)SendDatagram->Header->Identification << endl;
	cout << "Datagram with Flag: " << (int)SendDatagram->Header->Flags << endl;
	cout << "Datagram with Fragment Offset: " << (int)SendDatagram->Header->Offset << endl;
	cout << "Datagram with Time: " << (int)SendDatagram->Header->TimeToLive << endl;
	cout << "Datagram with Protocol: " << (int)SendDatagram->Header->Protocol << endl << endl;

	delete SendDatagram->Header;
	delete SendDatagram;
	return;
}


//Fragmentation procedure
//Notation:
//FO - Fragment Offset
//IHL - Internet Header Length
//DF - Don't Fragment flag
//MF - More Fragments flag
//TL - Total Length
//OFO - Old Fragment Offset
//OIHL - Old Internet Header Length
//OMF - Old More Fragments flag
//OTL - Old Total Length
//NFB - Number of Fragment Blocks
//MTU - Maximum Transmission Unit

void proc_fragmentation(IDatagram* SendDatagram) {
	if (SendDatagram->Header->TotalLength <= MTU) { proc_send_datagram(SendDatagram); return; }
	//IF FLAG DF == 1
	else if ((SendDatagram->Header->Flags & 0x2) == 0x2) { 
		delete SendDatagram->Header;
		delete SendDatagram;
		return; 
	}
	//Produce the first fragment
	//(1) Copy the original internet header
	else {
		IDatagram* OldSendDatagram = new IDatagram;
		OldSendDatagram->Header = new IDatagramHeader;
		
		OldSendDatagram->Header->Version = SendDatagram->Header->Version;
		OldSendDatagram->Header->Identification = SendDatagram->Header->Identification;
		OldSendDatagram->Header->TimeToLive = SendDatagram->Header->TimeToLive;
		OldSendDatagram->Header->Protocol = SendDatagram->Header->Protocol;
		
		//(2)
		OldSendDatagram->Header->IHL = SendDatagram->Header->IHL;
		OldSendDatagram->Header->TotalLength = SendDatagram->Header->TotalLength;
		OldSendDatagram->Header->Offset = SendDatagram->Header->Offset;

		//Copy pointers to Data
		OldSendDatagram->Data = SendDatagram->Data;
		//Copy Data length
		OldSendDatagram->DataLength = SendDatagram->DataLength;

		//MF flag
		OldSendDatagram->Header->Flags = 0x0 | (SendDatagram->Header->Flags & 0x1);

		//(3)
		uint16_t NFB = (MTU - SendDatagram->Header->IHL * 4) / 8;
		
		//(4) Attach the first NFB*8 data octets
		SendDatagram->DataLength = NFB * 8;

		//Change start pointer to data
		OldSendDatagram->Data = OldSendDatagram->Data + (NFB * 8);
		OldSendDatagram->DataLength = OldSendDatagram->DataLength - (NFB * 8);

		//(5) Correct the header
		SendDatagram->Header->Flags = SendDatagram->Header->Flags | 0x1;
		SendDatagram->Header->TotalLength = (SendDatagram->Header->IHL * 4) + (NFB * 8);

		//Recompute Checksum
		proc_calc_checksum(SendDatagram);

		//(6) Submit fragment
		proc_send_datagram(SendDatagram);

		//Produce the second fragment
		//(7) Selectively copy the internet header
		IDatagram* SecondFragmentSendDatagram = new IDatagram;
		SecondFragmentSendDatagram->Header = new IDatagramHeader;

		SecondFragmentSendDatagram->Header->Version = OldSendDatagram->Header->Version;
		SecondFragmentSendDatagram->Header->Identification = OldSendDatagram->Header->Identification;
		SecondFragmentSendDatagram->Header->TimeToLive = OldSendDatagram->Header->TimeToLive;
		SecondFragmentSendDatagram->Header->Protocol = OldSendDatagram->Header->Protocol;
		
		SecondFragmentSendDatagram->Header->IHL = ((OldSendDatagram->Header->IHL * 4 - 0) + 3) / 4;
		
		SecondFragmentSendDatagram->Header->TotalLength =
			OldSendDatagram->Header->TotalLength -
			NFB * 8 -
			(OldSendDatagram->Header->IHL - SecondFragmentSendDatagram->Header->IHL) * 4;

		SecondFragmentSendDatagram->Header->Offset =
			OldSendDatagram->Header->Offset + NFB;

		SecondFragmentSendDatagram->Header->Flags = OldSendDatagram->Header->Flags & 0x1;

		//Recompute Checksum
		proc_calc_checksum(SecondFragmentSendDatagram);

		//(10) Submit fragment to the fragmentation test
		delete OldSendDatagram->Header;
		delete OldSendDatagram;
		proc_fragmentation(SecondFragmentSendDatagram);
	}
}