1,配置文件 bcConfig.txt
#bcServConfig //#用来注释
$bcConfigIndex=1 //$用在表示这是索引，索引从1开始递增，这里表示这是第一项配置
bcServIP=192.168.20.119 //索引以下表示属于这个索引的具体的配置 ，这里第一个索引项用来表示bc服务器的配置，这里是IP地址
bcServBidPort=5001  //监听bidder的端口
bcServVastPort=5002 //监听vast 超时信息的端口

#saveRedisConfig
$bcConfigIndex=2  //redis存储服务器
redisSaveServIP=192.168.20.119 //redis存储服务器的地址
redisSaveServPort=6379 //redis存储服务器的端口
redisSaveServTime=120 //redis存储服务器的数据保存时间

#publishRedisConfig
$bcConfigIndex=3 //发布publish的redis服务器
redisPublishIP=192.168.20.119 //发布publish的redis服务器地址
redisPublishPort=6379 //发布publish的redis服务器端口
redisPublishKey=Publish.UUID //发布publish的频道

2，程序需要的环境
a， redis服务器
b， 安装zeromq