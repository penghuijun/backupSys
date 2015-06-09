PKG=`pkg-config --cflags --libs protobuf`
#G11=g++ `pkg-config --cflags --libs protobuf` -g -DDEBUG
#G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` -g -DDEBUG
G11=g++ --std=c++11 `pkg-config --cflags --libs protobuf` 
http: http.o 
	$(G11) -o http http.o   $(PKG) -lhiredis -lzmq -lpthread -levent
http.o: http.cpp
	$(G11) -c http.cpp
#json_reader.o: json_reader.cpp
#	$(G11) -c json_reader.cpp
#json_writer.o: json_writer.cpp
#	$(G11) -c json_writer.cpp
#json_value.o: json_value.cpp
#	$(G11) -c json_value.cpp
clean:
	rm  *.o http
