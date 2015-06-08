#include "httpProto.h"
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
#include <errno.h>

 bool httpProtocol::pro_connect()
{
    struct sockaddr_in servAddr;
    struct sockaddr_in myAddr;

    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_fd < 0)
    {
        g_worker_logger->error("create socket exception:{0}", strerror(errno));
        return false;
    }
    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(m_port);
    g_worker_logger->trace("ip:{0}",m_ip);
    if(inet_pton(AF_INET, m_ip.c_str(), &servAddr.sin_addr) <= 0)
    {
        g_worker_logger->error("inet_pton exception:{0}",strerror(errno));
        close(m_fd);
        m_fd = -1;
        return false;
    }


    bzero(&myAddr, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(0);
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(m_fd, (struct sockaddr*)&myAddr, sizeof(myAddr))< 0)
    {
        g_worker_logger->error("bind exception:{0}",strerror(errno));
        close(m_fd);
        m_fd = -1;
        return false;
    }

    if(connect(m_fd, (struct sockaddr*)&servAddr, sizeof(servAddr))< 0)
    {
        g_worker_logger->error("connect exception:{0}",strerror(errno));
        close(m_fd);
        m_fd = -1;
        return false;
    }
    else
    {
        g_worker_logger->trace("connect success:{0:d}",m_fd);
    }
    
    int flags = fcntl(m_fd, F_GETFL);
    fcntl(m_fd, F_SETFL, flags | O_NONBLOCK); 

    return true;
}

int httpProtocol::pro_send(const char * str)
{
    if(m_sending) return -1;
    m_sending = true;
    
    int len = strlen(str);
    char httpRequest[BUF_SIZE];
    char tmpBuf[BUF_SIZE];
    memset(httpRequest, 0, BUF_SIZE);
    sprintf(tmpBuf, "POST http://service.reachjunction.com/dsp HTTP/1.1\r\n");
  //  sprintf(tmpBuf, "POST /br HTTP/1.1\r\n");
    // sprintf(tmpBuf, "GET /%s HTTP/1.1\r\n", str);
    strcat(httpRequest, tmpBuf);//index.html
    sprintf(tmpBuf, "Host: %s:%d\r\n", m_ip.c_str(), m_port);
    strcat(httpRequest, tmpBuf);//202.108.22.5  www.baidu.com
    //    strcat(httpRequest, "Host: 192.168.20.119:8001\r\n");//202.108.22.5  www.baidu.com
    strcat(httpRequest, "Content-Type: application/json\r\n");//202.108.22.5  www.baidu.com
    strcat(httpRequest, "x-openrtb-version: 2.2\r\n");//202.108.22.5  www.baidu.com
    sprintf(tmpBuf, "Content-Length: %d\r\n", len);//202.108.22.5  www.baidu.comContent-Length: 40
    strcat(httpRequest, tmpBuf);
    strcat(httpRequest, "Connection: keep-alive\r\n\r\n");//keep-alive
    sprintf(tmpBuf, "%s", str);
    strcat(httpRequest, tmpBuf);

    int size = write(m_fd, httpRequest, strlen(httpRequest));   
    if(size <= 0)
    {
        m_sending = false;
        return -1;
    }
    return m_fd;
}

bool httpProtocol::pro_connect_asyn(struct event_base * base, event_callback_fn fn, void * arg)
{
    if(m_fd < 0)
    {
        if(pro_connect() == false) return false;
    }
    if(m_fd < 0) return false;
    m_sending = false;
    struct timeval five_seconds = {5,0};
    m_event = event_new(base, m_fd, EV_TIMEOUT|EV_READ|EV_PERSIST, fn, arg);    
    event_add(m_event, &five_seconds); 
}
bool httpProtocol::pro_close()
{
    close(m_fd);
    event_free(m_event);
    m_fd = -1;
    m_event = NULL;
}


bool httpProtocol::parse_http_response_head(string &resp_head)
{
    int idx = resp_head.find("HTTP/");
    if(idx != string::npos)
    {
        string r_status = resp_head.substr(9, 3);    
      //  cout<<"http response status:"<< r_status<<":"<<r_status.size()<<endl;   

        if(r_status.compare("200") != 0)
        {
            g_worker_logger->debug("http response status:{0:d}",r_status);
            return false;
        }   
        
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
        }  

    }
    return true;
}

bool httpProtocol::get_content_length(string &src,string& subStr, int &value)
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


int httpProtocol::hex_to_dec(char *str, int size)
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

char *httpProtocol::gen_http_response(char *buf, int recvSize,int &httpLen)
{
    bool ret = parse_http_truck(buf, recvSize);
    if(ret)
    {   
        int length = 0;
        for(auto it = m_httpTrunkList.begin(); it != m_httpTrunkList.end(); it++)
        {
            httpTrunkNode *node = *it;
            if(node) 
            {
                int trunk_size = node->get_bufLen() - node->get_truckSizeLen() - 4;
                length += trunk_size;
            }
        }

        char *str = new char[length+1];
        memset(str, 0x00, length+1);
        char *strPtr = str;   
        for(auto it = m_httpTrunkList.begin(); it != m_httpTrunkList.end();)
        {
            httpTrunkNode *node = *it;
            if(node)
            {
                int trunk_size = node->get_bufLen() - node->get_truckSizeLen() - 4;
                memcpy(strPtr, node->get_buf() + node->get_truckSizeLen() + 2, trunk_size);
                strPtr += trunk_size;
                delete node;
            }
            it = m_httpTrunkList.erase(it);
        } 
        httpLen = length;
        return str;
    }
    return NULL;
}


