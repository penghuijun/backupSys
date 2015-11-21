#include "connectorManager.h"
#include "login.h"
using namespace com::rj::protos::manager;

connectorObject::connectorObject()
{
}

connectorObject::connectorObject(const string &ip, unsigned short port):m_registed(false),m_heart_times(0)
{
    set( ip, port);
}

void connectorObject::set(const string &ip, unsigned short port)
{
    m_ip = ip;
    m_manangerPort = port;
}

bool connectorObject::connect(zeromqConnect& conntor, string& bc_id, struct event_base* base, event_callback_fn fn, void *arg)
{
    m_managerHander = conntor.establishConnect(true, true, bc_id, "tcp", ZMQ_DEALER, m_ip.c_str(),
                                            m_manangerPort, &m_manangerFd);//client sub 
    m_manangerEvent = event_new(base, m_manangerFd, EV_READ|EV_PERSIST, fn, arg); 
    event_add(m_manangerEvent, NULL);
    return true;
}

bool connectorObject::login(string &ip, unsigned short managerPort, unsigned short dataPort)
{
    if(m_registed == false)
    {
        managerProPackage::send(m_managerHander, managerProtocol_messageTrans_BC
            ,managerProtocol_messageType_LOGIN_REQ, ip, managerPort, dataPort);
        g_manager_logger->info("[login req][bc -> connector]:{0},{1:d},{2:d}", m_ip, m_manangerPort, dataPort);
        return true;
    }
    return false;
}


const string& connectorObject::get_connector_ip() const
{
    return  m_ip;
}

unsigned short connectorObject::get_connector_port() const
{
    return m_manangerPort;
}

int connectorObject::get_fd()
{
    return m_manangerFd;
}

void *connectorObject::get_managerHandler()
{
    return m_managerHander;
}

void connectorObject::set_register_status(bool registerStatus)
{
    m_registed = registerStatus;
}

void connectorObject::heart_times_increase()
{
    m_heart_times++;
}
void connectorObject::heart_times_clear()
{
    m_heart_times=0;
}

int  connectorObject::get_heart_times()
{
    return m_heart_times;
}

connectorObject::~connectorObject()
{
    if(m_manangerEvent)event_del(m_manangerEvent);
    if(m_managerHander)zmq_close(m_managerHander);
    g_manager_logger->info("[erase connector]:{0},{1:d}", m_ip, m_manangerPort);
}

connectorManager::connectorManager():c_lost_times_max(3)
{
    m_manager_lock.init();
}

void connectorManager::init(vector<connectorConfig*>& confList)
{
    for(auto it = confList.begin(); it != confList.end(); it++)
    {
        connectorConfig *conf = *it;
        if(conf == NULL) continue;
        connectorObject *cc = new connectorObject(conf->get_connectorIP(), conf->get_connectorManagerPort());
        m_manager_lock.lock();
        m_connector_list.push_back(cc);
        m_manager_lock.unlock();
    }
}

void connectorManager::connect(zeromqConnect& conntor, string& bc_id, struct event_base* base, event_callback_fn fn, void *arg)
{
    m_manager_lock.lock();
    for(auto it = m_connector_list.begin(); it != m_connector_list.end(); it++)
    {
        connectorObject *connector = *it;
        if(connector) connector->connect(conntor, bc_id, base, fn, arg);
    }
    m_manager_lock.unlock();
}

void connectorManager::login(string &ip, unsigned short managerPort, unsigned short dataPort)
{
    m_manager_lock.lock();
    for(auto it = m_connector_list.begin(); it != m_connector_list.end(); it++)
    {
        connectorObject *connector = *it;
        if(connector)
        {
            connector->login(ip, managerPort, dataPort);
        }
    }
    m_manager_lock.unlock();
}



