#!/bin/sh
SERVER_HOME=/home/ubuntu/ad_c/thro
SERVER_BIN=${SERVER_HOME}/throttle
SERVER_COMMAND="$SERVER_BIN"
SERVLOG=${SERVER_HOME}/throttle.log
SERVERRORLOG=$SERVER_HOME/throttle_error.log

#PROCESS=`ps -eaf | grep "ip2server"`
#ALIVE=`echo $PROCESS | grep "IP-COUNTRY-REGION-CITY-LATITUDE-LONGITUDE-ISP-DOMAIN-MOBILE-USAGETYPE.BIN"`
count=0
while true
do
        PROCESS=`ps -eaf | grep "throttle" | grep -v "grep"`
        if [[ -z "$PROCESS" ]]
        then
          echo -e "`date`:throttle died!" | tee -a $SERVERRORLOG
          $SERVER_COMMAND >./thro.log 2>&1 &
        else
             echo -e "`date`:throttle is still alive." | tee $SERVLOG
             PID=`pidof bcServ`
             arr=$(echo $PID|tr " " "\n")
             num=0
             for x in $arr;do
                 num=`expr $num + 1`
             done
             if [ $num -le 1 ];then
                 for x in $arr;do
                      kill -9 $x
                 done
             fi
        fi
        sleep 10
	
	count=`expr $count + 1`
        if [ $count -gt 360 ];then
                echo "restart bcServ"
                count=`expr $count - 360`
                PIDD=`pidof bcServ`
                aarr=$(echo $PIDD|tr " " "\n")

                for x in $aarr;do
                	echo $x 
                	kill -9 $x
                done
        fi
done
