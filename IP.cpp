#include "IP.h"
#include <algorithm>

void ISocketSender::proc_calc_checksum(IDatagram* SendDatagram) {
	//TODO: CHECK SUM PROCEDURE
}

void ISocketSender::proc_print_datagram(IDatagram* SendDatagram) {
	//Transport time is 4 seconds (4 rout steps)
	SendDatagram->Header->TimeToLive -= 4;

	cout << "Datagram [FRAGMENT/FULL] is sent!" << endl;
	cout << "Datagram with Version: " << (int)SendDatagram->Header->Version << endl;
	cout << "Datagram with IHL: " << (int)SendDatagram->Header->IHL << endl;
	cout << "Datagram with Total length: " << (int)SendDatagram->Header->TotalLength << endl;
	cout << "Datagram with Identification: " << (int)SendDatagram->Header->Identification << endl;
	cout << "Datagram with Flag: " << (int)SendDatagram->Header->Flags << endl;
	cout << "Datagram with Fragment Offset: " << (int)SendDatagram->Header->Offset << endl;
	cout << "Datagram with Time: " << (int)SendDatagram->Header->TimeToLive << endl;
	cout << "Datagram with Protocol: " << (int)SendDatagram->Header->Protocol << endl << endl;

	SendingDatagrams.push_back(SendDatagram);
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

void ISocketSender::proc_fragmentation(IDatagram* SendDatagram) {
	if (SendDatagram->Header->TotalLength <= MTU) { proc_print_datagram(SendDatagram); return; }
	//IF FLAG DF == 1
	else if ((SendDatagram->Header->Flags & 0x02) == 0x02) {
		delete SendDatagram;
		return;
	}
	//Produce the first fragment = SendDatagram
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

		//MF flag
		OldSendDatagram->Header->Flags = 0x0 | (SendDatagram->Header->Flags & 0x01);

		//(3)
		uint16_t NFB = (MTU - SendDatagram->Header->IHL * 4) / 8;

		//(4) Attach the first NFB*8 data octets
		vector<uint8_t>* FirstFragmentData = new vector<uint8_t>((size_t)NFB * 8, 0x00);
		memcpy(
			FirstFragmentData->data(),
			SendDatagram->Data->data(),
			(size_t)NFB * 8
		);
		SendDatagram->Data = FirstFragmentData;

		//(5) Correct the header
		//MF <- 1; TL <- (IHL*4)+(NFB*8)
		//Recompute Checksum
		SendDatagram->Header->Flags = SendDatagram->Header->Flags | 0x01;
		SendDatagram->Header->TotalLength = (SendDatagram->Header->IHL * 4) + (NFB * 8);
		proc_calc_checksum(SendDatagram);

		//(6) Submit fragment to the next step in datagram processing
		proc_print_datagram(SendDatagram);

		//Produce the second fragment
		//(7) Selectively copy the internet header (some options are not copied, see option definition)
		IDatagram* SecondFragmentSendDatagram = new IDatagram;
		SecondFragmentSendDatagram->Header = new IDatagramHeader;

		SecondFragmentSendDatagram->Header->Version = OldSendDatagram->Header->Version;
		SecondFragmentSendDatagram->Header->Identification = OldSendDatagram->Header->Identification;
		SecondFragmentSendDatagram->Header->TimeToLive = OldSendDatagram->Header->TimeToLive;
		SecondFragmentSendDatagram->Header->Protocol = OldSendDatagram->Header->Protocol;

		//(8) Append the remaining data
		vector<uint8_t>* SecondFragmentData = new vector<uint8_t>(OldSendDatagram->Data->size() - ((size_t)NFB * 8), 0x00);
		memcpy(
			SecondFragmentData->data(),
			OldSendDatagram->Data->data() + ((size_t)NFB * 8),
			OldSendDatagram->Data->size() - ((size_t)NFB * 8)
		);

		SecondFragmentSendDatagram->Data = SecondFragmentData;

		//(9) Correct the header:
		//IHL <- (((OIHL*4)-(length of options not copied))+3)/4
		SecondFragmentSendDatagram->Header->IHL = ((OldSendDatagram->Header->IHL * 4 - 0) + 3) / 4;
		//TL <- OTL - NFB*8 - (OIHL-IHL)*4)
		SecondFragmentSendDatagram->Header->TotalLength =
			OldSendDatagram->Header->TotalLength -
			NFB * 8 -
			(OldSendDatagram->Header->IHL - SecondFragmentSendDatagram->Header->IHL) * 4;
		//FO <- OFO + NFB
		SecondFragmentSendDatagram->Header->Offset =
			OldSendDatagram->Header->Offset + NFB;
		//MF <- OMF
		SecondFragmentSendDatagram->Header->Flags = OldSendDatagram->Header->Flags & 0x01;
		//Recompute Checksum
		proc_calc_checksum(SecondFragmentSendDatagram);

		//(10) Submit fragment to the fragmentation test
		delete OldSendDatagram;
		proc_fragmentation(SecondFragmentSendDatagram);
	}
}

