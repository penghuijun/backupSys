#!/bin/sh

SERVER_NAME=bc_mobile

MASTER_PID=0

get_master_pid()
{
	CHILD_PROCESS=`ps -ef | grep $SERVER_NAME | grep -v "grep" | awk -F ' ' '{print $2}'`
	PARENT_PROCESS=`ps -ef | grep $SERVER_NAME | grep -v "grep" | awk -F ' ' '{print $3}'`
	for x in $CHILD_PROCESS; do
		for j in $PARENT_PROCESS; do
			if [ $x -eq $j ]; then
				MASTER_PID=$x
				return
			fi
		done
	done
	return
}

check_process_status()
{
	PROCESS=`ps -ef | grep $SERVER_NAME | grep -v "grep"`
	if [ -z "$PROCESS" ]; then		
		echo "$SERVER_NAME is null"
		return 0
	else
		echo "$SERVER_NAME running"
		echo "$PROCESS"
		return 1
	fi
}

start()
{
	nohup ./$SERVER_NAME > /dev/null 2>&1 &
	sleep 1
	PROCESS=`ps -ef | grep $SERVER_NAME | grep -v "grep"`	
	echo "$PROCESS"
}

stop()
{
	TRY_TIME=0
	while true
	do
		PROCESS=`ps -ef | grep $SERVER_NAME | grep -v "grep"`		
		if [ -z "$PROCESS" ]; then
			echo "TRY_TIME : $TRY_TIME"
			echo "$SERVER_NAME EXIT SUCCESS"			
			break
		else			
			if [ $TRY_TIME -eq 0 ]; then
				get_master_pid				
				if [ $MASTER_PID -ne 0 ]; then
					echo "kill -s SIGINT $MASTER_PID"
					/bin/bash -c "kill -s SIGINT $MASTER_PID"
				fi				
			else
				PID=`pidof $SERVER_NAME`
				ARR=$(echo $PID | tr " " "\n")
				echo "kill -9 $PID"
				for x in $ARR;do				
					kill -9 $x
				done
			fi		
			TRY_TIME=`expr $TRY_TIME + 1`	
		fi
	done
}

case "$1" in
	start)
		echo "$SERVER_NAME starting ..."
		check_process_status
		if [ $? -ne 1 ]; then			
			start
			echo "create new running"
		else
			echo "keep old running"
		fi
		exit 1
		;;

	stop)
		echo "$SERVER_NAME stoping ..."
		check_process_status
		if [ $? -eq 1 ]; then			
			stop
		fi
		exit 1
		;;

	restart)
		echo "$SERVER_NAME restarting ..."		
		check_process_status
		if [ $? -eq 1 ]; then
			stop			
		fi
		start
		exit 1
		;;

	*)
		echo "Usage: sh thro_server.sh [start|stop|restart]"
		exit 1
		;;
esac