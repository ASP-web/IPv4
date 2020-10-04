//TEST RFC 791 INTERNET PROTOCOL VERSION 4
#include "stdio.h"
#include "IP.h"

int main(char argc, char** argv)
{

    /*TEST FRAGMENTATION WITH (MTU = 280) FROM RFC 791 EXAMPLE*/
    ISocketSender* SocketSender = (ISocketSender*)malloc(sizeof(ISocketSender));
    SocketSender->pSendingDatagrams = (Vector*)malloc(sizeof(Vector));
    vector_setup(SocketSender->pSendingDatagrams, 512, sizeof(IDatagram));

    IDatagram* MainSendDatagram =  (IDatagram*)malloc(sizeof(IDatagram));
    MainSendDatagram->pData = (Vector*)malloc(sizeof(Vector));
    MainSendDatagram->pHeader = (IDatagramHeader*)malloc(sizeof(IDatagram));

    //452 SEND DATA BYTES
    vector_setup(MainSendDatagram->pData, 452, sizeof(uint8_t));
    vector_resize(MainSendDatagram->pData, 100);
    memset(MainSendDatagram->pData->data, 0x0, 100);
    vector_resize(MainSendDatagram->pData, 200);
    memset(MainSendDatagram->pData->data + 100, 0x1, 100);
    vector_resize(MainSendDatagram->pData, 300);
    memset(MainSendDatagram->pData->data + 200, 0x2, 100);
    vector_resize(MainSendDatagram->pData, 400);
    memset(MainSendDatagram->pData->data + 300, 0x3, 100);
    vector_resize(MainSendDatagram->pData, 452);
    memset(MainSendDatagram->pData->data + 400, 0x4, 52);

    MainSendDatagram->pHeader->Version = 0x4;
    MainSendDatagram->pHeader->IHL = 0x5;
    //TOTAL LENGTH = HEADER LENGTH + DATA LENGTH
    MainSendDatagram->pHeader->TotalLength =
        (uint16_t)(MainSendDatagram->pData->size) +
        MainSendDatagram->pHeader->IHL * 4;
    MainSendDatagram->pHeader->Identification = 0x0;
    MainSendDatagram->pHeader->Flags = 0x0;
    MainSendDatagram->pHeader->Offset = 0x0;
    MainSendDatagram->pHeader->TimeToLive = 123;
    MainSendDatagram->pHeader->Protocol = 6;
    proc_fragmentation(MainSendDatagram, SocketSender);

    /*TEST REASSEBLY*/
    ISocketReceiver* pSocketReceiver = CreateISocketReceiver();
    for (size_t i = 0; i < SocketSender->pSendingDatagrams->size; i++)
    {
        proc_reassembly(
            (IDatagram*)vector_get(SocketSender->pSendingDatagrams, i),
            pSocketReceiver
        );
    }
    return 0;
}
