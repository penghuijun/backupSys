#include "DSPconfig.h"

extern shared_ptr<spdlog::logger> g_worker_logger;
extern shared_ptr<spdlog::logger> g_workerGYIN_logger;
extern shared_ptr<spdlog::logger> g_workerSMAATO_logger;
extern shared_ptr<spdlog::logger> g_workerINMOBI_logger;
#if 0
bool chinaTelecomObject::string_find(string& str1, const char* str2)
{    
    return (str1.compare(0, strlen(str2), str2)==0);
}
void chinaTelecomObject::strGet(string& Dest,const char* Src)
{
    const char *ch = Src;
    int pos = 0;
    int len = 0;
    while( (*ch != '\0')&&(*ch++ != '"'))	
        pos++;
    while( (*ch != '\0')&&(*ch++ != '"'))	
        len++;
    Dest.assign(Src+pos+1,len);	
    Dest[len] = '\0';    
}
#endif

char* mem_ncat(char *strDest,const char* strSrc,int size)
{
    char* address = strDest;
    assert((strDest != NULL)&&(strSrc != NULL));
    while(*strDest)
        strDest++;
    while(size)
    {        
        *strDest++ = *strSrc++;
        size--;
    } 
    return address;
}

#define     EPOLL_MAXEVENTS     64

int checkConnect(int fd, int connect_ret)
{
    int epfd, nevents, status;
    struct epoll_event ev, events[EPOLL_MAXEVENTS];
    socklen_t slen = sizeof(int);
    
    if(connect_ret == 0)    //non-blocking connect success, connect complete immediately
    {
        close(fd);
        return -1;
    }

    if(connect_ret < 0 && errno != EINPROGRESS)     //connect error
    {
        return -1;
    }

    epfd = epoll_create(EPOLL_MAXEVENTS);
    ev.events = EPOLLOUT;
    ev.data.fd = fd;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)   //epoll_ctl error
    {
        goto finish;
    }

    //add connect fd into epoll

    memset(events, 0, sizeof(events));

    for(;;)
    {
        nevents = epoll_wait(epfd, events, EPOLL_MAXEVENTS, -1);

        if(nevents < 0)     //epoll_wait failed
        {
            goto finish;
        }

        for(int i=0; i<nevents; i++)
        {
            if(events[i].data.fd == fd)
            {
                if(getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *) &status, &slen) < 0)   //getsockopt error
                {
                    goto finish;
                }
                if(status != 0)     //connect error
                {
                    goto finish;
                }

                //non-blocking connect success

                if(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1)  //epoll_ctl error
                {
                    return 0;
                }

                /* DO write...*/
                return 1;
            }
        }
    }

    finish:
        close(fd);
        close(epfd);

    return 0;
}

