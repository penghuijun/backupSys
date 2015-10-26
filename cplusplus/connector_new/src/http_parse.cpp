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

int chunkedbodyParse(struct chunkedData_t *chData_t, char *input, int inLen)
{
    if(input == NULL || inLen <= 0)
    {
        return chData_t->curLen;
    }

    g_worker_logger->debug("\r\n{0}", input);

    char *data_start = input;
    int data_len = inLen;

    int tempLen = 0;
    sscanf(data_start, "%x", &tempLen);
    tempLen = (tempLen > 0 ? tempLen - 2 : tempLen);

    if(tempLen > 0)
    {
        data_start = memstr(data_start, data_len, "\r\n");
        data_start += 2;        
        
        char *curPos = chData_t->data + chData_t->curLen;
        memcpy(curPos, data_start, tempLen);
        chData_t->curLen += tempLen;

        data_start = memstr(data_start, data_len, "\r\n");
        data_start += 2;
        data_len -= (data_start - input);
        
        chunkedbodyParse(chData_t, data_start, data_len);
    }
    else    //end
    {
        return chData_t->curLen;
    }
    
}

int httpChunkedParse(struct chunkedData_t *chData_t, char *input, int inLen)
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

