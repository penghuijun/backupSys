#include <vector>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <event.h>
#include "httpPool.h"

char *exchangeConnector::gen_http_response(char *buf, int recvSize,int *httpLen)
{
    bool ret = parse_http_truck(buf, recvSize);
    if(ret == true && httpLen != NULL)
    {   
        int length = 0;
        auto it  = m_httpTrunkList.begin();
        for(it = m_httpTrunkList.begin(); it != m_httpTrunkList.end(); it++)
        {
            httpTrunkNode *node = *it;
            if(node) 
            {
                int trunk_size = node->bufLen-node->trunkSizeLen-4;
                length += trunk_size;
            }
        }

        char *str = new char[length+1];
        memset(str, 0x00, length+1);
        char *strPtr = str;   
        it  = m_httpTrunkList.begin();
        for(it = m_httpTrunkList.begin(); it != m_httpTrunkList.end();)
        {
            httpTrunkNode *node = *it;
            if(node)
            {
                int trunk_size = node->bufLen-node->trunkSizeLen-4;
                memcpy(strPtr, node->buf+node->trunkSizeLen+2, trunk_size);
                strPtr += trunk_size;
                delete[] node->buf;
                delete node;
            }
            it = m_httpTrunkList.erase(it);
        } 
        *httpLen = length;
        return str;
    }
    return NULL;
}


bool exchangeConnector::parse_http_truck(char *buf, int bufSize)
{

    if(m_httpTrunkList.size() > 0)//如果缓存中有数据了
    {
        httpTrunkNode *node = m_httpTrunkList.back();//the last node

        if(node && (node->bufLen != node->haveRcevedLen))//看是否缓存中的每一个truck都满了，未满之间继续填充
        {
            int needRecvByte = node->bufLen - node->haveRcevedLen;//需要填满最后一个truck所需字节
            if(needRecvByte < bufSize)//if recv byte more than one truck ,then parse next truck
            {
                memcpy(node->buf+node->haveRcevedLen, buf, needRecvByte);
                node->haveRcevedLen = node->bufLen;
                bool result =  parse_http_truck(buf+needRecvByte, bufSize-needRecvByte);
                return result;
            }
            else
            {
            
                memcpy(node->buf+node->haveRcevedLen, buf, bufSize);
                node->haveRcevedLen += bufSize;
                return false;
            }
        }
    }

    //if the first truck or last truck is full, then parse next parse  
    
    
    char *pos=strstr(buf, "\r\n");//find truck size
    if(pos == NULL) return false;//not find, game over   
    
    int trunkSizeLen = pos-buf;//trunksize len
    int trunkSize = hex_to_dec(buf,trunkSizeLen);//trunk size
    int haveRecvByte = 0;//have recv byte;
    int truckCacheSize = trunkSize+trunkSizeLen+2+2;//trunk size'len + \r\n+trunk +\r\n
    char *trunk = new char[truckCacheSize]; 
    if(trunkSize == 0)//the last trunk , body recv over
    {    
        memcpy(trunk, buf, truckCacheSize);
        httpTrunkNode *node = new httpTrunkNode;
        node->buf = trunk;
        node->trunkSizeLen = trunkSizeLen;
        node->bufLen = truckCacheSize;
        node->haveRcevedLen = truckCacheSize;
        m_httpTrunkList.push_back(node);
        return true;
    }
    

    if(bufSize > truckCacheSize)   //one buf have more than one truck;
    {
        memcpy(trunk, buf, truckCacheSize);
        httpTrunkNode *node = new httpTrunkNode;
        node->buf = trunk;
        node->trunkSizeLen = trunkSizeLen;
        node->bufLen = truckCacheSize;
        node->haveRcevedLen = truckCacheSize;
        m_httpTrunkList.push_back(node);
        bool result = parse_http_truck(buf+truckCacheSize, bufSize-truckCacheSize); 
        return result;
    }
    else // one buf is one truck or less than one
    {
        memcpy(trunk, buf, bufSize);
        httpTrunkNode *node = new httpTrunkNode;
        node->buf = trunk;
        node->trunkSizeLen = trunkSizeLen;
        node->bufLen = truckCacheSize;
        node->haveRcevedLen = bufSize;
        m_httpTrunkList.push_back(node);    
    }
    return false;
}



