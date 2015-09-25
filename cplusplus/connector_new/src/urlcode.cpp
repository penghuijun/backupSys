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

#if 0
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
#endif

char *ReplaceStr(const char *sSrc,const char *sMatchStr, const char *sReplaceStr)
{
    if((sSrc == NULL)||(sMatchStr == NULL)||(sReplaceStr == NULL))
        return (char *)sSrc;

    int count = 0;
    char *pos = strstr((char *)sSrc,sMatchStr);

    while(pos)
    {
        count++;
        pos += strlen(sMatchStr);
        pos = strstr(pos,sMatchStr);
    }

    int len = strlen(sSrc)+count*(strlen(sReplaceStr)-strlen(sMatchStr));
    char *destStr = new char[len];
    memset(destStr, 0, len*sizeof(char));

    if(count == 0)
        strcpy(destStr, sSrc);
    else
    {
        char *next = (char *)sSrc;
        while((pos = strstr(next, sMatchStr)) != NULL)
        {
            strncat(destStr, next, pos-next);
            strcat(destStr, sReplaceStr);
            next = pos + strlen(sMatchStr);
        }
    }
    return destStr;
}

/**
  * like "12" -> "21"  :  "21212" -> "22211"
  */
string&   replace_all(string&   str,const   string&   old_value,const   string&   new_value)   
{   
    while(true)   {   
        string::size_type   pos(0);   
        if(   (pos=str.find(old_value))!=string::npos   )   
            str.replace(pos,old_value.length(),new_value);   
        else   break;   
    }   
    return   str;   
}   

/**
  * like "12" -> "21"  :  "21212" -> "22121"
  */

string&   replace(string&   str,const   string&   old_value,const   string&   new_value)   
{   
    for(string::size_type   pos(0);   pos!=string::npos;   pos+=new_value.length())   {   
        if(   (pos=str.find(old_value,pos))!=string::npos   )   
            str.replace(pos,old_value.length(),new_value);   
        else   break;   
    }   
    return   str;   
}   






