PKG=`pkg-config --cflags --libs protobuf`
#G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` -g -DDEBUG
G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` -g
serv : main.o redisClient.o  bcConfig.o zeroMqServer.o subscribeClient.o CommonMessage.pb.o AdBidderResponseTemplate.pb.o AdVastRequestTemplate.pb.o AdBidderResponseServ.o expiredDataServ.o expireddata.pb.o
	$(G11) -o serv main.o redisClient.o bcConfig.o  zeroMqServer.o subscribeClient.o CommonMessage.pb.o AdBidderResponseTemplate.pb.o AdVastRequestTemplate.pb.o AdBidderResponseServ.o expiredDataServ.o expireddata.pb.o $(PKG) -lhiredis -lzmq -lpthread
main.o: main.cpp
	$(G11) -c main.cpp
redisClient.o: redisClient.cpp redisClient.h
	$(G11) -c redisClient.cpp
zeroMqServer.o: zeroMqServer.cpp zeroMqServer.h
	$(G11) -c zeroMqServer.cpp
subscribeClient.o: subscribeClient.cpp subscribeClient.h
	$(G11) -c subscribeClient.cpp 
CommonMessage.pb.o: CommonMessage.pb.cc CommonMessage.pb.h
	$(G11) -c CommonMessage.pb.cc
AdBidderResponseTemplate.pb.o: AdBidderResponseTemplate.pb.cc AdBidderResponseTemplate.pb.h
	$(G11) -c AdBidderResponseTemplate.pb.cc 
AdVastRequestTemplate.pb.o: AdVastRequestTemplate.pb.cc AdVastRequestTemplate.pb.h
	$(G11) -c AdVastRequestTemplate.pb.cc 
expireddata.pb.o: expireddata.pb.cc expireddata.pb.h
	$(G11) -c expireddata.pb.cc
AdBidderResponseServ.o: AdBidderResponseServ.cpp AdBidderResponseServ.h
	$(G11) -c AdBidderResponseServ.cpp
expiredDataServ.o: expiredDataServ.cpp expiredDataServ.h
	$(G11) -c expiredDataServ.cpp
bcConfig.o: bcConfig.cpp bcConfig.h
	$(G11) -c bcConfig.cpp
clean:
	rm  *.o serv
