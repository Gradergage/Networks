#pragma comment(lib,"Ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <stdio.h>
#include <thread>
#include <vector>
#include <conio.h>
#include <pthread.h>
#include <mutex>
#define DEFAULT_BUFLEN 512
#define MAX_THREADS 10


std::mutex vMutex; //Client's vector mutex
std::mutex cMutex; //Console mutex


//Main structure which defines client parameters
struct CLIENT
{
    int id;
    SOCKET sock;
    sockaddr_in addr;
    bool connected;
    std::thread * cTh;
};
std::vector <CLIENT *> clients; //Vector of clients structures
int readn(SOCKET & sock, char *recvbuf, int recvbuflen)
{
        int iResult=0;
        memset(recvbuf,0,recvbuflen);
        int recievedBytes=0;

        while(recievedBytes<recvbuflen)
        {

            iResult = recv( sock, recvbuf+recievedBytes, recvbuflen-recievedBytes, 0 );
            if (iResult == INVALID_SOCKET) {
                return INVALID_SOCKET;
            }
            if (iResult == SOCKET_ERROR) {
                return SOCKET_ERROR;
            }
            recievedBytes+=iResult;
        }
        return 0;
}

// IP and Port of socket
void socketInfo(sockaddr_in & client_info)
{
    char *connected_ip = inet_ntoa(client_info.sin_addr);
    int port = ntohs(client_info.sin_port);
    printf("%s:%d\n", connected_ip,port);
}

// Disconnecting and clearing memory of client
bool disconnectClientById(int id)
{
    vMutex.lock();
    for(unsigned int i=0; i<clients.size() ; i++)
    {
        if(clients[i]->id==id)
        {
            clients[i]->connected=false;
            shutdown(clients[i]->sock, 2);
            closesocket(clients[i]->sock);
            delete(clients[i]);
            clients.erase(clients.begin()+i);
            cMutex.lock();
            printf("Client %d has been disconnected\n",id);
            cMutex.unlock();
            vMutex.unlock();
            return true;
        }
    }
    vMutex.unlock();
    return false;
}

// Checking of existing object
bool findById(int id)
{
    vMutex.lock();
    for(unsigned int i=0; i<clients.size() ; i++)
    {
        if(clients[i]->id==id)
        {
            vMutex.unlock();
            return true;
        }
    }
    vMutex.unlock();
    return false;
}

// Client thread which exchanges messages and queries
int handleClient(CLIENT *client)
{
    SOCKET cSocket = client->sock;
    unsigned int iResult;
    int cId=client->id;
    char recvbuf[DEFAULT_BUFLEN];
    char sendbuf[DEFAULT_BUFLEN];

    while(true)
    {
        iResult = readn(cSocket, recvbuf, sizeof(recvbuf));
        if(iResult==SOCKET_ERROR||iResult==INVALID_SOCKET)
        {
            return 0;
        }
        if(strcmp(recvbuf,"-close")==0)
        {

            disconnectClientById(cId);
            return 0;
        }
        else
        {
                cMutex.lock();
                if(client->connected)
                // printf("Message \"%s\" from %s:%d\n",recvbuf,connected_ip,port);
                printf("- Message \"%s\" from %d\n",recvbuf,cId);
                memset(recvbuf,0,DEFAULT_BUFLEN);
                cMutex.unlock();
        }
        if(client->connected)
        {
            char responcebuf[DEFAULT_BUFLEN]="Response";

            vMutex.lock();
            iResult = send( cSocket, responcebuf, DEFAULT_BUFLEN, 0 );
            if (iResult == SOCKET_ERROR|| iResult==INVALID_SOCKET) {
                printf("send failed: %d\n", WSAGetLastError());
                disconnectClientById(cId);
                return 1;
            }
            vMutex.unlock();
        }
        else
        {
            disconnectClientById(cId);
            return 0;
        }
    }
    return 0;
}

