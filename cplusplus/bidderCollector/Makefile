PKG=`pkg-config --cflags --libs protobuf`
#G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` -g -DDEBUG
G11=g++ -g --std=c++11 `pkg-config --cflags --libs protobuf` 
bcServ : main.o taskpool.o threadpool.o threadpoolmanager.o bcServ.o  bcConfig.o CommonMessage.pb.o AdBidderResponseTemplate.pb.o AdVastRequestTemplate.pb.o expireddata.pb.o
	$(G11) -o bcServ bcServ.o taskpool.o threadpool.o threadpoolmanager.o main.o bcConfig.o  CommonMessage.pb.o AdBidderResponseTemplate.pb.o AdVastRequestTemplate.pb.o expireddata.pb.o $(PKG) -lhiredis -lzmq -lpthread -levent
main.o: main.cpp
	$(G11) -c main.cpp
bcServ.o: bcServ.cpp bcServ.h
	$(G11) -c bcServ.cpp
CommonMessage.pb.o: CommonMessage.pb.cc CommonMessage.pb.h
	$(G11) -c CommonMessage.pb.cc
AdBidderResponseTemplate.pb.o: AdBidderResponseTemplate.pb.cc AdBidderResponseTemplate.pb.h
	$(G11) -c AdBidderResponseTemplate.pb.cc 
AdVastRequestTemplate.pb.o: AdVastRequestTemplate.pb.cc AdVastRequestTemplate.pb.h
	$(G11) -c AdVastRequestTemplate.pb.cc 
expireddata.pb.o: expireddata.pb.cc expireddata.pb.h
	$(G11) -c expireddata.pb.cc
bcConfig.o: bcConfig.cpp bcConfig.h
	$(G11) -c bcConfig.cpp
taskpool.o: taskpool.cpp
	$(G11) -c taskpool.cpp
threadpool.o: threadpool.cpp
	$(G11) -c threadpool.cpp
threadpoolmanager.o: threadpoolmanager.cpp
	$(G11) -c threadpoolmanager.cpp
clean:
	rm  *.o bcServ
