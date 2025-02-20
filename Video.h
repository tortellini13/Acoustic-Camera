#include <opencv2/opencv.hpp>
#include <atomic>
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_opengl3.h"
#include <SDL2/SDL.h>
#include <GL/glew.h>

// add this to try on pi to makefile:            sdl2-config --cflags

using namespace std;
using namespace cv;

class video 
{
public:
    video(int frame_width, int frame_height, int frame_rate);

    ~video();

    void startCapture();

    void stopCapture();

    bool processFrame(Mat& data_input, const float lower_limit, const float upper_limit);

private:
    // Gets frame from other thread
    bool getFrame(Mat& frame); 
    
    // Actively captures video stream
    void captureVideo(VideoCapture& cap, atomic<bool>& is_running);

    // Creates heatmap from input data, thesholds, clamps, and merges with video frame. Also contains generators for test data.
    Mat createHeatmap(Mat& data_input, const float lower_limit, const float upper_limit, Mat& frame);

    // Draws color bar onto frame
    void drawColorBar(int scale_width, int scale_height);

    // Draws UI onto frame
    Mat drawUI(Mat& data_input);

    //IMGUI
    bool startIMGui();

    bool renderIMGui(Mat& frame_in);
    
    void shutdownIMGui();

    GLuint textureID;

    // Camera params
    int frame_width;
    int frame_height;
    int frame_rate;

    VideoCapture cap;

    // For multithreading
    atomic<bool> is_running;
    thread video_thread;

    // Frame queue for inter-thread communication
    queue<Mat> frame_queue;
    mutex frame_mutex;  // Protects access to the frame queue

    // Variables for IMGUI buttons
    bool mark_max_mag_state = true;
    bool color_scale_state = true;
    bool heat_map_state = true;
    bool data_clamp_state = true;
    bool threshold_state = true;

    bool options_menu = false; //if false, the slider menu displays, if true, the options window displays

    int imgui_threshold = -50;
    int imgui_clamp_min = -100;
    int imgui_clamp_max = 0;
    float imgui_alpha = 0.5;

    void UISetup();

    // For magnitude proccessing
    double magnitude_min;
    double magnitude_max;

    // Coords for max magnitude
    Point max_coord;        // Coords from data
    Point max_point_scaled; // Coords scaled to frame

    // Color bar
    Mat static_color_bar;
    Mat color_bar;
    Mat scaleColor;

    // FPS counter
    void FPSCalculator();
    timer FPSTimer;
    timer camFPSTimer;
    double FPS;
    double camFPS;

    SDL_Window* window = nullptr;
    
    SDL_GLContext gl_context;

    SDL_Event event;

    Mat frame;

};

//=====================================================================================

video::video(int frame_width, int frame_height, int frame_rate) :
    frame_width(frame_width),
    frame_height(frame_height),
    frame_rate(frame_rate),
    cap(0, CAP_V4L2),
    FPSTimer("FPS Timer"),
    camFPSTimer("CAM FPS Timer") {}

video::~video() 
{
    stopCapture();
} // end ~video

//=====================================================================================

void video::startCapture() 
{
    // Configure capture properties
    cap.set(CAP_PROP_FRAME_WIDTH, frame_width);
    cap.set(CAP_PROP_FRAME_HEIGHT, frame_height);
    cap.set(CAP_PROP_FPS, frame_rate);
    cap.set(CAP_PROP_BUFFERSIZE, 1);

    // Record video on separate thread
    is_running = true;
    video_thread = thread(&video::captureVideo, this, ref(cap), ref(is_running));

    UISetup();
} // end startCapture

void video::stopCapture() 
{
    is_running = false;
    if (video_thread.joinable()) 
    {
        video_thread.join();
    }

    destroyAllWindows();
    shutdownIMGui();
    
} // end stopCapture

//=====================================================================================

void video::UISetup() 
{   
    // Make a window for the buttons to go on
    drawColorBar(SCALE_WIDTH, SCALE_HEIGHT);

    if (startIMGui() == false) {
        cout << "IMGui init FAILED :(" << endl;
    }

    Mat initial_frame(Size(RESOLUTION_HEIGHT, RESOLUTION_WIDTH), CV_32FC1);

    while (!getFrame(initial_frame)) 
    {
        cerr << "Error: Could not retreive frame from capture thread!" << "\n";
        this_thread::sleep_for(chrono::seconds(1));
    }

    FPSTimer.start();

} // end UISetup

