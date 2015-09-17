TARGET 		:= connector_mobile
OBJ_PATH 	:= objs
LOG_PATH	:= logs

CC		:= gcc --std=c++11
INCLUDES	:= -I./include

#CPPFLAGS  	:= -Wall -Werror 						# show all warnings and take them as errors
CPPFLAGS  	:= -g -DTIMELOG  -L/usr/local/zeromq-4.0.4/lib -L/usr/lib/mysql -I/usr/local/zeromq-4.0.4/include -I/usr/include/mysql
CPPFLAGS  	+= `pkg-config --cflags --libs protobuf`  

LIBS	   	:= -lzmq -levent -lpthread -lhiredis -lmysqlclient

SRCDIRS		:= src src/json src/thread src/redis
SRCTYPES   	:= .cpp .cc

SOURCES 	= $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCTYPES))))
OBJS    	= $(foreach x,$(SRCTYPES),$(patsubst %$(x),$(OBJ_PATH)/%.o,$(filter %$(x),$(SOURCES))))


all: init protobuf compile

init:
	$(foreach d,$(SRCDIRS), mkdir -p $(OBJ_PATH)/$(d);)
	rm -rf $(LOG_PATH)
	mkdir $(LOG_PATH)
	
protobuf:
	protoc -I=./src/proto --cpp_out=./src ./src/proto/*.proto	
	mv src/*.pb.h  include/
	
objs: $(OBJS)
$(OBJ_PATH)/%.o : %.cc
	$(CC) -c $(CPPFLAGS) $(INCLUDES) $< -o $@  
$(OBJ_PATH)/%.o : %.cpp
	$(CC) -c $(CPPFLAGS) $(INCLUDES) $< -o $@  

compile: $(OBJS)
	$(CC) -o $(TARGET) $^ $(CPPFLAGS) $(LIBS)	

	
clean:
	rm -rf $(OBJ_PATH) $(LOG_PATH)
	rm -f $(TARGET)
	rm include/*.pb.h
	rm src/*.pb.cc
	rm masterworker workermaster 	
	
rebuild: clean all