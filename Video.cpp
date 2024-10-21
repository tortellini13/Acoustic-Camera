#include <opencv2/opencv.hpp>
#include <iostream>
#include "PARAMS.h"
#include "sharedMemory.h"
#include <vector>

using namespace cv;
using namespace std;

//==============================================================================================
int listMaxMagState = 1;
int markMaxMagState = 1;
int colorScaleState = 1;

void onListMaxMag(int state, void* userdata) {
    listMaxMagState = state; // Update button state
}
void onMarkMaxMag(int state, void* userdata) {
    markMaxMagState = state; // Update button state
}
void onColorScaleState(int state, void* userdata) {
    colorScaleState = state;
}
//==============================================================================================


int main() 
{
    //==============================================================================================
    
    // Initializes array to receive data from Audio
    vector<vector<float>> magnitudeInput(NUM_ANGLES, vector<float>(NUM_ANGLES));   

    // Initializes array to send user config to Audio. Filled with default settings
    vector<int> userConfigs(NUM_CONFIGS, 0);     // ***Might need to manually set default settings     
    
    // Initialize shared memory class
    sharedMemory shm(AUDIO_SHM, CONFIG_SHM, SEM_1, SEM_2, NUM_ANGLES, NUM_ANGLES, NUM_CONFIGS);   


    
    /* Configs Legend
    0. 0 = Broadband
       1 = Full octave
       2 = Third Octave 
    1. 0 - 8 Full octave band selection
    2. 0 - 26 Third octave band selection
    3. 0 = Is not recording
       1 = Is recording
    4. 
    5. 
    */

    //==============================================================================================

    // Open shared memory for Audio
    if (!shm.shmStart2()) 
    {                                                             
        cerr << "2. shmStart2 Failed\n";
    }
    //cout << "Shared Memory Configured.\n"; // For debugging

    //==============================================================================================
    // Setup video capture
    
    VideoCapture cap(0, CAP_V4L2);
    VideoWriter video1;
    
    // Video frame and capture settings
    cap.set(CAP_PROP_FRAME_WIDTH, RESOLUTION_WIDTH);   // Set frame width for capture
    cap.set(CAP_PROP_FRAME_HEIGHT, RESOLUTION_HEIGHT); // Set frame height for capture
    cap.set(CAP_PROP_FPS, FRAME_RATE);                 // Set framerate of capture from PARAMS.h

    int recording = 0; // Recording setting
    int codec = VideoWriter::fourcc('M', 'J', 'P', 'G'); // Set codec of video recording
    string videoFileName = "./testoutput.avi";           // Set video file output file name


    // Open recording if enabled, report error if it doesn't work
    if (recording == 1) 
    {
        video1.open(videoFileName, codec, FRAME_RATE, Size(RESOLUTION_WIDTH, RESOLUTION_HEIGHT), 1);
        
        if (!video1.isOpened()) 
        {
            cerr << "Could not open the output video file for write.\n";
            return 1;
        }
    }

    //initialize a ton of stuff
    double magnitudeMin;
    double magnitudeMax;
    Point maxPoint;
    Mat heatMapData, heatMapDataNormal, heatMapRGB, heatMapRGBA, blended, frameRGBA, frame;  // Establish matricies for 
    heatMapData = Mat(frame.size(), CV_32FC1);                                                          // Make heat map data matrix
    Mat magnitudeFrame(NUM_ANGLES, NUM_ANGLES, CV_32FC1, Scalar(0)); // Single channel, magnitude matrix, initialized to 0

    

    //==============================================================================================
    
    // ***Why are you reassigning them?***
    int thresholdValue;
    int threshTrackMax = MAP_THRESHOLD_MAX;
    int threshTrackMin = MAP_THRESHOLD_MIN;

    //==============================================================================================
    
    //Initially open window
    cap >> frame;
    imshow("Heat Map Overlay", frame);

    // Create trackbar and buttons
    createTrackbar("Threshold", "Heat Map Overlay", nullptr, MAP_THRESHOLD_MAX);    
    createButton("List Maximum Magnitude",onListMaxMag, NULL, QT_CHECKBOX,1);
    createButton("Mark Maximum Magnitude",onMarkMaxMag, NULL, QT_CHECKBOX,1);
    createButton("Show Color Scale",onColorScaleState, NULL, QT_CHECKBOX,1);

    
    //int i = 15; //temp int for testing 
    Mat colorBar(SCALE_HEIGHT, SCALE_WIDTH, CV_8UC3);
    Mat scaleColor(SCALE_HEIGHT, SCALE_WIDTH, CV_8UC1);

    for(int y = 0; y < SCALE_HEIGHT; y++) {

        int scaleIntensity = 255 * (static_cast<double>((SCALE_HEIGHT - y)) / static_cast<double>(SCALE_HEIGHT));
        
        for(int x = 0; x < SCALE_WIDTH; x++) {
            scaleColor.at<uchar>(y, x) = scaleIntensity;

        }
    }
    
    applyColorMap(scaleColor, colorBar, COLORMAP_INFERNO);
    cvtColor(colorBar, colorBar, COLOR_RGB2RGBA);
    Point colorBarPosition(SCALE_POS_X, SCALE_POS_Y);

    while                                                                                                                                                                                                                                                                                                                                                                                                                           (1) 
    {
        cap >> frame; // Capture the frame
        if(frame.empty()) 
        {
           cout << "Frame is empty ;(\n";
        } 
        //cout << "Frame Captured!\n"; // For debugging
        
        //==============================================================================================       
        
        for (int rows = 0; rows < NUM_ANGLES; rows++) // Converts vector<vector<float>> into an OpenCV matrix
        { 
            for (int columns = 0; columns < NUM_ANGLES; columns++)
            {
                magnitudeFrame.at<int>(rows, columns) = magnitudeInput[rows][columns];
            }
        } 

        // Generates random data for testing
        randu(magnitudeFrame, Scalar(0), Scalar(300));

        //==============================================================================================

        // Magnitude Data Proccessing
        resize(magnitudeFrame, heatMapData, Size(RESOLUTION_WIDTH, RESOLUTION_HEIGHT), 0, 0, INTER_CUBIC);       // Scaling and interpolating into camera resolution
        normalize(heatMapData, heatMapDataNormal, 0, 255, NORM_MINMAX); // Normalize the data into a (0-255) range
        heatMapData.convertTo(heatMapData, CV_32FC1);                   // Convert heat map data data type
        heatMapDataNormal.convertTo(heatMapDataNormal, CV_8UC1);        // Convert normalized heat map data data type

        //==============================================================================================
        //Finding maximum magnitude in incoming data, scaling location for marking
       
        //only update maximum every 15 frames for dev and testing purposes
        //Point maxPoint(10,10);
        //if(i == 15) {
            minMaxLoc(magnitudeFrame, &magnitudeMin, &magnitudeMax, NULL, &maxPoint); //Find maximum magnitude from incoming data with location
           // i = 0;
        //}
       // ++i;
     
        int scaledPointX = (static_cast<double>(maxPoint.x)/static_cast<double>(magnitudeFrame.cols)) * RESOLUTION_WIDTH; //Scale max point to video resolution 
        int scaledPointY = (static_cast<double>(maxPoint.y)/static_cast<double>(magnitudeFrame.rows)) * RESOLUTION_HEIGHT;
        Point maxPointScaled(scaledPointX, scaledPointY);
        ostringstream magnitudeMaxStream;
        magnitudeMaxStream << fixed << setprecision(LABEL_PRECISION) << magnitudeMax;
        String maximumText = "Maximum = " + magnitudeMaxStream.str();
        //==============================================================================================
        
        // Colormap creation
        applyColorMap(heatMapDataNormal, heatMapRGB, COLORMAP_INFERNO); // Convert the normalized heat map data into a colormap
        cvtColor(heatMapRGB, heatMapRGBA, COLOR_RGB2RGBA);              // Convert heat map from RGB to RGBA
        cvtColor(frame, frameRGBA, COLOR_RGB2RGBA);                     // Convert video frame from RGB to RGBA
        
        //==============================================================================================

        
        
        //==============================================================================================

        // Compare value of input array to threshold. Don't render heatmap if below threshold
        thresholdValue = getTrackbarPos("Threshold", "Heat Map Overlay");
        
        for (int y = 0; y < heatMapData.rows; ++y) 
        {
            for (int x = 0; x < heatMapData.cols; ++x) 
            {
                if (heatMapData.at<uchar>(y, x) > thresholdValue) 
                {
                    Vec4b& pixel = frameRGBA.at<Vec4b>(y, x);         // Current pixel in camera
                    Vec4b heatMapPixel = heatMapRGBA.at<Vec4b>(y, x); // Current pixel in heatmap

                    // Loops through R G & B channels
                    for (int c = 0; c < 3; c++) 
                    {
                        // Merges heatmap with video and applies alpha value
                        pixel[c] = static_cast<uchar>(ALPHA * heatMapPixel[c] + (1 - ALPHA) * pixel[c]);
                    } 
                }
            }
        } // end alphaMerge

        // Graphics and Text
        Point maxTextLocation(MAX_LABEL_POS_X, MAX_LABEL_POS_Y);
    
        int textBaseline=0;
        Size textSize = getTextSize(maximumText, FONT_TYPE, FONT_SCALE, FONT_THICKNESS, &textBaseline);
        if(listMaxMagState == 1){
            
            rectangle(frameRGBA, maxTextLocation + Point(0, textBaseline), maxTextLocation + Point(textSize.width, -textSize.height), Scalar(0, 0, 0), FILLED); //Draw rectangle for text
            putText(frameRGBA, maximumText, maxTextLocation + Point(0, +5), FONT_TYPE, FONT_SCALE, Scalar(255, 255, 255), FONT_THICKNESS); //Write text for maximum magnitude
        }

        if(markMaxMagState == 1) {
            
            drawMarker(frameRGBA, maxPointScaled, Scalar(255, 255, 255), MARKER_CROSS, CROSS_SIZE, CROSS_THICKNESS, 8); //Mark the maximum magnitude point
        }

        if(colorScaleState == 1) {
            rectangle(frameRGBA, colorBarPosition + Point(-SCALE_BORDER, -SCALE_BORDER), colorBarPosition + Point(SCALE_WIDTH + SCALE_BORDER - 1, SCALE_HEIGHT + SCALE_BORDER - 1), Scalar(0, 0, 0), FILLED);
            colorBar.copyTo(frameRGBA(Rect(colorBarPosition.x, colorBarPosition.y, SCALE_WIDTH, SCALE_HEIGHT)));
        }
        // Shows frame
        imshow("Heat Map Overlay", frameRGBA);

       
 
        //==============================================================================================                               

        // Record if set to record
        if (recording == 1) 
        { 
            
            video1.write(blended);
        }
        
        
        if (getWindowProperty("Heat Map Overlay", WND_PROP_VISIBLE) < 1) 
        {
            //Exit the loop if the window is closed
            break;
        }
        

        //==============================================================================================

        // Break loop if key is pressed
        if (waitKey(30) >= 0) break;

    } // end loop

    //==============================================================================================

    // Close video
    cap.release();
    video1.release();
    //destroyAllWindows(); 

    // Close shm
    shm.closeAll();
    shm.~sharedMemory();

    return 0;
} // end main
