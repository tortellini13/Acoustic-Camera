OPENCV_FLAGS = `pkg-config --cflags --libs opencv4`

FLAGS = -lrt -lpthread -lasound

FFT_FLAGS = -O3 -lfftw3 -lm

HEADERS = PARAMS.h sharedMemory.h Video.h

NAME_1 = Audio

NAME_2 = Video

PROGRAM_NAMES = launch $(NAME_1) $(NAME_2)

all: $(PROGRAM_NAMES)

launch: launch.cpp $(NAME_1).cpp $(NAME_2).cpp
	g++ -o launch launch.cpp

$(NAME_1): $(NAME_1).cpp $(HEADERS)
	g++ -o $(NAME_1) $(NAME_1).cpp $(FLAGS) $(FFT_FLAGS)

$(NAME_2): $(NAME_2).cpp $(HEADERS)
	g++ -o $(NAME_2) $(NAME_2).cpp $(FLAGS) $(OPENCV_FLAGS)

clean:
	rm -f $(PROGRAM_NAMES)