// List of all clients with id
void showAllClients()
{
    cMutex.lock();
    printf("\nClients connected:\n\n");
    int i=0;
    vMutex.lock();
    if (clients.empty())
    {
        printf("EMPTY\n");
        vMutex.unlock();
        cMutex.unlock();
        return;
    }
    for ( CLIENT * n : clients ) {
        i++;
        printf("%d) id:%d ",i,n->id);
        socketInfo((sockaddr_in &)n->addr);
    }
    vMutex.unlock();
    printf("\n");
    cMutex.unlock();
}
int disconnectAll()
{
    for(CLIENT *n : clients)
    {

            int cId=n->id;
            shutdown(n->sock, 2);
            closesocket(n->sock);
            n->connected=false;
            n->cTh->join();
            delete(n->cTh);
            delete(n);
            vMutex.lock();
            vMutex.unlock();
            cMutex.lock();
            printf("Client %d has been disconnected\n",cId);
            cMutex.unlock();
    }
    clients.erase(clients.begin(),clients.end());
    return 0;
}
// Unique id from ip and port
int hashByIp(char* ip, int len, int port)
{
    int sum=1;
    for(int i=0; i<len; i++)
    {
        sum*=(ip[i]-35);

    }
    return sum+port;
}

// Function accepting new clients and providing new threads to them
int acceptThread(SOCKET & sock)
{

    while(true)
   {
        SOCKET acceptSocket;
        CLIENT * entrySocket=new CLIENT();
        sockaddr_in client_info={0};
        int len = sizeof(client_info);
        if(sock==INVALID_SOCKET || sock==SOCKET_ERROR)
        {
            break;
        }
        else
        {
            acceptSocket = accept(sock, (SOCKADDR *) & client_info, & len);
            if(acceptSocket==INVALID_SOCKET)
            {
                break;
            }
        }

        // Attaching socket's info to socket itself
        char* innerIp=inet_ntoa(client_info.sin_addr);
        entrySocket->addr = client_info;
        entrySocket->sock = acceptSocket;
        entrySocket->connected=true;
        int id=hashByIp(innerIp,sizeof(innerIp),client_info.sin_port);
        entrySocket->id=id;
        int port=ntohs(entrySocket->addr.sin_port);

        std::thread *sockTh= new std::thread(handleClient, std::ref(entrySocket));
        entrySocket->cTh=sockTh;

        vMutex.lock();
        clients.push_back(entrySocket);
        vMutex.unlock();

        cMutex.lock();
        printf("New client %s:%d connected using id: %d\n", innerIp,port,id);
        cMutex.unlock();
   }

   disconnectAll();
   cMutex.lock();
   printf("Accepting closed\n");
   cMutex.unlock();
   return 0;
}

int main()
{
    WSADATA wsaData = {0};

    int iResult=0;
    int iFamily = AF_INET;
    int iType = SOCK_STREAM;
    int iProtocol = IPPROTO_TCP;

    char IP_ADDRESS[] = "127.0.0.1";
    int PORT=27015;

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
        printf("bind function failed with error %d\n", WSAGetLastError());
        iResult = closesocket(sock);
        if (iResult == SOCKET_ERROR)
            printf("closesocket function failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("-Starting server at %s:%d\n",IP_ADDRESS,PORT);



    if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
        wprintf(L"listen function failed with error: %d\n", WSAGetLastError());



    // Starting accept thread
    std::thread acceptTh(acceptThread, std::ref(sock));

    // Server menu working in main thread
    while(true)
    {
        cMutex.lock();
        printf("\nMENU \n1 - show all clients menu\n2 - close connection with some client\n3 - shutdown server\n\n");
        cMutex.unlock();

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
            case '4':{

                break;}
            case '3':{
                cMutex.lock();
                printf("\nServer is closing...\n");
                cMutex.unlock();

                shutdown(sock,2);
                closesocket(sock);

                cMutex.lock();
                printf("Accepting is closing...\n");
                cMutex.unlock();

                acceptTh.join();

                cMutex.lock();
                printf("Server closed\n");
                cMutex.unlock();
                return 0;
                break;}
        }
    }
    return 0;
}