ISocketReceiver::~ISocketReceiver() { DestroyTimerThread = true; }

void ISocketReceiver::CreateTimerThread() {
	if (DestroyTimerThread == true) {
		DestroyTimerThread = false;
		new thread([this]() { this->proc_timer_checker(); });
	}
}

void ISocketReceiver::proc_print_datagram(IDatagram* ReassembledReceivedDatagram) {
	cout << "Datagram [FULL] is received!" << endl;
	cout << "Datagram with Version: " << (int)ReassembledReceivedDatagram->Header->Version << endl;
	cout << "Datagram with IHL: " << (int)ReassembledReceivedDatagram->Header->IHL << endl;
	cout << "Datagram with Total length: " << (int)ReassembledReceivedDatagram->Header->TotalLength << endl;
	cout << "Datagram with Identification: " << (int)ReassembledReceivedDatagram->Header->Identification << endl;
	cout << "Datagram with Flag: " << (int)ReassembledReceivedDatagram->Header->Flags << endl;
	cout << "Datagram with Fragment Offset: " << (int)ReassembledReceivedDatagram->Header->Offset << endl;
	cout << "Datagram with Time: " << (int)ReassembledReceivedDatagram->Header->TimeToLive << endl;
	cout << "Datagram with Protocol: " << (int)ReassembledReceivedDatagram->Header->Protocol << endl << endl;

	delete ReassembledReceivedDatagram;
	return;
}

//Reassembly procedure
//Notation:
//FO - Fragment Offset
//IHL - Internet Header Length
//MF - More Fragments flag
//TTL - Time To Live
//NFB - Number of Fragment Blocks
//TL - Total Length
//TDL - Total Data Length
//BUFID - Buffer Identifier
//RCVBT - Fragment Received Bit Table
//TLB - Timer Lower Bound

