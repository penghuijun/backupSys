package com.rj.elastic.bidcollector.uuid;

import com.rh.elastic.common.inf.IRhLog;
import com.rh.elastic.common.util.RhLogFactory;
import com.rj.elastic.bidcollector.beans.BiddingUUID;
import com.rj.elastic.bidcollector.beans.BiddingUUID.UUIDStatus;
import com.rj.elastic.datasource.redis.conn.IRedisMessageHandler;

public class ADUUIDHandler implements IRedisMessageHandler {
	private static IRhLog log = RhLogFactory.getLog(ADUUIDHandler.class);
	
	@Override
	public void handleMessage(String channel, byte[] message) {
		log.info("handleMessage: "+new String(message));
		//TODO to put into UUID State pool
		
		String uuid = new String(message);
		BiddingUUID uuidObj = new BiddingUUID();
		uuidObj.setUuid(uuid);
		uuidObj.setStatus(UUIDStatus.WAITTING);
		/*IRjElasticCacheService cacheService = (IRjElasticCacheService)BundleUtil.waitingService(Activator.getContext(),IRjElasticCacheService.class);
		if(!cacheService.contains(uuid)){
			cacheService.keepOnCache(uuid, uuidObj);
		}*/
		
	}

}