bool exchangeConnector::get_content_length(string &src,string& subStr, int &value)
{
    int idx =src.find(subStr);
    if(idx == string::npos)
    {
        return false;
    }
    
    idx += subStr.size();
    int idx1 = idx;
    bool valid = false;
    while(isdigit(src.at(idx1)))
    {
        idx1++;
        valid = true;
    }
    if(valid == false) return false;
    
    string numStr = src.substr(idx,idx1);
    value = atoi(numStr.c_str());
    return true;
}

bool parse_http_response_head(string &resp_head)
{
    int idx = resp_head.find("HTTP/");
    if(idx != string::npos)
    {
        string r_status = resp_head.substr(9, 3);    
      //  cout<<"http response status:"<< r_status<<":"<<r_status.size()<<endl;   

        if(r_status.compare("200") != 0)
        {
             cout<<"http response status is not 200"<<endl;   
            return false;
        }   
    }

    return true;
}


/*
  *function name:http_Rsp_handler
  *fun: asyn recv http response with libevent, and parse recv data, if the http response recv over, the gen the http response data
  *return: if the http response is not recved over, then return NULL. if recv over, return the complete http response
  *param: buf, recv data. recvSize, recv data size, httpLen, return httpLen;
  */
char* exchangeConnector::recv_async( char *buf, int recvSize, int *httpLen)
{
    int pos;
    int idx = 0;
    char *rspBuf;
    if(buf==NULL) return NULL;

    //if it is the header of http, then start recv header
    if((memcmp(buf,"HTTP/1.1",8) == 0)||(memcmp(buf,"HTTP/1.0", 8) == 0))//http head start recv
    {
        clearRecvCache();
    }

    if(m_recvHead)//recv head
    {    
        m_recvCache+=buf;
        pos = m_recvCache.find("\r\n\r\n");//"\r\n\r\n" represent the header of the http response is recv over, this is the seperat sym of header, body 
       
        if(pos == string::npos) //head not recv all
        {
            return NULL;
        }

         //head recv over,
        m_recvHead = false;
   //     cout<<m_recvCache<<endl;
        if(parse_http_response_head(m_recvCache)==false) return NULL;
        
        int idx =m_recvCache.find("Transfer-Encoding: chunked");
        if(idx != string::npos)//it is chunked
        {
            m_truckEncode = true;
        }
        else//not truck
        {
            string contextLenStr("Content-Length: ");
            //it is not chunked, and body's length was knowed
            if(get_content_length(m_recvCache, contextLenStr, m_context_length) == true)
            {
                m_truckEncode = false;
            }
            else//if it is not chunked and not content-length, then is invalid data, can not parse
            {
                m_recvCache.clear();
                cout <<"unknow thrans-encoding, not trunk and no context length!"<<endl;
                return NULL;
            }
        }      

        idx = pos+4;//http response header's length 
        m_headSize = idx;
        if((idx+m_context_length)<=m_recvCache.size())
        {
           m_http_context = new char[m_context_length+1];
           memcpy(m_http_context, m_recvCache.c_str()+idx, m_context_length);
           *(m_http_context+m_context_length)='\0';
           *httpLen = m_context_length+1;
            return m_http_context;         
        }
        else
        {
            return NULL;
        }

        /*
        if(idx == recvSize)//recv data is http response header all or rsponse body is null
        {
            return NULL;
        }
        else if(idx<recvSize)//start recv body
        {
            int dataLen = recvSize -idx;//reserv datalen
            char *data = buf+idx;//http response body

            if(m_truckEncode)//trunk code
            {
                //generate http response
                rspBuf = gen_http_response(data, dataLen, httpLen);
                if(rspBuf != NULL)
                {
                    m_recvHead = true;
                    return rspBuf;
                }   
            }
            else//content length
            {
                m_have_recved_length = ((m_context_length < dataLen)?m_context_length:dataLen);
                m_http_context = new char[m_context_length+1];
                memset(m_http_context, 0x00, sizeof(m_http_context));
                memcpy(m_http_context, data, m_have_recved_length);
                if(m_have_recved_length >= m_context_length) 
                {
                    *httpLen = m_context_length+1;
                    return m_http_context;
                }
                return NULL;
            }
        }*/
    }
    else//recv body
    {
        m_recvCache+=buf;
        if(m_truckEncode)
        {
           /* rspBuf = gen_http_response(buf, recvSize, httpLen);
            if(rspBuf != NULL)
            {
                m_recvHead = true;
                return rspBuf;
            }*/
        }
        else
        {
            if((m_headSize + m_context_length)<=m_recvCache.size())
            {
               m_http_context = new char[m_context_length+1];
               memcpy(m_http_context, m_recvCache.c_str()+m_headSize, m_context_length);
               *(m_http_context+m_context_length)='\0';
               *httpLen = m_context_length+1;
                return m_http_context;         
            }
        }
    }
    return NULL;
}


