#include "bidderManager.h"

using namespace com::rj::protos::manager;

bidderObject::bidderObject():m_registed(false),m_heart_times(0),c_lost_times_max(3)
{
}

bidderObject::bidderObject(const string &bidder_ip, unsigned short bidder_port):m_registed(false),m_heart_times(0),c_lost_times_max(3)
{
    set( bidder_ip, bidder_port);
}

void bidderObject::set(const string &bidder_ip, unsigned short bidder_port)
{
    m_bidderIP = bidder_ip;
    m_bidderManangerPort = bidder_port;
}

bool bidderObject::startConnectBidder(zeromqConnect& conntor, string& bc_id, struct event_base* base, event_callback_fn fn, void *arg)
{
    m_bidderManagerHander = conntor.establishConnect(true, true, bc_id, "tcp", ZMQ_DEALER, m_bidderIP.c_str(),
                                            m_bidderManangerPort, &m_bidderManangerFd);//client sub 
    m_bidderManangerEvent = event_new(base, m_bidderManangerFd, EV_READ|EV_PERSIST, fn, arg); 
    event_add(m_bidderManangerEvent, NULL);
    return true;
}

bool bidderObject::registerToBidder(string &ip, unsigned short managerPort, unsigned short dataPort)
{
    if(m_registed == false)
    {
        managerProPackage::send(m_bidderManagerHander, managerProtocol_messageTrans_BC
            ,managerProtocol_messageType_LOGIN_REQ, ip, managerPort, dataPort);
        g_manager_logger->info("[login req][bc -> bidder]:{0},{1:d},{2:d}", ip, managerPort, dataPort);
        return true;
    }
    return false;
}

string& bidderObject::get_bidder_ip()
{
    return  m_bidderIP;
}

unsigned short bidderObject::get_bidder_port()
{
    return m_bidderManangerPort;
}

int bidderObject::get_fd()
{
    return m_bidderManangerFd;
}

void *bidderObject::get_managerHandler()
{
    return m_bidderManagerHander;
}

void bidderObject::set_register_status(bool registerStatus)
{
    m_registed = registerStatus;
}

void bidderObject::heart_times_increase()
{
    m_heart_times++;
}
void bidderObject::heart_times_clear()
{
    m_heart_times=0;
}

int  bidderObject::get_heart_times()
{
    return m_heart_times;
}

bidderObject::~bidderObject()
{
    if(m_bidderManangerEvent)event_del(m_bidderManangerEvent);
    if(m_bidderManagerHander)zmq_close(m_bidderManagerHander);
    g_manager_logger->info("[erase bidder]:{0},{1:d}", m_bidderIP, m_bidderManangerPort);
}

bidderManager::bidderManager()
{
    m_bidderList_lock.init();
}

void bidderManager::init(vector<bidderConfig*>& bidderConfList)
{
    for(auto it = bidderConfList.begin(); it != bidderConfList.end(); it++)
    {
        bidderConfig *b_conf = *it;
        if(b_conf == NULL) continue;
        bidderObject *bidder = new bidderObject(b_conf->get_bidderIP(), b_conf->get_bidderLoginPort());
        m_bidderList_lock.lock();
        m_bidder_list.push_back(bidder);
        m_bidderList_lock.unlock();
    }
}

void bidderManager::startConnectToBidder(zeromqConnect& conntor, string& bc_id, struct event_base* base, event_callback_fn fn, void *arg)
{
    m_bidderList_lock.lock();
    for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)
    {
        bidderObject *bidder = *it;
        if(bidder) bidder->startConnectBidder(conntor, bc_id, base, fn, arg);
    }
    m_bidderList_lock.unlock();
}

void bidderManager::loginBidder(string &ip, unsigned short managerPort, unsigned short dataPort)
{
    m_bidderList_lock.lock();
    for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)
    {
        bidderObject *bidder = *it;
        if(bidder) bidder->registerToBidder(ip, managerPort, dataPort);
    }
    m_bidderList_lock.unlock();
}

