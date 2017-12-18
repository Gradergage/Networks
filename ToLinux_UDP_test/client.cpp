#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <conio.h>
#include <pthread.h>
#include <mutex>
#include <fstream>
#define DEFAULT_BUFLEN 512
using namespace std;

int PORT=0;
char IP_ADDRESS[]="192.168.213.1";//="127.0.0.1";//

struct CLIENT_INFO
{
    char login[DEFAULT_BUFLEN];
    char password[DEFAULT_BUFLEN];
    bool online=false;
};

struct CLIENT
{
    int id;
    SOCKET sock;
    sockaddr_in addr;
    bool connected;
    std::thread * cTh;
    CLIENT_INFO info;
    int lowBound;

};
std::vector <CLIENT *> clients;

int jkeoen()
{
    WSADATA wsaData = {0};
    int iResult = 0;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("-WSAStartup failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    SOCKET sock = INVALID_SOCKET;
    int iFamily = AF_INET;
    int iType = SOCK_DGRAM;
    int iProtocol = IPPROTO_UDP;

    sock = socket(iFamily,iType,iProtocol);

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = htonl(INADDR_ANY);
    service.sin_port = htons(PORT);

    iResult = bind(sock, (SOCKADDR *) & service, sizeof (service));
    if (iResult == SOCKET_ERROR) {
        printf("bind function failed with error %d\n", WSAGetLastError());
        iResult = closesocket(sock);
        if (iResult == SOCKET_ERROR)
            printf("closesocket function failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("-Starting client at %s:%d\n",inet_ntoa(service.sin_addr),ntohs(service.sin_port));


    sockaddr_in sender;
    sender.sin_family = AF_INET;
    sender.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    sender.sin_port = htons(27015);
    int len=sizeof(sender);

    iResult=connect(sock,(SOCKADDR *) &sender,len);
    if (iResult != 0) {
        printf("Connect failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
        }
    while(true)
    {

        char sendbuf[DEFAULT_BUFLEN];
        char recvbuf[DEFAULT_BUFLEN];
        //char* recvbuf;
        sockaddr_in recc;
        int rlen=sizeof(sender);
        cin>>sendbuf;
        //iResult = sendto(sock,sendbuf,sizeof(sendbuf),0,(SOCKADDR*) &sender,len);
        iResult = send(sock,sendbuf,sizeof(sendbuf),0);
        if (iResult == SOCKET_ERROR) {
            printf("Send failed with error %d\n", WSAGetLastError());
            iResult = closesocket(sock);
            if (iResult == SOCKET_ERROR)
                printf("closesocket function failed with error %d\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        //iResult = recvfrom(sock,recvbuf,sizeof(recvbuf),0,(SOCKADDR*) &recc,&rlen);
        iResult = recv(sock,recvbuf,sizeof(recvbuf),0);
        if (iResult == SOCKET_ERROR) {
            printf("Send failed with error %d\n", WSAGetLastError());
            iResult = closesocket(sock);
            if (iResult == SOCKET_ERROR)
                printf("closesocket function failed with error %d\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        printf("%s from %s\n",recvbuf,inet_ntoa(recc.sin_addr));//,ntohs(recv.sin_port));

    }
    closesocket(sock);
    WSACleanup();
    return 0;
}
