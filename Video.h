#ifndef VIDEO_H
#define VIDEO_H

// Internal Libraries
#include <iostream>
#include <ctime>

// External Libraries
#include <opencv2/opencv.hpp>


// Header Files
#include "PARAMS.h"
#include "Timer.h"

using namespace std;
using namespace cv;

//=====================================================================================

class video
{
public:
    // Initialize video class
    video(int frame_width, int frame_height, int frame_rate, int initial_heatmap_threshold, int initial_heatmap_alpha);

    // Clear memory for all arrays
    ~video();

    // Creates window
    void initializeWindow();

    // Captures a frame of video and renders all elements
    Mat processFrame(Mat& data_input, int codec, string video_file_name);


private:
    /* Naming Key:
       - frame = the entire rendered frame
       - magnitude = refers to the actual input audio data
       - heat map = refers to objects related to the rendered heat map
    */
   timer proccessFrame_time;

    // Required for OpenCV
    void onListMaxMag(int state, void* user_data);
    void onMarkMaxMag(int state, void* user_data);
    void onColorScaleState(int state, void* user_data);
    void onHeatMap(int state, void* user_data);
    void onFPSCount(int state, void* user_data);
    void onRecording(int state, void* user_data);
    void onResetUI(int state, void* user_data);

    // Creates color bar
    void makeColorBar(int scale_height, int scale_width);

    // Draws UI elements
    Mat drawUI(string maximum_text, double magnitude_max, double magnitude_min);

    // Generates Static Elements of UI
    void generateUI();

    // Converts a float2D to a Mat
    Mat float2DtoMat(float2D& data_input);

    // Input Parameters
    int frame_width;
    int frame_height;
    int frame_rate;
    int heatmap_threshold;
    int heatmap_alpha;

    // From slider ***maybe change***
    double alpha;
    int threshold_value;

    // Initialize classes
    VideoCapture cap;
    VideoWriter video1;

    // Color bar
    Mat color_bar;

    // Storage for the frame
    Mat frame;
    Mat frame_RGB;
    Mat frame_RGBA;
    Mat heat_map_data_norm;
    Mat heat_map_RGB;
    Mat heat_map_RGBA;
    Mat blended;
    Mat static_UI;

    // Min and max magnitude
    double magnitude_min;
    double magnitude_max;

    // Coords for max magnitude
    Point max_coord;        // Coords from data
    Point max_point_scaled; // Coords scaled to frame
   // Point textSize(1,1);

    // Variables
    double fps_time_start;
    double fps_time_end;
    double fps = 0;
    int frame_count = 0;
    int text_baseline = 0;
    
    // Configuration of callbacks for buttons
    int list_max_mag_state = 1;
    int mark_max_mag_state = 1;
    int color_scale_state = 1;
    int heat_map_state = 1;
    int FPS_count_state = 1;
    int reset_ui_state = 0;
    int ui_change_flag = 1;
    int recording_state = 0;
    int recording_state_change_flag = 0;
};

//=====================================================================================

video::video(int frame_width, int frame_height, int frame_rate, int initial_heatmap_threshold, int initial_heatmap_alpha):
    frame_width(frame_width),
    frame_height(frame_height),
    frame_rate(frame_rate),
    heatmap_threshold(initial_heatmap_threshold),
    heatmap_alpha(initial_heatmap_alpha),
    cap(0, CAP_V4L2),
    proccessFrame_time("proccessFrame")

    // Allocate memory for all arrays
    {
        
    };

//=====================================================================================

video::~video()
{
    cap.release();
    video1.release();
} // end ~video

//=====================================================================================


void video::onListMaxMag(int state, void* user_data) 
{
    list_max_mag_state = state;
}

void video::onMarkMaxMag(int state, void* user_data) 
{
    mark_max_mag_state = state;
}

void video::onColorScaleState(int state, void* user_data) 
{
    color_scale_state = state;
}

void video::onHeatMap(int state, void* user_data) 
{
    heat_map_state = state;
}

void video::onFPSCount(int state, void* user_data) 
{
    FPS_count_state = state;
}

void video::onRecording(int state, void* user_data) 
{
    recording_state = state;
    recording_state_change_flag = 1;
}

void video::onResetUI(int state, void* user_data) 
{
    // Set initial trackbar positions
    setTrackbarPos("Threshold", "Heat Map Overlay", heatmap_threshold);
    setTrackbarPos("Alpha", "Heat Map Overlay", heatmap_alpha);
    fps_time_start = getTickCount();
    ui_change_flag = 1;
}