ssize_t socket_send(int sockfd, const char *buffer, size_t buflen)
{
    ssize_t ret = 0;
    size_t total = buflen;
    const char *data_start = buffer;

    while(1)
    {
        ret = send(sockfd, data_start, total, 0);
        if(ret < 0)     //SOCKET_ERROR
        {
        
          /**
            **#define   EINTR       4     //Interrupted system call 
            **#define   EAGAIN    11    //Try again 
            **/
            
            if(errno == EINTR)      //The call was interrupted by a signal before any data was written
            {
                return -1;
            }
            else if(errno == EAGAIN)    //send_buf is full,please try again
            {
                //usleep(1000);
                continue;
            }

            return -1;
        }

        if((size_t)ret == total)    
            return buflen;

        //send incomplete
        total -= ret;
        data_start +=ret;
    }
    
    return ret;
}
void dspObject::readDSPconfig(dspType type)
{
    string filename;
    shared_ptr<spdlog::logger> g_logger;
    switch(type)
    {
        case TELE:
            filename = "./conf/chinaTelecomConfig.json";
            g_logger = g_worker_logger;
            break;
        case GYIN:
            filename = "./conf/guangYinConfig.json";
            g_logger = g_workerGYIN_logger;
            break;
        case SMAATO:
            filename = "./conf/smaatoConfig.json";
            g_logger = g_workerSMAATO_logger;
            break;
        case INMOBI:
            filename = "./conf/inmobiConfig.json";
            g_logger = g_workerINMOBI_logger;
            break;
        default:
            break;
    }

    ifstream ifile;
    ifile.open(filename, ios::in);
    if(ifile.is_open() == false)
    {		        
        g_logger->error("Open {0} failure...", filename);
        exit(1);	
    }	

    Json::Reader reader;
    Json::Value root;

    if(reader.parse(ifile, root))
    {
        ifile.close();
        name        = root["name"].asString();       
        adReqType   = root["adReqType"].asString();
        adReqIP     = root["adReqIP"].asString();
        adReqDomain = root["adReqDomain"].asString();
        adReqPort   = root["adReqPort"].asString();
        adReqUrl    = root["adReqUrl"].asString();
        
        httpVersion = root["httpVersion"].asString();

        //HTTP header
        Connection  = root["Connection"].asString();
        UserAgent   = root["User-Agent"].asString();
        ContentType = root["Content-Type"].asString();
        charset     = root["charset"].asString();
        Host        = root["Host"].asString();
        Cookie      = root["Cookie"].asString();
        Forwarded = root["X-Forwarded-For"].asString();
        Accept     = root["Accept"].asString();

        //filter
        extNetId    = root["extNetId"].asString();
        intNetId    = root["intNetId"].asString();        

        maxConnectNum = root["maxConnectNum"].asInt();
        preConnectNum = root["preConnectNum"].asInt();
        maxFlowLimit  = root["maxFlowLimit"].asInt();
        
    }
    else
    {
        g_logger->error("Parse {0} failure...", filename);
        ifile.close();
        exit(1);
    }
    
}

void dspObject::addConnectToDSP(void * arg)
{   
    struct connectDsp_t * con_t = (struct connectDsp_t *)arg;
    dspObject *dspObj = (dspObject *)con_t->dspObj;
    
    sockaddr_in sin;
    unsigned short httpPort = atoi(dspObj->getAdReqPort().c_str());      
    
    sin.sin_family = AF_INET;    
    sin.sin_port = htons(httpPort);    
    if(!dspObj->getAdReqIP().empty())
    {
        g_worker_logger->debug("adReq IP: {0}", dspObj->getAdReqIP());
        sin.sin_addr.s_addr = inet_addr(dspObj->getAdReqIP().c_str());
    }
    else if(!dspObj->getAdReqDomain().empty())
    {
        struct hostent *m_hostent = NULL;
        m_hostent = gethostbyname(dspObj->getAdReqDomain().c_str());
        if(m_hostent == NULL)
        {
            g_worker_logger->error("gethostbyname error for host: {0}", dspObj->getAdReqDomain());
            dspObj->connectNumReduce();
            return ;
        }
        sin.sin_addr.s_addr = *(unsigned long *)m_hostent->h_addr;
        g_worker_logger->debug("DOMAIN IP: {0}", inet_ntoa(sin.sin_addr));
    }
    else
    {
        g_worker_logger->error("ADD CON GET IP FAIL");
        dspObj->connectNumReduce();
        return ;
    }
    
    

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        g_worker_logger->error("ADD CON SOCK CREATE FAIL ...");
        dspObj->connectNumReduce();
        return ;
    }   

    //非阻塞
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    //建立连接
    int ret = connect(sock, (const struct sockaddr *)&sin, sizeof(sockaddr_in));    
    if(checkConnect(sock, ret) <= 0)
    {
        g_worker_logger->error("ADD CON CONNECT FAIL ...");      
        close(sock);
        dspObj->connectNumReduce();
        return ;
    }

    //add this socket to event listen queue
    struct event *sock_event;
    sock_event = event_new(con_t->base, sock, EV_READ|EV_PERSIST, con_t->fn, con_t->arg);     
    event_add(sock_event, NULL);

    struct listenObject *listen = new listenObject();    
    listen->sock = sock;
    listen->_event = sock_event;

    dspObj->listenObjectList_Lock();
    dspObj->getListenObjectList()->push_back(listen);
    dspObj->listenObjectList_unLock();
    
}



