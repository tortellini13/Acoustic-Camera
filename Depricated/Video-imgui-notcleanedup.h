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


using namespace std;
using namespace cv;
/*
    - initialize class
    - startCapture()
    - processData() in loop with audio
    - exit key in loop to break
    - stopCapture() outside loop
*/
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

    bool renderIMGui();
    
    void shutdownIMGui();

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

    // Variables for buttons
    //int list_max_mag_state = 1;
    //int mark_max_mag_state = 1;
    //int color_scale_state = 1;
    //int heat_map_state = 1;
    //int FPS_count_state = 1;
    //int data_clamp_state = 0;
    //int threshold_state = 0;
    // Variables for IMGUI buttons
    bool list_max_mag_state = true;
    bool mark_max_mag_state = true;
    bool color_scale_state = true;
    bool heat_map_state = true;
    bool FPS_count_state = true;
    bool data_clamp_state = true;
    bool threshold_state = true;

    int imgui_threshold = -50;
    int imgui_clamp_min = -100;
    int imgui_clamp_max = 0;
    float imgui_alpha = 0.5;

    // Button callbacks
  /*  void onListMaxMag(int state, void* user_data);
    void onMarkMaxMag(int state, void* user_data);
    void onColorScaleState(int state, void* user_data);
    void onHeatMap(int state, void* user_data);
    void onFPSCount(int state, void* user_data);
    void onDataClamp(int state, void* user_data);
    void onThresholdState(int state, void* user_data);
*/
    void UISetup();

    // For magnitude proccessing
    double magnitude_min;
    double magnitude_max;

   // int Lower_limit;
   // int Upper_limit;

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
    double FPS;

    SDL_Window* window = nullptr;
    
    SDL_GLContext gl_context;

    SDL_Event event;

};

//=====================================================================================

video::video(int frame_width, int frame_height, int frame_rate) :
    frame_width(frame_width),
    frame_height(frame_height),
    frame_rate(frame_rate),
    cap(0, CAP_V4L2),
    FPSTimer("FPS Timer") {}

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

    shutdownIMGui();
} // end stopCapture

//=====================================================================================

// Callbacks for UI Buttons
/*void video::onListMaxMag(int state, void* user_data) 
{list_max_mag_state = state;}

void video::onMarkMaxMag(int state, void* user_data) 
{mark_max_mag_state = state;}

void video::onColorScaleState(int state, void* user_data) 
{color_scale_state = state;}

void video::onHeatMap(int state, void* user_data) 
{heat_map_state = state;}

void video::onFPSCount(int state, void* user_data) 
{
    FPS_count_state = state;
    if (state == 1) {
        FPSTimer.start();
    }
    if (state == 0) {
        FPSTimer.end();
    }
    
} 

void video::onDataClamp(int state, void* user_data) 
{data_clamp_state = state;}

void video::onThresholdState(int state, void* user_data) 
{threshold_state = state;}
*/
//=====================================================================================

// Setup UI Buttons and trackbars