//=====================================================================================

void video::initializeWindow() 
{   
    cap >> frame;
    cap.set(CAP_PROP_FRAME_WIDTH, frame_width);   // Set frame width for capture
    cap.set(CAP_PROP_FRAME_HEIGHT, frame_height); // Set frame height for capture
    cap.set(CAP_PROP_FPS, frame_rate);            // Set frame rate
    imshow("Heat Map Overlay", frame);            // Show frame

    // Create trackbar and buttons
    createTrackbar("Threshold", "Heat Map Overlay", nullptr, MAP_THRESHOLD_MAX);    
    createTrackbar("Alpha", "Heat Map Overlay", nullptr, 100);
    
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

    // Reset UI button
    createButton("ResetUI",
        [](int state, void* user_data)
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onResetUI(state, user_data);
        }, 
        this, QT_CHECKBOX, 0);

    // Record video button
    createButton("Record Video",
        [](int state, void* user_data)
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onRecording(state, user_data);
        }, 
        this, QT_CHECKBOX, 0);

    // Reset UI
    onResetUI(1,0);
}

//=====================================================================================

void video::generateUI() {
    if (ui_change_flag == 1) {

        if (list_max_mag_state == 1) {
        Point max_text_location(MAX_LABEL_POS_X, MAX_LABEL_POS_Y);

        //rectangle(static_UI, max_text_location + Point(0, text_baseline), max_text_location + Point(textSize.width, -textSize.height), Scalar(0, 0, 0), FILLED); //Draw rectangle for text 
    }


    if (color_scale_state == 1) 
    {
        makeColorBar(SCALE_HEIGHT, SCALE_WIDTH);
        rectangle(static_UI, Point(SCALE_POS_X, SCALE_POS_Y) + Point(-SCALE_BORDER, -SCALE_BORDER - 10), Point(SCALE_POS_X, SCALE_POS_Y) + Point(SCALE_WIDTH + SCALE_BORDER - 1, SCALE_HEIGHT + SCALE_BORDER + 5), Scalar(0, 0, 0), FILLED); //Draw rectangle behind scale to make a border
        color_bar.copyTo(static_UI(Rect(SCALE_POS_X, SCALE_POS_Y, SCALE_WIDTH, SCALE_HEIGHT))); //Copy the scale onto the image

        // Draw text indicating various points on the scale
        float scaleTextRatio = (1 / static_cast<float>(SCALE_POINTS ));
        
        for (int i = 0; i < (SCALE_POINTS + 1); i++) 
        {
            Point scaleTextStart(SCALE_POS_X, (SCALE_POS_Y + ((1 - static_cast<double>(i) * scaleTextRatio) * SCALE_HEIGHT) - 3)); //Starting point for the text
            line(static_UI, scaleTextStart + Point(0,3), scaleTextStart + Point(SCALE_WIDTH, 3), Scalar(255, 255, 255), 1, 8, 0);
        }



    }
        ui_change_flag = 0;
    }
}


void video::makeColorBar(int scale_height, int scale_width) 
{
    color_bar = Mat(scale_height, scale_width, CV_8UC3);
    Mat scaleColor(scale_height, scale_width, CV_8UC1);

    for (int y = 0; y < scale_height; y++) 
    {
        int scaleIntensity = 255 * (static_cast<double>((scale_height - y)) / static_cast<double>(scale_height));
        for(int x = 0; x < scale_width; x++) 
        {
            scaleColor.at<uchar>(y, x) = scaleIntensity;
        }
    }
    applyColorMap(scaleColor, color_bar, COLORMAP_INFERNO);
} // end makeColorBar

//=====================================================================================

