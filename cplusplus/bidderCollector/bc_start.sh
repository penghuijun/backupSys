#!/bin/sh
SERVER_HOME=/home/hadoop/xyj/test/bcServ
SERVER_NAME=bcServ
SERVER_BIN=${SERVER_HOME}/${SERVER_NAME}
SERVER_COMMAND="$SERVER_BIN"
SERVERLOG=${SERVER_HOME}/bc.log
SERVERRORLOG=$SERVER_HOME/bc_error.log
SERVER_WORKER_NUM=1
CHECK_RESOURCE_PORT1=7010
CHECK_RESOURCE_PORT2=7012

kill_service_process()
{
    pkill -9 $SERVER_NAME
    pkill -9 $SERVER_NAME
    pkill -9 $SERVER_NAME
    KILLPID=`pidof "$SERVER_NAME"`
    KILLARR=$(echo $KILLPID|tr " " "\n")
    for x in $KILLARR;do
        kill -9 $x
    done
    pkill -9 $SERVER_NAME
    pkill -9 $SERVER_NAME
    pkill -9 $SERVER_NAME
}

release_port_resouce()
{	
     PORTPROCESS=`lsof -i:"$1" |awk '{print $2}'|grep -v "PID"|uniq`
     PROCESSARR=$(echo $PORTPROCESS|tr " " "\n")
     for x in $PROCESSARR;do
      	kill -9 $x
     done
}


check_service_process()
{
    PID=`pidof "$SERVER_NAME"`
    ARR=$(echo $PID|tr " " "\n")
    NUM=0
    for x in $ARR;do
	NUM=`expr $NUM + 1`
    done
    if [ $NUM -le $SERVER_WORKER_NUM ];then
	echo -e "$SERVER_NAME worker num is not norm" # >>  $SERVERRORLOG
        echo -e "`ps -eaf | grep "$SERVER_BIN" | grep -v "grep"`" #>> $SERVERRORLOG
	kill_service_process
	release_port_resouce "$CHECK_RESOURCE_PORT1"
	release_port_resouce "$CHECK_RESOURCE_PORT2"
    fi
}

#count=0
while true
do
        PROCESS=`ps -eaf | grep "$SERVER_BIN" | grep -v "grep"`
        if [[ -z "$PROCESS" ]]
        then
          echo -e "`date`:$SERVER_NAME died!" #>> $SERVERRORLOG
		  chmod +x $SERVER_COMMAND
		  $SERVER_COMMAND >>$SERVERLOG 2>&1 &
  	  echo -e "`date`: $SERVER_NAME restart" # >>  $SERVLOG
        else
             echo -e "`date`:$SERVER_NAME is still alive." # > $SERVLOG
             check_service_process
        fi
        sleep 10

 #       count=`expr $count + 1`
  #      if [ $count -gt 36000 ];then
   #             count=`expr $count - 36000`
#		kill_service_process
#		release_port_resouce $CHECK_RESOURCE_PORT1
#		release_port_resouce $CHECK_RESOURCE_PORT2
 #       fi
done
