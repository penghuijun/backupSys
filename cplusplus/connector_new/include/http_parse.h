#ifndef __HTTP_PARSE_H__
#define __HTTP_PARSE_H__

#include <iostream>
#include <stdio.h>
#include <string.h>
#include "spdlog/spdlog.h"

using namespace std;

struct chunkedData_t
{
	char *data;	
	int curLen;
};
char* memstr(char* full_data, int full_data_len, const char* substr);
int chunkedbodyParse(struct chunkedData_t *chData_t, char *input, int inLen);
int httpChunkedParse(struct chunkedData_t *chData_t, char *data, int dataLen);

#endif
