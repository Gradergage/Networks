#pragma comment(lib,"Ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <conio.h>
#include <pthread.h>
#include <thread>
#include <mutex>
#include <fcntl.h>
#define DEFAULT_BUFLEN 512
#define PREFIXLEN 11
#define BLOCK_SIZE 16384
#include "UDP.h"
unsigned short AUTO_TRANSMIT = 0;
unsigned short calculate = 0;
//std::vector <int> numbers;
std::mutex cMutex;
std::mutex vMutex;
std::mutex sMutex;


int startNumber=100;
int endNumber;

//Sending calculated numbers
void send_numbers(int sock,std::vector <int> numbers)
{
    char sendbuf[DEFAULT_BUFLEN]="-numbers";

    sendUDP(sock,sendbuf,sizeof(sendbuf));
    for(int n : numbers)
    {
        snprintf(sendbuf,sizeof(sendbuf),"%d",n);
        sendUDP(sock,sendbuf,sizeof(sendbuf));
    }
    strcpy(sendbuf,"-end");
    sendUDP(sock,sendbuf,sizeof(sendbuf));
}
int getBound(SOCKET sock)
{
    char sendbuf[DEFAULT_BUFLEN]="-bound";
    sendUDP(sock,sendbuf,sizeof(sendbuf));
    readUDP(sock,sendbuf,sizeof(sendbuf));
    return atoi(sendbuf);
}
int calculateNumbers(SOCKET sock,int bound)
{
    std::vector <int> numbers;
    for(int n=bound+1; n<bound+BLOCK_SIZE; n++ )
    {
        if(n%10 == 0 || n%2 == 0 || n%5 == 0)
        {
            continue;
        }

        int j=2;

        while(true)
        {

            if(n%j==0)
            {
                break;
            }

            if(j*j-1 > n)
            {

                numbers.push_back(n);
                break;
            }
            j++;
            //	usleep(100);
        }
    }
    /*cMutex.lock();
    printf("Calculating in bounds %d : %d completed\n",bound,bound+BLOCK_SIZE);
    cMutex.unlock(); */
    //send_numbers(sock,numbers);
    char sendbuf[DEFAULT_BUFLEN]="-numbers";
    if(sendUDP(sock,sendbuf,sizeof(sendbuf))<0)
    {
        closesocket(sock);
        return 0;
    }
    //send bound
    snprintf(sendbuf,sizeof(sendbuf),"%d",bound);
    if(sendUDP(sock,sendbuf,sizeof(sendbuf))<0)
    {
        closesocket(sock);
        return 0;
    }
    //send numbers
    for(int n : numbers)
    {
        /* cMutex.lock();
         printf("%d\n",n);
         cMutex.unlock();*/
        snprintf(sendbuf,sizeof(sendbuf),"%d",n);
        sendUDP(sock,sendbuf,sizeof(sendbuf));
    }
    memset(sendbuf,NULL,DEFAULT_BUFLEN);
    strcpy(sendbuf,"-end");
    if(sendUDP(sock,sendbuf,sizeof(sendbuf))<0);
    {
        // perror("Send numbers");
        return 0;
    }
    /*   cMutex.lock();
       printf("Send completed\n");
       cMutex.unlock(); */
    return 0;
}
int calculateAndTransmit(SOCKET sock)
{
    while(AUTO_TRANSMIT)
    {
        if(sock <0)
        {
            return 0;
        }
        sMutex.lock();
        int bound=getBound(sock);
        calculateNumbers(sock,bound);
        sMutex.unlock();
    }
    return 0;
}


