OPENCV_FLAGS = `pkg-config --cflags --libs opencv4`

FLAGS = -lrt -lpthread -lasound

FFT_FLAGS = -O3 -lfftw3f -lm

HEADERS = PARAMS.h Video.h ALSA.h Beamform.h 

NAME = Video

all: $(NAME)

$(NAME): $(NAME).cpp $(HEADERS)
	g++ -g -o $(NAME) $(NAME).cpp $(FLAGS) $(FFT_FLAGS) $(OPENCV_FLAGS)

clean:
	rm -f $(NAME)