#ifndef __GEO_INIT_H__
#define __GEO_INIT_H__
#include <map>
#include <iostream>
#include <string>

using namespace std;


class geo_country
{
public:

private:
	unsigned int m_country_id;
	string m_country_name;
	string m_country_code;
};


class geo_region
{
public:

private:
	unsigned int m_region_id;
	unsigned int m_country_id;
	string m_country_code;
	string m_country_name;
	string m_region_name;
};


class geo_city
{
public:

private:
	unsigned int m_city_id;
	unsigned int m_country_id;
	unsigned int m_region_id;
	string m_country_code;
	string m_country_name;
	string m_region_name;
	string m_city_name;
};


class geo_map
{
public:

private:
	map<unsigned int, string> m_country_list;
	map<unsigned int, string> m_region_list;
	map<unsigned int, string> m_city_list;	
};
#endif