//=====================================================================================

void video::captureVideo(VideoCapture& cap, atomic<bool>& is_running) 
{
    while (is_running) 
    {
        camFPSTimer.start();
        Mat temp_frame;
        if (cap.read(temp_frame)) 
        {
            lock_guard<mutex> lock(frame_mutex);  // Protect access to the frame queue with a mutex
            frame_queue.push(temp_frame.clone()); // Store a copy of the frame in the queue
        }

        else 
        {
            cerr << "Error: Could not capture frame from video source!" << "\n";
            is_running = false;
        }
        camFPSTimer.end();
    }

    
} // end videoCapture

//=====================================================================================

bool video::getFrame(Mat& frame) 
{
    // Try to fetch a frame without blocking
    lock_guard<mutex> lock(frame_mutex);
    if (!frame_queue.empty()) 
    {
        frame = frame_queue.front(); // Retrieve the frame
        frame_queue.pop();           // Remove the frame from the queue
        return true;
    }
    return false; // No frame available
} // end getFrame

//=====================================================================================

Mat video::createHeatmap(Mat& data_input, const float lower_limit, const float upper_limit, Mat& frame)
{
    // Ensure data_input is of type CV_32F for proper normalization
    Mat workingFrame;
    frame.copyTo(workingFrame);
    #ifdef ENABLE_RANDOM_DATA
    randu(data_input, Scalar(-100), Scalar(0));
    #endif
    #ifdef ENABLE_STATIC_DATA
    
    double st_height = NUM_PHI;
    double st_width = NUM_THETA;
    double st_max = 0;
    double st_min = -100;

    array2D<float> static_test_frame(st_width, st_height);

    for (int i = 0; i < static_test_frame.dim_1; i++)
    {
        for (int j = 0; j < static_test_frame.dim_2; j++)
        {
            float value;
            float distance = sqrt(((st_width / 2 - i)*(st_width / 2 - i)) + ((st_height / 2 - j)*(st_height / 2 - j)));
            if (distance == st_width) {
                 value = st_min;
            } else {
                value = (static_cast<float>(st_max - st_min) * (1 - static_cast<float>(distance / st_width))) + st_min;
            }

            static_test_frame.at(i, j) = value;
        } // end j
    } // end i

    // Create a Mat and reassign data
    cv::Mat mat(static_test_frame.dim_1, static_test_frame.dim_2, CV_32FC1, static_test_frame.data);

    mat.copyTo(data_input);

    #endif
    
    if (data_clamp_state == true) 
    {
    Mat data_clamped;
    
    data_input.convertTo(data_clamped, CV_32F);

        if (imgui_clamp_min >= imgui_clamp_max) {
            if(imgui_clamp_min == 0) {
                imgui_clamp_min = -1;
                imgui_clamp_max = 0;
            } else if(imgui_clamp_max == -100) {
                imgui_clamp_max = -99;
                imgui_clamp_min = -100;
            } else {
                imgui_clamp_max = imgui_clamp_min + 1;
            }

        }

    for(int col = 0; col < data_clamped.cols; col++ ) {
        for (int row = 0; row < data_clamped.rows; row++) {

            if(data_clamped.at<float>(row, col) < imgui_clamp_min) {
                data_clamped.at<float>(row, col) = imgui_clamp_min;
            }

            if(data_clamped.at<float>(row, col) > imgui_clamp_max) {
                data_clamped.at<float>(row, col) = imgui_clamp_max;
            }   

        }
    }

    data_clamped.copyTo(data_input);
    }

    // Normalizes data to be 0-255
    Mat data_normalized;
    data_input.convertTo(data_normalized, CV_32F);

     // Find coord of max and min magnitude
    minMaxLoc(data_input, &magnitude_min, &magnitude_max, NULL, &max_coord);
    
    // Scale max point to video resolution
    max_point_scaled.x = (static_cast<double>(max_coord.x) / static_cast<double>(data_normalized.cols)) * RESOLUTION_WIDTH; 
    max_point_scaled.y = (static_cast<double>(max_coord.y) / static_cast<double>(data_normalized.rows)) * RESOLUTION_HEIGHT;


    // Normalize the input to the range 0-255 (required for color mapping)
    normalize(data_normalized, data_normalized, 0, 255, NORM_MINMAX);

    // Convert to 8-bit format (required for applying color map)
    Mat heatmap_input;
    data_normalized.convertTo(heatmap_input, CV_8U);

    // Apply a colormap to create the heatmap
    Mat heatmap;
    applyColorMap(heatmap_input, heatmap, COLORMAP_JET);

    Mat frame_merged;
    
    if(heat_map_state == false) 
    {   
        workingFrame.copyTo(frame_merged);
    }

    if(heat_map_state == true) 
    {
    // Ensure the heatmap is the same size as the frame
    Mat resized_heatmap;
    Mat resized_data_input;
    resize(heatmap, resized_heatmap, workingFrame.size());
    resize(data_input, resized_data_input, workingFrame.size());

    // Blend the heatmap and the frame with an alpha value
    //double alpha2 = static_cast<double>(getTrackbarPos("Alpha", "Window"))/100;
    
    if (threshold_state == false) {
    addWeighted(workingFrame, 1.0, resized_heatmap, imgui_alpha, 0.0, frame_merged);
    }
    if(threshold_state == true) {
        for(int y = 0; y < resized_heatmap.rows; ++y)
            {
                for (int x = 0; x < resized_heatmap.cols; ++x) 
                {
                    
                    if (resized_data_input.at<float>(y, x) > imgui_threshold) 
                    {
                        Vec3b& pixel = workingFrame.at<Vec3b>(y, x);         // Current pixel in camera
                        Vec3b heatMapPixel = resized_heatmap.at<Vec3b>(y, x); // Current pixel in heatmap

                        // Loops through R G & B channels
                        for (int c = 0; c < 3; c++) 
                        {
                            // Merges heatmap with video and applies alpha value
                            pixel[c] = static_cast<uchar>(imgui_alpha * heatMapPixel[c] + (1 - imgui_alpha) * pixel[c]);
                        } 
                    }
                }
            } // end alphaMerge
        
        workingFrame.copyTo(frame_merged);
    }
    }

    return frame_merged; // Return the generated heatmap
} // end createHeatmap

