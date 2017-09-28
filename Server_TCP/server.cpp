#pragma comment(lib,"Ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <stdio.h>
#include <thread>
#include <vector>
#include <conio.h>
#include <mutex>
#define DEFAULT_BUFLEN 512
#define MAX_THREADS 10
#define _WIN32_WINNT 0x0501

std::mutex hMutex;

bool serverTerminated=false;
bool acceptTerminated =false;
int currentClientCount=0;

struct CLIENT
{
    int id;
    SOCKET * sock;
    sockaddr_in addr;
};

std::vector <CLIENT *> clients;

int readn(SOCKET & sock, char *recvbuf, int recvbuflen)
{
        int iResult=0;
        memset(recvbuf,NULL,recvbuflen);
        int recievedBytes=0;

        while(recievedBytes<recvbuflen)
        {

            iResult = recv( sock, recvbuf+recievedBytes, recvbuflen-recievedBytes, 0 );
            if (iResult == SOCKET_ERROR) {
                return 1;
            }
            recievedBytes+=iResult;
        }
        return 0;
}

void socketInfo(sockaddr_in & client_info)
{
    int iResult;
    char *connected_ip = inet_ntoa(client_info.sin_addr);
    int port = ntohs(client_info.sin_port);
    printf("%s:%d\n", connected_ip,port);
}
bool disconnectClientById(int id)
{
    hMutex.lock();
    for(int i=0; i<clients.size() ; i++)
    {
        if(clients[i]->id==id)
        {
            shutdown(*clients[i]->sock, 2);
            closesocket(*clients[i]->sock);
            delete(clients[i]);
            clients.erase(clients.begin()+i);
            printf("Client %d has been disconnected\n",id);
            currentClientCount--;
            hMutex.unlock();
            return true;
        }
    }
    hMutex.unlock();
    return false;
}
bool findById(int id)
{
    hMutex.lock();
    for(int i=0; i<clients.size() ; i++)
    {
        if(clients[i]->id==id)
        {
            hMutex.unlock();
            return true;
        }
    }
    hMutex.unlock();
    return false;
}

int handleClient(CLIENT *client)
{
    sockaddr_in addr = client->addr;
    SOCKET cSocket = *client->sock;
    int iResult;
    int cId=client->id;
    char *connected_ip = inet_ntoa(addr.sin_addr);
    int port = ntohs(addr.sin_port);
    bool terminated=false;

    char recvbuf[DEFAULT_BUFLEN];
    char sendbuf[DEFAULT_BUFLEN];

    while(!terminated)
    {
        iResult = readn(cSocket, recvbuf, sizeof(recvbuf));
        if(strcmp(recvbuf,"-close")==0)
        {
            disconnectClientById(cId);
            terminated=true;
            return 0;
        }
        if (iResult == SOCKET_ERROR||cSocket==INVALID_SOCKET) {
            wprintf(L"send failed with error: %d\n", WSAGetLastError());
            disconnectClientById(cId);
            terminated=true;
            return 1;
        }
        else
        {
            hMutex.lock();
            printf("Message [ %s ] from %s:%d\n",recvbuf,connected_ip,port);
            hMutex.unlock();
        }
        if(!terminated)
        {
            char responcebuf[DEFAULT_BUFLEN]="Response";
            if(!findById(cId))
            {
                return 0;
            }
            else
            {
                iResult = send( cSocket, responcebuf, DEFAULT_BUFLEN, 0 );
                if (iResult == SOCKET_ERROR) {
                    printf("-send failed: %d\n", WSAGetLastError());
                    disconnectClientById(cId);
                    return 1;
                }
            }
        }
    }
    return 0;
}
void showAllClients()
{
    hMutex.try_lock();
    printf("\nClients connected:\n\n");
    int i=0;
    if (clients.empty())
    {
        printf("EMPTY\n");
        hMutex.unlock();
        return;
    }
    for ( CLIENT * n : clients ) {
        i++;
        printf("%d) id:%d ",i,n->id);
        socketInfo((sockaddr_in &)n->addr);
    }
    printf("\n");
    hMutex.unlock();
}

int serverThread()
{
    char* command="";
    while(!::serverTerminated)
    {
        printf("MENU \n1 - show all clients menu\n2 - close connection with some client\n3 - close all connections\n4 - shutdown server\n");
        std::cin>>command;
        if(strcmp(command,"-close")==0)
        {
            ::serverTerminated=true;
        }
    }
}
int hashByIp(char* ip, int len, int port)
{
    int sum=1;
    for(int i=0; i<len; i++)
    {
        sum*=(ip[i]-35);

    }
    return sum+port;
}
int acceptThread(SOCKET & sock)
{
    while(!acceptTerminated)
   {
        hMutex.lock();
        printf("-Accepting \n");
        hMutex.unlock();

        SOCKET acceptSocket;
        CLIENT * entrySocket=new CLIENT();
        sockaddr_in client_info={0};

        int len = sizeof(client_info);

        acceptSocket = accept(sock, (SOCKADDR *) & client_info, & len);

        hMutex.lock();

        //attaching socket's info to socket itself
        char* innerIp=inet_ntoa(client_info.sin_addr);
        entrySocket->addr = client_info;
        entrySocket->sock = &acceptSocket;
        entrySocket->id=hashByIp(innerIp,sizeof(innerIp),client_info.sin_port);

        clients.push_back(entrySocket);
        currentClientCount++;

        std::thread sockTh(handleClient, std::ref(entrySocket));
        sockTh.detach();
        hMutex.unlock();
   }
   return 0;
}

int main()
{
    WSADATA wsaData = {0};

    int iResult=0;
    int threadId[MAX_THREADS];
    int iFamily = AF_INET;
    int iType = SOCK_STREAM;
    int iProtocol = IPPROTO_TCP;

    char *IP_ADDRESS = "127.0.0.1";
    int PORT=27015;
    int recvbuflen = 512;

    SOCKET sock = INVALID_SOCKET;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("-WSAStartup failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    sock = socket(iFamily,iType,iProtocol);
    if(sock==INVALID_SOCKET)
    {
        WSACleanup();
        return 1;
    }

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    service.sin_port = htons(PORT);

    iResult = bind(sock, (SOCKADDR *) & service, sizeof (service));
    if (iResult == SOCKET_ERROR) {
        printf("-bind function failed with error %d\n", WSAGetLastError());
        iResult = closesocket(sock);
        if (iResult == SOCKET_ERROR)
            printf("-closesocket function failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("-Starting server at %s:%d\n",IP_ADDRESS,PORT);



    if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
        wprintf(L"listen function failed with error: %d\n", WSAGetLastError());

    std::thread acceptTh(acceptThread, std::ref(sock));
    acceptTh.detach();

    while(!::serverTerminated)
    {
        hMutex.lock();
        printf("\nMENU \n1 - show all clients menu\n2 - close connection with some client\n3 - close all connections\n4 - shutdown server\n");
        hMutex.unlock();

        char choise = getch();

        switch(choise)
        {
            case '1':{
                showAllClients();
                break;}
            case '2':{
                showAllClients();
                printf("Chose id to disconnect, or 0 to exit\n");
                bool success=false;
                int delChoise;
                while(!success)
                {
                    std::cin>>delChoise;
                    if(delChoise==0)
                    {
                        break;
                    }
                    if(disconnectClientById(delChoise)) break;
                    else printf("Incorrect id! Type again\n");
                }
                break;
                }
            case '3':{

                break;}
            case '4':{
                ::serverTerminated=true;
                return 0;
                break;}
        }
    }
    return 0;
}


