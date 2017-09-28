// Simple_Server.cpp : Defines the entry point for the console application.
//
#pragma comment(lib,"Ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <stdio.h>
#define DEFAULT_BUFLEN 512
//include <WinSock2.h>

using namespace std;

//Makes new defined length byte array from incoming frames
int readn(SOCKET & sock, char *recvbuf, int recvbuflen)
{
        int iResult=0;
        memset(recvbuf,NULL,recvbuflen);
        int recievedBytes=0;

        while(recievedBytes<recvbuflen)
        {

            iResult = recv( sock, recvbuf+recievedBytes, recvbuflen-recievedBytes, 0 );
            if (iResult == SOCKET_ERROR) {
            wprintf(L"send failed with error: %d\n", WSAGetLastError());
            return 1;
            }
            recievedBytes+=iResult;
        }
        return 0;
}

int main()
{

    WSADATA wsaData = {0};
    int iResult=0;
    int iFamily = AF_INET;
    int iType = SOCK_STREAM;
    int iProtocol = IPPROTO_TCP;
    char sendbuf[DEFAULT_BUFLEN];
    memset(sendbuf,NULL,sizeof(sendbuf));
    char recvbuf[DEFAULT_BUFLEN];
    memset(recvbuf,NULL,sizeof(recvbuf));
   // char recvbuf[DEFAULT_BUFLEN]="";
    int recvbuflen = 512;
    bool terminated=false;
    SOCKET sock = INVALID_SOCKET;
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    sock = socket(iFamily,iType,iProtocol);
    if(sock==INVALID_SOCKET)
    {
        cout<<("Init error");
        WSACleanup();
        return 1;
    }

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr("127.0.0.1");
    service.sin_port = htons(27015);

    iResult = connect(sock, (SOCKADDR *) & service, sizeof (service));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"connect function failed with error: %ld\n", WSAGetLastError());
        iResult = closesocket(sock);
        if (iResult == SOCKET_ERROR)
            wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    printf("Connected to server\n");
    while(true)
    {
        cin>>sendbuf;
        printf("Entered message: %s\n",sendbuf);
        iResult = send( sock, sendbuf, DEFAULT_BUFLEN, 0 );
        if (iResult == SOCKET_ERROR) {
            wprintf(L"send failed with error: %d\n", WSAGetLastError());
            closesocket(sock);
            WSACleanup();
            return 1;
        }
        if(strcmp(sendbuf,"-close")==0)
        {
           // closesocket(sock);
            WSACleanup();
            return 0;
        }

        iResult = readn(sock, recvbuf, DEFAULT_BUFLEN);
        printf("%s\n",recvbuf);
        /*iResult = closesocket(sock);
        if (iResult == SOCKET_ERROR) {
            wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        } */
    }
    printf("send success\n");
	return 0;
}