Mat video::drawUI(string maximum_text, double magnitude_max, double magnitude_min) 
{
    // Graphics and Text
    cvtColor(frame_RGBA, frame_RGB, COLOR_RGBA2RGB);
    
    if (list_max_mag_state == 1)
    {
        Point max_text_location(MAX_LABEL_POS_X, MAX_LABEL_POS_Y);

        Size textSize = getTextSize(maximum_text, FONT_TYPE, FONT_SCALE, FONT_THICKNESS, &text_baseline);
        
        rectangle(frame_RGB, max_text_location + Point(0, text_baseline), max_text_location + Point(textSize.width, -textSize.height), Scalar(0, 0, 0), FILLED); //Draw rectangle for text
        putText(frame_RGB, maximum_text, max_text_location + Point(0, +5), FONT_TYPE, FONT_SCALE, Scalar(255, 255, 255), FONT_THICKNESS); //Write text for maximum magnitude
    }

    if (mark_max_mag_state == 1) 
    {
        drawMarker(frame_RGB, max_point_scaled, Scalar(255, 255, 255), MARKER_CROSS, CROSS_SIZE, CROSS_THICKNESS, 8); //Mark the maximum magnitude point
    }

    if (color_scale_state == 1) 
    {
        rectangle(frame_RGB, Point(SCALE_POS_X, SCALE_POS_Y) + Point(-SCALE_BORDER, -SCALE_BORDER - 10), Point(SCALE_POS_X, SCALE_POS_Y) + Point(SCALE_WIDTH + SCALE_BORDER - 1, SCALE_HEIGHT + SCALE_BORDER + 5), Scalar(0, 0, 0), FILLED); //Draw rectangle behind scale to make a border
        color_bar.copyTo(frame_RGB(Rect(SCALE_POS_X, SCALE_POS_Y, SCALE_WIDTH, SCALE_HEIGHT))); //Copy the scale onto the image

         //Draw text indicating various points on the scale
        float scaleTextRatio = (1 / static_cast<float>(SCALE_POINTS ));
        for (int i = 0; i < (SCALE_POINTS + 1); i++) 
        {
            Point scaleTextStart(SCALE_POS_X, (SCALE_POS_Y + ((1 - static_cast<double>(i) * scaleTextRatio) * SCALE_HEIGHT) - 3)); //Starting point for the text
            double scaleTextValue = ((static_cast<double>(magnitude_max - magnitude_min) * scaleTextRatio * static_cast<double>(i)) + magnitude_min); //Value of text for each point

            ostringstream scaleTextStream;
            scaleTextStream << fixed << setprecision(LABEL_PRECISION) << scaleTextValue;
            String scaleTextString = scaleTextStream.str() + " ";

            putText(frame_RGB, scaleTextString, scaleTextStart, FONT_TYPE, FONT_SCALE - 0.2, Scalar(255, 255, 255), FONT_THICKNESS);
            line(frame_RGB, scaleTextStart + Point(0,3), scaleTextStart + Point(SCALE_WIDTH, 3), Scalar(255, 255, 255), 1, 8, 0);
        }
    }
    return frame_RGB;
} // end drawUI

//=====================================================================================

// Merges heat map into the frame and applies alpha
Mat heatMapAlphaMerge(Mat heat_map_data, Mat heat_map_RGBA, Mat frame_RGBA, int heatmap_threshold_value, double alpha) 
{
    for (int y = 0; y < heat_map_data.rows; y++)
        {
            for (int x = 0; x < heat_map_data.cols; x++) 
            {
                if (heat_map_data.at<uchar>(y, x) > heatmap_threshold_value) 
                {
                    Vec4b& pixel = frame_RGBA.at<Vec4b>(y, x);         // Current pixel in camera
                    Vec4b heatMapPixel = heat_map_RGBA.at<Vec4b>(y, x); // Current pixel in heatmap

                    // Loops through R G & B channels
                    for (int c = 0; c < 3; c++) 
                    {
                        // Merges heatmap with video and applies alpha value
                        pixel[c] = static_cast<uchar>(alpha * heatMapPixel[c] + (1 - alpha) * pixel[c]);
                    } 
                }
            }
        } // end alphaMerge
    return frame_RGBA;
} // end heatMapAlphaMerge

