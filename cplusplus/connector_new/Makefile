TARGET 		:= mobile
OBJ_PATH 	:= objs
LOG_PATH	:= logs
TELE_LOG	:= bin_TELE/$(LOG_PATH)
GYIN_LOG	:= bin_GYIN/$(LOG_PATH)
SMAATO_LOG	:= bin_SMAATO/$(LOG_PATH)
TELE_MOBILE	:= bin_TELE/tele_mobile
GYIN_MOBILE	:= bin_GYIN/gyin_mobile
SMAATO_MOBILE	:= bin_SMAATO/smaato_mobile
TARGET_ALL 	:= $(TARGET) $(TELE_MOBILE) $(GYIN_MOBILE) $(SMAATO_MOBILE)

CC		:= gcc --std=c++11
INCLUDES	:= -I./include

#CPPFLAGS  	:= -Wall -Werror 						# show all warnings and take them as errors
CPPFLAGS  	:= -g -DTIMELOG  -L/usr/local/zeromq-4.0.4/lib -L/usr/lib/mysql -I/usr/local/zeromq-4.0.4/include -I/usr/include/mysql -I/usr/local/include/libxml2/
CPPFLAGS  	+= `pkg-config --cflags --libs protobuf`  

LIBS	   	:= -lzmq -levent -lpthread -lhiredis -lmysqlclient -lxml2 -lstdc++

SRCDIRS		:= src src/json src/thread src/redis
SRCTYPES   	:= .cpp .cc

SOURCES 	= $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCTYPES))))
OBJS    	= $(foreach x,$(SRCTYPES),$(patsubst %$(x),$(OBJ_PATH)/%.o,$(filter %$(x),$(SOURCES))))

$(OBJ_PATH)/%.o : %.cc
	$(CC) -c $(CPPFLAGS) $(INCLUDES) $< -o $@  
$(OBJ_PATH)/%.o : %.cpp
	$(CC) -c $(CPPFLAGS) $(INCLUDES) $< -o $@  

all: init protobuf compile cptarget

init:	
	$(foreach d,$(SRCDIRS), mkdir -p $(OBJ_PATH)/$(d);)
	rm -rf $(LOG_PATH) $(TELE_LOG) $(GYIN_LOG) $(SMAATO_LOG)
	mkdir $(LOG_PATH) $(TELE_LOG) $(GYIN_LOG) $(SMAATO_LOG)	
	
protobuf:
	protoc -I=./src/proto --cpp_out=./src ./src/proto/*.proto	
	mv src/*.pb.h  include/

compile: $(OBJS)
	$(CC) -o $(TARGET) $^ $(CPPFLAGS) $(LIBS)	

cptarget:
	cp -f mobile $(TELE_MOBILE)
	cp -f mobile $(GYIN_MOBILE)
	cp -f mobile $(SMAATO_MOBILE)
	
clean:
	rm -rf $(OBJ_PATH) $(LOG_PATH) $(TELE_LOG) $(GYIN_LOG) $(SMAATO_LOG)
	rm -f $(TARGET_ALL)
#	rm -f include/*.pb.h
#	rm -f src/*.pb.cc
#	rm -f masterworker workermaster 	
	
rebuild: clean all