bool httpProtocol::parse_http_truck(char *buf, int bufSize)
{
    if(m_httpTrunkList.size() > 0)//如果缓存中有数据了
    {
        httpTrunkNode *node = m_httpTrunkList.back();//the last node

        if(node && (node->get_bufLen() != node->get_recvedLen()))//看是否缓存中的每一个truck都满了，未满之间继续填充
        {
            int notTreatSize = 0;
            char* dataBuf = node->add_buf(buf, bufSize, notTreatSize);

            if(dataBuf)//if recv byte more than one truck ,then parse next truck
            {
                bool result =  parse_http_truck(dataBuf, notTreatSize);
                return result;
            }
            else
            {
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
        httpTrunkNode *node = new httpTrunkNode(trunk, truckCacheSize, trunkSizeLen, truckCacheSize);
        m_httpTrunkList.push_back(node);
        return true;
    }

    if(bufSize > truckCacheSize)   //one buf have more than one truck;
    {
        memcpy(trunk, buf, truckCacheSize);
        httpTrunkNode *node = new httpTrunkNode(trunk, truckCacheSize, trunkSizeLen, truckCacheSize);
        m_httpTrunkList.push_back(node);
        bool result = parse_http_truck(buf+truckCacheSize, bufSize-truckCacheSize); 
        return result;
    }
    else // one buf is one truck or less than one
    {
        memcpy(trunk, buf, bufSize);
        httpTrunkNode *node = new httpTrunkNode(trunk, truckCacheSize, trunkSizeLen, bufSize);
        m_httpTrunkList.push_back(node);    
    }
    return false;
}

/*
  *function name:http_Rsp_handler
  *fun: asyn recv http response with libevent, and parse recv data, if the http response recv over, the gen the http response data
  *return: if the http response is not recved over, then return NULL. if recv over, return the complete http response
  *param: buf, recv data. recvSize, recv data size, httpLen, return httpLen;
  */
transErrorStatus httpProtocol::recv_async(stringBuffer &recvBuf)
{
    int idx = 0;

    char buf[BUFSIZE*10];
    int  recvSize = read(m_fd, buf, sizeof(buf));
    if(recvSize == 0)
    {
        return error_socket_close;
    }
    else if(recvSize < 0)
    {
        return error_recv_null;
    }
    buf[recvSize] ='\0';
    //if it is the header of http, then start recv header
    if((memcmp(buf,"HTTP/1.1",8) == 0)||(memcmp(buf,"HTTP/1.0", 8) == 0))//http head start recv
    {
        init();
    }

    if(m_recvHead)//recv head
    {    
        m_recvCache += buf;
        int pos = m_recvCache.find("\r\n\r\n");//"\r\n\r\n" represent the header of the http response is recv over, this is the seperat sym of header, body 
       
        if(pos == string::npos) //head not recv all
        {
            return error_data_imcomplete;
        }

         //head recv over,
        m_recvHead = false;
        if(parse_http_response_head(m_recvCache)==false) return error_complete;

        idx = pos + 4;//http response header's length 
        if(m_truckEncode == false)
        {
            m_headSize = idx;
            if((m_headSize+m_context_length)<=m_recvCache.size())
            {
               char *http_context = new char[m_context_length+1];
               memcpy(http_context, m_recvCache.c_str() + m_headSize, m_context_length);
               *(http_context+m_context_length) = '\0';
               recvBuf.string_set(http_context, m_context_length+1);
               return error_complete;         
            }
            else
            {
               return error_data_imcomplete;
            }
        }
        else
        {
            if(idx<recvSize)//start recv body
            {
                int dataLen = recvSize -idx;//reserv datalen
                char *data = buf+idx;//http response body
                //generate http response
                int httpLen = 0;
                char *rspBuf = gen_http_response(data, dataLen, httpLen);
                if(rspBuf != NULL)
                {
                    recvBuf.string_set(rspBuf, httpLen);
                    return error_complete;
                }   
            }
        }
    }
    else//recv body
    {
        m_recvCache+=buf;
        if(m_truckEncode)
        {
            int httpLen = 0;
            char *rspBuf = gen_http_response(buf, recvSize, httpLen); 
            if(rspBuf != NULL)
            {
                recvBuf.string_set(rspBuf, httpLen);
                return error_complete;
            }
        }
        else
        {
            if((m_headSize + m_context_length) <= m_recvCache.size())
            {
               char *http_context = new char[m_context_length+1];
               memcpy(http_context, m_recvCache.c_str()+m_headSize, m_context_length);
               *(http_context+m_context_length)='\0';
                recvBuf.string_set(http_context, m_context_length+1);
                return error_complete;     
            }
        }
    }
    return error_data_imcomplete;
}


