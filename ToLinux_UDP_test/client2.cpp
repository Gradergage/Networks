#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <conio.h>
#include <pthread.h>
#define MAX_PACKLEN 512*3
#define DEFAULT_BUFLEN 512
#define PREFIXLEN 11
// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
#include "UDP.h"
char IP_ADDRESS[]="192.168.213.1";

int main()
{

    int iResult = 0;

    WSADATA wsaData;

    SOCKET RecvSocket;
    sockaddr_in RecvAddr;

    unsigned short Port = 27015;

    char RecvBuf[MAX_PACKLEN];
    int BufLen = DEFAULT_BUFLEN;

    sockaddr_in SenderAddr;
    int SenderAddrSize = sizeof (SenderAddr);

    //-----------------------------------------------
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR)
    {
        wprintf(L"WSAStartup failed with error %d\n", iResult);
        return 1;
    }
    //-----------------------------------------------
    // Create a receiver socket to receive datagrams
    RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (RecvSocket == INVALID_SOCKET)
    {
        wprintf(L"socket failed with error %d\n", WSAGetLastError());
        return 1;
    }
    int SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (SendSocket == INVALID_SOCKET)
    {
        wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    //-----------------------------------------------
    // Bind the socket to any address and the specified port.
    RecvAddr.sin_family = AF_INET;
    RecvAddr.sin_port = htons(Port);
    RecvAddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    sockaddr_in SendAddr;
    int len=sizeof(SendAddr);
    iResult = bind(RecvSocket, (SOCKADDR *) & RecvAddr, sizeof (RecvAddr));
    if (iResult != 0)
    {
        wprintf(L"bind failed with error %d\n", WSAGetLastError());
        return 1;
    }
    printf("-Starting client at %s:%d\n",inet_ntoa(RecvAddr.sin_addr),ntohs(RecvAddr.sin_port));
    //-----------------------------------------------
    // Call the recvfrom function to receive datagrams
    // on the bound socket.
    wprintf(L"Receiving datagrams...\n");
    recvfrom(RecvSocket,RecvBuf,sizeof(RecvBuf),0,(SOCKADDR *)&SendAddr,&len);
    iResult = connect(RecvSocket, (SOCKADDR *) & SendAddr, sizeof (SendAddr));
    if (iResult != 0)
    {
        wprintf(L"Connect failed with error %d\n", WSAGetLastError());
        return 1;
    }
    printf("Connected\n");
    ///test 1, dublicates recieve
    printf("\n\ntest 1, dublicates\n");
    strcpy(RecvBuf,"");


    iResult=readUDP(RecvSocket,RecvBuf,sizeof(RecvBuf));

    if (iResult<0)
    {
        wprintf(L"send failed with error %d\n", WSAGetLastError());
    }


    ///test 2, mixed packets recieve
    printf("\n\ntest 2, mixed packets\n");
    iResult=readUDP(RecvSocket,RecvBuf,sizeof(RecvBuf));

    if (iResult<0)
    {
        wprintf(L"send failed with error %d\n", WSAGetLastError());
    }

    ///test 3, missing packets recieve
    printf("\n\ntest 3, missing packets\n");
    memset(RecvBuf,'b',sizeof(RecvBuf));
    iResult = sendUDP(RecvSocket,RecvBuf,sizeof(RecvBuf));
    if (iResult < 0)
    {
        wprintf(L"sendUDP failed with error %d\n", WSAGetLastError());
        return 1;
    }
    iResult = closesocket(RecvSocket);
    if (iResult == SOCKET_ERROR)
    {
        wprintf(L"closesocket failed with error %d\n", WSAGetLastError());
        return 1;
    }

    wprintf(L"Exiting.\n");
    WSACleanup();
    return 0;
}
