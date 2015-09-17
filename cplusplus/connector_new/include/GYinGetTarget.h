#ifndef __GYINGETTARGET_H__
#define __GTINGETTARGET_H__
#include <iostream>
#include <string.h>
#include "GMobileAdRequestResponse.pb.h"

using namespace std;
using namespace com::rj::adsys::dsp::connector::obj::g::proto;

ContentCategory GYin_AndroidgetTargetCat(string& cat);
ContentCategory GYin_IOSgetTargetCat(string& cat);
ConnectionType GYin_getConnectionType(int id);

#endif
