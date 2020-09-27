//TEST RFC 791 INTERNET PROTOCOL VERSION 4
#include "stdio.h"
#include "IP.h"

int main(char argc, char** argv)
{

    /*TEST FRAGMENTATION WITH (MTU = 280) FROM RFC 791 EXAMPLE*/
    ISocketSender* SocketSender = (ISocketSender*)malloc(sizeof(ISocketSender));
    SocketSender->SendingDatagrams = (Vector*)malloc(sizeof(Vector));
    vector_setup(SocketSender->SendingDatagrams, 512, sizeof(IDatagram));

    IDatagram* MainSendDatagram =  (IDatagram*)malloc(sizeof(IDatagram));
    MainSendDatagram->Data = (Vector*)malloc(sizeof(Vector));
    MainSendDatagram->Header = (IDatagramHeader*)malloc(sizeof(IDatagram));

    //452 SEND DATA BYTES
    vector_setup(MainSendDatagram->Data, 452, sizeof(uint8_t));
    vector_resize(MainSendDatagram->Data, 100);
    memset(MainSendDatagram->Data->data, 0x0, 100);
    vector_resize(MainSendDatagram->Data, 200);
    memset(MainSendDatagram->Data->data + 100, 0x1, 100);
    vector_resize(MainSendDatagram->Data, 300);
    memset(MainSendDatagram->Data->data + 200, 0x2, 100);
    vector_resize(MainSendDatagram->Data, 400);
    memset(MainSendDatagram->Data->data + 300, 0x3, 100);
    vector_resize(MainSendDatagram->Data, 452);
    memset(MainSendDatagram->Data->data + 400, 0x4, 52);

    MainSendDatagram->Header->Version = 0x4;
    MainSendDatagram->Header->IHL = 0x5;
    //TOTAL LENGTH = HEADER LENGTH + DATA LENGTH
    MainSendDatagram->Header->TotalLength =
        (uint16_t)(MainSendDatagram->Data->size) +
        MainSendDatagram->Header->IHL * 4;
    MainSendDatagram->Header->Identification = 0x0;
    MainSendDatagram->Header->Flags = 0x0;
    MainSendDatagram->Header->Offset = 0x0;
    MainSendDatagram->Header->TimeToLive = 123;
    MainSendDatagram->Header->Protocol = 6;
    proc_fragmentation(MainSendDatagram, SocketSender);

    /*TEST REASSEBLY*/
/*    ISocketReceiver* SocketReceiver = (ISocketReceiver*)malloc(sizeof(ISocketReceiver));
    for (size_t i = 0; i < SocketSender->SendingDatagrams->size; i++)
    {
        proc_reassembly(
            (IDatagram*)vector_get(SocketSender->SendingDatagrams, i)
        );
    }
*/
    return 0;
}
