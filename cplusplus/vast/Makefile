PKG=`pkg-config --cflags --libs protobuf`
#G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` -g -DDEBUG
G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` 
vast: *.pb.c vast.o  vastConfig.o main.o MobileAdRequest.pb.o  CommonMessage.pb.o AdVastRequestTemplate.pb.o 
	$(G11) -o vast vast.o vastConfig.o MobileAdRequest.pb.o  CommonMessage.pb.o AdVastRequestTemplate.pb.o main.o $(PKG) -lhiredis -lzmq -lpthread -levent
*.pb.c: *.proto
	protoc --cpp_out=. *.proto
main.o: main.cpp
	$(G11) -c main.cpp
vastConfig.o: vastConfig.cpp vastConfig.h
	$(G11) -c vastConfig.cpp
vast.o: vast.cpp vast.h
	$(G11) -c vast.cpp
CommonMessage.pb.o: CommonMessage.pb.cc CommonMessage.pb.h
	$(G11) -c CommonMessage.pb.cc
AdVastRequestTemplate.pb.o: AdVastRequestTemplate.pb.cc AdVastRequestTemplate.pb.h
	$(G11) -c AdVastRequestTemplate.pb.cc 
MobileAdRequest.pb.o: MobileAdRequest.pb.cc MobileAdRequest.pb.h
	$(G11) -c MobileAdRequest.pb.cc
clean:
	rm  *.o vast