void dspObject::gen_HttpHeader(char *headerBuf, int Con_len)
{
    //头信息
    char *send_str = headerBuf;            

    if(Con_len)
    {
        char content_header[100];
        sprintf(content_header,"Content-Length: %d\r\n", Con_len);
        strcat(send_str, content_header); 
    }    

    if(Connection.empty() == false)
    {
        strcat(send_str, "Connection: ");
        strcat(send_str,Connection.c_str());
        strcat(send_str, "\r\n");
    }    

    if(UserAgent.empty() == false)
    {
        strcat(send_str, "User-Agent: ");
        strcat(send_str,UserAgent.c_str());
        strcat(send_str, "\r\n");
    }    

    if(ContentType.empty() == false)
    {
        strcat(send_str, "Content-Type: ");    
        strcat(send_str,ContentType.c_str());
        strcat(send_str, "\r\n");
    }    

   if(charset.empty() == false)
    {
        strcat(send_str, "charset: ");
        strcat(send_str,charset.c_str());
        strcat(send_str, "\r\n");
    }

    if(Host.empty() == false)
    {
        strcat(send_str, "Host: ");
        strcat(send_str,Host.c_str());
        strcat(send_str, "\r\n");
    }    

    if(Cookie.empty() == false)
    {
        strcat(send_str, "Cookie: ");
        strcat(send_str,Cookie.c_str());
        strcat(send_str, "\r\n");
    }  

    if(Forwarded.empty() == false)
    {
        strcat(send_str, "X-Forwarded-For: ");
        strcat(send_str,Forwarded.c_str());
        strcat(send_str, "\r\n");
    }

    if(Accept.empty() == false)
    {
        strcat(send_str, "Accept: ");
        strcat(send_str, Accept.c_str());
        strcat(send_str, "\r\n");
    }
    
}

struct listenObject* dspObject::findListenObject(int sock)
{
    m_listenObjectListLock.lock();
    list<listenObject *>::iterator it = m_listenObjectList->begin();
    for( ; it != m_listenObjectList->end(); it++)
    {
        struct listenObject *object = *it;
        if(object->sock == sock)
        {
            m_listenObjectListLock.unlock();
            return object;            
        }
    }
    m_listenObjectListLock.unlock();
    return NULL;
}

void dspObject::eraseListenObject(int sock)
{
    m_listenObjectListLock.lock();    
    list<listenObject *>::iterator it = m_listenObjectList->begin();
    for( ; it != m_listenObjectList->end(); it++)
    {
        listenObject *object = *it;
        if(object->sock == sock)
        {
            m_listenObjectList->erase(it);         
            curConnectNum--;
            m_listenObjectListLock.unlock();
            return ;
        }            
    }    
    m_listenObjectListLock.unlock();
}

