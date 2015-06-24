package com.rj.elastic.bidcollector.config;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Set;

import org.osgi.framework.BundleContext;
import org.springframework.beans.factory.config.PropertyPlaceholderConfigurer;

import com.rh.elastic.common.inf.IRhLog;
import com.rh.elastic.common.util.BundleUtil;
import com.rh.elastic.common.util.RJFileUtil;
import com.rh.elastic.common.util.RhLogFactory;
import com.rh.elastic.common.util.RhUtil;
import com.rj.elastic.bidcollector.Activator;

/**
 * it can load configuration file from outside of bundle jar rather than the original PropertyPlaceholderConfigurer
 * @author Francis.Hu
 * @since 2013 07 09
 *
 */
public class CollectorConfigParser extends PropertyPlaceholderConfigurer  { //implements BundleContextAware  
	private static IRhLog log = RhLogFactory.getLog(CollectorConfigParser.class);
	
	private static BundleContext context;
	private static Properties properties = new Properties();
	private final static String CONFIG_PROPERTIES_FILE = "config.properties";
	public final static String K_REDIS_SCRIBER_QUEUE = "redis.uuid.scriber.queue";
	/*msg.router.rev.queue=Bindding.Signal
			msg.router.rev.host=192.168.219.200
			msg.router.rev.port=5001*/
	public final static String K_MSG_ROUTER_REV_QUEUE = "msg.router.rev.queue";
	public final static String K_MSG_ROUTER_REV_HOST = "msg.router.rev.host";
	public final static String K_MSG_ROUTER_REV_PORT = "msg.router.rev.port";

	private final static String USER_DIR = "user.dir"; 
	
	
	
	public static void initConfig(BundleContext con) throws Exception{
		log.info("initializing core config....");
		context = con;
		
		InputStream s = null;
		try{
			File confF = new File(BundleUtil.getBundleConfigFilePath(context, CONFIG_PROPERTIES_FILE));
			
			
			RJFileUtil.assertFileExists(new File[]{confF});
			
			s = new FileInputStream(confF);
			properties.load(s);
		}finally{
			if(s != null) s.close();
		}
	}
	
	public static String getProperty(String key){
		return properties.getProperty(key);
	}
	
	public static String getProperty(String key,String defaultValue){
		return properties.getProperty(key,defaultValue);
	}
	
	public static List<String> getProperties(String keyStartWith){
		List<String> values = new ArrayList<String>();
		Set keys = properties.keySet();
		for(Object key : keys){
			String keyStr = (String)key;
			if(keyStr.startsWith(keyStartWith)){
				String value = properties.getProperty(keyStr);
				if(!RhUtil.isNullOrBlank(value)){
					values.add(value);
				}
			}
		}
		return values;
	}
	
	public static Map<String,String> getPropertiesMap(String keyStartWith){
		Map<String,String> kvs = new HashMap<String,String>();
		Set keys = properties.keySet();
		for(Object key : keys){
			String keyStr = (String)key;
			if(keyStr.startsWith(keyStartWith)){
				String value = properties.getProperty(keyStr);
				if(!RhUtil.isNullOrBlank(value)){
					kvs.put(keyStr, value);
				}
				
			}
		}
		return kvs;
	}
	
	public static boolean getBooleanProperty(String key){
		return Boolean.valueOf(properties.getProperty(key));
	}
	public static boolean getBooleanProperty(String key,boolean defaultValue){
		boolean value = defaultValue;
		if(!RhUtil.isNullOrBlank(properties.getProperty(key))){
			value = Boolean.valueOf(properties.getProperty(key));
		}
		return value;
	}
	public static int getIntProperty(String key,int defaultValue){
		int returnV = defaultValue;
		if(!RhUtil.isNullOrBlank(properties.getProperty(key))){
			returnV = Integer.valueOf(properties.getProperty(key));
		}
		return returnV;
	}
	public static long getLongProperty(String key,long defaultValue){
		long returnV = defaultValue;
		if(!RhUtil.isNullOrBlank(properties.getProperty(key))){
			returnV = Integer.valueOf(properties.getProperty(key));
		}
		return returnV;
	}

	

	@Override
	protected Properties mergeProperties() throws IOException {
		properties = super.mergeProperties();
		try {
			initConfig(Activator.getContext());
		} catch (Exception e) {
			throw new IOException(e);
		}
		/*Set keys = properties.keySet();
		for(Object o : keys){
			System.out.println("key="+o+"\tvalue="+properties.getProperty((String)o));
		}*/
		return properties;
	}
	
	public static String getFelixHome(){
		return System.getProperty(USER_DIR);
	}
	
}
