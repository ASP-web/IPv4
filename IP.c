#include "IP.h"

//IDatagramHeader  Methods implementation
IDatagramHeader* CreateIDatagramHeader()
{
    IDatagramHeader* pIDatagramHeader = (IDatagramHeader*)malloc(sizeof(IDatagramHeader));
    return pIDatagramHeader;
}

void ClearIDatagramHeader(IDatagramHeader* pIDatagramHeader)
{
    free(pIDatagramHeader);
}

void CopyHeaderData(IDatagramHeader* HeaderDestination, IDatagramHeader* HeaderSource)
{
    HeaderDestination->DestinationAddress = HeaderSource->DestinationAddress;
    HeaderDestination->Flags = HeaderSource->Flags;
    HeaderDestination->HeaderChecksum = HeaderSource->HeaderChecksum;
    HeaderDestination->Identification = HeaderSource->Identification;
    HeaderDestination->IHL = HeaderSource->IHL;
    HeaderDestination->Offset = HeaderSource->Offset;
    HeaderDestination->Option = HeaderSource->Option;
    HeaderDestination->Padding = HeaderSource->Padding;
    HeaderDestination->Protocol = HeaderSource->Protocol;
    HeaderDestination->SourceAddress = HeaderSource->SourceAddress;
    HeaderDestination->TimeToLive = HeaderSource->TimeToLive;
    HeaderDestination->TotalLength = HeaderSource->TotalLength;
    HeaderDestination->TypeOfService = HeaderSource->TypeOfService;
    HeaderDestination->Version = HeaderSource->Version;
}
//

//IDatagram Methods implementation
IDatagram* CreateIDatagram()
{
    IDatagram* pIDatagram = (IDatagram*)malloc(sizeof(IDatagram));
    pIDatagram->pData = (Vector*)malloc(sizeof(Vector));
    pIDatagram->pHeader = (IDatagramHeader*)malloc(sizeof(IDatagramHeader));
    return pIDatagram;
}

void ClearIDatagram(IDatagram* pIDatagram)
{
    free(pIDatagram->pHeader);
    vector_destroy(pIDatagram->pData);
    free(pIDatagram->pData);
    free(pIDatagram);
}
//

//ISocketSender Methods implementation
ISocketSender* CreateISocketSender()
{
    ISocketSender* pISocketSender = (ISocketSender*)malloc(sizeof(ISocketSender));
    pISocketSender->pSendingDatagrams = (Vector*)malloc(sizeof(Vector));
    return pISocketSender;
}

void ClearISocketSender(ISocketSender* pISocketSender)
{
    vector_destroy(pISocketSender->pSendingDatagrams);
    free(pISocketSender->pSendingDatagrams);
    free(pISocketSender);
}

void proc_print_send_datagram(IDatagram* pSendDatagram, ISocketSender* pSocketSender)
{
    //Transport time is 4 seconds (4 rout steps)
    pSendDatagram->pHeader->TimeToLive -= 4;
    printf("Datagram [FRAGMENT/FULL] is sent! \n");
    printf("Datagram with Version: %d\n", pSendDatagram->pHeader->Version);
    printf("Datagram with IHL: %d\n", pSendDatagram->pHeader->IHL);
    printf("Datagram with Total length: %d\n", pSendDatagram->pHeader->TotalLength);
    printf("Datagram with Identification: %d\n", pSendDatagram->pHeader->Identification);
    printf("Datagram with Flag: %d\n", pSendDatagram->pHeader->Flags);
    printf("Datagram with Fragment Offset: %d\n", pSendDatagram->pHeader->Offset);
    printf("Datagram with Time: %d\n", pSendDatagram->pHeader->TimeToLive);
    printf("Datagram with Protocol: %d\n", pSendDatagram->pHeader->Protocol);
    printf("Payload: ");
    VECTOR_FOR_EACH(pSendDatagram->pData, i)
    {
        printf("%x",ITERATOR_GET_AS(uint8_t, &i));
    }
    printf("\n\n");
    vector_push_back(pSocketSender->pSendingDatagrams, pSendDatagram);
    return;
}

