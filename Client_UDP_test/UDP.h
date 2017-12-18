int getData(char *recvbuf,int &number,char* data)
{
    char *buf = strtok(recvbuf, " ");
    char numberbuf[DEFAULT_BUFLEN];
    strcpy(numberbuf,buf);
    buf = strtok(NULL, " ");
    strcpy(data,buf);
    number = atoi(numberbuf);

    return 0;
}

int recv_to(int fd, char *buffer, int len, int flags, int to)
{

    fd_set tempset;
    int result;
    u_long iof = -1;
    struct timeval tv;

    // Initialize the set
    FD_ZERO(&tempset);
    FD_SET(fd, &tempset);

    // Initialize time out struct
    tv.tv_sec = 0;
    tv.tv_usec = to * 1000;
    // select()
    result = select(fd+1, &tempset, NULL, NULL, &tv);

    // Check status
    if (result < 0)
        return -1;
    else if (result > 0 && FD_ISSET(fd, &tempset))
    {
        // Set non-blocking mode
        if ((iof = ioctlsocket(fd, FIONBIO, 0)) != -1)
            ioctlsocket(fd, FIONBIO, &iof);
        // receive
        result = recv(fd, buffer, len, flags);
        // set as before
        if (iof != -1)
            ioctlsocket(fd, FIONBIO, &iof);
        return result;
    }
    return -2;
}

int approve(SOCKET socket)
{
    char app[2]="a";
    int iResult=send(socket,app,sizeof(app),0);
    if(iResult<=0)
    {
        return -1;
    }
    return 0;
}

int packetExist(std::vector <int> numbers,int number)
{
    for(int n : numbers)
    {
        if(n==number)
            return 1;
    }
    return 0;
}

int readUDP(SOCKET socket,char* recvbufn,int recvbuflen)
{
    std::vector <int>numbers;
    std::vector <char*>packets;
    strcpy(recvbufn,"");
    char tempbuf[DEFAULT_BUFLEN];
    char recvbuf[DEFAULT_BUFLEN+PREFIXLEN];
    int recvBytes=0;
    int iResult=0;
    int firstTransmit=1;
    while(recvBytes<recvbuflen)
    {
        if(firstTransmit)
        {
            iResult=recv(socket,recvbuf,sizeof(recvbuf),0);
        }
        else
        {
            iResult=recv_to(socket,recvbuf,sizeof(recvbuf),0,200);
        }
        if (iResult <=0)
        {
            return -1;
        }
     //   printf("%s",recvbuf);

        /*approve*/
        iResult=approve(socket);
        if(iResult<0)
        {
            return -1;
        }
       // printf(" approve sended\n");

        char message[DEFAULT_BUFLEN];
        int number;
        getData(recvbuf, number,message);
        //  printf("%d %s \n",number,message);
        //  printf("%s \n",message);
        //  char*temp=new char();
        if(packetExist(numbers,number))
        {
            iResult=approve(socket);
            if(iResult<0)
            {
                return -1;
            }
          //  printf(" approve dublicated sended\n");
        }
        else
        {
            char *temp=new char[DEFAULT_BUFLEN];
            strncpy(temp,message,DEFAULT_BUFLEN);
            packets.push_back(temp);
            numbers.push_back(number);
            recvBytes+=DEFAULT_BUFLEN;
        }
    }
    //assembly of packets
    int countOfPackets=packets.size();
    for(int i=0; i<countOfPackets; i++)
    {
        for(int j=0; j<countOfPackets; j++)
        {
            if(numbers[j]==i)
            {
                strncat(recvbufn,packets[j],DEFAULT_BUFLEN);
                delete(packets[j]);
            }
        }
    }
 //   printf("recieved: %s\n",recvbufn);
    return 0;
}

int sendUDP(SOCKET sock,char *sendbuff, int sendbufflen)
{
    int sendedBytesSize=0;
    int i=0;
    char tempbuf[DEFAULT_BUFLEN];
    char numbuf[PREFIXLEN];
    char tempbufn[PREFIXLEN+DEFAULT_BUFLEN]="0 ";

    while(sendedBytesSize<sendbufflen)
    {
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
        strncat(tempbufn,tempbuf,DEFAULT_BUFLEN);


        //printf("%s ",tempbufn);
        //count of resend cases is three
        int cnt=0;

        while(true)
        {

            int iResult = send(sock,tempbufn,sizeof(tempbufn),0);

            if(iResult<=0)
            {
                return -1;
            }

            char app[2];

            //recv with 100msec timeout
            iResult = recv_to(sock,app,sizeof(app),0,100);

            if (iResult == 0)
            {
                /* This means the other side closed the socket or error got */
                return -1;
            }
            else if(iResult==-2)
            {
           //     printf("approve %d failed\n",cnt);
                if(cnt++==3)
                {
                    return -1;
                }
                continue;
            }
            else if(iResult<0)
            {
                return -1;
            }

            break;
        }
        sendedBytesSize+=lastBytes;
        i++;
    }
 //   printf("sended\n");
    return 0;
}


