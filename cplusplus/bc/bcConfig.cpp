
#include"bcConfig.h"
using namespace std;

bcConfig::bcConfig(string configTxt):m_configTxt(configTxt)
{ 
    try
    {
        m_infile.open(m_configTxt, ios::in);
        if(m_infile.is_open() == false)
        {
            cout << "open bcConfig.txt failure!!!" << endl;
            exit(1);
        }
        cout << "open bcConfig.txt success!!!" << endl;
        
        string bbuf;
        while(getline(m_infile, bbuf))
        {
           // cout << bbuf << endl;
      
            static int index = 0;
            static int nextI=0;
            int symInd;
            string subS;
            
            if(bbuf.empty() || bbuf.at(0) == '#' || bbuf.at(0) == ' ') continue;
            if(bbuf.find('$') != string::npos)
            {
                if((symInd = bbuf.find('=')) != string::npos)
                {
                    subS = bbuf.substr(++symInd);
                    index = atoi(subS.c_str());
                }
                nextI = 0;
                continue;
            }
        
            switch(index)
            {

                case 1:
                    if(nextI==0)
                    {
                        if( bbuf.compare(0, 8, "bcServIP")==0 &&
                            (symInd = bbuf.find('=')) != string::npos)
                        {
                            symInd++;
                            subS = bbuf.substr(symInd);
                            m_bcServIP = subS;
                            nextI++;
                            cout << "bcServIP:" << m_bcServIP << endl;
                        }
                        else
                        {
                            throw -1;
                        }
                    }
                    else if(nextI==1)
                    {
                        if( bbuf.compare(0, 13, "bcServBidPort")==0 &&
                            (symInd = bbuf.find('=')) != string::npos)
                        {
                            symInd++;
                            subS = bbuf.substr(symInd);
                            m_bcListenBidPort = atoi(subS.c_str());
                            nextI++;
                            cout << "bcListenBidPort:" <<  m_bcListenBidPort<< endl;   
                        }    
                        else
                        {
                            throw -1;
                        }
                    }
                    else if(nextI==2)
                    {
                        if( bbuf.compare(0, 14, "bcServVastPort")==0 &&
                            (symInd = bbuf.find('=')) != string::npos)
                        {
                            symInd++;
                            subS = bbuf.substr(symInd);
                            m_bcListenExpirePort = atoi(subS.c_str());
                            nextI++;
                            cout << "bcListenExpirePort:" <<  m_bcListenExpirePort<< endl;
                        } 
                        else
                        {
                            throw -1;
                        }
                    }
                    else
                    {
                       
                    }	
                    break;
                case 2:
                    if(nextI==0)
                    {
                        if(bbuf.compare(0, 15, "redisSaveServIP")==0 &&
                            (symInd = bbuf.find('=')) != string::npos)
                        {
                            symInd++;
                            subS = bbuf.substr(symInd);
                            m_redisSaveIP = subS;
                            nextI++;
                            cout << "redisSaveIP:" << m_redisSaveIP << endl;
                        }
                        else
                        {
                            throw -1;
                        }
                    }
                    else if(nextI==1)
                    {
                  
                        if(bbuf.compare(0, 17, "redisSaveServPort")==0 &&
                            (symInd = bbuf.find('=')) != string::npos)
                        {
                            symInd++;
                            subS = bbuf.substr(symInd);
                            m_redisSavePort = atoi(subS.c_str());
                            nextI++;
                            cout << "redisSavePort:" <<  m_redisSavePort<< endl;
                            
                        } 
                        else
                        {
                            throw -1;
                        }
                    }
                    else if(nextI==2)
                    {
                  
                        if(bbuf.compare(0, 17, "redisSaveServTime")==0 &&
                            (symInd = bbuf.find('=')) != string::npos)
                        {
                            symInd++;
                            subS = bbuf.substr(symInd);
                            m_redisSaveTime= atoi(subS.c_str());
                            nextI++;
                            cout << "redisSaveTime:" <<  m_redisSaveTime<< endl;   
                        }    
                        else
                        {
                            throw -1;
                        }
                    }
                    else
                    {
                       
                    }
                    break;
                case 3:
                    if(nextI==0)
                    {
                        if(bbuf.compare(0, 14, "redisPublishIP")==0 &&
                            (symInd = bbuf.find('=')) != string::npos)
                        {
                            symInd++;
                            subS = bbuf.substr(symInd);
                            m_redisPublishIP = subS;
                            nextI++;
                            cout << "redisPublishIP:" << m_redisPublishIP << endl;
                        }
                        else
                        {
                            throw -1;
                        }
                    }
                    else if(nextI==1)
                    {
                  
                        if(bbuf.compare(0, 16, "redisPublishPort")==0 &&
                            (symInd = bbuf.find('=')) != string::npos)
                        {
                            symInd++;
                            subS = bbuf.substr(symInd);
                            m_redisPublisPort = atoi(subS.c_str());
                            nextI++;
                            cout << "redisPublisPort:" <<  m_redisPublisPort<< endl;
                            
                        } 
                        else
                        {
                            throw -1;
                        }
                    }
                    else if(nextI==2)
                    {
                        if(bbuf.compare(0, 15, "redisPublishKey")==0 &&
                            (symInd = bbuf.find('=')) != string::npos)
                        {
                            symInd++;
                            subS = bbuf.substr(symInd);
                            m_redisPublishKey = subS;
                            nextI++;
                            cout << "redisPublishKey:" << m_redisPublishKey << endl;
                        }
                        else
                        {
                            throw -1;
                        }
                    }
                    else
                    {
                       
                    }
                    break;
                default: 
                    break;
            }			
    	}
    }
    catch(...)
    {
        cout << get_configFlieName() <<":some err ocur" << endl;
        m_infile.close();
        exit(1);
    }
}

void bcConfig::display() const
{
    cout << "config file name:" << get_configFlieName()<< endl;
    cout << "bc serv IP:" << get_bcServIP()<< endl;
    cout << "bc listen bidder port:" << get_bcListenBidPort()<< endl;
    cout << "bc listen expire port:" << get_bcListenExpirePort()<< endl;
    cout << "save redis ip:" << get_redisSaveIP()<< endl;
    cout << "save redis port:" << get_redisSavePort()<< endl;
    cout << "save redis time:" << get_redisSaveTime() << endl;
    cout << "publish redis ip:" << get_redisPublishIP()<< endl;
    cout << "publish redis port:"<<get_redisPublishPort()<< endl;
    cout << "publish key:" << get_redisPublishKey()<< endl;
}