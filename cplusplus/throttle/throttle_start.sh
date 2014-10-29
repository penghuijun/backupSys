SERVER_HOME=/home/hadoop/xyj/test/throttle
SERVER_BIN=${SERVER_HOME}/throttle
SERVER_COMMAND="$SERVER_BIN"
SERVLOG=${SERVER_HOME}/throttle.log
SERVERRORLOG=$SERVER_HOME/throttle_error.log

#PROCESS=`ps -eaf | grep "ip2server"`
#ALIVE=`echo $PROCESS | grep "IP-COUNTRY-REGION-CITY-LATITUDE-LONGITUDE-ISP-DOMAIN-MOBILE-USAGETYPE.BIN"`

while true
do
        PROCESS=`ps -eaf | grep "throttle" | grep -v "grep"`
        if [[ -z "$PROCESS" ]]
        then
          echo -e "`date`:throttle died!" | tee -a $SERVERRORLOG
          $SERVER_COMMAND >/dev/null 2>&1 &
        else
                echo -e "`date`:throttle is still alive." | tee $SERVLOG
        fi
        sleep 10
done
