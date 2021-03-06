#pragma comment(lib,"Ws2_32.lib")
#include <WinSock2.h>
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
#define BLOCK_SIZE 16384
#define MAX_THREADS 10
#define NAME_OF_VAR(VAR) #VAR

std::mutex vMutex; //Clients vector mutex
std::mutex cMutex; //Console mutex
std::mutex nMutex; //Numbers vector mutex
std::mutex bMutex; //Bounds vector mutex
std::mutex wMutex; //Write file mutex
std::mutex mMutex; //Write file mutex
std::mutex aMutex; //Write file mutex
std::mutex dMutex; //Write file mutex
int lowBound;
int maxNumber=0;
char IP_ADDRESS[DEFAULT_BUFLEN];

//Main structure which defines client parameters
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


std::vector <int> queueBounds;
std::vector <CLIENT *> clients; //Vector of clients structures
std::vector <CLIENT_INFO> infos;
std::vector <int>   allNumbers;

int getBound()
{
    if(queueBounds.empty())
    {
        return lowBound;
    }
    else
    {
        int bound=queueBounds.back();
        queueBounds.pop_back();
        return bound;
    }
}


int writeBounds()
{
    std::ofstream ofQueues("queues.txt");
    for(int n : queueBounds)
    {
        ofQueues<<n<<"\r\n";
    }
    return 0;
}

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

void load_infos()
{
    std::ifstream inLogin("logins.txt");
    std::ifstream inPassword("Passwords.txt");

    char data[DEFAULT_BUFLEN];
    while(inLogin>>data)
    {

        CLIENT_INFO new_info={};
        strcpy(new_info.login,data);
        inPassword>>data;
        strcpy(new_info.password,data);
        infos.push_back(new_info);
    }

    std::ifstream inQueues("queues.txt");
    while(inQueues>>data)
    {
        queueBounds.push_back(atoi(data));
    }
    inLogin.close();
    inPassword.close();
    inQueues.close();
}

void add_infos(CLIENT_INFO user)
{
    std::ofstream ofLogin("logins.txt",std::ios_base::app );
    std::ofstream ofPassword("Passwords.txt",std::ios_base::app );
    ofLogin<<user.login<<"\r\n";
    ofPassword<<user.password<<"\r\n";
    infos.push_back(user);
    ofLogin.close();
    ofPassword.close();
}
void loadSettings()
{
    std::ifstream inSettings("settings.txt");
    inSettings>>lowBound;
    inSettings>>IP_ADDRESS;
    load_infos();
}
void saveSettings()
{
    std::ofstream ofSettings("settings.txt");
    ofSettings<<lowBound<<"\r\n";
    ofSettings<<IP_ADDRESS<<"\r\n";

}
int save_numbers(std::vector <int> data)
{
    std::ofstream ofNumbers("numbers.txt",std::ios_base::app );
    for(int number : data)
    {
        ofNumbers<<number<<"\r\n";
    }
    ofNumbers.close();
    return 0;
}
int sendNNumbers(SOCKET sock,int n)
{
    char sendbuf[DEFAULT_BUFLEN];
    int iResult;
    if(n>allNumbers.size())
    {
        n=allNumbers.size();
    }
    if(n!=0)
    //Sending numbers
    {
        for(int i=0;i<n;i++)
        {
            snprintf(sendbuf,sizeof(sendbuf),"%d",allNumbers[allNumbers.size()-1-i]);
            iResult=send(sock,sendbuf,sizeof(sendbuf),0);
            if(iResult==SOCKET_ERROR||iResult==INVALID_SOCKET)
            {
                return -1;
            }
        }
    }
    strcpy(sendbuf,"-end");
    send(sock,sendbuf,sizeof(sendbuf),0);
    return 0;
}
int recieve_numbers(SOCKET sock,int bound)
{
    std::vector <int> numbers;

    char recvbuf[DEFAULT_BUFLEN]="";
    int iResult;
    while(true)
    {
        iResult=readn(sock,recvbuf,sizeof(recvbuf));
        if(iResult==SOCKET_ERROR||iResult==INVALID_SOCKET)
        {
            queueBounds.push_back(bound);
            return -1; //aborted
        }
        else if(strcmp(recvbuf,"-end")==0)
        {
            break;
        }
        else
        {
            numbers.push_back(atoi(recvbuf));
        }
    }
    mMutex.lock();
    maxNumber=numbers.back();
    mMutex.unlock();
    aMutex.lock();
    allNumbers.insert(std::end(allNumbers), std::begin(numbers), std::end(numbers));
    aMutex.unlock();
    wMutex.lock();
    save_numbers(numbers);
    wMutex.unlock();
    return 0;
}

