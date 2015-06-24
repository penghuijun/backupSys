#!/bin/bash
SERVER_NAME='bidder_mobile'
SERVER_DIR=$1

if [ ! -d $SERVER_DIR ]
then
    echo "$SERVER_DIR not exists."
    exit 0
fi
if [ ! -f "$SERVER_DIR/$SERVER_NAME" ]
then
    echo "$SERVER_NAME not exists."
    exit 0
fi

#
# check $SERVER_NAME 
#
prco_num=`pgrep $SERVER_NAME|wc -l`
if [ $prco_num -eq 0 ]; then
    cd $SERVER_DIR
    nohup "./$SERVER_NAME" > /dev/null 2>&1 &
    ps -aux|grep $SERVER_NAME
fi
