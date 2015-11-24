#include "http_parse.h"

extern shared_ptr<spdlog::logger> g_worker_logger;

char* memstr(char* full_data, int full_data_len, const char* substr)  
{  
    if (full_data == NULL || full_data_len <= 0 || substr == NULL) {  
        return NULL;  
    }  
  
    if (*substr == '\0') {  
        return NULL;  
    }  
  
    int sublen = strlen(substr);  
  
    int i;  
    char* cur = full_data;  
    int last_possible = full_data_len - sublen + 1;  
    for (i = 0; i < last_possible; i++) {  
        if (*cur == *substr) {  
            //assert(full_data_len - i >= sublen);  
            if (memcmp(cur, substr, sublen) == 0) {  
                //found  
                return cur;  
            }  
        }  
        cur++;  
    }  
  
    return NULL;  
}  

int chunkedbodyParse(struct spliceData_t *chData_t, char *input, int inLen)
{
    if(input == NULL || inLen <= 0)
    {
        return chData_t->curLen;
    }
    
    char *data_start = input;
    int data_len = inLen;

    int tempLen = 0;
    sscanf(data_start, "%x", &tempLen);
    //tempLen = (tempLen > 0 ? tempLen - 2 : tempLen);
   
    if(tempLen > 0)
    {    
        data_start = memstr(data_start, data_len, "\r\n");
        data_start += 2;        
        //data_len -= (data_start - input); 

        char *chunked_end = memstr(data_start, data_len, "\r\n");
        int chunked_len = chunked_end - data_start;

        if(chunked_len < tempLen)   //compare real len with len field value
        {
            g_worker_logger->trace("Field value: {0:d}, Real value: {1:d}", tempLen, chunked_len);
            return HTTP_BODY_DATA_LOSE;
        }
        
        char *curPos = chData_t->data + chData_t->curLen;
        memcpy(curPos, data_start, tempLen);
        chData_t->curLen += tempLen;

        data_start = memstr(data_start, data_len, "\r\n");
        data_start += 2;
        data_len -= (data_start - input);
        
        return chunkedbodyParse(chData_t, data_start, data_len);
    }
    else    //end
    {
        return chData_t->curLen;
    }
    
}

int httpContentLengthParse(struct spliceData_t *conData_t, char *input, int inLen)
{
    if(input == NULL || inLen <= 0)
    {
        return conData_t->curLen;
    }

    char *data_start = input;
    int data_len = inLen;

    char *temp = memstr(data_start, data_len, "Content-Length");

    if(temp)
    {
        temp += 15;        
        int tempLen = 0;
        sscanf(temp, "%d", &tempLen);
        
        data_start = memstr(data_start, data_len, "\r\n\r\n");

        if(data_start)
        {
            data_start += 4;
            data_len -= (data_start - input);

            if(data_len < tempLen)  //compare real len with len field value
            {
                return HTTP_BODY_DATA_LOSE;
            }

            char *curPos = conData_t->data + conData_t->curLen;
            memcpy(curPos, data_start, tempLen);
            conData_t->curLen += tempLen;
        }        
    }
    
    return conData_t->curLen;    
}

int httpChunkedParse(struct spliceData_t *chData_t, char *input, int inLen)
{
    if(input == NULL || inLen <= 0)
    {
        return chData_t->curLen;
    }

    char *data_start = input;
    int data_len = inLen;

    data_start = memstr(data_start, data_len, "\r\n\r\n");

    if(data_start)  //has HTTP header
    {
        data_start += 4;
        data_len -= (data_start - input);

        //http_body
        return chunkedbodyParse(chData_t, data_start, data_len);
        
    }
    else    //no HTTP header
    {
        //http_body
        return chunkedbodyParse(chData_t, input, inLen);
    }
    
}

int httpBodyTypeParse(char *input, int inLen)
{
    if(input == NULL || inLen <= 0)
    {
        return HTTP_UNKNOW_TYPE;
    }
    
    char *data_start = input;
    int data_len = inLen;

    if(memstr(data_start, data_len, "204 No Content"))
    {
        return HTTP_204_NO_CONTENT;
    }
    else if(memstr(data_start, data_len, "400 Bad Request"))
    {
        return HTTP_400_BAD_REQUEST;
    }
    else if(memstr(data_start, data_len, "200 OK"))
    {
        if(memstr(data_start, data_len, "Transfer-Encoding: chunked"))
        {
            return HTTP_CHUNKED;
        }
        else if(memstr(data_start, data_len, "Content-Length"))
        {
            return HTTP_CONTENT_LENGTH;
        }        
    }
    
    return   HTTP_UNKNOW_TYPE;
}

