package com.rj.elastic.bidcollector.listeners;

import java.util.concurrent.atomic.AtomicLong;

import org.eclipse.gemini.blueprint.context.event.OsgiBundleApplicationContextListener;
import org.eclipse.gemini.blueprint.context.event.OsgiBundleContextRefreshedEvent;

import com.rh.elastic.common.inf.IRhLog;
import com.rh.elastic.common.util.RhLogFactory;

/**
 * this listener is for refresh event of Spring DM
 * detail see :http://www.eclipse.org/gemini/blueprint/documentation/reference/1.0.2.RELEASE/html/app-deploy.html#app-deploy:extender-configuration:events
 * @author test
 *
 */
public class RJAppRefreshContextListener implements OsgiBundleApplicationContextListener<OsgiBundleContextRefreshedEvent>{
	private static IRhLog log = RhLogFactory.getLog(RJAppRefreshContextListener.class);
	private static AtomicLong flag = new AtomicLong(0);
	
	@Override
	public void onOsgiApplicationEvent(OsgiBundleContextRefreshedEvent event) {
		log.info(RJAppRefreshContextListener.class.getName()+" is triggered!");
		//do what you want after the applicationContext is loaded completely
		if(flag.get() == 0){
			//RedisHelper.testRedisService();
			flag.incrementAndGet();
		}
	}

}