void video::UISetup() 
{   
    // Make a window for the buttons to go on
    drawColorBar(SCALE_WIDTH, SCALE_HEIGHT);

    if (startIMGui() == false) {
        cout << "IMGui init FAILED :(" << endl;
    }

    //#ifdef ENABLE_STATIC_DATA
    //staticTestFrame(RESOLUTION_WIDTH, RESOLUTION_HEIGHT, -100, 0);
    //#endif

    Mat initial_frame(Size(RESOLUTION_HEIGHT, RESOLUTION_WIDTH), CV_32FC1);

    while (!getFrame(initial_frame)) 
    {
        cerr << "Error: Could not retreive frame from capture thread!" << "\n";
        this_thread::sleep_for(chrono::seconds(1));
    }

    imshow("Window",initial_frame); 

    FPSTimer.start();

   
   /*
    cout << "window shown!" << endl;

    createTrackbar("Threshold", "Window", nullptr, (MAP_THRESHOLD_MAX + MAP_THRESHOLD_OFFSET));

    createTrackbar("Alpha", "Window", nullptr, 100);

    createTrackbar("Clamp minimum", "Window", nullptr, 100);

    createTrackbar("Clamp maximum", "Window", nullptr, 100);

    setTrackbarPos("Threshold", "Window", MAP_THRESHOLD_TRACKBAR_VAL);
    
    setTrackbarPos("Alpha", "Window", DEFAULT_ALPHA);

    setTrackbarPos("Clamp minimum", "Window", 0);

    setTrackbarPos("Clamp maximum", "Window", 100);

    
    // Maximum magnitude button
    createButton("List Maximum Magnitude", 
        // Lambda function to call onListMaxMag
        [](int state, void* user_data) 
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onListMaxMag(state, user_data);
        }, 
        this, QT_CHECKBOX, 1);
    
    // Mark maximum magnitude button
    createButton("Mark Maximum Magnitude", 
        [](int state, void* user_data)
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onMarkMaxMag(state, user_data);
        },
        this, QT_CHECKBOX, 1);

    // Show color scale button
    createButton("Show Color Scale",
        [](int state, void* user_data)
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onColorScaleState(state, user_data);
        },
        this, QT_CHECKBOX, 1);

    // Show heat map button
    createButton("Show Heat Map",
        [](int state, void* user_data)
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onHeatMap(state, user_data);
        },
        this, QT_CHECKBOX, 1);

    // Show fps button
    createButton("Show fps",
        [](int state, void* user_data)
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onFPSCount(state, user_data);
        }, 
        this, QT_CHECKBOX, 1);

    createButton("Data Clamp",
        [](int state, void* user_data)
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onDataClamp(state, user_data);
        }, 
        this, QT_CHECKBOX, 1);

    createButton("Threshold",
        [](int state, void* user_data)
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onThresholdState(state, user_data);
        }, 
        this, QT_CHECKBOX, 1);

        //FPSTimer.start();
    */
} // end UISetup

//=====================================================================================

