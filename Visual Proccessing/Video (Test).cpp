#include <iostream>
#include <SDL2/SDL.h>
#include <libcamera/libcamera/libcamera.h>
#include <libcamera-apps/core/libcamera_app.h>
#include <libcamera-apps/core/buffer_sync.hpp>


using namespace std;

int main(int argc, char *argv[]) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cerr << "SDL failure: " << SDL_GetError() << endl;
        return -1;
    }

 SDL_Window* window = SDL_CreateWindow("Raspberry Pi Camera C++ TEST", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);
 if (!window) {
        cerr << "SDL window failure: " << SDL_GetError() << endl;
        SDL_Quit();
        return -1;
    }

SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
if (!renderer) {
        cerr << "SDL renderer failure: " << SDL_GetError() << endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

libcamera::LibcameraApp app;
    app.OpenCamera();
    app.Configure();
    app.StartCamera();

    if (!app.HasCamera()) {
        cerr << "No camera found" << endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

 bool quit = false;
    SDL_Event e;

    while (!quit) {
        // Poll for events (to check for quit signal)
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        
        libcamera::Request* request = app.CaptureRequest();
        if (!request) {
            cerr << "Failed to capture frame" << endl;
            continue;
        }

        const libcamera::FrameBuffer& buffer = request->buffers[0];
        void* frame_data = buffer.planes[0].mem;
        int frame_size = buffer.planes[0].length;

      
        SDL_Surface* frame_surface = SDL_CreateRGBSurfaceFrom(frame_data, 1280, 720, 24, 1280 * 3, 0xff0000, 0x00ff00, 0x0000ff, 0);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, frame_surface);
        SDL_FreeSurface(frame_surface);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        
        app.ReleaseRequest(request);

        /
        SDL_DestroyTexture(texture);
    }

    
    app.StopCamera();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;





}