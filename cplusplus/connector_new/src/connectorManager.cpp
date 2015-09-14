#include "connectorManager.h"
#include "connector.h"

void connectorManager::init(connectorInformation & connectorInfo)
{
    try
    {
        vector<connectorConfig*>& connectorConfigList = connectorInfo.get_connectorConfigList();
        vector<string> ipAddrList;
        ipAddress::get_local_ipaddr(ipAddrList);
        bool estatblish = false;
        for(auto it = ipAddrList.begin(); it != ipAddrList.end(); it++)
        {
            string str = *it;
            for(auto et = connectorConfigList.begin(); et != connectorConfigList.end(); et++)
            {
                connectorConfig* connector = *et;
                if(connector == NULL) continue;
                string ip_str = connector->get_connectorIP();
                if(ip_str.compare(0,ip_str.size(), str)==0)
                {
                    m_connectorConfig.set(connector);      
                    m_redisIP = connectorInfo.get_redis_ip();
	                m_redisPort = connectorInfo.get_redis_port();

                    ostringstream os;
                    os.str("");
                    os<<ip_str<<":"<<connector->get_connectorManagerPort();
                    m_connectorIdentify = os.str();
                    estatblish = true;
                    break;
                }
            }
            if(estatblish) break;
        }
    }
    catch(...)
    {
        g_manager_logger->emerg("connectorManager init error");
        exit(1);
    }
}
bool connectorManager::update(connectorInformation &new_connector_info)
{
    auto old_connectorip = m_connectorConfig.get_connectorIP();
    auto old_connectorManagerPort = m_connectorConfig.get_connectorManagerPort();
    auto old_connectorWorkerNum = m_connectorConfig.get_connectorWorkerNum();
    auto new_connectorList = new_connector_info.get_connectorConfigList();

    for(auto it = new_connectorList.begin(); it != new_connectorList.end(); it++)
    {
        connectorConfig *connector = *it;
        if(connector == NULL) continue;  
        if((connector->get_connectorIP() == old_connectorip)&&(connector->get_connectorManagerPort() == old_connectorManagerPort))
        {
             if(connector->get_connectorWorkerNum()!= old_connectorWorkerNum)
             {
                 m_connectorConfig.set_connectorWorkerNum(connector->get_connectorWorkerNum());
                 return true;
             }
        }
        else
        {
             g_manager_logger->error("Not implemented connector address update");
        }
    }
    return false;
}