void chinaTelecomObject::readChinaTelecomConfig()
{
    ifstream ifile;	
    ifile.open("./conf/chinaTelecomConfig.json",ios::in);
    if(ifile.is_open() == false)
    {		        
        g_worker_logger->error("Open chinaTelecomConfig.json failure...");
        exit(1);	
    }	

    Json::Reader reader;
    Json::Value root;

    if(reader.parse(ifile, root))
    {
        ifile.close();       
        tokenType   = root["tokenType"].asString();
        tokenIP     = root["tokenIP"].asString();
        tokenPort   = root["tokenPort"].asString();
        tokenUrl    = root["tokenUrl"].asString();
        user        = root["user"].asString();
        passwd      = root["passwd"].asString();          
        
    }
    else
    {
        g_worker_logger->error("Parse clientConfig.json failure...");
        ifile.close();
        exit(1);
    }

}
bool chinaTelecomObject::parseCertifyStr(char * Src)
{
    string temp = Src;
	int pos1 = temp.find("0020");
	if(pos1 == -1)
	    return false;
	pos1 = pos1+4;
	char *ch = Src+pos1;
	while(!((*ch>='0'&&*ch<='9')||(*ch>='a'&&*ch<='z')||(*ch>='A'&&*ch<='Z')))
	{
		ch++;
		pos1++;		
	}	
	int len = 0;
	while((*ch>='0'&&*ch<='9')||(*ch>='a'&&*ch<='z')||(*ch>='A'&&*ch<='Z'))
	{
		ch++;
		len++;		
	}
	strncpy(Src,Src+pos1,len);
	Src[len] = '\0';	
	return true;
}
bool chinaTelecomObject::getCeritifyCodeFromChinaTelecomDSP()
{
    sockaddr_in sin;
    unsigned short httpPort = atoi(tokenPort.c_str());    

    char Url[100] = {0};
    strcpy(Url,tokenUrl.c_str());        
    string str = "?username="+user+"&password="+passwd;
    strcat(Url,str.c_str());    
    
    sin.sin_family = AF_INET;    
	sin.sin_port = htons(httpPort);    
	sin.sin_addr.s_addr = inet_addr(tokenIP.c_str());

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        g_worker_logger->error("tokenSock create failed ...");
        return false;
    }	

    //建立连接
    if (connect(sock, (const struct sockaddr *)&sin, sizeof(sockaddr_in) ) == -1)
    {    
        g_worker_logger->error("tokenSock connect failed ...");
        return false;
    }	

    //初始化发送信息
    char send_str[2048] = {0};

    //请求行
    strcat(send_str, tokenType.c_str());
    strcat(send_str, Url);
    strcat(send_str, getHttpVersion().c_str());    
    strcat(send_str, "\r\n");
    
    //头信息
    gen_HttpHeader(send_str, 0);
   
    //内容信息
    strcat(send_str, "\r\n");
    //cout << "send_str: " << send_str << endl;
	
	if (send(sock, send_str, strlen(send_str),0) == -1)
    {   
        g_worker_logger->error("tokenSock send failed ...");
        return false;
    }	

    //获取返回信息	
    char recv_str[4096] = {0};
    if (recv(sock, recv_str, sizeof(recv_str), 0) == -1)
    {        
        g_worker_logger->error("tokenSock recv failed ...");
        return false;
    }	
	
	if(!parseCertifyStr(recv_str))
	{
        g_worker_logger->error("parse [CERITIFY CODE] fail ...");
        return false;
	}	    
	g_worker_logger->debug("CERITIFY CODE : {0}",recv_str);	
	CeritifyCode = recv_str;
	close(sock);
	return true;
}
bool chinaTelecomObject::isCeritifyCodeEmpty()
{    
    if(CeritifyCode.empty() == true)
    {        
        return true;
    }        
    else
        return false;
}
bool chinaTelecomObject::sendAdRequestToChinaTelecomDSP(const char *data, int dataLen, bool enLogRsq)
{
    //初始化发送信息
    //char send_str[2048] = {0};
    char *send_str = new char[4096];
    memset(send_str,0,4096*sizeof(char));  

    char Url[100] = {0};
    strcpy(Url,getAdReqUrl().c_str());        
    string str = "?username="+user+"&password="+CeritifyCode;
    strcat(Url,str.c_str());    
    g_worker_logger->trace("ADREQ URL : {0}",Url);

    //ostringstream os;
    //os<<adReqIP<<":"<<adReqPort;    

    //请求行
    strcat(send_str, getAdReqType().c_str());
    strcat(send_str, Url);
   
    strcat(send_str, getHttpVersion().c_str());    
    strcat(send_str, "\r\n");     
    
    //头信息
    gen_HttpHeader(send_str, dataLen);


    //内容信息
    
    strcat(send_str, "\r\n");    
    strcat(send_str, data);
    
    if(enLogRsq)
    {
        g_worker_logger->debug("\r\n{0}",send_str);
    }   
    
    if(getCurConnectNum() == 0)
    {
        g_worker_logger->debug("NO CONNECTION TO GYIN");
        return false;
    }
    
    
    listenObject *obj = NULL;
    
    listenObjectList_Lock();
    if(!getListenObjectList()->empty())
    {
        obj = getListenObjectList()->front();
        getListenObjectList()->pop_front();
    }    
    listenObjectList_unLock();  

    if(!obj)
    {
        g_worker_logger->debug("NO IDLE SOCK ");
        delete [] send_str;
        return false;
    }
    
    int sock =  obj->sock;

    bool ret = true;
    //cout << "@@@@@ This msg send by PID: " << getpid() << endl; 
    //if (send(sock, send_str, strlen(send_str),0) == -1)
    if(socket_send(sock, send_str, strlen(send_str)) == -1)
    {        
        g_worker_logger->error("adReqSock send failed ...");
        ret = false;
    }         
    listenObjectList_Lock();
    getListenObjectList()->push_back(obj);
    listenObjectList_unLock();  
    delete [] send_str;
    return ret;
}

