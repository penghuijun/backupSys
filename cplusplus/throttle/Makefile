PKG=`pkg-config --cflags --libs protobuf`
#G11=g++ `pkg-config --cflags --libs protobuf` -g -DDEBUG
#G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` -g -DDEBUG

G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` 
EXE_FILE=thro_mobile
PRO_FILE=main.o taskpool.o threadpool.o threadpoolmanager.o throttleConfig.o throttle.o  CommonMessage.pb.o AdVastRequestTemplate.pb.o AdBidderResponseTemplate.pb.o expireddata.pb.o MobileAdRequest.pb.o
$(EXE_FILE): *.pb.c ${PRO_FILE}
	$(G11) -o $(EXE_FILE) $(PRO_FILE) $(PKG) -lhiredis -lzmq -lpthread -levent
*.pb.c: *.proto
	protoc --cpp_out=. *.proto
main.o: main.cpp
	$(G11) -c main.cpp
#redisClient.o: redisClient.cpp redisClient.h
#	$(G11) -c redisClient.cpp
#zeroMqServer.o: zeroMqServer.cpp zeroMqServer.h
#	$(G11) -c zeroMqServer.cpp
CommonMessage.pb.o: CommonMessage.pb.cc CommonMessage.pb.h
	$(G11) -c CommonMessage.pb.cc
AdVastRequestTemplate.pb.o: AdVastRequestTemplate.pb.cc AdVastRequestTemplate.pb.h
	$(G11) -c AdVastRequestTemplate.pb.cc 
AdBidderResponseTemplate.pb.o: AdBidderResponseTemplate.pb.cc AdBidderResponseTemplate.pb.h
	$(G11) -c AdBidderResponseTemplate.pb.cc
MobileAdRequest.pb.o: MobileAdRequest.pb.cc MobileAdRequest.pb.h
	$(G11) -c MobileAdRequest.pb.cc
expireddata.pb.o: expireddata.pb.cc expireddata.pb.h
	$(G11) -c expireddata.pb.cc
throttleConfig.o: throttleConfig.cpp throttleConfig.h
	$(G11) -c throttleConfig.cpp
throttle.o: throttle.cpp throttle.h
	$(G11) -c throttle.cpp 
taskpool.o: taskpool.cpp taskpool.h
	$(G11) -c taskpool.cpp
threadpool.o: threadpool.cpp threadpool.h
	$(G11) -c threadpool.cpp
threadpoolmanager.o: threadpoolmanager.cpp threadpoolmanager.h
	$(G11) -c threadpoolmanager.cpp
clean:
	rm  *.o $(EXE_FILE)
