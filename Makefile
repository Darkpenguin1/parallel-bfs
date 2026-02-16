CXXFLAGS=-I./third_party/rapidjson/include
LDFLAGS=-lcurl
LD=g++
CC=g++

all: level_client par_level_client

level_client: level_client.o
	$(LD) $< -g -o $@ $(LDFLAGS)



clean:
	-rm level_client level_client.o
