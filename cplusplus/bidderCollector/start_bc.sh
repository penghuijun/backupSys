SERVER_HOME=/home/hadoop/xyj/test/bc
SERVER_BIN=${SERVER_HOME}/bcServ
SERVER_COMMAND="$SERVER_BIN"
SERVLOG=${SERVER_HOME}/bcServ.log
SERVERRORLOG=$SERVER_HOME/bcServ_error.log

#PROCESS=`ps -eaf | grep "ip2server"`
#ALIVE=`echo $PROCESS | grep "IP-COUNTRY-REGION-CITY-LATITUDE-LONGITUDE-ISP-DOMAIN-MOBILE-USAGETYPE.BIN"`

while true
do
        PROCESS=`ps -eaf | grep "bcServ" | grep -v "grep"`
        if [[ -z "$PROCESS" ]]
        then
          echo -e "`date`:bcserver died!" | tee -a $SERVERRORLOG
          $SERVER_COMMAND >/dev/null 2>&1 &
        else
                echo -e "`date`:bcserver is still alive." | tee $SERVLOG
        fi
        sleep 10
done