void guangYinObject::readGuangYinConfig()
{
    ifstream ifile; 
    ifile.open("./conf/guangYinConfig.json",ios::in);
    if(ifile.is_open() == false)
    {               
        g_worker_logger->error("Open guangYinConfig.json failure...");
        exit(1);    
    }   
    
    Json::Reader reader;
    Json::Value root;
    
    if(reader.parse(ifile, root))
    {
        ifile.close();       
    
        //filter
        publisherId = root["publisherId"].asString();        
        test        = root["test"].asBool();
        
    }
    else
    {
        g_worker_logger->error("Parse guangYinConfig.json failure...");
        ifile.close();
        exit(1);
    }    

}

bool guangYinObject::sendAdRequestToGuangYinDSP(const char *data, int dataLen)
{
    //初始化发送信息
    //char send_str[2048] = {0};
    char *send_str = new char[4096];
    memset(send_str,0,4096*sizeof(char));       

    //头信息
    strcat(send_str, getAdReqType().c_str());
    strcat(send_str, getAdReqUrl().c_str());    
    strcat(send_str, getHttpVersion().c_str());    
    strcat(send_str, "\r\n");     
    
    gen_HttpHeader(send_str, dataLen);


    //内容信息
    
    strcat(send_str, "\r\n");    
    int headerLen = strlen(send_str);
    send_str[headerLen] = '\0';
    
    //strcat(send_str, data);
    mem_ncat(send_str,data,dataLen);
    int wholeLen = headerLen + dataLen;
    
    //g_workerGYIN_logger->debug("GYin ADREQ datalen: {0:d}  ",dataLen);
    
    if(getCurConnectNum() == 0)
    {
        g_workerGYIN_logger->debug("NO CONNECTION TO GYIN");
        return false;
    }
    
    
    listenObject *obj = NULL;
    
    listenObjectList_Lock();
    if(!getListenObjectList()->empty())
    {
        obj = getListenObjectList()->front();
        getListenObjectList()->pop_front();
    }    
    listenObjectList_unLock();  

    if(!obj)
    {
        g_workerGYIN_logger->debug("NO IDLE SOCK ");
        delete [] send_str;
        return false;
    }
    
    int sock =  obj->sock;
    
    bool ret = true;
    //cout << "@@@@@ This msg send by PID: " << getpid() << endl; 
    //if (send(sock, send_str, wholeLen,0) == -1)
    if(socket_send(sock, send_str, wholeLen) == -1)
    {        
        g_workerGYIN_logger->error("adReqSock send failed ...");
        ret  = false;
    }         
    listenObjectList_Lock();
    getListenObjectList()->push_back(obj);
    listenObjectList_unLock();  
    delete [] send_str;    
    return ret;
}

