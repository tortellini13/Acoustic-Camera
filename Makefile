FLAGS = -lrt -lpthread

HEADERS = PARAMS.h sharedMemory.h

PROGRAM_NAMES = launch Audio Video

all: $(PROGRAM_NAMES)

launch: launch.cpp Audio.cpp Video.cpp
	g++ -o launch launch.cpp

Audio: Audio.cpp $(HEADERS)
	g++ -o Audio Audio.cpp $(FLAGS)

Video: Video.cpp $(HEADERS)
	g++ -o Video Video.cpp $(FLAGS)

clean:
	rm -f $(PROGRAM_NAMES)