package com.rj.elastic.bidcollector.helpers;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.rh.elastic.common.inf.IRhLog;
import com.rh.elastic.common.util.BundleUtil;
import com.rh.elastic.common.util.RhLogFactory;
import com.rh.elastic.common.util.RhUtil;
import com.rj.elastic.bidcollector.Activator;
import com.rj.elastic.datasource.redis.conn.Connection;
import com.rj.elastic.datasource.redis.conn.inf.IRedisConnectionFactoryService;
import com.rj.elastic.datasource.redis.conn.inf.IRedisMessageListener;


/**
 * this is a helper class for redis
 * @author francis.Hu
 * @since 2013-11-22
 *
 */
public class RedisHelper {
	private static IRhLog log = RhLogFactory.getLog(RedisHelper.class);
	/**
	 * a test method 
	 */
	public static void testRedisService(){
		try {
			IRedisConnectionFactoryService connFactory = (IRedisConnectionFactoryService)BundleUtil.waitingService(Activator.getContext(),IRedisConnectionFactoryService.class);
			IRedisMessageListener listener = (IRedisMessageListener)BundleUtil.waitingService(Activator.getContext(),IRedisMessageListener.class);
			
			connFactory.getPoolConnection().lpush("test.log", "test...");
			System.out.println("redis service....."+listener != null);
			System.out.println("redis Factory service....."+connFactory != null);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	public static void scribe(){
		
	}
	
	/**
	 * send indexed data into redis 
	 * @param channel
	 * @param message
	 * @throws Exception
	 */
	/*public static void sendToRedisQueue(String channel,String message) throws Exception{
		IRedisConnectionFactoryService connFactory = (IRedisConnectionFactoryService)BundleUtil.waitingService(Activator.getContext(),IRedisConnectionFactoryService.class);
		connFactory.getPoolConnection().lpush(channel, message);
	}*/
	
	/**
	 * send indexed map data into redis ,will clear all values of hash key before sending.
	 * @param key
	 * @param indexedFieldValues
	 * @throws Exception
	 */
	/*public static void sendToRedisQueue(String key,Map<String,List<String>> indexedFieldValues) throws Exception{
		Connection redCon = getConnection();
		
		//clear old indices out of redis.
		//if(redCon.checkHashDataExists(key)){
		Set<String> actions = redCon.hashFields(key);
		for(String ac : actions){
			redCon.hashDel(key, ac);
		}
		//}
		//reset all indexes
		redCon.hashMultiSet(key, convertToRedisHashMap(indexedFieldValues));
		//redCon.disconnect();
	}*/
	
	/**
	 * send indexed map data into redis ,will clear all values of hash key before sending.
	 * @param key
	 * @param indexedFieldValues
	 * @throws Exception
	 */
	public static void clearAndSendToRedisMap(Map<String,Map<String,List<String>>> indexedDatas) throws Exception{
		log.info("clearAndSendToRedisMap()...");
		Connection redCon = getConnection();
		
		Set<String> keys = indexedDatas.keySet();
		for(String key : keys){
			
			//clear old indices out of redis.
			Set<String> actions = redCon.hashFields(key);
			for(String ac : actions){
				redCon.hashDel(key, ac);
			}
			
			//reset all indexes
			redCon.hashMultiSet(key, convertToRedisHashMap(indexedDatas.get(key)));
		}
		//redCon.disconnect();
		printCacheResult(indexedDatas);
	}
	
	public static void sendToRedisMap(Map<String,Map<String,List<String>>> indexedDatas) throws Exception{
		log.info("sendToRedisMap()...");
		Connection redCon = getConnection();
		
		Set<String> keys = indexedDatas.keySet();
		for(String key : keys){
			//reset all indexes
			redCon.hashMultiSet(key, convertToRedisHashMap(indexedDatas.get(key)));
		}
		printCacheResult(indexedDatas);
	}
	
	public static void printCacheResult(Map<String,Map<String,List<String>>> indexedDatas){
		Set<String> countrys = indexedDatas.keySet();
		
		for(String cou : countrys){
			Map<String,List<String>> actionsMap = indexedDatas.get(cou);
			
			Set<String> actions = actionsMap.keySet();
			for(String ac : actions){
				List<String> camps = actionsMap.get(ac);
				String logStr = "\tindex key:"+cou+"\taction:"+ac+"\tcampaigns:";
				for(String cam : camps){
					logStr += cam+",";
				}
				System.out.println(logStr);
			}
		}
	}
	
	/**
	 * retrieve all values from the passed keys
	 * @param channels
	 * @return
	 * @throws Exception
	 */
	public static Map<String,Map<String,List<String>>> getHashFieldValues(Set<String> channels) throws Exception{
		Connection redCon = getConnection();
		Map<String,Map<String,List<String>>> workingIndexs = new HashMap<String,Map<String,List<String>>>();
		for(String channel : channels){
			if(redCon.checkHashDataExists(channel)){
				Map<String,String> actionsMap = redCon.hashGetAll(channel);
				if(actionsMap != null && !actionsMap.isEmpty()){
					workingIndexs.put(channel, convertToBusiHashMap(actionsMap));
				}
			}
		}
		
		return workingIndexs;
	}
	
	private final static String CAMPAIN_SPLITER = ",";
	
	/**
	 * convert  comma separated String values to a list 
	 * @param indexedFieldValues
	 * @return
	 */
	private static Map<String,List<String>> convertToBusiHashMap(Map<String,String> indexedFieldValues){
		Map<String,List<String>> redisHashMap = new HashMap<String,List<String>>();
		Set<String> keys = indexedFieldValues.keySet();
		for(String k : keys){
			String values = indexedFieldValues.get(k);
			String[] vs = RhUtil.split(values, CAMPAIN_SPLITER);
			if(vs != null && vs.length > 0){
				redisHashMap.put(k, new ArrayList<String>(Arrays.asList(vs)));
			}else{
				redisHashMap.put(k, new ArrayList<String>());
			}
		}
		return redisHashMap;
	}
	
	/**
	 * convert a list<String> as a standard-alone string values so that can be saved into redis
	 * @param indexedFieldValues
	 * @return
	 */
	private static Map<String,String> convertToRedisHashMap(Map<String,List<String>> indexedFieldValues){
		Map<String,String> redisHashMap = new HashMap<String,String>();
		Set<String> keys = indexedFieldValues.keySet();
		StringBuffer buf = null;
		for(String key : keys){
			buf = new StringBuffer();
			List<String> fieldValues = indexedFieldValues.get(key);
			for(String v : fieldValues){
				buf.append(v).append(CAMPAIN_SPLITER);
			}
			String vs = buf.toString();
			if(vs.endsWith(CAMPAIN_SPLITER))
				vs = vs.substring(0,vs.length()-1);
			
			redisHashMap.put(key, vs);
		}
		return redisHashMap;
	}
	
	/**
	 * get a redis connection, it will be waitting until the connection is available
	 * @return
	 * @throws Exception
	 */
	public static Connection getConnection() throws Exception{
		IRedisConnectionFactoryService connFactory = (IRedisConnectionFactoryService)BundleUtil.waitingService(Activator.getContext(),IRedisConnectionFactoryService.class);
		Connection redCon = connFactory.getPoolConnection();
		return redCon;
	}

}