void ISocketReceiver::proc_reassembly(IDatagram* ReceivedDatagram) {
	//(1) BUFID <- source|destination|protocol|identification
	string BUFID =
		to_string(ReceivedDatagram->Header->SourceAddress) +
		to_string(ReceivedDatagram->Header->DestinationAddress) +
		to_string(ReceivedDatagram->Header->Protocol) +
		to_string(ReceivedDatagram->Header->Identification);	
	//(2) IF FO = 0 AND MF = 0
	if ((ReceivedDatagram->Header->Offset == 0x00) && ((ReceivedDatagram->Header->Flags & 0x01) == 0x00)) {	
		//(3) THEN IF buffer with BUFID is allocated
		if (SOCKET_BUFFER.find(BUFID) != SOCKET_BUFFER.end()) {		
			//(4)THEN flush all reassembly resources for this BUFID
			ResourcesBuffer* BUFID_RESOURCES = SOCKET_BUFFER.at(BUFID);
			delete BUFID_RESOURCES;
			SOCKET_BUFFER.erase(BUFID);
			//(5) Submit datagram to next step; DONE
			proc_print_datagram(ReceivedDatagram);
			return;
		}
		else {
			//(5) Submit datagram to next step; DONE
			proc_print_datagram(ReceivedDatagram);
			return;
		}
	}
	ResourcesBuffer* BUFID_RESOURCES;
	//(6) ELSE IF no buffer with BUFID is allocated
	if (SOCKET_BUFFER.find(BUFID) == SOCKET_BUFFER.end()) {
		//(7) THEN allocate reassembly resources
		//with BUFID
		//TIMER <- TLB; TDL <- 0;
		BUFID_RESOURCES = new ResourcesBuffer;
		BUFID_RESOURCES->TIMER = (int)TLB; BUFID_RESOURCES->TDL = 0;
		SOCKET_BUFFER.insert(pair<string, ResourcesBuffer*>(BUFID, BUFID_RESOURCES));
	}
	BUFID_RESOURCES = SOCKET_BUFFER.at(BUFID);
	//(8) put data from fragment into data buffer with
	//BUFID from octet FO*8 to octet (TL-(IHL*4))+FO*8
	uint32_t FirstOctet = ReceivedDatagram->Header->Offset * 8;
	uint32_t LastOctet =
		ReceivedDatagram->Header->TotalLength -
		ReceivedDatagram->Header->IHL * 4 +
		ReceivedDatagram->Header->Offset * 8;
	if (BUFID_RESOURCES->DATA_BUFFER.size() < LastOctet) { BUFID_RESOURCES->DATA_BUFFER.resize(LastOctet); }
	for (uint32_t i = FirstOctet; i < LastOctet; i++) { BUFID_RESOURCES->DATA_BUFFER[i] = (*ReceivedDatagram->Data)[i - FirstOctet]; }

	//(9) set RCVBT bits from FO to FO+((TL-(IHL*4)+7)/8)
	uint32_t FirstBit = ReceivedDatagram->Header->Offset;
	uint32_t LastBit =
		ReceivedDatagram->Header->Offset +
		((ReceivedDatagram->Header->TotalLength - (ReceivedDatagram->Header->IHL * 4) + 7) / 8);
	if (BUFID_RESOURCES->RCVBT.size() < LastBit) { BUFID_RESOURCES->RCVBT.resize(LastBit); }
	for (uint32_t i = FirstBit; i < LastBit; i++) { BUFID_RESOURCES->RCVBT[i] = true; }

	//(10) IF MF = 0 THEN TDL <- TL-(IHL*4)+(F0*8)
	if ((ReceivedDatagram->Header->Flags & 0x01) == 0x00) {
		BUFID_RESOURCES->TDL =
			ReceivedDatagram->Header->TotalLength -
			ReceivedDatagram->Header->IHL * 4 +
			ReceivedDatagram->Header->Offset * 8;
	}

	//(11) IF FO = 0 THEN put header in header buffer
	if (ReceivedDatagram->Header->Offset == 0x00) { BUFID_RESOURCES->HEADER_BUFFER = *ReceivedDatagram->Header; }

	//(12) IF TDL != 0
	if (BUFID_RESOURCES->TDL != 0) {
		//(13) AND all RCVBT bits from 0 to (TDL+7)/8 are set
		for (uint32_t i = 0; i < (uint32_t)((BUFID_RESOURCES->TDL + 7) / 8); i++) {
			if (BUFID_RESOURCES->RCVBT[i] != true) {
				//(17) TIMER <- MAX(TIMER, TTL)
				BUFID_RESOURCES->TIMER = max(BUFID_RESOURCES->TIMER, (int)BUFID_RESOURCES->HEADER_BUFFER.TimeToLive);
				//(18) give up until next fragment or timer expires
				time_t current_time;
				BUFID_RESOURCES->TimerStartTime = time(&current_time);
				return;
			}
		}

		//(14) THEN TL <- TDL+(IHL*4)
		BUFID_RESOURCES->HEADER_BUFFER.TotalLength = BUFID_RESOURCES->TDL + (BUFID_RESOURCES->HEADER_BUFFER.IHL * 4);

		//(15) Submit datagram to next step
		IDatagram* ReassembledReceivedDatagram = new IDatagram;
		ReassembledReceivedDatagram->Data = new vector<uint8_t>;
		ReassembledReceivedDatagram->Header = new IDatagramHeader;

		*ReassembledReceivedDatagram->Data = BUFID_RESOURCES->DATA_BUFFER;
		*ReassembledReceivedDatagram->Header = BUFID_RESOURCES->HEADER_BUFFER;
		ReassembledReceivedDatagram->Header->Flags = 0x00;
		proc_print_datagram(ReassembledReceivedDatagram);

		//(16) free all reassembly resources
		//for this BUFID; DONE
		delete BUFID_RESOURCES;
	}
}

void ISocketReceiver::proc_timer_checker() {
	//(19) timer expires: flush all reassembly with this BUFID; DONE.
	while (DestroyTimerThread != true) {
		this_thread::sleep_for(2s);
		for (auto it = SOCKET_BUFFER.begin(); it != SOCKET_BUFFER.end(); ++it) {
			time_t current_time;
			//TIMER = TIMER-(CURRENT_TIME-START_TIME)
			it->second->TIMER -= (int)((time(&current_time) - it->second->TimerStartTime));
			//CHECK TIMER = 0
			if (it->second->TIMER >= 0) {
				delete it->second;
				SOCKET_BUFFER.erase(it->first);
			}
		}
	}
}

ResourcesBuffer::ResourcesBuffer() { time_t current_time; TimerStartTime = time(&current_time); }

ResourcesBuffer::~ResourcesBuffer() {
	DATA_BUFFER.clear();
	RCVBT.clear();
}

IDatagram::~IDatagram() {
	delete Header;
	delete Data;
}