//=====================================================================================

void video::drawColorBar(int scale_width, int scale_height) 
{
    color_bar = Mat(scale_height, scale_width, CV_8UC3);
    scaleColor = Mat(scale_height, scale_width, CV_8UC1, Scalar(0));
    cout << scaleColor.size() << endl;

    for (int y = 0; y < scale_height; y++) 
    {
        int scaleIntensity = 255 * (static_cast<double>((scale_height- y)) / static_cast<double>(scale_height));
        
        for(int x = 0; x < scale_width; x++) 
        {
            scaleColor.at<uchar>(y, x) = scaleIntensity;
        }
    }
    cout << scale_width << " " << scale_height << endl;
    applyColorMap(scaleColor, color_bar, COLORMAP_JET);
    
} // end drawColorBar

//=====================================================================================

void video::FPSCalculator()
{
    FPSTimer.end();
    double FPSTime = FPSTimer.time_avg(AVG_SAMPLES, false);
    if (FPSTime != -1) 
    { FPS = 1/FPSTime; }
    FPSTimer.start();

    double camFPSTime = camFPSTimer.time_avg(AVG_SAMPLES, false);
    if (camFPSTime != -1) 
    { camFPS = 1/camFPSTime; }

}   

//=====================================================================================

Mat video::drawUI(Mat& data_input)
{
    // Color Bar Scale
    if (color_scale_state == true) 
    {   
        Rect color_bar_location(SCALE_POS_X, SCALE_POS_Y, SCALE_WIDTH, SCALE_HEIGHT);
        rectangle(data_input, Point(SCALE_POS_X, SCALE_POS_Y) + Point(-SCALE_BORDER, -SCALE_BORDER - 10), Point(SCALE_POS_X, SCALE_POS_Y) + Point(SCALE_WIDTH + SCALE_BORDER - 1, SCALE_HEIGHT + SCALE_BORDER + 5), Scalar(0, 0, 0), FILLED); //Draw rectangle behind scale to make a border
        
        color_bar.copyTo(data_input(color_bar_location)); //Copy the scale onto the image

         //Draw text indicating various points on the scale
        float scaleTextRatio = (1 / static_cast<float>(SCALE_POINTS ));
        for (int i = 0; i < (SCALE_POINTS + 1); i++) 
        {
            Point scaleTextStart(SCALE_POS_X - 6, (SCALE_POS_Y + ((1 - static_cast<double>(i) * scaleTextRatio) * SCALE_HEIGHT) - 3)); //Starting point for the text
            double scaleTextValue = ((static_cast<double>(magnitude_max - magnitude_min) * scaleTextRatio * static_cast<double>(i)) + magnitude_min); //Value of text for each point

            ostringstream scaleTextStream;
            String scaleTextString;
            String scaleTextSymbol = " ";

            if (data_clamp_state == 1) {
                if (i == 0) {
                    scaleTextSymbol = "<";
                }
                if (i == SCALE_POINTS) {
                    scaleTextSymbol = ">";
                }
            }
            scaleTextStream << fixed << setprecision(LABEL_PRECISION) << scaleTextValue;
            scaleTextString = scaleTextSymbol + scaleTextStream.str() + " ";

            putText(data_input, scaleTextString, scaleTextStart, FONT_TYPE, FONT_SCALE - 0.2, Scalar(255, 255, 255), FONT_THICKNESS);
            line(data_input, scaleTextStart + Point(0,3), scaleTextStart + Point(SCALE_WIDTH + 6, 3), Scalar(255, 255, 255), 1, 8, 0);
        }
    } // end colar bar scale
    
    // Mark maximum location
    if (mark_max_mag_state == true) 
    {
        drawMarker(data_input, max_point_scaled, Scalar(0, 0, 0), MARKER_CROSS, CROSS_SIZE + 1, CROSS_THICKNESS + 1, 8); //Mark the maximum magnitude point
        drawMarker(data_input, max_point_scaled, Scalar(255, 255, 255), MARKER_CROSS, CROSS_SIZE, CROSS_THICKNESS, 8); //Mark the maximum magnitude point
    }
    
    return data_input;
} // end drawUI