void *bidderManager::get_bidderManagerHandler(int fd)
{
    m_bidderList_lock.lock();
    for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)
    {
        bidderObject *bidder = *it;
        if(bidder&&(bidder->get_fd() == fd))
        {
            void *handler = bidder->get_managerHandler();
            m_bidderList_lock.unlock();
            return handler;
        }
    }
    m_bidderList_lock.unlock();
    return NULL;
}

void bidderManager::register_sucess(const string &bidderIP, unsigned short bidderPort)
{
    m_bidderList_lock.lock();
    for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)
    {
        bidderObject *bidder = *it;
        if(bidder&&(bidder->get_bidder_ip()== bidderIP)&&(bidder->get_bidder_port() == bidderPort))
        {
            bidder->set_register_status(true);
            m_bidderList_lock.unlock();
            return;
        }
    }
    m_bidderList_lock.unlock();
}
bool bidderManager::devDrop(string& bcID, vector<string>& unSubList)
{  
    bool ret = false;
    ostringstream os;
    m_bidderList_lock.lock();
    for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)   
    {
        bidderObject *bidder = *it;
        if(bidder)
        {
            bool result = bidder->devDrop(bcID, unSubList);
            if(result == true) ret = true;
        }
    }
    m_bidderList_lock.unlock();
    return ret;
}

void bidderManager::update_bidder(const vector<bidderConfig*>& bidderConfigList)
{
    m_bidderList_lock.lock();
    for(auto it = m_bidder_list.begin(); it != m_bidder_list.end();)
    {
        bidderObject *bidder = *it;
        if(bidder)
        {
            auto itor = bidderConfigList.begin();
            for(itor  = bidderConfigList.begin(); itor != bidderConfigList.end(); itor++)
            {
                bidderConfig * bConf = *itor;
                if(bConf == NULL) continue;
                if((bConf->get_bidderIP()==bidder->get_bidder_ip())
                    &&(bConf->get_bidderLoginPort() == bidder->get_bidder_port()))
                    break;
            }       

            if(itor == bidderConfigList.end())
            {
                delete bidder;
                it = m_bidder_list.erase(it);
            }
            else
            {
                it++;
            }
        }
        else
        {
            it++;
        }
    }
    m_bidderList_lock.unlock();
}


bool bidderManager::recv_heart_from_bidder(const string& bidderIP, unsigned short bidderPort)
{
    m_bidderList_lock.lock();
    for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)   
    {
        bidderObject *bidder = *it;
        if(bidder&&(bidder->get_bidder_ip() == bidderIP)&&(bidder->get_bidder_port() == bidderPort))
        {
            bidder->heart_times_clear();
            m_bidderList_lock.unlock();
            return true;    
        }
    }       
    m_bidderList_lock.unlock();
    return false;
}
bool bidderManager::add_bidder(const string& bidderIP, unsigned short bidderPort
                                    ,zeromqConnect & conntor,string & bc_id,struct event_base * base,event_callback_fn fn,void * arg)
{
    m_bidderList_lock.lock();
    for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)   
    {
        bidderObject *bidder = *it;
        if(bidder == NULL ) continue;
        if((bidder->get_bidder_ip() == bidderIP)&&(bidder->get_bidder_port() == bidderPort))
        {
            bidder->init_bidder();
            m_bidderList_lock.unlock();
            g_manager_logger->info("[add a bidder]:{0},{1:d}", bidderIP, bidderPort);
            return false;
        }
    }   
    bidderObject *bidder = new bidderObject(bidderIP, bidderPort);
    bidder->startConnectBidder(conntor, bc_id, base, fn, arg);
    m_bidder_list.push_back(bidder);
    m_bidderList_lock.unlock();
    g_manager_logger->info("[add a bidder]:{0},{1:d}", bidderIP, bidderPort);
    return true;
}



bidderManager::~bidderManager()
{
    m_bidderList_lock.lock();
    for(auto it = m_bidder_list.begin(); it != m_bidder_list.end();)
    {
        delete *it;
        it = m_bidder_list.erase(it);
    }
    m_bidderList_lock.unlock();
    m_bidderList_lock.destroy();
}

