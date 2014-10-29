PKG=`pkg-config --cflags --libs protobuf`
#G11=g++ `pkg-config --cflags --libs protobuf` -g -DDEBUG
#G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` -g -DDEBUG
G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` 
PRO_FILE=  CommonMessage.pb.o main.o taskpool.o threadpool.o threadpoolmanager.o bidderConfig.o bidder.o\
 AdVastRequestTemplate.pb.o AdBidderResponseTemplate.pb.o AdMobileRequest.pb.o redisPool.o httpPool.o AdMobileResponse.pb.o
throttle: *.pb.c $(PRO_FILE) 
	$(G11) -o bidder $(PRO_FILE) $(PKG) -lhiredis -lzmq -lpthread -levent
main.o: main.cpp
	$(G11) -c main.cpp
*.pb.c: *.proto
#	protoc --cpp_out=. *.proto
CommonMessage.pb.o: CommonMessage.pb.cc CommonMessage.pb.h
	$(G11) -c CommonMessage.pb.cc
AdVastRequestTemplate.pb.o: AdVastRequestTemplate.pb.cc AdVastRequestTemplate.pb.h
	$(G11) -c AdVastRequestTemplate.pb.cc 
AdBidderResponseTemplate.pb.o: AdBidderResponseTemplate.pb.cc AdBidderResponseTemplate.pb.h
	$(G11) -c AdBidderResponseTemplate.pb.cc
AdMobileRequest.pb.o: AdMobileRequest.pb.cc AdMobileRequest.pb.h
	$(G11) -c AdMobileRequest.pb.cc
AdMobileResponse.pb.o: AdMobileResponse.pb.cc AdMobileResponse.pb.h
	$(G11) -c AdMobileResponse.pb.cc
bidderConfig.o: bidderConfig.cpp bidderConfig.h
	$(G11) -c bidderConfig.cpp
bidder.o: bidder.cpp bidder.h
	$(G11) -c bidder.cpp 
taskpool.o: taskpool.cpp taskpool.h
	$(G11) -c taskpool.cpp
threadpool.o: threadpool.cpp threadpool.h
	$(G11) -c threadpool.cpp
threadpoolmanager.o: threadpoolmanager.cpp threadpoolmanager.h
	$(G11) -c threadpoolmanager.cpp
redisPool.o: redisPool.cpp redisPool.h
	$(G11) -c redisPool.cpp
httpPool.o: httpPool.cpp httpPool.h
	$(G11) -c httpPool.cpp
clean:
	rm  *.o bidder