void video::captureVideo(VideoCapture& cap, atomic<bool>& is_running) 
{
    while (is_running) 
    {
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
    }

    destroyAllWindows();
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
    #ifdef ENABLE_RANDOM_DATA
    randu(data_input, Scalar(-100), Scalar(0));
    #endif
    #ifdef ENABLE_STATIC_DATA
    //resize(static_test_frame, static_test_frame, data_input.size());
    //Mat test_frame = staticTestFrame(NUM_ANGLES, NUM_ANGLES, -100, 0);
    
    double st_height = NUM_ANGLES;
    double st_width = NUM_ANGLES;
    double st_max = 0;
    double st_min = -100;
    Mat static_test_frame(st_height, st_width, CV_32F);

    for(int i = 0; i < st_width; ++i) {
        for(int j = 0; j < st_height; ++j) {
        float value;
        float distance = sqrt(((st_width / 2 - i)*(st_width / 2 - i)) + ((st_height / 2 - j)*(st_height / 2 - j)));
        if (distance == st_width) {
            value = st_min;
        } else {
            value = static_cast<float>(st_max - st_min) * (1 - static_cast<float>(distance / st_width));
            
        }

        static_test_frame.at<float>(j, i) = value;

        cout << value << endl;

        }
    }
    static_test_frame.copyTo(data_input);
    #endif
    
    
    if (data_clamp_state == true) 
    {
    Mat data_clamped;
    

    data_input.convertTo(data_clamped, CV_32F);

    /*
    if (Lower_limit != imgui_clamp_min) {
        
        Lower_limit = imgui_clamp_min;
        
        if (Lower_limit >= Upper_limit) {
            if (Upper_limit == -100) {
                Upper_limit = -99;
            }
            if (Lower_limit == 0) {
                Lower_limit = -1;    
            }
        Upper_limit = Lower_limit + 1;

        imgui_clamp_min = Lower_limit;
        imgui_clamp_max = Upper_limit;

        }
    }

    if (Upper_limit != imgui_clamp_max) {
        
        Upper_limit = imgui_clamp_max;
        
        if (Lower_limit >= Upper_limit) {
            
            if (Upper_limit == -100) {
                Upper_limit = -99;
            }
            
            if (Lower_limit == 0) {
                Lower_limit = -1;    
            }
        
        Lower_limit = Upper_limit - 1;

        imgui_clamp_min = Lower_limit;
        imgui_clamp_max = Upper_limit;
        }
    }
        */

        if (imgui_clamp_min >= imgui_clamp_min) {
            if(imgui_clamp_min == 0) {
                imgui_clamp_min = -1;
                imgui_clamp_max = 0;
            }
            if(imgui_clamp_max == -100) {
                imgui_clamp_max = -99;
                imgui_clamp_min = -100;
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
    applyColorMap(heatmap_input, heatmap, COLORMAP_INFERNO);

    Mat frame_merged;
    
    if(heat_map_state == false) 
    {   
        frame.copyTo(frame_merged);
    }

    if(heat_map_state == true) 
    {
    // Ensure the heatmap is the same size as the frame
    Mat resized_heatmap;
    Mat resized_data_input;
    resize(heatmap, resized_heatmap, frame.size());
    resize(data_input, resized_data_input, frame.size());

    // Blend the heatmap and the frame with an alpha value
    //double alpha2 = static_cast<double>(getTrackbarPos("Alpha", "Window"))/100;
    
    if (threshold_state == false) {
    addWeighted(frame, 1.0, resized_heatmap, imgui_alpha, 0.0, frame_merged);
    }
    if(threshold_state == true) {
        // Get threshold value of trackbar
        //int thresholdValue = (getTrackbarPos("Threshold", "Window") - MAP_THRESHOLD_OFFSET);
        
        for(int y = 0; y < resized_heatmap.rows; ++y)
            {
                for (int x = 0; x < resized_heatmap.cols; ++x) 
                {
                    
                    if (resized_data_input.at<float>(y, x) > imgui_threshold) 
                    {
                        Vec3b& pixel = frame.at<Vec3b>(y, x);         // Current pixel in camera
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
        
        frame.copyTo(frame_merged);
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
    applyColorMap(scaleColor, color_bar, COLORMAP_INFERNO);
    
} // end drawColorBar

//=====================================================================================

void video::FPSCalculator()
{
    FPSTimer.end();

    double FPSTime = FPSTimer.time_avg(AVG_SAMPLES, false);
    if (FPSTime != -1) 
    { FPS = 1/FPSTime; }
    
    //cout << FPSTime << endl;
    //cout << FPSTimer.getCurrentAvgCount() << endl;
    FPSTimer.start();
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
    
    if (list_max_mag_state == true)
    {
        Point max_text_location(MAX_LABEL_POS_X, MAX_LABEL_POS_Y);
        ostringstream max_magnitude_stream;
        String max_magnitude_string;
        int text_baseline = 0;
    
        max_magnitude_stream << fixed << setprecision(LABEL_PRECISION) << magnitude_max;
        max_magnitude_string = "Maximum = " + max_magnitude_stream.str();

        Size textSize = getTextSize(max_magnitude_string, FONT_TYPE, FONT_SCALE, FONT_THICKNESS, &text_baseline);
        
        rectangle(data_input, max_text_location + Point(0, text_baseline), max_text_location + Point(textSize.width, -textSize.height), Scalar(0, 0, 0), FILLED); //Draw rectangle for text
        putText(data_input, max_magnitude_string, max_text_location + Point(0, +5), FONT_TYPE, FONT_SCALE, Scalar(255, 255, 255), FONT_THICKNESS); //Write text for maximum magnitude
    } // end max mag
    
    // Draw FPS box and text
    if (FPS_count_state == true) {
        int FPSBaseline = 0;
        Point FPSTextLocation(20, 460);
        string FPS_string;
        ostringstream FPS_stream;
        
        FPSCalculator();
        FPS_stream << fixed << setprecision(LABEL_PRECISION) << FPS;
        FPS_string = "FPS: " + FPS_stream.str();
        Size FPStextSize = getTextSize(FPS_string, FONT_TYPE, FONT_SCALE, FONT_THICKNESS, &FPSBaseline);
        
        rectangle(data_input, FPSTextLocation + Point(0, 6), FPSTextLocation + Point(80, - 10 - 3), Scalar(0, 0, 0), FILLED); //Draw rectangle for text
        putText(data_input, FPS_string, FPSTextLocation, FONT_TYPE, FONT_SCALE, Scalar(255, 255, 255), FONT_THICKNESS); //Write text for FPS
    } // end fps
    
    return data_input;
} // end drawUI

//=====================================================================================

// Function to initialize IMGui
bool video::startIMGui() {
    // Initialize SDL

    //cout << "starting imgui..." << endl;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Error: SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create an OpenGL window
    window = SDL_CreateWindow("IMGui Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 430, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
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
    cout << "finsihed starting imgui..." << endl;
    return true;
}

//=====================================================================================

bool video::renderIMGui() {
    // Start a new frame
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event); // Pass events to ImGui

        if (event.type == SDL_QUIT) {
           return false;
        }
    }
    
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(290, 480), ImGuiCond_Always);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        // First Column: Threshold
        ImGui::BeginGroup(); // Start grouping for alignment
        ImGui::Text("Threshold");
        ImGui::VSliderInt("##Threshold", ImVec2(50, 420), &imgui_threshold, -100, 0, "%d");
        ImGui::EndGroup();

        ImGui::SameLine(); // Move to the next column

        // Second Column: Clamp Min
        ImGui::BeginGroup();
        ImGui::Text("Clamp Min");
        ImGui::VSliderInt("##Clamp_Min", ImVec2(50, 420), &imgui_clamp_min, -100, 0, "%d");
        ImGui::EndGroup();

        ImGui::SameLine(); 

        // Third Column: Clamp Max
        ImGui::BeginGroup();
        ImGui::Text("Clamp Max");
        ImGui::VSliderInt("##Clamp_Max", ImVec2(50, 420), &imgui_clamp_max, -100, 0, "%d");
        ImGui::EndGroup();

        ImGui::SameLine(); 

        // Fourth Column: Alpha
        ImGui::BeginGroup();
        ImGui::Text("Alpha");
        ImGui::VSliderFloat("##Alpha", ImVec2(50, 420), &imgui_alpha, 0.0f, 1.0f, "%.2f");
        ImGui::EndGroup();

    ImGui::End();

    // Create a simple window
    ImGui::SetNextWindowPos(ImVec2(290,0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(140, 480), ImGuiCond_Always);
    ImGui::Begin("Options", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::Checkbox("List Max Mag", &list_max_mag_state);
        ImGui::Checkbox("Mark Max Mag", &mark_max_mag_state);
        ImGui::Checkbox("Color Scale", &color_scale_state);
        ImGui::Checkbox("Heat Map", &heat_map_state);
        ImGui::Checkbox("FPS Count", &FPS_count_state);
        ImGui::Checkbox("Data Clamp", &data_clamp_state);
        ImGui::Checkbox("Threshold", &threshold_state);
    ImGui::End();

    // Render UI
    ImGui::Render();
    glViewport(0, 0, 400, 600);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
    return true;
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
   
    Mat frame;
    if (getFrame(frame)) 
    {
        // Creates heatmap from beamformed audio data, thresholds, clamps, and merges
        Mat frame_merged = createHeatmap(data_input, lower_limit, upper_limit, frame);
        
        // Draw UI onto frame
        Mat frame_merged_UI = drawUI(frame_merged);
        
        imshow("Window", frame_merged_UI);
        //cout << "rendering imgui..." << endl;
        if (renderIMGui() == false) {
            return false;
        }
    }

    return true;
    //
} // end processData

//=====================================================================================S