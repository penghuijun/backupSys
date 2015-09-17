#include "DSPconfig.h"

extern shared_ptr<spdlog::logger> g_worker_logger;
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

struct listenObject* dspObject::findListenObject(int sock)
{
    list<listenObject *>::iterator it = m_listenObjectList.begin();
    for( ; it != m_listenObjectList.end(); it++)
    {
        listenObject *object = *it;
        if(object->sock == sock)
            return object;
    }
    return NULL;
}

void dspObject::eraseListenObject(int sock)
{
    list<listenObject *>::iterator it = m_listenObjectList.begin();
    for( ; it != m_listenObjectList.end(); it++)
    {
        listenObject *object = *it;
        if(object->sock == sock)
        {
            m_listenObjectList.erase(it);
            return ;
        }            
    }
}

void chinaTelecomObject::readChinaTelecomConfig()
{
    ifstream ifile;	
    ifile.open("chinaTelecomConfig.json",ios::in);
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
        name        = root["name"].asString();
        tokenType   = root["tokenType"].asString();
        tokenIP     = root["tokenIP"].asString();
        tokenPort   = root["tokenPort"].asString();
        tokenUrl    = root["tokenUrl"].asString();
        user        = root["user"].asString();
        passwd      = root["passwd"].asString();
        adReqType   = root["adReqType"].asString();
        adReqIP     = root["adReqIP"].asString();
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

        //filter
        extNetId    = root["extNetId"].asString();
        intNetId    = root["intNetId"].asString();        
        
    }
    else
    {
        g_worker_logger->error("Parse clientConfig.json failure...");
        ifile.close();
        exit(1);
    }

    #if 0
    string buf;	    
    while(getline(ifile,buf))	
    {		
        if(buf.empty() || buf.at(0) == '#' || buf.at(0)==' ') 
            continue;		
        if(string_find(buf,"name"))		
        {			
            strGet(name,buf.c_str());			
            //cout << "name: " << name <<endl; 		
        }			
        else if(string_find(buf,"tokenType"))		
        {			
            strGet(tokenType,buf.c_str());			
            //cout << "tokenType: " << tokenType <<endl; 		
        }	
        else if(string_find(buf,"tokenIP"))		
        {			
            strGet(tokenIP,buf.c_str());			
            //cout << "tokenIP: " << tokenIP <<endl; 		
        }	
        else if(string_find(buf,"tokenPort"))		
        {			
            strGet(tokenPort,buf.c_str());			
            //cout << "tokenPort: " << tokenPort <<endl; 		
        }	
        else if(string_find(buf,"tokenUrl"))		
        {			
            strGet(tokenUrl,buf.c_str());			
            //cout << "tokenUrl: " << tokenUrl <<endl; 		
        }	
        else if(string_find(buf,"user"))		
        {			
            strGet(user,buf.c_str());			
            //cout << "user: " << user <<endl; 		
        }	
        else if(string_find(buf,"user"))		
        {			
            strGet(user,buf.c_str());			
            //cout << "user: " << user <<endl; 		
        }	
        else if(string_find(buf,"passwd"))		
        {			
            strGet(passwd,buf.c_str());			
            //cout << "passwd: " << passwd <<endl; 		
        }	
        else if(string_find(buf,"adReqType"))		
        {			
            strGet(adReqType,buf.c_str());			
            //cout << "adReqType: " << adReqType <<endl; 		
        }
        else if(string_find(buf,"adReqIP"))		
        {			
            strGet(adReqIP,buf.c_str());			
            //cout << "adReqIP: " << adReqIP <<endl; 		
        }
        else if(string_find(buf,"adReqPort"))		
        {			
            strGet(adReqPort,buf.c_str());			
            //cout << "adReqPort: " << adReqPort <<endl; 		
        }
        else if(string_find(buf,"adReqUrl"))		
        {			
            strGet(adReqUrl,buf.c_str());			
            //cout << "adReqUrl: " << adReqUrl <<endl; 		
        }
        else if(string_find(buf,"httpVersion"))		
        {			
            strGet(httpVersion,buf.c_str());			
            //cout << "httpVersion: " << httpVersion <<endl; 		
        }
        else if(string_find(buf,"Connection"))		
        {			
            strGet(Connection,buf.c_str());			
            //cout << "Connection: " << Connection <<endl; 		
        }
        else if(string_find(buf,"Cache-Control"))		
        {			
            strGet(CacheControl,buf.c_str());			
            //cout << "Cache-Control: " << CacheControl <<endl; 		
        }
        else if(string_find(buf,"User-Agent"))		
        {			
            strGet(UserAgent,buf.c_str());			
            //cout << "UserAgent: " << UserAgent <<endl; 		
        }        
        else if(string_find(buf,"Content-Type"))		
        {			
            strGet(ContentType,buf.c_str());			
            //cout << "Content-Type: " << ContentType <<endl; 		
        }
        else if(string_find(buf,"AcceptType"))		
        {			
            strGet(Accept,buf.c_str());			
            //cout << "Accept: " << Accept <<endl; 		
        }
        else if(string_find(buf,"Accept-Encoding"))		
        {			
            strGet(AcceptEncoding,buf.c_str());			
            //cout << "Accept-Encoding: " << AcceptEncoding <<endl; 		
        }
        else if(string_find(buf,"Accept-Language"))		
        {			
            strGet(AcceptLanguage,buf.c_str());			
            //cout << "Accept-Language: " << AcceptLanguage <<endl; 		
        }
        else if(string_find(buf,"charset"))		
        {			
            strGet(charset,buf.c_str());			            
        }
        else if(string_find(buf,"Host"))		
        {			
            strGet(Host,buf.c_str());			            	
        }
        else if(string_find(buf,"Cookie"))		
        {			
            strGet(Cookie,buf.c_str());			            	
        }
    }	
    #endif
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

    char *Url = new char[100];
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


    //头信息
    strcat(send_str, tokenType.c_str());
    strcat(send_str, Url);
    strcat(send_str, httpVersion.c_str());    
    strcat(send_str, "\r\n");
    
    
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
    

    //内容信息
    strcat(send_str, "\r\n");
    //cout << "send_str: " << send_str << endl;
	delete [] Url;
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
bool chinaTelecomObject::sendAdRequestToChinaTelecomDSP(struct event_base * base, const char *data, int dataLen, event_callback_fn fn, void *arg)
{
    //初始化发送信息
    //char send_str[2048] = {0};
    char *send_str = new char[4096];
    memset(send_str,0,4096*sizeof(char));  

    char *Url = new char[100];
    strcpy(Url,adReqUrl.c_str());        
    string str = "?username="+user+"&password="+CeritifyCode;
    strcat(Url,str.c_str());    
    g_worker_logger->debug("ADREQ URL : {0}",Url);

    ostringstream os;
    os<<adReqIP<<":"<<adReqPort;    

    //头信息
    strcat(send_str, adReqType.c_str());
    strcat(send_str, Url);
    delete [] Url;
    strcat(send_str, httpVersion.c_str());    
    strcat(send_str, "\r\n");             
    
    char content_header[100];
    sprintf(content_header,"Content-Length: %d\r\n", dataLen);
    strcat(send_str, content_header); 

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


    //内容信息
    
    strcat(send_str, "\r\n");    
    strcat(send_str, data);
    
    
    //cout << "send_str : " << send_str << endl;
    //g_worker_logger->debug("ADREQ DATA : ",data);
    
    
    sockaddr_in sin;
    unsigned short httpPort = atoi(adReqPort.c_str());      
    
    sin.sin_family = AF_INET;    
	sin.sin_port = htons(httpPort);    
	sin.sin_addr.s_addr = inet_addr(adReqIP.c_str());

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        g_worker_logger->error("adReqSock create failed ...");
        return false;
    }	

    //建立连接
    if (connect(sock, (const struct sockaddr *)&sin, sizeof(sockaddr_in) ) == -1)
    {
        g_worker_logger->error("adReqSock connect failed ...");   
        close(sock);
        return false;
    }
    
    //add this socket to event listen queue
	struct event *sock_event;
	sock_event = event_new(base, sock, EV_READ|EV_PERSIST, fn, arg);     
    event_add(sock_event, NULL);

    struct listenObject *listen = new listenObject();    
    listen->sock = sock;
    listen->_event = sock_event;
    //m_listenObjectList.push_back(listen);
    getListenObjectList().push_back(listen);
    
    //cout << "@@@@@ This msg send by PID: " << getpid() << endl; 
    if (send(sock, send_str, strlen(send_str),0) == -1)
    {        
        g_worker_logger->error("adReqSock send failed ...");
        delete [] send_str;
        return false;
    }         
	delete [] send_str;
    return true;
}

