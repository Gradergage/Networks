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

int readUDP(int socket,char* recvbufn,int recvbuflen)
{
    std::vector <int>numbers;
    std::vector <char*>packets;
    strcpy(recvbufn,"");
    char tempbuf[DEFAULT_BUFLEN];
    char recvbuf[DEFAULT_BUFLEN+PREFIXLEN];
    int recvBytes=0;
    int iResult=0;
    while(recvBytes<recvbuflen)
    {
        /*  fd_set readset;
          int result;
          struct timeval tv;
          tv.tv_sec = 0;
          tv.tv_usec = 100000;
          do
          {
              FD_ZERO(&readset);
              FD_SET(socket, &readset);
              result = select(socket + 1, &readset, NULL, NULL, &tv);
          }
          while (result == -1 && errno == EINTR);

          if (result > 0)
          {
              if (FD_ISSET(socket, &readset))
              { */
        iResult=recv(socket,recvbuf,sizeof(recvbuf),0);

        /*      printf("%s \n",recvbuf);
          }
        } */
        if(iResult<0)
        {
            return -1;
        }
        /*approve*/
        char app[2]="a";
        iResult=send(socket,app,sizeof(app),0);
        if(iResult<0)
        {
            return -1;
        }
        char message[DEFAULT_BUFLEN];
        int number;
        getData(recvbuf, number,message);
        //    printf("%d %s \n",number,message);
        //  printf("%s \n",message);
        char * temp = new char[DEFAULT_BUFLEN];
        // char temp[DEFAULT_BUFLEN];
        strncpy(temp,message,DEFAULT_BUFLEN);
        packets.push_back(temp);
        numbers.push_back(number);
        recvBytes+=DEFAULT_BUFLEN;
    }
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
    return 0;
}
//int sendUDP(SOCKET & sock, char *sendbuff, int sendbufflen)
int sendUDP(int sock,char *sendbuff, int sendbufflen)
{
    int sendedBytesSize=0;
    int i=0;
    char tempbuf[DEFAULT_BUFLEN];
    char numbuf[PREFIXLEN];
    char tempbufn[PREFIXLEN+DEFAULT_BUFLEN]="0 ";

    snprintf(numbuf,sizeof(numbuf),"%d",sendbufflen);
    strcat(tempbufn,numbuf);
    //int iResult = send(sock,tempbufn,sizeof(tempbufn),0);
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
        //   printf("tempbuf %s\n",tempbuf);


        strncat(tempbufn,tempbuf,DEFAULT_BUFLEN);

        sendedBytesSize+=lastBytes;
        //   printf("tempbufn %s ",tempbufn);
        int iResult = send(sock,tempbufn,sizeof(tempbufn),0);
        if(iResult<0)
        {
            return -1;
        }
        char app[2];
        int cnt=0;

        while(true)
        {
            if(cnt==3)
            {
                return -1;
            }
            fd_set readset;
            int result;
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            do
            {
                FD_ZERO(&readset);
                FD_SET(sock, &readset);
                result = select(sock + 1, &readset, NULL, NULL, &tv);
            }
            while (result == -1 && errno == EINTR);

            if (result > 0)
            {
                if (FD_ISSET(sock, &readset))
                {

                    result = recv(sock,app,sizeof(app),0);
                    if (result < 0)
                    {
                        /* This means the other side closed the socket */
                        return -1;
                    }
                    else
                    {
                        //  printf("approved\n");
                        break;
                    }
                }
            }
            else
            {
                cnt++;
            }
        }
        // printf("approved \n");
        i++;
    }

    return 0;
}
