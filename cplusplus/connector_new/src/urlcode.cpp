#include "urlcode.h"

unsigned char ToHex(unsigned char x) 
{ 
    return  x > 9 ? x + 55 : x + 48; 
}

unsigned char FromHex(unsigned char x) 
{ 
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else assert(0);
    return y;
}

string UrlEncode(const string& str)
{
    string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) || 
            (str[i] == '-') ||
            (str[i] == '_') || 
            (str[i] == '.') || 
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}

string UrlDecode(const string& str)
{
    string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+') strTemp += ' ';
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high*16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}
char *mystrstr(const char*s1,const char*s2)
{
    const char*p=s1;
    const size_t len=strlen(s2);
    for(;(p=strchr(p,*s2))!=0;p++)
    {
        if(strncmp(p,s2,len)==0)
            return (char*)p;
    }
    return(0);
}


// Ìæ»»×Ö·û´®ÖÐÌØÕ÷×Ö·û´®ÎªÖ¸¶¨×Ö·û´®
void ReplaceStr(char *sDest,const char *sSrc,const char *sMatchStr, const char *sReplaceStr)
{
        int  StringLen;
        int srcLen = strlen(sSrc);
        int replaceLen = strlen(sReplaceStr);
        int caNewStringLen = srcLen + replaceLen;
        char caNewString[caNewStringLen];          

        char *FindPos = mystrstr(sSrc, sMatchStr);
        if( (!FindPos) || (!sMatchStr) )
                return;

        while( FindPos )
        {
                memset(caNewString, 0, sizeof(caNewString));
                StringLen = FindPos - sSrc;
                strncpy(caNewString, sSrc, StringLen);
                strcat(caNewString, sReplaceStr);
                strcat(caNewString, FindPos + strlen(sMatchStr));
                strcpy(sDest, caNewString);

                FindPos = strstr(sDest, sMatchStr);
        }        
}