int verify(CLIENT_INFO user)
{
    for(CLIENT *n: clients)
    {
        if(strcmp(user.login,n->info.login)==0)
        {
            if (n->info.online)
            return 1;
        }
    }
    for ( CLIENT_INFO n : infos )
    {
        if(strcmp(user.login,n.login)==0)
        {
            if(strcmp(user.password,n.password)==0)
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }
    }
    return -1;
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
            clients[i]->info.online=false;
            shutdown(clients[i]->sock, 2);
            closesocket(clients[i]->sock);
            cMutex.lock();
            printf("Client %s has been disconnected\n",clients[i]->info.login);
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

int auth(CLIENT *user)
{
    char recvbuf[DEFAULT_BUFLEN];

    int iResult = readn(user->sock,recvbuf,DEFAULT_BUFLEN);
    if(iResult==SOCKET_ERROR||iResult==INVALID_SOCKET)
    {
        disconnectClientById(user->id);
        return -1;
    }
    iResult = readn(user->sock,user->info.login,DEFAULT_BUFLEN);
    if(iResult==SOCKET_ERROR||iResult==INVALID_SOCKET)
    {
        disconnectClientById(user->id);
        return -1;
    }
    iResult = readn(user->sock,user->info.password,DEFAULT_BUFLEN);
    if(iResult==SOCKET_ERROR||iResult==INVALID_SOCKET)
    {
        disconnectClientById(user->id);
        return -1;
    }
    if(strcmp(recvbuf,"-reg")==0)
    {
        if(verify(user->info)>=0)
        {
            cMutex.lock();
            printf("- UAE from %d\n",user->id);
            cMutex.unlock();

            char sendbuf[DEFAULT_BUFLEN]="-uae"; //user already exists
            send( user->sock, sendbuf, sizeof(sendbuf), 0 );
            return 1;
        }
        else
        {
            cMutex.lock();
            printf("- Registrated SA from %s\n",user->info.login);
            cMutex.unlock();

            add_infos(user->info);
            char sendbuf[DEFAULT_BUFLEN]="-sa"; //successful authentication
            user->info.online=true;
            send( user->sock, sendbuf, sizeof(sendbuf), 0 );
            return 0;
        }
    }
    else{
        if(strcmp(recvbuf,"-auth")==0)
        {
            if(verify(user->info)==0)
            {
                cMutex.lock();
                printf("- SA from %s\n",user->info.login);
                cMutex.unlock();

                char sendbuf[DEFAULT_BUFLEN]="-sa"; //successful authentication
                user->info.online=true;
                send( user->sock, sendbuf, sizeof(sendbuf), 0 );
                return 0;
            }
            else if(verify(user->info)>0)
            {
                cMutex.lock();
                printf("- UAE from %d\n",user->id);
                cMutex.unlock();

                char sendbuf[DEFAULT_BUFLEN]="-uae"; //user already exists
                send( user->sock, sendbuf, sizeof(sendbuf), 0 );
                return 1;
            }
            else
                {
                    cMutex.lock();
                printf("- ILP from %d\n",user->id);
                cMutex.unlock();

                char sendbuf[DEFAULT_BUFLEN]="-ilp"; //invalid login or password
                send( user->sock, sendbuf, sizeof(sendbuf), 0 );
                return 1;

            }
        }
    }
    return 1;
}
int sendMaxNumber(SOCKET sock)
{
    char sendbuf[DEFAULT_BUFLEN];
    mMutex.lock();
    snprintf(sendbuf,sizeof(sendbuf),"%d",maxNumber);
    mMutex.unlock();
    int iResult=send(sock,sendbuf,sizeof(sendbuf),0);
    if(iResult==SOCKET_ERROR||iResult==INVALID_SOCKET)
    {
            return -1;
    }
    return 0;
}
// Client thread which exchanges messages and queries
int handleClient(CLIENT *client)
{
    SOCKET cSocket = client->sock;
    unsigned int iResult;
    int cId=client->id;
    char recvbuf[DEFAULT_BUFLEN];
    char sendbuf[DEFAULT_BUFLEN];
    int authFailed=1;
    char name[DEFAULT_BUFLEN];
    while(true)
    {
        authFailed = auth(client);

        if(authFailed==0)
        {
            break;
        }
        if(authFailed<0)
        {
            return 0;
        }
    }
    strcpy(name,client->info.login);
    //Main cycle
    while(true)
    {
        if(!client->connected)
        {
            disconnectClientById(cId);
            return 0;
        }
        //First incoming message
        iResult = readn(cSocket, recvbuf, sizeof(recvbuf));
        //errors and disconnect command
        if(iResult==SOCKET_ERROR||iResult==INVALID_SOCKET||strcmp(recvbuf,"-close")==0)
        {
            disconnectClientById(cId);
            return 0;
        }


        if(strcmp(recvbuf,"-bound")==0)
        {
            bMutex.lock();
            int newBound=getBound();
            bMutex.unlock();
            snprintf(sendbuf,sizeof(sendbuf),"%d",newBound);
            send(cSocket,sendbuf,sizeof(sendbuf),0);
            bMutex.lock();
            lowBound+=BLOCK_SIZE;
            bMutex.unlock();
            saveSettings();
        }
        //Start of sequence command
        if(strcmp(recvbuf,"-numbers")==0)
        {
            iResult = readn(cSocket, recvbuf, sizeof(recvbuf));
            int bound=atoi(recvbuf);

            bMutex.lock();
            if(recieve_numbers(cSocket,bound)<0)
            {
                disconnectClientById(cId);
                return 0;
            }
            bMutex.unlock();

        }
        //Get numbers sequence
        if(strcmp(recvbuf,"-getn")==0)
        {
            iResult = readn(cSocket, recvbuf, sizeof(recvbuf));
            int n=atoi(recvbuf);
            //Recieving defined count of numbers
            aMutex.lock();
            if(sendNNumbers(cSocket,n)<0)
            {
                disconnectClientById(cId);
                return 0;
            }
            aMutex.unlock();
        }
        if(strcmp(recvbuf,"-sts")==0)
        {
            cMutex.lock();
            printf("TRANSMIT STARTED from %s\n", name);
            cMutex.unlock();
        }
        if(strcmp(recvbuf,"-ets")==0)
        {
            cMutex.lock();
            printf("TRANSMIT ENDED from %s\n", name);
            cMutex.unlock();
        }
        if(strcmp(recvbuf,"-max")==0)
        {
            iResult=sendMaxNumber(cSocket);
            if(iResult<0)
            {
                disconnectClientById(cId);
                return 0;
            }
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
        if(n->connected)
        {
            i++;
            printf("%d) id:%d %s ",i,n->id,n->info.login);
            socketInfo((sockaddr_in &)n->addr);
        }
    }
    vMutex.unlock();
    printf("\n");
    cMutex.unlock();
}
void showAllInfos()
{
    cMutex.lock();
    vMutex.lock();
    for ( CLIENT_INFO n : infos )
    {
        printf("%s : %s\n", n.login,n.password);
    }
    vMutex.unlock();
    cMutex.unlock();
}

int disconnectAll()
{
    for(CLIENT *n : clients)
    {
            shutdown(n->sock, 2);
            closesocket(n->sock);
            n->connected=false;
            n->info.online=false;
    }
    for(CLIENT *n : clients)
    {
            n->cTh->join();
            delete(n->cTh);
            delete(n);
    }
    clients.clear();
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
   writeBounds();
   return 0;
}

int main()
{
    loadSettings();
    WSADATA wsaData = {0};

    int iResult=0;
    int iFamily = AF_INET;
    int iType = SOCK_STREAM;
    int iProtocol = IPPROTO_TCP;
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
    printf("Starting server at %s:%d\n",IP_ADDRESS,PORT);

    if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
        wprintf(L"listen function failed with error: %d\n", WSAGetLastError());

    // Starting accept thread
    std::thread acceptTh(acceptThread, std::ref(sock));
    // Server menu working in main thread
    while(true)
    {
        cMutex.lock();
        printf("\nMENU \n1 - show all clients menu\n2 - close connection with some client\n3 - show infos\n4 - show max\n6 - shutdown server\n\n");
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
            case '3':{
                showAllInfos();
                break;}
            case '4':{
                printf("Max number is %d\n", maxNumber);

                break;}
            case '6':{
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
                printf("Accepting closed\n");
                cMutex.unlock();

                saveSettings();
                cMutex.lock();
                printf("Server closed\n");
                cMutex.unlock();
                return 0;
                break;}
        }
    }
    return 0;
}