void smaatoObject::readSmaatoConfig()
{
    ifstream ifile; 
    ifile.open("./conf/smaatoConfig.json",ios::in);
    if(ifile.is_open() == false)
    {               
        g_workerSMAATO_logger->error("Open smaatoConfig.json failure...");
        exit(1);    
    }   
    
    Json::Reader reader;
    Json::Value root;
    
    if(reader.parse(ifile, root))
    {
        ifile.close();       
    
        //filter
        publisherId = root["publisherId"].asString();        
        adSpaceId  = root["adSpaceId"].asString();
        
    }
    else
    {
        g_workerSMAATO_logger->error("Parse smaatoConfig.json failure...");
        ifile.close();
        exit(1);
    }    

}

int smaatoObject::sendAdRequestToSmaatoDSP(const char *data, int dataLen, string& uuid)
{
    //初始化发送信息
    char *send_str = new char[4096];
    memset(send_str,0,4096*sizeof(char));       

    //请求行
    strcat(send_str, getAdReqType().c_str());
    string Url = getAdReqUrl() + "?" + data;
    strcat(send_str, Url.c_str());    
    strcat(send_str, getHttpVersion().c_str());    
    strcat(send_str, "\r\n");        

    //头信息
    gen_HttpHeader(send_str, 0);

    strcat(send_str, "\r\n");

    
    if(getCurConnectNum() == 0)
    {
        g_workerSMAATO_logger->debug("NO CONNECTION TO SMAATO");
        return false;
    }
    
    
    int sock = 0;

    smaatoSocketList_Lock.lock();
    if(!smaatoSocketList->empty())
    {
        sock = smaatoSocketList->front();
        smaatoSocketList->pop_front();
    }    
    smaatoSocketList_Lock.unlock();  

    if(sock == 0)
    {
        g_workerSMAATO_logger->debug("NO IDLE SOCK ");
        delete [] send_str;
        return 0;
    }
    
    
    int ret_t = sock;
    g_workerSMAATO_logger->debug("start send to dsp uuid: {0}", uuid);
    g_workerSMAATO_logger->debug("SEND\r\n{0}", send_str);
    if(socket_send(sock, send_str, strlen(send_str)) == -1)
    {        
        g_workerSMAATO_logger->error("adReqSock send failed ...");
        close(sock);
        smaatoSocketList_Lock.lock();
        connectNumReduce();
        smaatoSocketList_Lock.unlock();
        ret_t  = -1;
    }         

    delete [] send_str;    
    return ret_t;

}

