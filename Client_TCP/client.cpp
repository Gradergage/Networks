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
#define DEFAULT_BUFLEN 512
#define BLOCK_SIZE 16384

unsigned short AUTO_TRANSMIT = 0;
unsigned short calculate = 0;
//std::vector <int> numbers;
std::mutex cMutex;
std::mutex vMutex;
std::mutex sMutex;

int readn(int &sock, char *recvbuf, int recvbuflen)
{
        int iResult=0;
        memset(recvbuf,NULL,recvbuflen);

        int recievedBytes=0;

        while(recievedBytes<recvbuflen)
        {

            iResult = recv( sock, recvbuf+recievedBytes, recvbuflen-recievedBytes, 0 );
            if (iResult < 0) {
            printf("send failed with error");
            return 1;
            }
            recievedBytes+=iResult;
        }
        return 0;
}
int startNumber=100;
int endNumber;

//Sending calculated numbers
void send_numbers(int sock,std::vector <int> numbers)
{   char sendbuf[DEFAULT_BUFLEN]="-numbers";

    send(sock,sendbuf,sizeof(sendbuf),0);
    for(int n : numbers)
    {
        snprintf(sendbuf,sizeof(sendbuf),"%d",n);
        send(sock,sendbuf,sizeof(sendbuf),0);
    }
    strcpy(sendbuf,"-end");
    send(sock,sendbuf,sizeof(sendbuf),0);
}
int getBound(int sock)
{
    char sendbuf[DEFAULT_BUFLEN]="-bound";
    send(sock,sendbuf,sizeof(sendbuf),0);
    readn(sock,sendbuf,sizeof(sendbuf));
  //  printf("Recieved bound %d\n",atoi(sendbuf));
    return atoi(sendbuf);
}
int calculateNumbers(int sock,int bound)
{
    std::vector <int> numbers;
	for(int n=bound+1; n<bound+BLOCK_SIZE;n++ )
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
    send(sock,sendbuf,sizeof(sendbuf),0);

    //send bound
    snprintf(sendbuf,sizeof(sendbuf),"%d",bound);
    send(sock,sendbuf,sizeof(sendbuf),0);

    //send numbers
    for(int n : numbers)
    {
       /* cMutex.lock();
        printf("%d\n",n);
        cMutex.unlock();*/
        snprintf(sendbuf,sizeof(sendbuf),"%d",n);
        send(sock,sendbuf,sizeof(sendbuf),0);
    }
    memset(sendbuf,NULL,DEFAULT_BUFLEN);
    strcpy(sendbuf,"-end");
    if(send(sock,sendbuf,sizeof(sendbuf),0)<0);
    {
       // perror("Send numbers");
        return 0;
    }
 /*   cMutex.lock();
    printf("Send completed\n");
    cMutex.unlock(); */
	return 0;
}
int calculateAndTransmit(int sock)
{
        while(AUTO_TRANSMIT)
        {
                if(sock <0)
                {return 0; }
                sMutex.lock();
                int bound=getBound(sock);
                calculateNumbers(sock,bound);
                sMutex.unlock();
        }
    return 0;
}


int auth(int sock,char param[],int paramLen)
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

    if(send(sock, param, DEFAULT_BUFLEN, 0)<0)
    {
        perror("Param");
        return 0;
    }
   // printf("param sended: \n");
    if(send(sock, login, sizeof(login), 0)<0)
    {
        perror("Param");
        return 0;
    }
  //  printf("login sended: \n");
    if(send(sock, password, sizeof(password), 0)<0)
    {
        perror("Param");
        return 0;
    }
 //   printf("password sended: \n");

    if(readn(sock, recvbuf, DEFAULT_BUFLEN)>0)
    {
        perror("Param");
        return 0;
    }

 // printf("Param received!\n");
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


void getNNumbers(int sock,int n)
{

    std::vector<int> numbers;
    char recvbuf[DEFAULT_BUFLEN]="-getn";
    send(sock,recvbuf,sizeof(recvbuf),0);

    snprintf(recvbuf,sizeof(recvbuf),"%d",n);
    send(sock,recvbuf,sizeof(recvbuf),0);

    while(true)
    {

        readn(sock,recvbuf,sizeof(recvbuf));
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
    if(AUTO_TRANSMIT){
        char sendbuf[DEFAULT_BUFLEN]="-sts";
        send(sock,sendbuf,sizeof(sendbuf),0);
    }
    else {
        char sendbuf[DEFAULT_BUFLEN]="-ets";
        send(sock,sendbuf,sizeof(sendbuf),0);
    }
}
int getMaxNumber(int sock)
{
    char sendbuf[DEFAULT_BUFLEN]="-max";
    send(sock,sendbuf,sizeof(sendbuf),0);
    readn(sock,sendbuf,sizeof(sendbuf));
    printf("Current max number is: %s\n",sendbuf);
    return 0;
}
int main()
{
    int sock;
    struct sockaddr_in addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("socket");
        cMutex.unlock();
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(27015); // или любой другой порт...
    addr.sin_addr.s_addr = inet_addr("192.168.213.1");
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        cMutex.lock();
        perror("connect");
        cMutex.unlock();
        return 0;
    }
   // std::thread calcThread(calculatingThread);
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

        char choise;
        std::cin>>choise;
        system("clear");

        switch(choise)
        {
            case '1':{
                authFailed=auth(sock, authParam,sizeof(authParam));
                break;}
            case '2':{
                authFailed=auth(sock, regParam,sizeof(regParam));
                break;}
            default :
            {
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
        char choise;
        std::cin>>choise;
      /*  system("clear");
        cMutex.lock();
        printf("MENU\n\
        1 - Calculating and transmitting %d\n\
        2 - Enable calculating and transmitting %d\n\
        3 - Exit\n\n",calculate,AUTO_TRANSMIT);
        cMutex.unlock();*/



        switch(choise)
        {
            case '2':{
              //  cal
              calculate^=0x1;
                  //calculateAndTransmit(sock);
                sMutex.lock();
                getMaxNumber(sock);
                sMutex.unlock();
                break;}
            case '1':{
                //inverce auto  transmitting
                AUTO_TRANSMIT^=0x1;
                if(AUTO_TRANSMIT){
                    char sendbuf[DEFAULT_BUFLEN]="-sts";
                    send(sock,sendbuf,sizeof(sendbuf),0);
                    calcsThread=new std::thread(calculateAndTransmit,std::ref(sock));
                }
                else {
                    calcsThread->join();
                    delete(calcsThread);
                    char sendbuf[DEFAULT_BUFLEN]="-ets";
                    send(sock,sendbuf,sizeof(sendbuf),0);
                }
                break;}
            case '3':{
                int cnt;
                printf("Enter count of numbers: ");
                std::cin>>cnt;
                sMutex.lock();
                getNNumbers(sock,cnt);
                sMutex.unlock();
                break;}
            case '4':{
                sMutex.lock();
                char sendbuf[DEFAULT_BUFLEN]="-close";
                send(sock,sendbuf,DEFAULT_BUFLEN,0);
                sMutex.unlock();
                calculate=0;
                shutdown(sock,2);
                close(sock);
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
    close(sock);

    return 0;
}
