#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <vector>
#include <mutex>
#include <pthread.h>
#include <iostream>
#include <thread>
#include <string>
#define DEFAULT_BUFLEN 512
#define BLOCK_SIZE 16384
#define SOCKET int
#define DEFAULT_PACKLEN 512*3
#define DEFAULT_BUFLEN 512
#define PREFIXLEN 11
//+11 char bytes, so 523
using namespace std;

int PORT=0;
char IP_ADDRESS[]="192.168.213.1";//="192.168.213.1";

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

int getData(char *recvbuf,int &number,char *message)
{
    char *buf = strtok(recvbuf, " ");
    char numberbuf[DEFAULT_BUFLEN];
    strcpy(numberbuf,buf);
    buf = strtok(NULL, " ");
    strcpy(message,buf);
    number = atoi(numberbuf);

    return 0;
}

int readnUDP(SOCKET & sock, char *recvbuf, int recvbuflen)
{
    int iResult=0;
    char message[recvbuflen];
    memset(recvbuf,0,recvbuflen);
    memset(message,0,recvbuflen);

    int recievedBytes=0;
    int packetsCount=0;


    while(recievedBytes<recvbuflen)
    {
        char messageTemp[DEFAULT_BUFLEN];


        int number;
        iResult=recv( sock, message+recievedBytes, recvbuflen-recievedBytes, 0 );
        if(iResult<0)
        {
            return -1;
        }

        strcpy(messageTemp,message);
        getData(messageTemp,number,message);
        if(number!=packetsCount)
        {
            char ack[DEFAULT_BUFLEN]="no"; //acknowledge
            send(sock,ack,sizeof(ack),0);
        }
        else
        {
            packetsCount++;
            recievedBytes+=iResult;
        }
    }
    return 0;
}


//int sendUDP(SOCKET & sock, char *sendbuff, int sendbufflen)
int trySendUDP(SOCKET sock,char *sendbuff, int sendbufflen)
{
    int sendedBytesSize=0;
    int i=0;
    char tempbuf[DEFAULT_BUFLEN];
    char numbuf[PREFIXLEN];
    char tempbufn[PREFIXLEN+DEFAULT_BUFLEN]="0 ";

    snprintf(numbuf,sizeof(numbuf),"%d",sendbufflen);
    strcat(tempbufn,numbuf);
    int iResult = send(sock,tempbufn,sizeof(tempbufn),0);
    while(sendedBytesSize<sendbufflen)
    {

        i++;
        memset(tempbufn,NULL,sizeof(tempbufn));
        snprintf(tempbufn,sizeof(tempbufn),"%d",i);
        strcat(tempbufn," ");
        int lastBytes=DEFAULT_BUFLEN;
        if(sendbufflen-sendedBytesSize<DEFAULT_BUFLEN)
        {
            lastBytes=sendbufflen-sendedBytesSize;
            strncpy(tempbuf,sendbuff+sendedBytesSize,lastBytes);
        }

        strncpy(tempbuf,sendbuff+sendedBytesSize,sizeof(tempbuf));
        printf("tempbuf %s\n",tempbuf);


        strncat(tempbufn,tempbuf,DEFAULT_BUFLEN);

        sendedBytesSize+=lastBytes;
        //printf("tempbufn %s\n",tempbufn);
        int iResult = send(sock,tempbufn,sizeof(tempbufn),0);
    }

    return 0;
}

int main()
{
    int iResult = 0;
    SOCKET sock;
    int iFamily = AF_INET;
    int iType = SOCK_DGRAM;
    int iProtocol = IPPROTO_UDP;

    sock = socket(iFamily,iType,iProtocol);

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = htonl(INADDR_ANY);
    service.sin_port = htons(PORT);


    char str[512]="215212 1234567";
    char message[512];
    int number;
    getData(str,number,message);

    printf("%d/%s\n",number,message);

    iResult = bind(sock, (struct sockaddr *)&service, sizeof (service));
//    if (iResult == SOCKET_ERROR) {
//        printf("bind function failed with error %d\n", WSAGetLastError());
//        iResult = closesocket(sock);
//        if (iResult == SOCKET_ERROR)
//            printf("closesocket function failed with error %d\n", WSAGetLastError());
//        WSACleanup();
//        return 1;
//    }
    printf("-Starting client at %s:%d\n",IP_ADDRESS,PORT);


    sockaddr_in sender;
    sender.sin_family = AF_INET;
    sender.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    sender.sin_port = htons(27015);
    int len=sizeof(sender);
    char message1[512]="";


    memset(message1,'a',sizeof(message1));
  //char message1[512]="13451132";

    iResult=connect(sock,(struct sockaddr *)&sender,len);
//    if (iResult != 0) {
//        printf("Connect failed: %d\n", WSAGetLastError());
//        WSACleanup();
//        return 1;
//        }
    //char str[] = "Это не баг, это фича.";
     trySendUDP(sock,message1,sizeof(message1));
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
//        if (iResult == SOCKET_ERROR) {
//            printf("Send failed with error %d\n", WSAGetLastError());
//            iResult = closesocket(sock);
//            if (iResult == SOCKET_ERROR)
//                printf("closesocket function failed with error %d\n", WSAGetLastError());
//            WSACleanup();
//            return 1;
//        }

        //iResult = recvfrom(sock,recvbuf,sizeof(recvbuf),0,(SOCKADDR*) &recc,&rlen);
        //iResult = recv(sock,recvbuf,sizeof(recvbuf),0);
//        if (iResult == SOCKET_ERROR) {
//            printf("Send failed with error %d\n", WSAGetLastError());
//            iResult = closesocket(sock);
//            if (iResult == SOCKET_ERROR)
//                printf("closesocket function failed with error %d\n", WSAGetLastError());
//            WSACleanup();
//            return 1;
//        }

       // printf("%s from %s\n",recvbuf,inet_ntoa(recc.sin_addr));//,ntohs(recv.sin_port));

    }
    close(sock);

    return 0;
}