bool smaatoObject::recvBidResponseFromSmaatoDsp(int sock, struct spliceData_t *fullData_t)
{
    //获取返回信息	
    char *recv_str = new char[BUF_SIZE];
    memset(recv_str, 0, BUF_SIZE*sizeof(char));    
    int recv_bytes = 0;    
    
    //g_workerSMAATO_logger->debug("RECV {0} HTTP RSP by PID: {1:d}", dspName, getpid());
    

    int temp = 0;
    bool waitFlag = true;
    timeval startTime;
    memset(&startTime,0,sizeof(struct timeval));
    gettimeofday(&startTime,NULL);
    long long start_timeMs = startTime.tv_sec*1000 + startTime.tv_usec/1000;
    

    timeval curTime;
    
    while(1)
        {
            memset(&curTime,0,sizeof(struct timeval));
            gettimeofday(&curTime,NULL);
            long long cur_timeMs = curTime.tv_sec*1000 + curTime.tv_usec/1000;

            if((cur_timeMs - start_timeMs) >= 600)  //600ms
            {
                g_workerSMAATO_logger->debug("WAIT TIMEOUT CLOSE SOCKET");        
                smaatoSocketList_Lock.lock();
                connectNumReduce();
                smaatoSocketList_Lock.unlock();
                close(sock);
                delete [] recv_str;
                delete [] fullData_t->data;
                delete [] fullData_t;
                return false;
            }
            
            memset(recv_str,0,BUF_SIZE*sizeof(char));    
            recv_bytes = recv(sock, recv_str, BUF_SIZE*sizeof(char), 0);
            if (recv_bytes == 0)    //connect abort
            {
                g_workerSMAATO_logger->debug("server {0} CLOSE_WAIT ... \r\n", "SMAATO");    
                smaatoSocketList_Lock.lock();
                connectNumReduce();
                smaatoSocketList_Lock.unlock();
                close(sock);
                delete [] recv_str;
                delete [] fullData_t->data;
                delete [] fullData_t;
                return false;
            }
            else if (recv_bytes < 0)  //SOCKET_ERROR
            {
                //socket type: O_NONBLOCK
                if(errno == EAGAIN)     //EAGAIN mean no data in recv_buf world be read, loop break
                {
                    //g_workerSMAATO_logger->trace("ERRNO EAGAIN: RECV END");
                    if(waitFlag)
                    {
                        usleep(10000); //10ms
                        continue ;
                    }
                    else
                    {
                        smaatoSocketList_Lock.lock();
                        smaatoSocketList->push_back(sock);
                        smaatoSocketList_Lock.unlock();
                        break;
                    }
                }
                else if(errno == EINTR) //function was interrupted by a signal that was caught, before any data was available.need recv again
                {
                    g_workerSMAATO_logger->trace("ERRNO EINTR: RECV AGAIN");
                    continue;
                }
            }
            else    //normal success
            {
                waitFlag = false;
                if(temp)
                    g_workerSMAATO_logger->trace("SPLICE HAPPEN");
                //g_logger->debug("\r\n{0}", recv_str);            
                int full_expectLen = fullData_t->curLen + recv_bytes;
                if(full_expectLen > BUF_SIZE)
                {
                    g_workerSMAATO_logger->error("RECV BYTES:{0:d} > BUF_SIZE[{1:d}], THROW AWAY", full_expectLen, BUF_SIZE);
                    smaatoSocketList_Lock.lock();
                    connectNumReduce();
                    smaatoSocketList_Lock.unlock();
                	close(sock);
                    delete [] recv_str;
                    delete [] fullData_t->data;
                    delete [] fullData_t;
                    return false;
                }
                char *curPos = fullData_t->data + fullData_t->curLen;
                memcpy(curPos, recv_str, recv_bytes);
                fullData_t->curLen += recv_bytes;
                temp++;            
            }
            //usleep(10000); //10ms
        }

    //close(sock);
    delete [] recv_str;
    return true;
}

void smaatoObject::smaatoConnectDSP()
{
	#if 0
	int preNum = getPreConnectNum();
    for(int i=0; i < preNum; i++)
    {
        if(smaatoAddConnectToDSP())
			connectNumIncrease();        
    }
	#endif
}

