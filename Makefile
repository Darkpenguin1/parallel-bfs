# ---- Compiler settings ----
CC = g++
CXXFLAGS = -O2 -std=c++17 -Ithird_party/rapidjson/include
LDFLAGS = -lcurl

# ---- Targets ----
all: seq_client level_client

# ---- Sequential build ----
seq_client: seq.o
	$(CC) $< -o $@ $(LDFLAGS)

seq.o: seq.cpp
	$(CC) $(CXXFLAGS) -c $<

# ---- Parallel build ----
level_client: level_client.o
	$(CC) $< -o $@ $(LDFLAGS) -pthread

level_client.o: level_client.cpp
	$(CC) $(CXXFLAGS) -c $<

# ---- Clean ----
clean:
	rm -f seq_client level_client *.o