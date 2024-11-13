#ifndef BEAMFORM
#define BEAMFORM

// Internal Libraries
#include <iostream>

// External Libraries
#include <opencv2/opencv.hpp>


// Header Files
#include "PARAMS.h"

using namespace std;
using namespace cv;

//=====================================================================================

class video
{
public:
    // Initialize video class
    video(int other);

    // Clear memory for all arrays
    ~video();

    // Creates window
    void initializeWindow();

private:
    // Functions
    void onListMaxMag(int state, void* userdata);
    void onMarkMaxMag(int state, void* userdata);
    void onColorScaleState(int state, void* userdata);
    void onHeatMap(int state, void* userdata);
    void onFPSCount(int state, void* userdata);
    void onRecording(int state, void* userdata);
    void onResetUI(int state, void* userdata);

    // Variables
    double FPSTimeStart;
    double FPS = 0;
    int frameCount = 0;
    
    // Configuration of callbacks for buttons
    int listMaxMagState = 1;
    int markMaxMagState = 1;
    int colorScaleState = 1;
    int heatMapState = 1;
    int FPSCountState = 1;
    int resetUIState = 0;
    int recordingState = 0;
    int recordingStateChangeFlag = 0;

    int thing;

};

//=====================================================================================

video::video(int other):
    thing(other)

    
    // Allocate memory for all arrays
    {
        
    };

//=====================================================================================

video::~video()
{

} // end ~video

//=====================================================================================
 VideoCapture cap(0, CAP_V4L2);
 VideoWriter video1;


void video::onListMaxMag(int state, void* userdata) {
    listMaxMagState = state; // Update button state
}
void video::onMarkMaxMag(int state, void* userdata) {
    markMaxMagState = state; // Update button state
}
void video::onColorScaleState(int state, void* userdata) {
    colorScaleState = state;
}
void video::onHeatMap(int state, void* userdata) {
    heatMapState = state;
  
}
void video::onFPSCount(int state, void* userdata) {
    FPSCountState = state;
}
void video::onRecording(int state, void* userdata) {
    recordingState = state;
    recordingStateChangeFlag = 1;
}
void video::onResetUI(int state, void* userdata) {
    //Set initial trackbar positions
    setTrackbarPos("Threshold", "Heat Map Overlay", DEFAULT_THRESHOLD);
    setTrackbarPos("Alpha", "Heat Map Overlay", DEFAULT_ALPHA);   
    FPSTimeStart = getTickCount();
}

void video::initializeWindow() {
    Mat frame;
   
    cap >> frame;
    cap.set(CAP_PROP_FRAME_WIDTH, RESOLUTION_WIDTH);   // Set frame width for capture
    cap.set(CAP_PROP_FRAME_HEIGHT, RESOLUTION_HEIGHT); // Set frame height for capture
    cap.set(CAP_PROP_FPS, FRAME_RATE);
    imshow("Heat Map Overlay", frame);
    // Create trackbar and buttons
    createTrackbar("Threshold", "Heat Map Overlay", nullptr, MAP_THRESHOLD_MAX);    
    createTrackbar("Alpha", "Heat Map Overlay", nullptr, 100);
    createButton("List Maximum Magnitude", [](int state, void* userdata) {
    auto* vid = static_cast<video*>(userdata);
    vid->onListMaxMag(state, userdata);  // Call the instance function
}, this, QT_CHECKBOX, 1);
    // createButton("Mark Maximum Magnitude",onMarkMaxMag, NULL, QT_CHECKBOX,1);
    // createButton("Show Color Scale",onColorScaleState, NULL, QT_CHECKBOX,1);
    // createButton("Show Heat Map",onHeatMap, NULL, QT_CHECKBOX,1);
    // createButton("Show FPS", onFPSCount, NULL, QT_CHECKBOX, 1);
    // //createButton("ResetUI", onResetUI, NULL, QT_CHECKBOX, 0);
    // createButton("Record Video", onRecording, NULL, QT_CHECKBOX, 0);

    onResetUI(1,0);
    
}
//=====================================================================================

#endif