int exchangeConnector::hex_to_dec(char *str, int size) const
{
    char *strPtr = str;
    int idx = 0;
    int dec = 0;
    for(idx = 0; idx < size; idx++)
    {
        char ch = *(str+idx);
        int num;
        switch(ch)
        {
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            {
                num = ch-'a'+10;
                break;
            }
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            {
                num = ch-'A'+10;
                break;
            }
            default:
            {
                if(isdigit(ch))
                {
                    num = ch -'0';
                }
                else
                {
                    return dec;
                }
                break;
            }       
        }
        dec <<= 4;
        dec += num;
    }
    return dec;
}



int exchangeConnector::exchange_connector_connect()
{
    struct sockaddr_in servAddr;
    struct sockaddr_in myAddr;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        perror("socket:");
        return -1;
    }
    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(m_port);
    if(inet_pton(AF_INET, m_ip.c_str(), &servAddr.sin_addr) <= 0)
    {
        perror("inet_pton:");
        close(sockfd);
        return -1;
    }


    bzero(&myAddr, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(0);
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(sockfd, (struct sockaddr*)&myAddr, sizeof(myAddr))< 0)
    {
        perror("bind");
        close(sockfd);
        return -1;
    }

    if(connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr))< 0)
    {
        perror("connect");
        close(sockfd);
        return -1;
    }
    else
    {
        cout <<"connect success:"<<sockfd<<endl; 
    }
    
    int flags = fcntl(sockfd, F_GETFL);
    if(flags == -1)
    {
        perror("fcntl-getfile");
        close(sockfd);
        return -1;        
    }

    flags = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK); 
    if(flags == -1)
    {
        perror("fcntl-setfile");
        close(sockfd);
        return -1;        
    }

    return sockfd;
}


void exchangeConnector::add_connector_libevent(struct event_base* base ,  libevent_fun fun, void *serv)
{
    m_reqSending = false;
    struct timeval five_seconds = {500,0};
    m_event = event_new(base, m_sockfd , EV_TIMEOUT|EV_READ|EV_PERSIST, fun, serv);    
    event_add(m_event, &five_seconds);   
}


int exchangeConnector::connect_async(struct event_base* base ,  libevent_fun fun, void *serv)
{
     m_reqSending = false;
     m_sockfd = exchange_connector_connect();
     if(m_sockfd>0)
     {
        add_connector_libevent(base, fun, serv);
     }
     else
     {
        m_event = NULL;
     }
     return m_sockfd;
}

void exchangeConnector::release()
{
    event_del(m_event);
    auto it = m_httpTrunkList.begin();
    for(it = m_httpTrunkList.begin();it != m_httpTrunkList.end();)
    {
        httpTrunkNode *node =  *it;
        if(node) delete[] node->buf;
        delete node;
        it=m_httpTrunkList.erase(it);
    }
    m_context_length = 0;
    m_have_recved_length = 0;
    m_httpHead.clear();
    m_http_context = NULL;
    m_recvHead = true;
    m_reqSending = false;
    close(m_sockfd);
    m_sockfd = -1;
}

void exchangeConnector::reset_recv_state()
{

    m_reqSending = false;
    clearRecvCache();

}

