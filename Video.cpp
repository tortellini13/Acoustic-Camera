#include <opencv2/opencv.hpp>
#include <iostream>
#include "PARAMS.h"
#include "sharedMemory.h"
#include <vector>

using namespace cv;
using namespace std;



int main() 
{
    // Initializes array to receive data from Audio
    vector<vector<float>> magnitudeInput(NUM_ANGLES, vector<float>(NUM_ANGLES));        
    
    // Initialize shared memory class
    sharedMemory audioData(AUDIO_SHM, AUDIO_SEM_1, AUDIO_SEM_2, NUM_ANGLES, NUM_ANGLES);    

    // Open shared memory
    if (!audioData.openAll()) 
    {                                                             
        cerr << "2. openAll Failed\n";
    }
    
    int recording = 0; // Recording setting
  
    Mat heatMapData, heatMapDataNormal, heatMapRGB, heatMapRGBA, blended, frameRGBA, frame, testframe;  // Establish matricies for 
    heatMapData = Mat(frame.size(), CV_32FC1);                                                          // Make heat map data matrix
    VideoCapture cap(0, CAP_V4L2);                                                                      // open camera

        // Video frame and capture settings
        int width = RESOLUTION_WIDTH;               // Width from PARAMS.h
        int height = RESOLUTION_HEIGHT;             // Height from PARAMS.h
        double alpha = 0.5;                         // Transparency factor
        cap.set(CAP_PROP_FRAME_WIDTH, width);       // Set frame width for capture
        cap.set(CAP_PROP_FRAME_HEIGHT, height);     // Set frame height for capture
        cap.set(CAP_PROP_FPS, FRAME_RATE);          // Set framerate of capture from PARAMS.h

        // Heat Map settings
        int magnitudeWidth = NUM_ANGLES;    // Set Dims of magnitude data coming in from PARAMS.h
        int magnitudeHeight = NUM_ANGLES; 
        double thresholdValue = 0;          // Set minimum threshold for heatmap (all data below this value is transparent)
        double thresholdPeak = 255;         // Set the maximum allowed value

        Mat magnitudeFrame(magnitudeHeight, magnitudeWidth, CV_32FC1, Scalar(0)); //single channel, magnitude matrix, initialized to 0

    // Setup recording if enabled
    VideoWriter vide0;                                      // Initialize recording vide0
    cap >> testframe;                                       // Capture a single test frame
    int codec = VideoWriter::fourcc('M', 'J', 'P', 'G');    // Set codec of video recording
    string videoFileName = "./testoutput.avi";              // Set video file output file name
    
        if (recording == 1) 
        {                                                       // Open recording if enabled, report error if it doesn't work
            vide0.open(videoFileName, codec, FRAME_RATE, testframe.size(), 1);
            if (!vide0.isOpened()) 
            {
                cerr << "Could not open the output video file for write\n";
                return -1;
            }
        }

      
    // Loop for capturing frames, heatmap, displaying, and recording
    while (true) 
    {
        cap >> frame; //capture the frame
        if(!frame.empty()) 
        {
            //cout << "Frame Captured!";
        } 
        
        else 
        {
            cout << "Frame is empty ;(";
        }
        
        /*   
        // Read the shared memory to obtain magnitude data
        if (!audioData.read2D(magnitudeInput)) 
        { 
            cerr << "2. read2D Failed\n";
        } 
        else 
        {
            cout << "Data read from shared memory";
        }

        // Nested loops for converting vector vector into an OpenCV matrix
        for(int rows = 0; rows < NUM_ANGLES; rows++) 
        { 
            for(int columns = 0; columns < NUM_ANGLES; columns++)
            {
                magnitudeFrame.at<int>(rows, columns) = magnitudeInput[rows][columns];
            }
        }      
        */

        // Magnitude Data Proccessing
        resize(magnitudeFrame, heatMapData, Size(width, height), 0, 0, INTER_LINEAR);       //Scaling and interpolating into camera resolution
        //threshold(heatMapData, heatMapData, thresholdValue, thresholdPeak, THRESH_TOZERO);  //Apply a threshold to the magnitude data
        normalize(heatMapData, heatMapDataNormal, 0, 255, NORM_MINMAX);                     //Normalize the data into a (0-255) range
        heatMapData.convertTo(heatMapData, CV_8UC1);                                        //Convert heat map data data type
        heatMapDataNormal.convertTo(heatMapDataNormal, CV_8UC1);                            //Convert normalized heat map data data type

        
        // Colormap creation
        applyColorMap(heatMapDataNormal, heatMapRGB, COLORMAP_INFERNO);                     //Convert the normalized heat map data into a colormap
        cvtColor(heatMapRGB, heatMapRGBA, COLOR_RGB2RGBA);                                  //Convert heat map from RGB to RGBA
        cvtColor(frame, frameRGBA, COLOR_RGB2RGBA);                                         //Convert video frame from RGB to RGBA
        
        // scary nested for loops to iterate through each value and set to clear if 0
        for (int y = 0; y < heatMapData.rows; ++y) 
        {
            for (int x = 0; x < heatMapData.cols; ++x) 
            {
                if (heatMapData.at<uchar>(y, x) < thresholdValue) 
                {
                    heatMapRGBA.at<Vec4b>(y, x)[4] = 0; // Set alpha channel 0 if the threshold data is zero
                }
            }
        }   
        
        // Merge the heat map and video frame
        addWeighted(heatMapRGBA, alpha, frameRGBA, 1.0 - alpha, 0.0, blended);  
        
        // Display the merged frame
        imshow("Heat Map Overlay", blended);                                    

        // Record if set to record
        if (recording == 1) 
        { 
            cvtColor(blended, blended, COLOR_RGBA2RGB);
            vide0.write(blended);
        }
        
        /*
        if (getWindowProperty("Heat Map Overlay", WND_PROP_VISIBLE) < 1) 
        {
            Exit the loop if the window is closed
            break;

        }
        */

        // Break loop if key is pressed
        if (waitKey(30) >= 0) break;

    } 

    cap.release();
    vide0.release();
    //destroyAllWindows(); 
    return 0;
}
