PKG=`pkg-config --cflags --libs protobuf`
#G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` -g -DDEBUG
G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` 
EXE_FILE=bc_mobile
PRO_FILE=main.o taskpool.o threadpool.o threadpoolmanager.o bcServ.o  bcConfig.o CommonMessage.pb.o AdBidderResponseTemplate.pb.o AdVastRequestTemplate.pb.o expireddata.pb.o MobileAdResponse.pb.o MobileAdRequest.pb.o
$(EXE_FILE): *.pb.c $(PRO_FILE)
	$(G11) -o $(EXE_FILE) $(PRO_FILE)  $(PKG) -lhiredis -lzmq -lpthread -levent
*.pb.c: *.proto
	protoc --cpp_out=. *.proto
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
MobileAdRequest.pb.o: MobileAdRequest.pb.h MobileAdRequest.pb.cc
	$(G11) -c MobileAdRequest.pb.cc
MobileAdResponse.pb.o: MobileAdResponse.pb.h MobileAdResponse.pb.cc
	$(G11) -c MobileAdResponse.pb.cc
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
	rm  *.o ${EXE_FILE}
