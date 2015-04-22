PKG=`pkg-config --cflags --libs protobuf`
#G11=g++ `pkg-config --cflags --libs protobuf` -g -DDEBUG
#G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` -g -DDEBUG
G11=g++  --std=c++11 `pkg-config --cflags --libs protobuf` 
OBJECTS=  bidderRequest.o CommonMessage.pb.o connectorPool.o main.o taskpool.o threadpool.o threadpoolmanager.o \
		   bidderConfig.o bidder.o AdVastRequestTemplate.pb.o AdBidderResponseTemplate.pb.o AdMobileRequest.pb.o AdMobileResponse.pb.o
#OBJECTS=  $(wildcard *.o)
connector: *.pb.c $(OBJECTS)
	$(G11) -o connector  $(OBJECTS) $(PKG) -lhiredis -lzmq -lpthread -levent
main.o: main.cpp
	$(G11) -c main.cpp
*.pb.c: *.proto
#	protoc --cpp_out=. *.proto
connectorPool.o: connectorPool.cpp connectorPool.h
	$(G11) -c connectorPool.cpp
bidderRequest.o: bidderRequest.cpp bidderRequest.h
	$(G11) -c bidderRequest.cpp
#json_reader.o: json_reader.cpp
#	$(G11) -c json_reader.cpp
#json_writer.o: json_writer.cpp
#	$(G11) -c json_writer.cpp
#json_value.o: json_value.cpp
#	$(G11) -c json_value.cpp
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
.PHONY: clean
clean:
	-rm  *.o connector
