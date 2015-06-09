#ifndef __ALGORITHM_COMM_H__
#define __ALGORITHM_COMM_H__
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;
class BasicAlgorithm
{
	bool is_number(string& str)
	{
		const char *id_str = str.c_str();
		size_t id_size = str.size();
		if(id_size==0) return false;
		for(int i=0; i < id_size; i++)
		{
			char ch = *(id_str+i);
			if(ch<'0'||ch>'9') return false;
		}
		return true;
	}
};

class bracesBlock
{
};
#endif