//=====================================================================================

// Function to initialize IMGui
bool video::startIMGui() {
    #ifdef UBUNTU
    
    // Initialize SDL

    //cout << "starting imgui..." << endl;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Error: SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXTsudo apt install libglew-dev_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create an OpenGL window
    window = SDL_CreateWindow("Acoustic Camera", 0, 0, 1024, 600, SDL_WINDOW_OPENGL);
    if (!window) {
        std::cerr << "Error: SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create OpenGL context
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        std::cerr << "Error: SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    //SDL_GL_SetSwapInterval(1); // Enable V-Sync

    // Initialize GLEW
    glewExperimental = GL_TRUE;  // Important for compatibility
    if (glewInit() != GLEW_OK) {
        std::cerr << "Error: Failed to initialize GLEW\n";
        return false;
    }

    // Initialize ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;  // Avoid unused variable warning

    // Setup ImGui style (optional)
    ImGui::StyleColorsDark();

    // Setup platform/renderer bindings
    if (!ImGui_ImplSDL2_InitForOpenGL(window, gl_context)) {
        std::cerr << "Error: ImGui_ImplSDL2_InitForOpenGL failed!\n";
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::cerr << "Error: ImGui_ImplOpenGL3_Init failed!\n";
        return false;
    }
    
    //Create texture for displaying each frame of video feed
    
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Allocate memory for texture (will be updated every frame)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RESOLUTION_WIDTH, RESOLUTION_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    return true;

    #endif

    #ifdef PI

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Error: SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set OpenGL ES attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    // Create an OpenGL ES window
    window = SDL_CreateWindow("Acoustic Camera", 1, 1, 1024, 600, SDL_WINDOW_OPENGL);
    if (!window) {
        std::cerr << "Error: SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create OpenGL ES context
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        std::cerr << "Error: SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    // SDL_GL_SetSwapInterval(1); // Enable V-Sync

    // Initialize ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;  // Avoid unused variable warning

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup platform/renderer bindings
    if (!ImGui_ImplSDL2_InitForOpenGL(window, gl_context)) {
        std::cerr << "Error: ImGui_ImplSDL2_InitForOpenGL failed!\n";
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 300 es")) {
        std::cerr << "Error: ImGui_ImplOpenGL3_Init failed!\n";
        return false;
    }

    std::cout << "Finished starting ImGui..." << std::endl;
    return true;

    #endif
}

//=====================================================================================

bool video::renderIMGui(Mat &frame_in) {
    // Start a new frame
    #ifdef UBUNTU
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event); // Pass events to ImGui

        if (event.type == SDL_QUIT) {
           return false;
        }
    }
    
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
   #endif
   
   #ifdef PI

   SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event); // Pass events to ImGui

        if (event.type == SDL_QUIT) {
            return false;
        }
    }

    // Start new ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();

   #endif

    Mat frameRGBA;
    cvtColor(frame_in, frameRGBA, COLOR_BGR2RGBA);  // Convert OpenCV BGR to RGBA

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frameRGBA.cols, frameRGBA.rows, GL_RGBA, GL_UNSIGNED_BYTE, frameRGBA.data);
    glBindTexture(GL_TEXTURE_2D, 0);
   
   
   
   
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(660, 500), ImGuiCond_Always);
    ImGui::Begin("Video", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        ImGui::Image(textureID, ImVec2(frame.cols, frame.rows));
    ImGui::End();

    // Slider Menu

    if (options_menu == false) {

    ImGui::SetNextWindowPos(ImVec2(660,0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(364, 600), ImGuiCond_Always);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        // First Column: Threshold
        ImGui::BeginGroup(); // Start grouping for alignment
        ImGui::Text("Thresh");
        ImGui::VSliderInt("##Threshold", ImVec2(40, 500), &imgui_threshold, -100, 0, "%d");
        ImGui::EndGroup();

        ImGui::SameLine(); // Move to the next column

        // Second Column: Clamp Min
        ImGui::BeginGroup();
        ImGui::Text("Min");
        ImGui::VSliderInt("##Clamp_Min", ImVec2(40, 500), &imgui_clamp_min, -100, 0, "%d");
        ImGui::EndGroup();

        ImGui::SameLine(); 

        // Third Column: Clamp Max
        ImGui::BeginGroup();
        ImGui::Text("Max");
        ImGui::VSliderInt("##Clamp_Max", ImVec2(40, 500), &imgui_clamp_max, -100, 0, "%d");
        ImGui::EndGroup();

        ImGui::SameLine(); 

        // Fourth Column: Alpha
        ImGui::BeginGroup();
        ImGui::Text("Alpha");
        ImGui::VSliderFloat("##Alpha", ImVec2(40, 500), &imgui_alpha, 0.0f, 1.0f, "%.2f");
        ImGui::EndGroup();

    ImGui::End();

    }
    
    ImGui::SetNextWindowPos(ImVec2(0,500), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(660, 100), ImGuiCond_Always);
    ImGui::Begin("Info", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Maximum: %.1f", magnitude_max);
        ImGui::Text("Program FPS: %.1f", FPS);
        ImGui::Text("Camera FPS: %.1f", camFPS);
        ImGui::Checkbox("Options", &options_menu);

    ImGui::End();

    // Options menu
    
    if (options_menu == true) {
 
    ImGui::SetNextWindowPos(ImVec2(660,0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(364, 600), ImGuiCond_Always);
    ImGui::Begin("Options", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::Checkbox("Mark Max Mag", &mark_max_mag_state);
        ImGui::Checkbox("Color Scale", &color_scale_state);
        ImGui::Checkbox("Heat Map", &heat_map_state);
        ImGui::Checkbox("Data Clamp", &data_clamp_state);
        ImGui::Checkbox("Threshold", &threshold_state);

    ImGui::End();

    }

    //#ifdef UBUNTU
    // Render UI
    ImGui::Render();
    glViewport(0, 0, 1024, 600);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
    return true;
   // #endif

    //#ifdef PI
    //int winWidth, winHeight;
    //SDL_GetWindowSize(window, &winWidth, &winHeight);
    glViewport(0, 0, 1024, 600);

    // Clear buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render ImGui draw data
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return true;
    //#endif
}

//=====================================================================================

void video::shutdownIMGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}


//=====================================================================================

bool video::processFrame(Mat& data_input, const float lower_limit, const float upper_limit)
{
    /*
    - convert input_data to correct range
    - turn input_data into a heatmap
    - merge frame and input_data
    - draw UI
    */
   
    Mat newframe;
    
   if (getFrame(newframe)) {
    newframe.copyTo(frame);
   }
    
    // Creates heatmap from beamformed audio data, thresholds, clamps, and merges
    Mat frame_merged = createHeatmap(data_input, lower_limit, upper_limit, frame);
        
    // Draw UI onto frame
    Mat frame_merged_UI = drawUI(frame_merged);
        
    //imshow("Window", frame_merged_UI);
    FPSCalculator();
    
    //cout << "rendering imgui..." << endl;
    if (renderIMGui(frame_merged_UI) == false) {
            return false;
    }
    
    return true;
    //
} // end processData

//=====================================================================================S