package com.rj.elastic.bidcollector.receivers;

import com.rh.elastic.common.inf.IRhLog;
import com.rh.elastic.common.util.RhLogFactory;
import com.rh.elastic.common.util.RhUtil;
import com.rj.elastic.bidcollector.config.CollectorConfigParser;
import com.rj.elastic.bidcollector.helpers.RedisHelper;
import com.rj.elastic.datasource.redis.conn.IRedisMessageHandler;

public class UUIDReceiver extends Thread{
	private static IRhLog log = RhLogFactory.getLog(UUIDReceiver.class);
	
	private IRedisMessageHandler handler;
	
	public UUIDReceiver(){
		
	}
	
	public void receive() throws Exception{
		String channels = CollectorConfigParser.getProperty(CollectorConfigParser.K_REDIS_SCRIBER_QUEUE, "");
		if(RhUtil.isNullOrBlank(channels)){
			throw new IllegalArgumentException("queue is blank or null in config.properties");
		}
		log.info("listening to redis queue..."+channels);
		RedisHelper.getConnection().subscribeDefault(handler, channels.getBytes());
	}

	@Override
	public void run() {
		try {
			receive();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public IRedisMessageHandler getHandler() {
		return handler;
	}

	public void setHandler(IRedisMessageHandler handler) {
		this.handler = handler;
	}
}