void exchangeConnector::clearRecvCache()
{
       m_recvHead = true;
       m_httpHead.clear();
       m_recvCache.clear();
       m_context_length = 0;
       m_have_recved_length = 0;
       m_http_context = NULL;
       m_reqSending = false;
       
       auto it = m_httpTrunkList.begin();
       for(it = m_httpTrunkList.begin(); it != m_httpTrunkList.end();)
       {
           httpTrunkNode *trunk = *it;
           if(trunk) 
           {
               delete[] trunk->buf;
               delete trunk;
           }
           it = m_httpTrunkList.erase(it);
       }

}

socket_state exchangeConnector::get_excahnge_connector_state()
{
    if(m_sockfd==-1)//reconnect
    {
        m_sockfd = exchange_connector_connect();
        m_reqSending=false;
        if(m_sockfd <= 0) return socket_invalid;// can not use
    }
    
    if(m_reqSending == false)//if 
    {
       m_reqSending = true;
       return socket_valid;
    }

    return socket_sending;
}

bool exchangeConnector::need_update_libevent()
{
    if(m_sockfd<0)
    {
        m_reqSending = false;
        m_sockfd = exchange_connector_connect();
        if(m_sockfd>0) return true;
    }
    return false;    
}


void httpPool::set_externBidInfo(string ip, unsigned short port, vector<string>& key,unsigned short connNum)
{
    m_externBidIP = ip;
    m_externBidPort = port;
    m_exChangeConnectorNum = connNum;
    m_publishKey = key;
}


bool httpPool::subsrcibeKeyExist(string &publishKey)
{
    if(find(m_publishKey.begin(), m_publishKey.end(), publishKey) != m_publishKey.end())
    {
        return true;
    }
    return false;

}

httpPool* httpPool::update_exchange_connector_pool(string ip, unsigned short port, vector<string> &subkey, struct event_base* base , libevent_fun fun, void *serv, unsigned short connNum)
{
    m_publishKey.clear();
    m_publishKey = subkey;

    int diff=0;
    if(connNum >= m_exChangeConnectorNum)
    {
        diff = connNum - m_exChangeConnectorNum;
        if(diff > 0)
        {
            auto i = 0;
            for(i = 0; i < diff; i++)
            {
                exchangeConnector *conn = new exchangeConnector(m_externBidIP, m_externBidPort);
                conn->connect_async( base, fun, serv);
                m_exchangeConnectorPool.push_back(conn);
            }
        }
    }
    else
    {
        diff = m_exChangeConnectorNum - connNum;
        auto it = m_exchangeConnectorPool.begin();
        for(it = m_exchangeConnectorPool.begin(); it != m_exchangeConnectorPool.end();)
        {
            exchangeConnector *conn = *it;
            if(conn&&conn->get_connector_send_state()==false)
            {
                conn->release();
                it = m_exchangeConnectorPool.erase(it);
                diff--;
            }
            else
            {
                it++;
            }
            if(diff==0)break;
        }

        if(diff!=0)
        {
            for(it = m_exchangeConnectorPool.begin(); it != m_exchangeConnectorPool.end();it++)
            {
                exchangeConnector *conn = *it;
                if(conn)
                {
                    conn->set_disconnect_sym(true);
                    diff--;
                }
                if(diff==0)break;
            }
        }
    }
    return this;
}

void httpPool::display()
{
    cout<<"==========================="<<endl;
    cout<<m_externBidIP<<"   "<<m_externBidPort<<"   "<<m_exChangeConnectorNum<<endl;
    for(auto it1=m_publishKey.begin(); it1 != m_publishKey.end(); it1++)
    {
        cout <<*it1<<"  "<<(*it1).size()<<"    ";
    }
    cout<<endl;
    cout<<"==========================="<<endl;
}


exchangeConnector *httpPool::get_http_connector(int fd)
{
    for(auto it = m_exchangeConnectorPool.begin(); it != m_exchangeConnectorPool.end(); it++)
    {
        exchangeConnector *sock = *it;
        if(sock)
        {
            if(sock->get_sockfd() == fd) 
            {
                return sock;
            }
        }

    }
    return NULL;
}

bool httpPool::traval_exchange_connector_pool()
{
    int fd;
    bool update = false;
    auto it = m_exchangeConnectorPool.begin();
    for(it = m_exchangeConnectorPool.begin(); it != m_exchangeConnectorPool.end(); it++)
    {
        exchangeConnector *ex =  *it;
        if(ex && ex->need_update_libevent()) update = true;
    }
    return update;
}


