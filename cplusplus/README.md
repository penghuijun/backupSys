adcosystem server side program
====

this is adcosystem server side cluster program. bidder/bidderCollector/connector/throttle.

Base environment installed
====

### gcc-4.8 install
```bash
sudo apt-get install libgmp-dev libmpfr4 libmpfr-dev libmpc-dev libmpc2 libtool m4 bison flex autoconf -y

sudo apt-get install python-software-properties 
sudo add-apt-repository ppa:ubuntu-toolchain-r/test

sudo apt-get update 
sudo apt-get install gcc-4.8 g++-4.8 gcc-4.8-doc
sudo apt-get install gcc-4.8-multilib （这个可以不装，看你自己主要用途）
sudo apt-get install g++-4.8-multilib

sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 20
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 20
sudo update-alternatives --config gcc
sudo update-alternatives --config g++
```

### libevent install
```bash
tar -zxvf throttleAndBidderInstall/bidderlib/libevent-2.0.21-stable.tar.gz 
cd libevent-2.0.21
./configure
make && sudo make install
```

### protobuf-2.5.0 install
```bash
tar -zxvf throttleAndBidderInstall/bidderlib/protobuf-2.5.0.tar.gz
cd protobuf-2.5.0
./configure
make && sudo make install
```

### zeromq-4.0.4 install
```bash
tar -zxvf throttleAndBidderInstall/bidderlib/zeromq-4.0.4.tar.gz
cd zeromq-4.0.4
./configure
make && sudo make install
```

### redis-3.0.0 install
```bash
tar -zxvf throttleAndBidderInstall/bidderlib/redis-3.0.0-beta2.tar.gz
cd zeromq-4.0.4
./configure
make && sudo make install
```

### hiredis install
```bash
git clone https://github.com/redis/hiredis.git
cd hiredis
make && sudo make install
```

Server side program installed
====
* bidder/connector
```bash
    cd cplusplus/bidder
    make protobuf
    make
    mkdir logs
 ```
 
* bidderCollector
```bash
    cd cplusplus/bidderCollector
    make protobuf
    make
    mkdir logs
```

* throttle
```bash
    cd cplusplus/throttle
    make protobuf
    make
    mkdir logs
```
