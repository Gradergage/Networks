int sendn(SOCKET s, char* buf, int lenbuf, int flags)
{
    int bytesSended = 0; //
    int n; //
    while (bytesSended < lenbuf) //шлем данные, пока количество переданных байт не сравняется с необходимым
    {
        n = send(s, buf + bytesSended, lenbuf - bytesSended, flags);
        if (n < 0)
        {
            //
            printf("Error with send in sendn\n");
            break;
        }
        bytesSended += n;
    }
    return (n == -1 ? -1 : bytesSended);
}
int recvLine(SOCKET sock, char* buffer, int buffSize)    //функция приема сообщения
{
    char* buff = buffer; //указатель на начало внешнего буфера
    char* currPosPointer; //указатель для работы со временным буфером
    int count = 0; //число прочитанных символов (без удаления из буфера сокета)
    char tempBuf[100]; //временный буфер для приема
    char currChar; //текущий анализируемый символ (ищем разделитель)
    int tmpcount = 0;
    while (--buffSize > 0) //пока во внешнем буфере (куда пишется сообщение) есть //место
    {
        if (--count <= 0)
        {
            recvn(sock, tempBuf, tmpcount);
            count = recv(sock, tempBuf, sizeof (tempBuf), MSG_PEEK);
            if (count <= 0)
            {
                return count;
            }
            currPosPointer = tempBuf;
            tmpcount = count;
        }
        currChar = *currPosPointer++;
        *buffer++ = currChar;
        if (currChar == '\n')
        {
            *(buffer - 1) = '\0';
            recvn(sock, tempBuf, tmpcount - count + 1);
            return buffer - buff-1;
        }
    }
    return -1;
}
int recvn(SOCKET fd, char *bp, size_t len)
{
    return recv(fd, bp, len, MSG_WAITALL);
}
int sendLine(int sock, char* str)
{
    char tempBuf[MAX_STR_LEN];
    strcpy(tempBuf, str);
    if(tempBuf[strlen(tempBuf)-1]!='\n') strcat(tempBuf, "\n");
    return sendn(sock, tempBuf, strlen(tempBuf), 0);
}
