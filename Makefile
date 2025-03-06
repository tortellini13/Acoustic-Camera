OPENCV_FLAGS = `pkg-config --cflags --libs opencv4`

FLAGS = -lrt -lpthread -lasound -fopenmp

FFT_FLAGS = -O3 -lfftw3f -lm -lfftw3f_threads

IMGUI_FLAGS = -I/usr/include/SDL2 -D_REENTRANT -lSDL2 -lGL -lGLEW -ldl -pthread 

OPTIMIZATION_FLAGS = #-march=native -ffast-math -funroll-loops -fprefetch-loop-arrays
 

IMGUI_SRC = imgui/imgui.cpp \
            imgui/imgui_demo.cpp \
            imgui/imgui_draw.cpp \
            imgui/imgui_widgets.cpp \
            imgui/imgui_tables.cpp \
            imgui/imgui_impl_sdl2.cpp \
            imgui/imgui_impl_opengl3.cpp \
            imgui/ImGuiFileDialog.cpp 


HEADERS = PARAMS.h Video.h ALSA.h Beamform.h 

NAME = main

all: $(NAME)

$(NAME): $(NAME).cpp $(HEADERS)
	g++ -g -o $(NAME) $(NAME).cpp $(IMGUI_SRC) $(FLAGS) $(OPTIMIZATION_FLAGS) $(FFT_FLAGS) $(OPENCV_FLAGS) $(IMGUI_FLAGS)

clean:
	rm -f $(NAME) imgui.ini