void guangYinObject::readGuangYinConfig()
{
    ifstream ifile; 
    ifile.open("guangYinConfig.json",ios::in);
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
        name        = root["name"].asString();
        
        adReqType   = root["adReqType"].asString();
        adReqIP     = root["adReqIP"].asString();
        adReqPort   = root["adReqPort"].asString();
        adReqUrl    = root["adReqUrl"].asString();
        
        httpVersion = root["httpVersion"].asString();
    
        //HTTP header
        Connection  = root["Connection"].asString();
        UserAgent   = root["User-Agent"].asString();
        ContentType = root["Content-Type"].asString();        
        Host        = root["Host"].asString();        
    
        //filter
        publisherId = root["publisherId"].asString();
        extNetId    = root["extNetId"].asString();
        intNetId    = root["intNetId"].asString();     
        test        = root["test"].asBool();
        
    }
    else
    {
        g_worker_logger->error("Parse guangYinConfig.json failure...");
        ifile.close();
        exit(1);
    }

}

bool guangYinObject::sendAdRequestToGuangYinDSP(struct event_base * base, const char *data, int dataLen, event_callback_fn fn, void *arg)
{
    //初始化发送信息
    //char send_str[2048] = {0};
    char *send_str = new char[4096];
    memset(send_str,0,4096*sizeof(char));       

    //头信息
    strcat(send_str, adReqType.c_str());
    strcat(send_str, adReqUrl.c_str());    
    strcat(send_str, httpVersion.c_str());    
    strcat(send_str, "\r\n");             
    
    char content_header[100];
    sprintf(content_header,"Content-Length: %d\r\n", dataLen);
    strcat(send_str, content_header); 

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

    if(Host.empty() == false)
    {
        strcat(send_str, "Host: ");
        strcat(send_str,Host.c_str());
        strcat(send_str, "\r\n");
    }      


    //内容信息
    
    strcat(send_str, "\r\n");    
    strcat(send_str, data);
    
    
    //cout << "send_str : " << send_str << endl;
    g_worker_logger->debug("GYin ADREQ datalen: {0:d}  ",dataLen);
    
    
    sockaddr_in sin;
    unsigned short httpPort = atoi(adReqPort.c_str());      
    
    sin.sin_family = AF_INET;    
    sin.sin_port = htons(httpPort);    
    sin.sin_addr.s_addr = inet_addr(adReqIP.c_str());

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        g_worker_logger->error("adReqSock create failed ...");
        return false;
    }   
    g_worker_logger->debug("adReqSock create success ");
    //建立连接
    if (connect(sock, (const struct sockaddr *)&sin, sizeof(sockaddr_in) ) == -1)
    {
        g_worker_logger->error("adReqSock connect failed ...");      
        close(sock);
        return false;
    }
    g_worker_logger->debug("adReqSock connect success ");
    
    //add this socket to event listen queue
    struct event *sock_event;
    sock_event = event_new(base, sock, EV_READ|EV_PERSIST, fn, arg);     
    event_add(sock_event, NULL);

    struct listenObject *listen = new listenObject();    
    listen->sock = sock;
    listen->_event = sock_event;
    //listenObjectList.push_back(listen);
    getListenObjectList().push_back(listen);
    
    //cout << "@@@@@ This msg send by PID: " << getpid() << endl; 
    if (send(sock, send_str, strlen(send_str),0) == -1)
    {        
        g_worker_logger->error("adReqSock send failed ...");
        delete [] send_str;
        return false;
    }         
    g_worker_logger->debug("adReqSock send success ");
    delete [] send_str;
    return true;
}



