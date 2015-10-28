#ifndef __HTTP_PARSE_H__
#define __HTTP_PARSE_H__

#include <iostream>
#include <stdio.h>
#include <string.h>
#include "spdlog/spdlog.h"

using namespace std;

#define HTTP_200_OK				0
#define HTTP_204_NO_CONTENT	1
#define HTTP_CONTENT_LENGTH	2
#define HTTP_CHUNKED			3
#define HTTP_UNKNOW_TYPE		9999
#define HTTP_BODY_DATA_LOSE	-1

#define RECV_COMPLETE			0
#define RECV_INCOMPLETE			1


struct spliceData_t
{
	char *data;	
	int curLen;
};

char* memstr(char* full_data, int full_data_len, const char* substr);
int chunkedbodyParse(struct spliceData_t *chData_t, char *input, int inLen);
int httpContentLengthParse(struct spliceData_t *conData_t, char *input, int inLen);
int httpChunkedParse(struct spliceData_t *chData_t, char *input, int inLen);
int httpBodyTypeParse(char *input, int inLen);

#endif
