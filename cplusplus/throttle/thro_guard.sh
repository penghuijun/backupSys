#!/bin/sh
START_SCRIPT_NAME=thro_start.sh
STOP_SCRIPT_NAME=thro_stop.sh

kill_guard_script()
{
    SHPRO=`ps -eaf|grep "$START_SCRIPT_NAME"|grep -v "grep"|awk '{print $2}'`
    arr=$(echo $SHPRO|tr " " "\n")
    for x in $arr;do
	 kill -9 $x
    done   
}

case "$1" in
start)
    echo "server starting"
    kill_guard_script 
    nohup sh $STOP_SCRIPT_NAME &
    nohup sh $START_SCRIPT_NAME &
    exit 1
    ;;  
stop)
    echo "server stoping"
    kill_guard_script
    nohup sh $STOP_SCRIPT_NAME &
    exit 1
    ;;  

restart)
    echo "server restarting"
    nohup sh $STOP_SCRIPT_NAME &
    nohup sh $START_SCRIPT_NAME &
    exit 1
    ;;  
*)
    echo "Usage: sh thro_guard.sh {start|stop|restart}"
    exit 1
    ;;  
esac

exit 0