void smaatoObject::smaatoAddConnectToDSP(void *arg)
{
    smaatoObject *smaatoObj = (smaatoObject *)arg;
    sockaddr_in sin;
    struct hostent *m_hostent = NULL;
    unsigned short httpPort = atoi(smaatoObj->getAdReqPort().c_str());      
    
    sin.sin_family = AF_INET;    
    sin.sin_port = htons(httpPort);    
    if(!smaatoObj->getAdReqIP().empty())
    {
        g_workerSMAATO_logger->debug("adReq IP: {0}", smaatoObj->getAdReqIP());
        sin.sin_addr.s_addr = inet_addr(smaatoObj->getAdReqIP().c_str());
    }
    else if(!smaatoObj->getAdReqDomain().empty())
    {
        m_hostent = gethostbyname(smaatoObj->getAdReqDomain().c_str());
        if(m_hostent == NULL)
        {
            g_workerSMAATO_logger->error("SMAATO: gethostbyname error for host: {0}", smaatoObj->getAdReqDomain());
            smaatoObj->connectNumReduce();
            return ;
        }
        sin.sin_addr.s_addr = *(unsigned long *)m_hostent->h_addr;
        g_workerSMAATO_logger->debug("SMAATO IP: {0}", inet_ntoa(sin.sin_addr));
    }
    else
    {
        g_workerSMAATO_logger->error("ADD CON GET IP FAIL");
        smaatoObj->connectNumReduce();
        return ;
    }
    
    

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        g_workerSMAATO_logger->error("ADD CON SOCK CREATE FAIL ...");
        smaatoObj->connectNumReduce();
        free(m_hostent);
        m_hostent = NULL;
        return ;
    }   

    //非阻塞
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    //建立连接
    int ret = connect(sock, (const struct sockaddr *)&sin, sizeof(sockaddr_in));    
    free(m_hostent);
    m_hostent = NULL;
    if(checkConnect(sock, ret) <= 0)
    {
        g_workerSMAATO_logger->error("ADD CON CONNECT FAIL ...");      
        smaatoObj->connectNumReduce();
        close(sock);
        return ;
    }

    //add this socket to event listen queue
    smaatoObj->smaatoSocketList_Locklock();
    smaatoObj->getSmaatoSocketList()->push_back(sock);
    smaatoObj->smaatoSocketList_Lockunlock();
    return ;
    
}

void inmobiObject::readInmobiConfig()
{
     ifstream ifile; 
    ifile.open("./conf/inmobiConfig.json",ios::in);
    if(ifile.is_open() == false)
    {               
        g_worker_logger->error("Open inmobiConfig.json failure...");
        exit(1);    
    }   
    
    Json::Reader reader;
    Json::Value root;
    
    if(reader.parse(ifile, root))
    {
        ifile.close();       
    
        //filter
        siteId = root["siteId"].asString();        
        placementId = root["placementId"].asBool();
        
    }
    else
    {
        g_worker_logger->error("Parse inmobiConfig.json failure...");
        ifile.close();
        exit(1);
    }    
}

bool inmobiObject::sendAdRequestToInMobiDSP(const char *data, int dataLen, bool enLogRsq)
{
    //初始化发送信息
    char *send_str = new char[4096];
    memset(send_str,0,4096*sizeof(char));  

    char Url[100] = {0};
    strcpy(Url,getAdReqUrl().c_str());        
    g_workerINMOBI_logger->trace("ADREQ URL : {0}",Url);

    //请求行
    strcat(send_str, getAdReqType().c_str());
    strcat(send_str, Url);
   
    strcat(send_str, getHttpVersion().c_str());    
    strcat(send_str, "\r\n");     
    
    //头信息
    gen_HttpHeader(send_str, dataLen);

    //内容信息    
    strcat(send_str, "\r\n");    
    strcat(send_str, data);
    
    if(enLogRsq)
    {
        g_workerINMOBI_logger->debug("\r\n{0}",send_str);
    }   

     if(getCurConnectNum() == 0)
    {
        g_workerINMOBI_logger->debug("NO CONNECTION TO GYIN");
        return false;
    }
    
    
    listenObject *obj = NULL;
    
    listenObjectList_Lock();
    if(!getListenObjectList()->empty())
    {
        obj = getListenObjectList()->front();
        getListenObjectList()->pop_front();
    }    
    listenObjectList_unLock();  

    if(!obj)
    {
        g_workerINMOBI_logger->debug("NO IDLE SOCK ");
        delete [] send_str;
        return false;
    }
    
    int sock =  obj->sock;

    bool ret = true;
    if(socket_send(sock, send_str, strlen(send_str)) == -1)
    {        
        g_workerINMOBI_logger->error("adReqSock send failed ...");
        ret = false;
    }         
    listenObjectList_Lock();
    getListenObjectList()->push_back(obj);
    listenObjectList_unLock();  
    delete [] send_str;
    return ret;
    
}