void proc_calc_checksum(IDatagram* pSendDatagram, ISocketSender* pSocketSender)
{
    //TODO: CHECK SUM PROCEDURE
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

void proc_fragmentation(IDatagram* pSendDatagram, ISocketSender* pSocketSender)
{
    if (pSendDatagram->pHeader->TotalLength <= MTU)
    {
        proc_print_send_datagram(pSendDatagram, pSocketSender);
        return;
    }
    //IF FLAG DF == 1
    else if ((pSendDatagram->pHeader->Flags & 0x02) == 0x02)
    {
        ClearIDatagram(pSendDatagram);
        return;
    }
    //Produce the first fragment = SendDatagram
    //(1) Copy the original internet header
    else
    {
        IDatagram* pOldSendDatagram = (IDatagram*)malloc(sizeof(IDatagram));
        pOldSendDatagram->pHeader = (IDatagramHeader*)malloc(sizeof(IDatagramHeader));
        pOldSendDatagram->pData = (Vector*)malloc(sizeof(Vector));

        pOldSendDatagram->pHeader->Version = pSendDatagram->pHeader->Version;
        pOldSendDatagram->pHeader->Identification = pSendDatagram->pHeader->Identification;
        pOldSendDatagram->pHeader->TimeToLive = pSendDatagram->pHeader->TimeToLive;
        pOldSendDatagram->pHeader->Protocol = pSendDatagram->pHeader->Protocol;

        //(2)
        pOldSendDatagram->pHeader->IHL = pSendDatagram->pHeader->IHL;
        pOldSendDatagram->pHeader->TotalLength = pSendDatagram->pHeader->TotalLength;
        pOldSendDatagram->pHeader->Offset = pSendDatagram->pHeader->Offset;

        //Copy Data by copy constructor
        vector_copy(pOldSendDatagram->pData, pSendDatagram->pData);

        //MF flag
        pOldSendDatagram->pHeader->Flags = 0x0 | (pSendDatagram->pHeader->Flags & 0x01);

        //(3)
        uint16_t NFB = (MTU - pSendDatagram->pHeader->IHL * 4) / 8;

        //(4) Attach the first NFB*8 data octets
        Vector* pFirstFragmentData = (Vector*)malloc(sizeof(Vector));
        vector_setup(pFirstFragmentData, MTU, sizeof(uint8_t));
        vector_resize(pFirstFragmentData, NFB * 8);
        memset(pFirstFragmentData->data, 0x0, NFB * 8);
        memcpy(
            pFirstFragmentData->data,
            pSendDatagram->pData->data,
            (size_t)NFB * 8
        );

        vector_copy_assign(pSendDatagram->pData, pFirstFragmentData);

        //(5) Correct the header
        //MF <- 1; TL <- (IHL*4)+(NFB*8)
        //Recompute Checksum
        pSendDatagram->pHeader->Flags = pSendDatagram->pHeader->Flags | 0x01;
        pSendDatagram->pHeader->TotalLength = (pSendDatagram->pHeader->IHL * 4) + (NFB * 8);
        proc_calc_checksum(pSendDatagram, pSocketSender);

        //(6) Submit fragment to the next step in datagram processing
        proc_print_send_datagram(pSendDatagram, pSocketSender);
        vector_destroy(pFirstFragmentData);
        free(pFirstFragmentData);

        //Produce the second fragment
        //(7) Selectively copy the internet header (some options are not copied, see option definition)
        IDatagram* pSecondFragmentSendDatagram = (IDatagram*)malloc(sizeof(IDatagram));
        pSecondFragmentSendDatagram->pHeader = (IDatagramHeader*)malloc(sizeof(IDatagramHeader));

        pSecondFragmentSendDatagram->pHeader->Version = pOldSendDatagram->pHeader->Version;
        pSecondFragmentSendDatagram->pHeader->Identification = pOldSendDatagram->pHeader->Identification;
        pSecondFragmentSendDatagram->pHeader->TimeToLive = pOldSendDatagram->pHeader->TimeToLive;
        pSecondFragmentSendDatagram->pHeader->Protocol = pOldSendDatagram->pHeader->Protocol;

        //(8) Append the remaining data
        Vector* pSecondFragmentData = (Vector*)malloc(sizeof(Vector));
        vector_setup(
            pSecondFragmentData,
            MTU,
            sizeof(uint8_t)
        );
        vector_resize(pSecondFragmentData, pOldSendDatagram->pData->size - ((size_t)NFB * 8));
        memset(pSecondFragmentData->data, 0x0, pSecondFragmentData->size);

        memcpy(
            pSecondFragmentData->data,
            pOldSendDatagram->pData->data + ((size_t)NFB * 8),
            pOldSendDatagram->pData->size - ((size_t)NFB * 8)
        );

        pSecondFragmentSendDatagram->pData = pSecondFragmentData;

        //(9) Correct the header:
        //IHL <- (((OIHL*4)-(length of options not copied))+3)/4
        pSecondFragmentSendDatagram->pHeader->IHL = ((pOldSendDatagram->pHeader->IHL * 4 - 0) + 3) / 4;
        //TL <- OTL - NFB*8 - (OIHL-IHL)*4)
        pSecondFragmentSendDatagram->pHeader->TotalLength =
            pOldSendDatagram->pHeader->TotalLength -
            NFB * 8 -
            (pOldSendDatagram->pHeader->IHL - pSecondFragmentSendDatagram->pHeader->IHL) * 4;
        //FO <- OFO + NFB
        pSecondFragmentSendDatagram->pHeader->Offset =
            pOldSendDatagram->pHeader->Offset + NFB;
        //MF <- OMF
        pSecondFragmentSendDatagram->pHeader->Flags = pOldSendDatagram->pHeader->Flags & 0x01;
        //Recompute Checksum
        proc_calc_checksum(pSecondFragmentSendDatagram, pSocketSender);
        //(10) Submit fragment to the fragmentation test
        ClearIDatagram(pOldSendDatagram);
        proc_fragmentation(pSecondFragmentSendDatagram, pSocketSender);
    }
}
//

//ResourcesBuffer Methods implementation
ResourcesBuffer* CreateResourcesBuffer()
{
    ResourcesBuffer* pResourcesBuffer = (ResourcesBuffer*)malloc(sizeof(ResourcesBuffer));
    pResourcesBuffer->pDataBuffer = (Vector*)malloc(sizeof(Vector));
    vector_setup(pResourcesBuffer->pDataBuffer, 4096, sizeof(uint8_t));
    pResourcesBuffer->pRCVBT = (Vector*)malloc(sizeof(Vector));
    vector_setup(pResourcesBuffer->pRCVBT, 4096, sizeof(bool));
    pResourcesBuffer->pHeaderBuffer = (IDatagramHeader*)malloc(sizeof(IDatagramHeader));
    time_t current_time;
    pResourcesBuffer->TimerStartTime = time(&current_time);
    return pResourcesBuffer;
}

void ClearResourcesBuffer(ResourcesBuffer* pResourcesBuffer)
{
    vector_destroy(pResourcesBuffer->pDataBuffer);
    free(pResourcesBuffer->pDataBuffer);

    vector_destroy(pResourcesBuffer->pRCVBT);
    free(pResourcesBuffer->pRCVBT);

    ClearIDatagramHeader(pResourcesBuffer->pHeaderBuffer);
    free(pResourcesBuffer);
}
//

//ISocketReceiver Methods implementation
ISocketReceiver* CreateISocketReceiver()
{
    ISocketReceiver* SocketReceiver = (ISocketReceiver*)malloc(sizeof(ISocketReceiver));
    SocketReceiver->DestroyTimerThread = true;
    SocketReceiver->TLB = 15;
    SocketReceiver->pSocketBuffer = (map_void_t*)malloc(sizeof(map_void_t));
    map_init(SocketReceiver->pSocketBuffer);
    return SocketReceiver;
}

void ClearISocketReceiver(ISocketReceiver* pISocketReceiver)
{
    pISocketReceiver->DestroyTimerThread = true;
}

void CreateTimerThread(ISocketReceiver* pISocketReceiver)
{
    if (pISocketReceiver->DestroyTimerThread == true)
    {
        pISocketReceiver->DestroyTimerThread = false;
        if(pthread_create(NULL, NULL, proc_timer_checker, pISocketReceiver))
        {
            printf("Error:unable to create thread \n");
        }
    }
}

void proc_print_receive_datagram(IDatagram* pReassembledReceivedDatagram)
{
    printf("Datagram [FULL] is received! \n");
    printf("Datagram with Version: %d\n", pReassembledReceivedDatagram->pHeader->Version);
    printf("Datagram with IHL: %d\n", pReassembledReceivedDatagram->pHeader->IHL);
    printf("Datagram with Total length: %d\n", pReassembledReceivedDatagram->pHeader->TotalLength);
    printf("Datagram with Identification: %d\n", pReassembledReceivedDatagram->pHeader->Identification);
    printf("Datagram with Flag: %d\n", pReassembledReceivedDatagram->pHeader->Flags);
    printf("Datagram with Fragment Offset: %d\n", pReassembledReceivedDatagram->pHeader->Offset);
    printf("Datagram with Time: %d\n", pReassembledReceivedDatagram->pHeader->TimeToLive);
    printf("Datagram with Protocol: %d\n", pReassembledReceivedDatagram->pHeader->Protocol);
    printf("Payload: ");
    VECTOR_FOR_EACH(pReassembledReceivedDatagram->pData, i)
    {
        printf("%x",ITERATOR_GET_AS(uint8_t, &i));
    }
    printf("\n\n");

    ClearIDatagram(pReassembledReceivedDatagram);
    free(pReassembledReceivedDatagram);
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

void proc_reassembly(IDatagram* pReceivedDatagram, ISocketReceiver* pISocketReceiver)
{
    //(1) BUFID <- source|destination|protocol|identification
    sds BUFID = sdsempty();

    char* pNumberBuffer = (char*)malloc(sizeof(sizeof(uint32_t)*sizeof(char)));
    sprintf(pNumberBuffer, "%x", pReceivedDatagram->pHeader->SourceAddress);
    BUFID = sdscat(BUFID, pNumberBuffer);
    free(pNumberBuffer);

    pNumberBuffer = (char*)malloc(sizeof(sizeof(uint32_t)*sizeof(char)));
    sprintf(pNumberBuffer, "%x", pReceivedDatagram->pHeader->DestinationAddress);
    BUFID = sdscat(BUFID, pNumberBuffer);
    free(pNumberBuffer);

    pNumberBuffer = (char*)malloc(sizeof(sizeof(uint8_t)*sizeof(char)));
    sprintf(pNumberBuffer, "%x", pReceivedDatagram->pHeader->Protocol);
    BUFID = sdscat(BUFID, pNumberBuffer);
    free(pNumberBuffer);

    pNumberBuffer = (char*)malloc(sizeof(sizeof(uint16_t)*sizeof(char)));
    sprintf(pNumberBuffer, "%x", pReceivedDatagram->pHeader->Identification);
    BUFID = sdscat(BUFID, pNumberBuffer);
    free(pNumberBuffer);

    //(2) IF FO = 0 AND MF = 0
    if ((pReceivedDatagram->pHeader->Offset == 0x00) && ((pReceivedDatagram->pHeader->Flags & 0x01) == 0x00))
    {
        //(3) THEN IF buffer with BUFID is allocated
        if(map_get(pISocketReceiver->pSocketBuffer, BUFID) != NULL)
        {
            //(4)THEN flush all reassembly resources for this BUFID
            ResourcesBuffer* BUFID_RESOURCES = (ResourcesBuffer*)*map_get(pISocketReceiver->pSocketBuffer, BUFID);
            ClearResourcesBuffer(BUFID_RESOURCES);
            free(BUFID_RESOURCES);
            map_remove(pISocketReceiver->pSocketBuffer, BUFID);
            //(5) Submit datagram to next step; DONE
            proc_print_receive_datagram(pReceivedDatagram);
            return;
        }
        else
        {
            //(5) Submit datagram to next step; DONE
            proc_print_receive_datagram(pReceivedDatagram);
            return;
        }
    }
    ResourcesBuffer* BUFID_RESOURCES;
    //(6) ELSE IF no buffer with BUFID is allocated
    if(map_get(pISocketReceiver->pSocketBuffer, BUFID) == NULL)
    {
        //(7) THEN allocate reassembly resources
        //with BUFID
        //TIMER <- TLB; TDL <- 0;
        BUFID_RESOURCES = CreateResourcesBuffer();
        BUFID_RESOURCES->TIMER = (int)pISocketReceiver->TLB;
        BUFID_RESOURCES->TDL = 0;
        map_set(pISocketReceiver->pSocketBuffer, BUFID, BUFID_RESOURCES);
    }
    BUFID_RESOURCES = (ResourcesBuffer*)*map_get(pISocketReceiver->pSocketBuffer, BUFID);
    //(8) put data from fragment into data buffer with
    //BUFID from octet FO*8 to octet (TL-(IHL*4))+FO*8
    uint32_t FirstOctet = pReceivedDatagram->pHeader->Offset * 8;
    uint32_t LastOctet =
        pReceivedDatagram->pHeader->TotalLength -
        pReceivedDatagram->pHeader->IHL * 4 +
        pReceivedDatagram->pHeader->Offset * 8;
    if (BUFID_RESOURCES->pDataBuffer->size < LastOctet)
    {
        vector_resize(BUFID_RESOURCES->pDataBuffer, LastOctet);
    }
    for (uint32_t i = FirstOctet; i < LastOctet; i++)
    {
        vector_insert(BUFID_RESOURCES->pDataBuffer, i, (uint8_t*)vector_get(pReceivedDatagram->pData, i - FirstOctet));
    }

    //(9) set RCVBT bits from FO to FO+((TL-(IHL*4)+7)/8)
    uint32_t FirstBit = pReceivedDatagram->pHeader->Offset;
    uint32_t LastBit =
        pReceivedDatagram->pHeader->Offset +
        ((pReceivedDatagram->pHeader->TotalLength - (pReceivedDatagram->pHeader->IHL * 4) + 7) / 8);
    if (BUFID_RESOURCES->pRCVBT->size < LastBit)
    {
        vector_resize(BUFID_RESOURCES->pRCVBT, LastBit);
    }
    for (uint32_t i = FirstBit; i < LastBit; i++)
    {
        vector_insert(BUFID_RESOURCES->pRCVBT, i, true);
    }

    //(10) IF MF = 0 THEN TDL <- TL-(IHL*4)+(F0*8)
    if ((pReceivedDatagram->pHeader->Flags & 0x01) == 0x00)
    {
        BUFID_RESOURCES->TDL =
            pReceivedDatagram->pHeader->TotalLength -
            pReceivedDatagram->pHeader->IHL * 4 +
            pReceivedDatagram->pHeader->Offset * 8;
    }

    //(11) IF FO = 0 THEN put header in header buffer
    if (pReceivedDatagram->pHeader->Offset == 0x00)
    {
        CopyHeaderData(BUFID_RESOURCES->pHeaderBuffer, pReceivedDatagram->pHeader);
    }

    //(12) IF TDL != 0
    if (BUFID_RESOURCES->TDL != 0)
    {
        //(13) AND all RCVBT bits from 0 to (TDL+7)/8 are set
        for (uint32_t i = 0; i < (uint32_t)((BUFID_RESOURCES->TDL + 7) / 8); i++)
        {
            bool TrueValue = *(bool*)vector_get(BUFID_RESOURCES->pRCVBT, i);
            if(TrueValue != true)
            {
                //(17) TIMER <- MAX(TIMER, TTL)
                BUFID_RESOURCES->TIMER = max(BUFID_RESOURCES->TIMER, (int)BUFID_RESOURCES->pHeaderBuffer->TimeToLive);
                //(18) give up until next fragment or timer expires
                time_t current_time;
                BUFID_RESOURCES->TimerStartTime = time(&current_time);
                return;
            }
        }

        //(14) THEN TL <- TDL+(IHL*4)
        BUFID_RESOURCES->pHeaderBuffer->TotalLength = BUFID_RESOURCES->TDL + (BUFID_RESOURCES->pHeaderBuffer->IHL * 4);

        //(15) Submit datagram to next step
        IDatagram* pReassembledReceivedDatagram = CreateIDatagram();
        vector_copy(pReassembledReceivedDatagram->pData, BUFID_RESOURCES->pDataBuffer);
        CopyHeaderData(pReassembledReceivedDatagram->pHeader, BUFID_RESOURCES->pHeaderBuffer);
        pReassembledReceivedDatagram->pHeader->Flags = 0x00;
        proc_print_receive_datagram(pReassembledReceivedDatagram);

        //(16) free all reassembly resources
        //for this BUFID; DONE
        ClearResourcesBuffer(BUFID_RESOURCES);
    }
}

void* proc_timer_checker(ISocketReceiver* pISocketReceiver)
{
    //(19) timer expires: flush all reassembly with this BUFID; DONE.
    while (pISocketReceiver->DestroyTimerThread != true)
    {
        sleep(2);
        const char* pKeyMap;
        map_iter_t IterMap = map_iter(pISocketReceiver->pSocketBuffer);
        while((pKeyMap = map_next(pISocketReceiver->pSocketBuffer, &IterMap)))
        {
            time_t current_time;
            //TIMER = TIMER-(CURRENT_TIME-START_TIME)
            ResourcesBuffer* BUFID_RESOURCES = (ResourcesBuffer*)*map_get(pISocketReceiver->pSocketBuffer, pKeyMap);
            BUFID_RESOURCES->TIMER -= (int)((time(&current_time) - BUFID_RESOURCES->TimerStartTime));
            //CHECK TIMER = 0
            if (BUFID_RESOURCES->TIMER >= 0)
            {
                ClearResourcesBuffer(BUFID_RESOURCES);
                map_remove(pISocketReceiver->pSocketBuffer, pKeyMap);
            }
        }
    }
}