void httpPool::add_exchange_connector_pool_libevent(struct event_base* base , libevent_fun fun, void *serv)
{
    for(auto it = m_exchangeConnectorPool.begin(); it != m_exchangeConnectorPool.end(); it++)
    {
        exchangeConnector *ex =  *it;
        if(ex) ex->add_connector_libevent(base, fun, serv);
    }
}

httpPool* httpPool::exchange_connector_pool_init(string ip, unsigned short port, vector<string> &subkey, struct event_base* base , libevent_fun fun, void *serv, unsigned short connNum)
{
    set_externBidInfo(ip, port, subkey,connNum);
    auto i = 0;
    for(i = 0; i < connNum; i++)
    {
        exchangeConnector *conn = new exchangeConnector(m_externBidIP, m_externBidPort);
        conn->connect_async( base, fun, serv);
        m_exchangeConnectorPool.push_back(conn);
    }
    return this;
}

bool httpPool::release_exchange_connector(int fd)
{
    for(auto it = m_exchangeConnectorPool.begin(); it != m_exchangeConnectorPool.end(); it++)
    {
        exchangeConnector *ch = *it;
        if(ch&& ch->get_sockfd()==fd)
        {
            ch->release();
            m_exchangeConnectorPool.erase(it);
            return true;
        }
    }
    return false;
}



exchangeConnector *httpPoolManager::get_exchange_connector(int fd)
{
    for(auto it = m_connectorPool.begin(); it != m_connectorPool.end();it++)
    {
        httpPool *ex = *it;
        if(ex) 
        {
            exchangeConnector *ch = ex->get_http_connector(fd);
            if(ch)
            {
                return ch;
            }
        }
    }
    return NULL;
}

void httpPoolManager::display()
{
    for(auto it = m_connectorPool.begin(); it!=m_connectorPool.end(); it++)
    {
        httpPool *ex = *it;
        ex->display();
    }
}

void httpPoolManager::connector_pool_libevent_add(struct event_base* base , libevent_fun fun, void *serv)
{
    for(auto it = m_connectorPool.begin(); it != m_connectorPool.end(); it++)
    {
        httpPool *ex = *it;
        if(ex)
        {
            ex->add_exchange_connector_pool_libevent(base, fun, serv);
        }
    }
}

httpPool *httpPoolManager::get_exchange_connector_pool(string &ip, unsigned short port)
{
    for(auto it = m_connectorPool.begin(); it != m_connectorPool.end(); it++)
    {
        httpPool *pool = *it;
        if(pool)
        {
            if(pool->compare_addr(ip,port))
            {
                return pool;
            }
        }
    }
    return NULL;
}

void httpPoolManager::update_connector_pool(string ip, unsigned short port, vector<string> &subkey, struct event_base* base , libevent_fun fun, void *serv,unsigned short connNum)
{
    httpPool *pool = get_exchange_connector_pool(ip, port);
    if(pool)
    {
        pool->update_exchange_connector_pool(ip,port,subkey, base, fun,serv, connNum);
    }
    else
    {
        httpPool* exCh = new httpPool;
        exCh->exchange_connector_pool_init(ip,port,subkey, base, fun,serv, connNum);
        m_connectorPool.push_back(exCh);
    }
}

void httpPoolManager::erase_connector(int fd)
{
    for(auto it =m_connectorPool.begin(); it != m_connectorPool.end(); it++)
    {
        httpPool *pool = *it;
        if(pool)
        {
            if(pool->release_exchange_connector(fd) == true) 
            {
                return;
            }
        }
    }
}


//init http connector pool
void httpPoolManager::init(const vector<exBidderInfo*>& vec, struct event_base* base , libevent_fun fun, void *serv)
{
    for(auto it = vec.begin(); it != vec.end(); it++)
    {
        exBidderInfo *info = *it;
        if(info)
        {
            httpPool* exCh = new httpPool;
            exCh->exchange_connector_pool_init(info->exBidderIP, info->exBidderPort, info->subKey, base, fun,serv, info->connectNum);
            m_connectorPool.push_back(exCh);
        }
    }
}

