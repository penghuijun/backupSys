package com.rj.elastic.bidcollector.beans;

import java.util.concurrent.CopyOnWriteArraySet;

import com.google.protobuf.GeneratedMessage;

public class BiddingUUID {

	public enum UUIDStatus{
		WAITTING,STARTED,BIDDING,EXPIRED;
	}
	private String uuid;
	private UUIDStatus status;
	private CopyOnWriteArraySet<GeneratedMessage> bidders;
	
	public String getUuid() {
		return uuid;
	}
	public void setUuid(String uuid) {
		this.uuid = uuid;
	}
	public CopyOnWriteArraySet<GeneratedMessage> getBidders() {
		return bidders;
	}
	
	public void setBidders(CopyOnWriteArraySet<GeneratedMessage> biders) {
		this.bidders = biders;
	}
	public synchronized void add(GeneratedMessage bidder){
		bidders.add(bidder);
	}
	public UUIDStatus getStatus() {
		return status;
	}
	public synchronized void setStatus(UUIDStatus status) {
		this.status = status;
	}
	@Override
	public String toString() {
		return "BiddingUUID [uuid=" + uuid + ", status=" + status
				+ ", bidders=" + bidders + "]";
	}
	
	@Override
	public boolean equals(Object obj){
		if(obj == null){
			return false;
		}
		if(this == obj){
			return true;
		}
		if(this.getClass() == obj.getClass()){
			BiddingUUID obj1 = (BiddingUUID)obj;
			if(this.getUuid().equals(obj1.getUuid())){
				return true;
			}
		}
		return false;
	
	}
	
}