void *connectorManager::get_managerHandler(int fd)
{
    m_manager_lock.lock();
    for(auto it = m_connector_list.begin(); it != m_connector_list.end(); it++)
    {
        connectorObject *connector = *it;
        if(connector&&(connector->get_fd() == fd))
        {
            void *handler = connector->get_managerHandler();
            m_manager_lock.unlock();
            return handler;
        }
    }
    m_manager_lock.unlock();
    return NULL;
}

void connectorManager::register_sucess(const string &ip, unsigned short port)
{
    m_manager_lock.lock();
    for(auto it = m_connector_list.begin(); it != m_connector_list.end(); it++)
    {
        connectorObject *connector = *it;
        if(connector&&(connector->get_connector_ip()== ip)&&(connector->get_connector_port()== port))
        {
            connector->set_register_status(true);
            m_manager_lock.unlock();
            return;
        }
    }
    m_manager_lock.unlock();
}
bool connectorManager::recv_heart_time_increase(string& bcID, vector<string>& unSubList)
{  
    bool ret = true;
    ostringstream os;
    m_manager_lock.lock();
    for(auto it = m_connector_list.begin(); it != m_connector_list.end(); it++)   
    {
        connectorObject *connector = *it;
        if(connector&&(connector->get_register_status()==true))
        {
            connector->heart_times_increase();
            if(connector->get_heart_times()>c_lost_times_max)
            {
                connector->set_register_status(false);
                os.str("");
                os<<bcID<<"-"<<connector->get_connector_ip()<<":"<<connector->get_connector_port();
                string unsub = os.str();
                unSubList.push_back(unsub);
                ret = false;
            }
        }
    }
    m_manager_lock.unlock();
    return ret;
}

void connectorManager::update(const vector<connectorConfig*>& configList)
{
    m_manager_lock.lock();
    for(auto it = m_connector_list.begin(); it != m_connector_list.end();)
    {
        connectorObject *connector = *it;
        if(connector)
        {
            auto itor = configList.begin();
            for(itor  = configList.begin(); itor != configList.end(); itor++)
            {
                connectorConfig* bConf = *itor;
                if(bConf == NULL) continue;
                if((bConf->get_connectorIP() == connector->get_connector_ip())
                    &&(bConf->get_connectorManagerPort() == connector->get_connector_port()))
                    break;
            }       

            if(itor == configList.end())
            {
                delete connector;
                it = m_connector_list.erase(it);
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
    m_manager_lock.unlock();
}

bool connectorManager::recv_heart(const string& ip, unsigned short port)
{
    m_manager_lock.lock();
    for(auto it = m_connector_list.begin(); it != m_connector_list.end(); it++)   
    {
        connectorObject *connector = *it;
        if(connector&&(connector->get_connector_ip()== ip)&&(connector->get_connector_port()== port))
        {
            connector->heart_times_clear();
            m_manager_lock.unlock();
            return true;
        }
    }       
    m_manager_lock.unlock();
    return false;
}

bool connectorManager::add_connector(const string& ip, unsigned short port,zeromqConnect& zmqconn
                                ,string & bc_id,struct event_base * base,event_callback_fn fn,void * arg)
{
    m_manager_lock.lock();
    for(auto it = m_connector_list.begin(); it != m_connector_list.end(); it++)   
    {
        connectorObject *connector = *it;
        if(connector == NULL ) continue;
        if((connector->get_connector_ip() == ip)&&(connector->get_connector_port() == port))
        {
            connector->init_connector();
            m_manager_lock.unlock();
            return false;
        }
    }   

    g_manager_logger->info("[add a connector]:{0},{1:d}", ip, port);
    connectorObject *connector = new connectorObject(ip, port);
    connector->connect(zmqconn, bc_id, base, fn, arg);
    m_connector_list.push_back(connector);
    m_manager_lock.unlock();
    return true;
}



connectorManager::~connectorManager()
{
    m_manager_lock.lock();
    for(auto it = m_connector_list.begin(); it != m_connector_list.end();)
    {
        delete *it;
        it = m_connector_list.erase(it);
    }
    m_manager_lock.unlock();
    m_manager_lock.destroy();
}

