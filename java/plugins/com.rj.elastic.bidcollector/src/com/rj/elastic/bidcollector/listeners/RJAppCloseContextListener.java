package com.rj.elastic.bidcollector.listeners;

import org.eclipse.gemini.blueprint.context.event.OsgiBundleApplicationContextListener;
import org.eclipse.gemini.blueprint.context.event.OsgiBundleContextClosedEvent;

import com.rh.elastic.common.inf.IRhLog;
import com.rh.elastic.common.util.RhLogFactory;


/**
 * this listener is for close event of Spring DM
 * detail see :http://www.eclipse.org/gemini/blueprint/documentation/reference/1.0.2.RELEASE/html/app-deploy.html#app-deploy:extender-configuration:events
 * @author test
 *
 */
public class RJAppCloseContextListener implements OsgiBundleApplicationContextListener<OsgiBundleContextClosedEvent>{
	private static IRhLog log = RhLogFactory.getLog(RJAppCloseContextListener.class);
	@Override
	public void onOsgiApplicationEvent(OsgiBundleContextClosedEvent event) {
		//do what you want after the applicationContext is loaded completely
		log.info(RJAppCloseContextListener.class.getName()+" is triggered!");
	}

}