int auth(SOCKET sock,char param[],int paramLen)
{
    char login[DEFAULT_BUFLEN];
    char password[DEFAULT_BUFLEN];
    char recvbuf[DEFAULT_BUFLEN];
    printf("Enter login: \n");
    // scanf("%s",login);
    std::cin>>login;
    printf("Enter password: \n");
    //scanf("%s",password);
    std::cin>>password;

    if(sendUDP(sock, param, DEFAULT_BUFLEN)<0)
    {
        closesocket(sock);
        return 0;
    }
    //  printf("param sended: \n");
    // printf("param sended: \n");
    if(sendUDP(sock, login, sizeof(login))<0)
    {
        closesocket(sock);
        return 0;
    }
//    printf("login sended: \n");
    //  printf("login sended: \n");
    if(sendUDP(sock, password, sizeof(password))<0)
    {
        closesocket(sock);
        return 0;
    }
    //  printf("password sended: \n");
//   printf("password sended: \n");

    if(readUDP(sock, recvbuf, DEFAULT_BUFLEN)<0)
    {
        closesocket(sock);
        return 0;
    }

//    printf("Param received!\n");
    if(strcmp(recvbuf,"-sa")==0)
    {
        cMutex.lock();
        printf("Successfull auth!\n");
        cMutex.unlock();
        return 0;
    }
    if(strcmp(recvbuf,"-ilp")==0)
    {
        cMutex.lock();
        printf("Invalid login or password!\n");
        cMutex.unlock();
        return 1;
    }
    if(strcmp(recvbuf,"-uae")==0)
    {
        cMutex.lock();
        printf("User online or username already exists!\n");
        cMutex.unlock();
        return -1;
    }
    return 0;
}


