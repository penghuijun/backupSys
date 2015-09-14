#ifndef __URLCODE_H__
#define __URLCODE_H__

#include <iostream>
#include <string.h>
#include <assert.h>

using namespace std;

unsigned char ToHex(unsigned char x);
unsigned char FromHex(unsigned char x);
string UrlEncode(const std::string& str);
string UrlDecode(const std::string& str);
char *mystrstr(const char*s1,const char*s2);

void ReplaceStr(char *sDest,const char *sSrc,const char *sMatchStr, const char *sReplaceStr);


#endif



