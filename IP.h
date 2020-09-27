#pragma once
//RFC 791
//THAT PROJECT CONSIST REFERENCES TO
//lib <https://github.com/rxi/map> by 2014 rxi
//lib <https://github.com/goldsborough/vector> by Peter Goldsborough

//#include <iostream>
//#include <string>
#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "vector/vector.h"
#include "map/map.h"
//#include <map>
//#include <thread>
//#include <chrono>
//#include <ctime>

#define MTU 280

//IDatagramHeader Structure and Methods declaration
typedef struct IDatagramHeader
{
    uint8_t Version;	        //(4 bits) IP Version
    uint8_t IHL;				//(4 bits) Internet Header Length (in 32 bit words) [! min value is 5]
    uint8_t TypeOfService;	    //(8 bits) Service precedence:
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
    uint16_t TotalLength;		//(16 bits) Length of the Datagram Maximum 65535 byte, Middle value is 576 bytes
    uint16_t Identification;	//(16 bits) Aid in assembling the fragments of a datagram
    uint8_t Flags;			    //(3 bits) Control Flags
    // 1) bit 0: reserved, must be zero
    // 2) bit 1: (DF) 0 = May Fragment, 1 = Don't Fragment
    // 3) bit 2: (MF) 0 = Last Fragment, 1 = More Fragments
    uint16_t Offset;			//(13 bits) Indicates where in the datagram this fragment belongs
    //The fragment is measured in units of 8 octets (64 bits). The firs fragment has offset zero.
    uint8_t TimeToLive;		    //(8 bits) Indicates the maximum time the datagram is allowed to remain in the internet system.
    //Time in seconds
    uint8_t Protocol;			//(8 bits) Indicates the next level protocol
    uint16_t HeaderChecksum;	//(16 bits) Checksum on header only
    //Algorithm: sum all 16 bit words in the header
    uint32_t SourceAddress;	    //(32 bits) Source Address IPv4
    uint32_t DestinationAddress;	//(32 bits) Destination Address
    uint8_t Option;			        //(8 bits) IP Option
    //0x0 a single octet of option-type (0x0 end of option list)
    uint32_t Padding;	        //(in our case is 24 bit) Padding variable use, because the internet header ends on a 32 bit boundary
} IDatagramHeader;
//

//IDatagram Structure and Methods declaration
typedef struct IDatagram
{
    IDatagramHeader* Header;		//Datagram head
    Vector* Data;		            //Datagram data
} IDatagram;

void ClearIDatagram(IDatagram* pIDatagram);
//


//ISocketSender Structure and Methods declaration
typedef struct ISocketSender
{
    Vector* SendingDatagrams;				         //Buffer for send datagrams
} ISocketSender;

void proc_print_send_datagram(IDatagram* SendDatagram, ISocketSender* pSocketSender);	//Print Datagram procedure
void proc_calc_checksum(IDatagram* SendDatagram, ISocketSender* pSocketSender);	        //Calculate checksum procedure
void proc_fragmentation(IDatagram* SendDatagram, ISocketSender* pSocketSender);	        //Fragmentation procedure
//
/*
//ResourcesBuffer Structure and Methods declaration
typedef struct ResourcesBuffer
{
    Vector* DATA_BUFFER;		        //data buffer
    Vector* RCVBT;					    //fragment block bit table
    IDatagramHeader* HEADER_BUFFER;		//header buffer
    int TIMER;						    //timer
    uint16_t TDL;					    //total data length field
    size_t TimerStartTime;			    //Current start time
} ResourcesBuffer;

ResourcesBuffer* CreateResourcesBuffer();					//Constructor ResourcesBuffer
void ClearResourcesBuffer();				                //Destructor ResourcesBuffer
//

//ISocketReceiver Structure and Methods declaration
typedef struct ISocketReceiver
{
    bool DestroyTimerThread;									//Variable for destroy TimerThread
    size_t TLB;													//Timer Lower Bound
    map_void_t* SOCKET_BUFFER;				                    //Socket Buffer
} ISocketReceiver;

void ClearISocketReceiver();									            //Destructor ISocketReceiver
void CreateTimerThread();											        //CreateTimerThread method
void proc_print_receive_datagram(IDatagram* ReassembledReceivedDatagram);	//Print Datagram procedure
void proc_reassembly(IDatagram* ReceivedDatagram);					        //Reassembly procedure
void proc_timer_checker();											        //Timer checker procedure
*/