Mat video::processFrame(Mat& magnitude_frame, int codec, string video_file_name) 
{
    proccessFrame_time.start();
    // Capture a frame from the camera
    cap >> frame;
    
    Mat heat_map_data(Size(RESOLUTION_HEIGHT, RESOLUTION_WIDTH), CV_32FC1); // Make heat map data matrix
    //Mat magnitude_frame(NUM_ANGLES, NUM_ANGLES, CV_32FC1, Scalar(0));       // Single channel, magnitude matrix, initialized to 0
    
/*      if (recordingStateChangeFlag == 1) {
             if (recordingState == 1) {
                 ostringstream videoFileStream;
                 videoFileStream << "./" << video_file_name << " " << time(nullptr) << ".avi" ;
                 string videoFileFullOutput = videoFileStream.str();
                 video1.open(videoFileFullOutput, codec, FRAME_RATE, Size(RESOLUTION_WIDTH, RESOLUTION_HEIGHT), 1);
             }
             if (recordingState == 0) {
                 video1.release();
             }
             recordingStateChangeFlag = 0;
         } */

    //randu(magnitude_frame, Scalar(0), Scalar(300));

    //==============================================================================================

    // ***SWAP MAGNITUDE_FRAME FOR DATA_INPUT WHEN READY TO SWITCH TO MICROPHONES***

    // Magnitude data processing
    resize(magnitude_frame, heat_map_data, 
           Size(RESOLUTION_WIDTH, RESOLUTION_HEIGHT), 0, 0, INTER_CUBIC); // Scaling and interpolating into camera resolution
    normalize(heat_map_data, heat_map_data_norm, 0, 255, NORM_MINMAX);    // Normalize the data into a (0-255) range
    heat_map_data_norm.convertTo(heat_map_data_norm, CV_8UC1);            // Convert normalized heat map data data type

    //==============================================================================================

    // Find coord of max and min magnitude
    minMaxLoc(magnitude_frame, &magnitude_min, &magnitude_max, NULL, &max_coord);
    
    // Scale max point to video resolution
    max_point_scaled.x = (static_cast<double>(max_coord.x) / static_cast<double>(magnitude_frame.cols)) * RESOLUTION_WIDTH; 
    max_point_scaled.y = (static_cast<double>(max_coord.y) / static_cast<double>(magnitude_frame.rows)) * RESOLUTION_HEIGHT;

    // Prepare maximum value to be printed
    ostringstream mag_max_stream;
    String mag_max_string;
    
    mag_max_stream << fixed << setprecision(LABEL_PRECISION) << magnitude_max;
    mag_max_string = "Maximum = " + mag_max_stream.str();
   
    
    //==============================================================================================
    
    // Colormap creation
    applyColorMap(heat_map_data_norm, heat_map_RGB, COLORMAP_INFERNO); // Convert the normalized heat map data into a colormap
    cvtColor(heat_map_RGB, heat_map_RGBA, COLOR_RGB2RGBA);             // Convert heat map from RGB to RGBA
    cvtColor(frame, frame_RGBA, COLOR_RGB2RGBA);                       // Convert video frame from RGB to RGBA
    
    //==============================================================================================

    // Compare value of input array to threshold. Don't render heatmap if below threshold
    threshold_value = getTrackbarPos("Threshold", "Heat Map Overlay");
    alpha = static_cast<double>(getTrackbarPos("Alpha", "Heat Map Overlay")) / 100;
    
    if (heat_map_state == 1) 
    {
        frame_RGBA = heatMapAlphaMerge(heat_map_data, heat_map_RGBA, frame_RGBA, threshold_value, alpha);
    }
    
    // Creates color bar
    //makeColorBar(SCALE_HEIGHT, SCALE_WIDTH);
    //generateUI();
    // Draws UI
    frame_RGB = drawUI(mag_max_string, magnitude_max, magnitude_min);
    
    
    if (FPS_count_state == 1) 
    {
        frame_count++;
        if (frame_count == 10) {
            fps_time_end = getTickCount();
            double FPSTimeDifference = (fps_time_end - fps_time_start) / getTickFrequency();
            fps_time_start = fps_time_end;
            fps = frame_count / FPSTimeDifference;
            frame_count = 0;
        }


        ostringstream fps_stream;
        String fps_string;
        fps_stream << fixed << setprecision(LABEL_PRECISION) << fps;
        fps_string = "FPS: " + fps_stream.str();
        int FPSBaseline = 0;
        Point FPSTextLocation(20, 460);
        Size FPStextSize = getTextSize(fps_string, FONT_TYPE, FONT_SCALE, FONT_THICKNESS, &FPSBaseline);
        
        rectangle(frame_RGB, FPSTextLocation + Point(0, 6), FPSTextLocation + Point(80, - 10 - 3), Scalar(0, 0, 0), FILLED); //Draw rectangle for text
        putText(frame_RGB, fps_string, FPSTextLocation, FONT_TYPE, FONT_SCALE, Scalar(255, 255, 255), FONT_THICKNESS); //Write text for FPS
    }
    
    proccessFrame_time.end();
    proccessFrame_time.print();
    
    return frame_RGB;

}


#endif