void getNNumbers(SOCKET sock,int n)
{

    std::vector<int> numbers;
    char recvbuf[DEFAULT_BUFLEN]="-getn";
    sendUDP(sock,recvbuf,sizeof(recvbuf));

    snprintf(recvbuf,sizeof(recvbuf),"%d",n);
    sendUDP(sock,recvbuf,sizeof(recvbuf));

    while(true)
    {

        readUDP(sock,recvbuf,sizeof(recvbuf));
        if(strcmp(recvbuf,"-end")==0)
        {
            break;
        }
        int num=atoi(recvbuf);
        numbers.push_back(num);

    }
    int ctr=0;
    for(int num : numbers)
    {
        cMutex.lock();

        if(ctr==6)
        {
            printf("\n");
            ctr=0;
        }
        printf("%d ",num);
        ctr++;

        cMutex.unlock();
    }
    printf("\n");
}
void changeTransmitType(int sock)
{
    AUTO_TRANSMIT^=0x1;
    if(AUTO_TRANSMIT)
    {
        char sendbuf[DEFAULT_BUFLEN]="-sts";
        sendUDP(sock,sendbuf,sizeof(sendbuf));
    }
    else
    {
        char sendbuf[DEFAULT_BUFLEN]="-ets";
        sendUDP(sock,sendbuf,sizeof(sendbuf));
    }
}
int getMaxNumber(SOCKET sock)
{
    char sendbuf[DEFAULT_BUFLEN]="-max";
    sendUDP(sock,sendbuf,sizeof(sendbuf));
    readUDP(sock,sendbuf,sizeof(sendbuf));
    printf("Current max number is: %s\n",sendbuf);
    return 0;
}
int main()
{
    WSADATA wsaData = {0};
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("-WSAStartup failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    SOCKET sock;
    struct sockaddr_in addr;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iResult != 0)
    {
        printf("Socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(27015); // или любой другой порт...
    addr.sin_addr.s_addr = inet_addr("192.168.182.128");
    int addrsize=sizeof(addr);

    sockaddr_in myAddr;
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(0); // или любой другой порт...
    myAddr.sin_addr.s_addr = inet_addr("192.168.182.1");
    iResult = bind(sock, (SOCKADDR *) & myAddr, sizeof (myAddr));
    if (iResult != 0)
    {
        printf("Bind failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    /*  iResult=connect(sock, (struct sockaddr *)&addr, sizeof(addr));
      if (iResult != 0)
      {
          printf("Connect failed: %d\n", WSAGetLastError());
          WSACleanup();
          return 1;
      } */
    // std::thread calcThread(calculatingThread);
    char sendbuf[DEFAULT_BUFLEN]="-req";
    iResult=sendto(sock,sendbuf,sizeof(sendbuf),0, (struct sockaddr *)&addr, sizeof(addr));
    if (iResult < 0)
    {
        printf("Request failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    iResult=recvfrom(sock,sendbuf,sizeof(sendbuf),0, NULL, NULL);
 //   printf("%s ",sendbuf);

    int newport=atoi(sendbuf);

    iResult=recvfrom(sock,sendbuf,sizeof(sendbuf),0, NULL, NULL);
 //   printf("%s\n",sendbuf);
    closesocket(sock);
    sock=socket(AF_INET, SOCK_DGRAM, 0);
    iResult = bind(sock, (SOCKADDR *) & myAddr, sizeof (myAddr));
    if (iResult != 0)
    {
        printf("Bind failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    sockaddr_in addr3;
    addr3.sin_family = AF_INET;
    addr3.sin_port = htons((newport)); // или любой другой порт...
    addr3.sin_addr.s_addr = inet_addr(sendbuf);
    int addrsize3=sizeof(addr3);
    iResult=connect(sock, (struct sockaddr *)&addr3, sizeof(addr3));
    if (iResult != 0)
    {
        printf("Connect failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    iResult=send(sock,sendbuf,sizeof(sendbuf),0);
//    iResult=recv(sock,sendbuf,sizeof(sendbuf),0);
//    printf("%s\n",sendbuf);
    int authFailed=1;
    printf("Client started\n\n");
    while(true)
    {
        if(authFailed==0)
        {
            break;
        }

        char authParam[]="-auth";
        char regParam[]="-reg";
        cMutex.lock();
        printf("1 - Authorize\n2 - Register\n\n");
        cMutex.unlock();

        char choise=getch();
        // std::cin>>choise;
        //  system("clear");

        switch(choise)
        {
        case '1':
        {
            authFailed=auth(sock, authParam,sizeof(authParam));
            break;
        }
        case '2':
        {
            authFailed=auth(sock, regParam,sizeof(regParam));
            break;
        }
        }
    }
    // printf("Exchange started \nAUTO_TRANSMIT %d\n",AUTO_TRANSMIT);
    /*  cMutex.lock();
          printf("MENU\n\
          1 - Calculating and transmitting %d\n\
          2 - Enable transmitting %d\n\
          3 - Close server\n\n",calculate,AUTO_TRANSMIT);
      cMutex.unlock(); */
    std::thread *calcsThread;//=new std::thread(calculateAndTransmit,std::ref(sock));
    while(true)
    {
        //system("clear");
        cMutex.lock();
        printf("MENU\n1 - Enable calculating and transmitting %d\n\
2 - Get max number\n\
3 - Get N last numbers\n\
4 - Exit\n\n",AUTO_TRANSMIT);
        cMutex.unlock();
        char choise=getch();
        // std::cin>>choise;
        /*  system("clear");
          cMutex.lock();
          printf("MENU\n\
          1 - Calculating and transmitting %d\n\
          2 - Enable calculating and transmitting %d\n\
          3 - Exit\n\n",calculate,AUTO_TRANSMIT);
          cMutex.unlock();*/



        switch(choise)
        {
        case '2':
        {
            //  cal
            calculate^=0x1;
            //calculateAndTransmit(sock);
            sMutex.lock();
            getMaxNumber(sock);
            sMutex.unlock();
            break;
        }
        case '1':
        {
            //inverce auto  transmitting
            AUTO_TRANSMIT^=0x1;
            if(AUTO_TRANSMIT)
            {
                char sendbuf[DEFAULT_BUFLEN]="-sts";
                sendUDP(sock,sendbuf,sizeof(sendbuf));
                calcsThread=new std::thread(calculateAndTransmit,std::ref(sock));
            }
            else
            {
                calcsThread->join();
                delete(calcsThread);
                char sendbuf[DEFAULT_BUFLEN]="-ets";
                sendUDP(sock,sendbuf,sizeof(sendbuf));
            }
            break;
        }
        case '3':
        {
            int cnt;
            printf("Enter count of numbers: ");
            std::cin>>cnt;
            sMutex.lock();
            getNNumbers(sock,cnt);
            sMutex.unlock();
            break;
        }
        case '4':
        {
            sMutex.lock();
            char sendbuf[DEFAULT_BUFLEN]="-close";
            sendUDP(sock,sendbuf,DEFAULT_BUFLEN);
            sMutex.unlock();
            calculate=0;
            shutdown(sock,2);
            closesocket(sock);
            printf("Closing\n");
            if(AUTO_TRANSMIT)
            {
                AUTO_TRANSMIT=0;
                calcsThread->join();
                delete(calcsThread);
            }
            return 0;
            break;
        }
        default :
        {
            cMutex.lock();
            printf("Wrong choise,try again\n");
            cMutex.unlock();
        }
        }
    }
    //  calcThread.join();
    closesocket(sock);

    return 0;
}
