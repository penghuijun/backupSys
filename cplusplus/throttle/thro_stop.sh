#!/bin/sh

SERVER_HOME=/home/hadoop/xyj/test/throttle
SERVER_NAME=throttle
SERVER_BIN=${SERVER_HOME}/${SERVER_NAME}
SERVER_COMMAND="$SERVER_BIN"
SERVERLOG=${SERVER_HOME}/throttle.log
SERVERRORLOG=$SERVER_HOME/throttle_error.log

while true
do
        PROCESS=`ps -eaf | grep "$SERVER_BIN" | grep -v "grep"`
        if [[ -z "$PROCESS" ]]
        then
          echo -e "`date`: $SERVER_NAME EXIT SUCCESS!"
          exit 1;
	else
	  pkill -9 $SERVER_NAME
	  PID=`pidof $SERVER_NAME`
	  ARR=$(echo $PID|tr " " "\n")
	  for x in $ARR;do
		kill -9 $x
	  done
          pkill -9 $SERVER_NAME
	fi
	exit 0
done
~                                                                                                                                                                                                                                                                             
~        
