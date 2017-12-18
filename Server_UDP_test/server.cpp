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
#include <fstream>
#define DEFAULT_BUFLEN 512
#define PREFIXLEN 11
#define BLOCK_SIZE 16384
#define MAX_THREADS 10
#include "UDP.h"
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
char IP_ADDRESS[DEFAULT_BUFLEN];// = "192.168.213.1";

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
    int sock;
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

void load_infos()
{
    std::ifstream inLogin("logins.txt");
    std::ifstream inPassword("Passwords.txt");

    char data[DEFAULT_BUFLEN];
    while(inLogin>>data)
    {

        CLIENT_INFO new_info= {};
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
int sendNNumbers(int sock,int n)
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
        for(int i=0; i<n; i++)
        {
            snprintf(sendbuf,sizeof(sendbuf),"%d",allNumbers[allNumbers.size()-1-i]);
            iResult=sendUDP(sock,sendbuf,sizeof(sendbuf));
            if(iResult<0)
            {
                close(sock);
                return -1;
            }
        }
    }
    strcpy(sendbuf,"-end");
    iResult=sendUDP(sock,sendbuf,sizeof(sendbuf));
    if(iResult<0)
    {
        close(sock);
        return -1;
    }
    return 0;
}
int recieve_numbers(int sock,int bound)
{
    std::vector <int> numbers;

    char recvbuf[DEFAULT_BUFLEN]="";
    int iResult;
    while(true)
    {
        iResult=readUDP(sock,recvbuf,sizeof(recvbuf));
        if(iResult<0)
        {
            //bMutex.lock();
            queueBounds.push_back(bound);
            //bMutex.unlock();
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
    /* for(int n : numbers)
     {
         printf("%d \n",n);
     } */
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
//    cMutex.lock();
//    printf("entered verify\n");
//    cMutex.unlock();
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
int disconnectInactive()
{
    vMutex.lock();
    int i=0;
    for(CLIENT *n : clients)
    {
        if(!n->connected)
        {
            n->cTh->join();
            delete(n->cTh);
            delete(n);
        }
        clients.erase(clients.begin()+i);
        i++;
    }

    vMutex.lock();
    return 0;
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
            close(clients[i]->sock);
            cMutex.lock();
            printf("Client %s has been disconnected\n",clients[i]->info.login);
            cMutex.unlock();
            // delete(clients[i]);
            //clients.erase(clients.begin()+i);
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
//    cMutex.lock();
//    printf("Entered Auth\n");
//    cMutex.unlock();
    //keyword
//    cMutex.lock();
//    printf("Entered auth \n");
//    cMutex.unlock();

    int iResult = readUDP(user->sock,recvbuf,DEFAULT_BUFLEN);
    if(iResult<0)
    {
        disconnectClientById(user->id);
        return -1;
    }
//    cMutex.lock();
//    printf("Received param\n");
//    cMutex.unlock();

    iResult = readUDP(user->sock,user->info.login,DEFAULT_BUFLEN);
    if(iResult<0)
    {
        disconnectClientById(user->id);
        return -1;
    }
//    cMutex.lock();
//    printf("Received login\n");
//    cMutex.unlock();
    iResult = readUDP(user->sock,user->info.password,DEFAULT_BUFLEN);
    if(iResult<0)
    {
        disconnectClientById(user->id);
        return -1;
    }
//    cMutex.lock();
//    printf("Received password\n");
//    cMutex.unlock();
    if(strcmp(recvbuf,"-reg")==0)
    {
        vMutex.lock();
        iResult=verify(user->info);
        vMutex.unlock();
        if(iResult>=0)
        {
            cMutex.lock();
            printf("- UAE from %d\n",user->id);
            cMutex.unlock();

            char sendbuf[DEFAULT_BUFLEN]="-uae"; //user already exists

            iResult = sendUDP( user->sock, sendbuf, sizeof(sendbuf));
            if(iResult<0)
            {
                close(user->sock);
                return -1;
            }
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
            iResult = sendUDP( user->sock, sendbuf, sizeof(sendbuf));
            if(iResult<0)
            {
                close(user->sock);
                return -1;
            }
            return 0;
        }
    }
    else
    {
        if(strcmp(recvbuf,"-auth")==0)
        {
            vMutex.lock();
            iResult=verify(user->info);
            vMutex.unlock();
            if(iResult>=0)
            {
                cMutex.lock();
                printf("- SA from %s\n",user->info.login);
                cMutex.unlock();

                char sendbuf[DEFAULT_BUFLEN]="-sa"; //successful authentication
                user->info.online=true;

                iResult = sendUDP( user->sock, sendbuf, sizeof(sendbuf));
                if(iResult<0)
                {
                    close(user->sock);
                    return -1;
                }
                return 0;
            }
            else
            {

                vMutex.lock();
                iResult=verify(user->info);
                vMutex.unlock();
                if(iResult>=0)
                {
                    cMutex.lock();
                    printf("- UAE from %d\n",user->id);
                    cMutex.unlock();

                    char sendbuf[DEFAULT_BUFLEN]="-uae"; //user already exists

                    iResult = sendUDP( user->sock, sendbuf, sizeof(sendbuf));
                    if(iResult<0)
                    {
                        close(user->sock);
                        return -1;
                    }
                    return 1;
                }
                else
                {
                    cMutex.lock();
                    printf("- ILP from %d\n",user->id);
                    cMutex.unlock();

                    char sendbuf[DEFAULT_BUFLEN]="-ilp";
                    cMutex.lock();
                    printf("Try send ilp\n");
                    cMutex.unlock();//invalid login or password
                    iResult = sendUDP( user->sock, sendbuf, sizeof(sendbuf));
                    if(iResult<0)
                    {
                        close(user->sock);
                        return -1;
                    }
                    return 1;

                }
            }
        }
    }
    return 1;
}
int sendMaxNumber(int sock)
{
    char sendbuf[DEFAULT_BUFLEN];
    mMutex.lock();
    snprintf(sendbuf,sizeof(sendbuf),"%d",maxNumber);
    mMutex.unlock();
    int iResult=sendUDP(sock,sendbuf,sizeof(sendbuf));
    if(iResult<0)
    {
        return -1;
    }
    return 0;
}
// Client thread which exchanges messages and queries
int handleClient(CLIENT *client)
{
//    cMutex.lock();
//    printf("In client thread info socket: %d ",client->sock);
//    char *connected_ip = inet_ntoa(client->addr.sin_addr);
//    int port = ntohs(client->addr.sin_port);
//    printf(" %s:%d\n", connected_ip,port);
//    cMutex.unlock();

    int cSocket = client->sock;
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
        iResult = readUDP(cSocket, recvbuf, sizeof(recvbuf));
        //errors and disconnect command
        if(iResult<0||strcmp(recvbuf,"-close")==0)
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
            sendUDP(cSocket,sendbuf,sizeof(sendbuf));
            /*  cMutex.lock();
              printf("Bound %d sended to client %d\n",newBound,cId);
              cMutex.unlock(); */
            bMutex.lock();
            lowBound+=BLOCK_SIZE;
            bMutex.unlock();
            saveSettings();
        }
        //Start of sequence command
        if(strcmp(recvbuf,"-numbers")==0)
        {
            iResult = readUDP(cSocket, recvbuf, sizeof(recvbuf));
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
            iResult = readUDP(cSocket, recvbuf, sizeof(recvbuf));
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
    for ( CLIENT * n : clients )
    {
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
    // disconnectInactive();
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
        close(n->sock);
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
int acceptThread(int & sock)
{

    while(true)
    {
        int acceptSocket;
        CLIENT * entrySocket=new CLIENT();
        sockaddr_in client_info= {0};
        // int len = sizeof(client_info);
        socklen_t len =sizeof(client_info);
        if(sock<0)
        {
            break;
        }
        char recvbuf[DEFAULT_BUFLEN];
        //acceptSocket=accept(sock,(sockaddr*)&client_info,&len);
        recvfrom(sock,recvbuf,sizeof(recvbuf),0,(sockaddr*)&client_info, &len);
        if(strcmp(recvbuf,"-req")==0)
        {
           // printf("%s\n", recvbuf);

            // Attaching socket's info to socket itself



            acceptSocket=socket(AF_INET,SOCK_DGRAM,0);
            sockaddr_in service;
            service.sin_family = AF_INET;
            service.sin_addr.s_addr =inet_addr(IP_ADDRESS);
            service.sin_port = htons(0);
            socklen_t lens=sizeof(service);

            if(bind(acceptSocket,(struct sockaddr *)&service,lens)!=0)
            {
              //  perror("Bind failed");
                delete(entrySocket);
                break;
            }

         //   printf("Accepted socket %d ",acceptSocket);
            socklen_t size = sizeof(service);
            getsockname(acceptSocket, (sockaddr*)&service, &size);
    //        printf("%d\n", ntohs(service.sin_port) );

            char tempbuf[DEFAULT_BUFLEN]="-resp";
            snprintf(tempbuf,sizeof(tempbuf),"%d",ntohs(service.sin_port));
           // snprintf(tempbuf,sizeof(tempbuf),"%d",ntohs(service.sin_port));
           /* if(sendto(acceptSocket,tempbuf,sizeof(tempbuf),0,(struct sockaddr *)&client_info,len)<0)
            {
                perror("Resp failed");
                delete(entrySocket);
                break;
            } */
            if(sendto(sock,tempbuf,sizeof(tempbuf),0,(struct sockaddr *)&client_info,len)<0)
            {
              //  perror("Resp failed");
                delete(entrySocket);
                break;
            }
            if(sendto(sock,IP_ADDRESS,sizeof(IP_ADDRESS),0,(struct sockaddr *)&client_info,len)<0)
            {
              //  perror("Resp failed");
                delete(entrySocket);
                break;
            }

          //  printf("Sended %s\n", tempbuf);

            if(recvfrom(acceptSocket,IP_ADDRESS,sizeof(IP_ADDRESS),0,(struct sockaddr *)&client_info,&len)<0)
            {
               // perror("Resp failed");
                delete(entrySocket);
                break;
            }
            if(connect(acceptSocket,(struct sockaddr *)&client_info,len)!=0)
            {
               // perror("Conn failed");
                delete(entrySocket);
                break;
            }
//            send(acceptSocket,tempbuf,sizeof(tempbuf),0);
//            recv(acceptSocket,tempbuf,sizeof(tempbuf),0);
            printf("%s",tempbuf);
            char* innerIp=inet_ntoa(client_info.sin_addr);
            entrySocket->addr = client_info;
            entrySocket->sock = acceptSocket;
            entrySocket->connected=true;
            int port=ntohs(entrySocket->addr.sin_port);
            int id=port;//hashByIp(innerIp,sizeof(innerIp),client_info.sin_port);
            entrySocket->id=id;


             std::thread *sockTh= new std::thread(handleClient, entrySocket);
             entrySocket->cTh=sockTh;

            vMutex.lock();
            clients.push_back(entrySocket);
            vMutex.unlock();
            cMutex.lock();
            printf("New client %s:%d connected using id: %d\n", innerIp,port,id);
            cMutex.unlock();
        }
        else
        {
            continue;
        }
        //  disconnectInactive();
    }

    disconnectAll();
    writeBounds();
    return 0;
}

int main()
{
    loadSettings();

    int iResult=0;
    int iFamily = AF_INET;
    int iType = SOCK_DGRAM;
    int iProtocol = 0;


    int PORT=27015;

    int sock;

    sock = socket(iFamily,iType,iProtocol);
    if(sock<0)
    {
        perror("Error in socket");
        return 1;
    }

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    service.sin_port = htons(PORT);

    iResult = bind(sock, (sockaddr *) & service, sizeof (service));
    if (iResult<0)
    {
        perror("bind function failed with error ");
        return 0;
    }
    printf("-Starting server at %s:%d\n",IP_ADDRESS,PORT);

    /*if (listen(sock, SOMAXCONN)<0)
    {
        perror("Listen error");
    } */

//   cMutex.lock();
    // printf("\nMENU \n1 - show all clients menu\n2 - close connection with some client\n3 - show infos\n4 - Show bound\n6 - shutdown server\n\n");
    //  cMutex.unlock();

    // Starting accept thread
    std::thread acceptTh(acceptThread, std::ref(sock));
    //acceptThread(sock);
    // Server menu working in main thread
    while(true)
    {
        cMutex.lock();
        printf("\nMENU \n1 - show all clients menu\n2 - close connection with some client\n3 - show infos\n4 - show max\n6 - shutdown server\n\n");
        cMutex.unlock();
        // char choise = getch();
        char choise;
        std::cin>>choise;

        //printf("\nMENU \n1 - show all clients menu\n2 - close connection with some client\n3 - show infos\n4 - Show bound\n6 - shutdown server\n\n");
        //
        switch(choise)
        {
        case '1':
        {
            showAllClients();
            break;
        }
        case '2':
        {
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
        case '3':
        {
            showAllInfos();
            break;
        }
        case '4':
        {
            printf("Max number is %d\n", maxNumber);

            break;
        }
        case '6':
        {
            cMutex.lock();
            printf("\nServer is closing...\n");
            cMutex.unlock();

            shutdown(sock,2);
            close(sock);

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
            break;
        }
        }
    }
    return 0;
}


