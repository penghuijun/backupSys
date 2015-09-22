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
    for(;(p=strchr(p,*s2))!=0;p++) //strchr函数原型：extern char *strchr(const char *s,char c);查找字符串s中首次出现字符c的位置。
    {
        if(strncmp(p,s2,len)==0)
            return (char*)p;
    }
    return(0);
}

char *ReplaceStr(const char *sSrc,const char *sMatchStr, const char *sReplaceStr)
{
	if((sSrc == NULL)||(sMatchStr == NULL)||(sReplaceStr == NULL))
		return NULL;
	char *pos = strstr((char *)sSrc,sMatchStr); //first pos sMatchStr appear in sSrc
	if(pos == NULL) //not found
		return NULL;
	int len = strlen(sSrc)+strlen(sReplaceStr)-strlen(sMatchStr);
	char *destStr = new char[len];
	memset(destStr, 0, len*sizeof(char));
	strncpy(destStr,sSrc,pos-sSrc);
	strcat(destStr,sReplaceStr);
	pos += strlen(sMatchStr);
	strcat(destStr,pos);
	return destStr; 
}




