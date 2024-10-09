#include <opencv2/opencv.hpp>
#include <iostream>
#include "PARAMS.h"
#include "sharedMemory.h"
#include <vector>

using namespace cv;
using namespace std;

int main() 
{
    //==============================================================================================
    
    // Initializes array to receive data from Audio
    vector<vector<float>> magnitudeInput(NUM_ANGLES, vector<float>(NUM_ANGLES));        
    
    // Initialize shared memory class for Audio
    sharedMemory audioData(AUDIO_SHM, AUDIO_SEM_1, AUDIO_SEM_2, NUM_ANGLES, NUM_ANGLES);   

    // Initializes array to send user config to Audio. Filled with default settings
    vector<int> USER_CONFIGS(NUM_CONFIGS, 0);     // ***Might need to manually set default settings
    //vector<int> PREVIOUS_CONFIGS(NUM_CONFIGS, 0); // For limiting amount of shm calls

    // Initializes shared memory class for userConfig
    sharedMemory userConfig(CONFIG_SHM, CONFIG_SEM_1, CONFIG_SEM_2, NUM_CONFIGS, 1);
    
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
    if (!audioData.openAll()) 
    {                                                             
        cerr << "1. openAll Failed\n";
    }
    
    // Creates shared memory for Configs
	if(!userConfig.createAll())
	{
		cerr << "1. createAll failed.\n";
	}

    //==============================================================================================
    
    int recording = 0; // Recording setting
  
    Mat heatMapData, heatMapDataNormal, heatMapRGB, heatMapRGBA, blended, frameRGBA, frame, testframe;  // Establish matricies for 
    heatMapData = Mat(frame.size(), CV_32FC1);                                                          // Make heat map data matrix
    VideoCapture cap(0, CAP_V4L2);                                                                      // Open camera

    // Video frame and capture settings
    int width = RESOLUTION_WIDTH;           // Width from PARAMS.h
    int height = RESOLUTION_HEIGHT;         // Height from PARAMS.h
    double alpha = 0.5;                     // Transparency factor
    cap.set(CAP_PROP_FRAME_WIDTH, width);   // Set frame width for capture
    cap.set(CAP_PROP_FRAME_HEIGHT, height); // Set frame height for capture
    cap.set(CAP_PROP_FPS, FRAME_RATE);      // Set framerate of capture from PARAMS.h

    // Heat Map settings
    int magnitudeWidth = NUM_ANGLES;  // Set Dims of magnitude data coming in from PARAMS.h
    int magnitudeHeight = NUM_ANGLES; 
    double thresholdValue = MAP_THRESHOLD;    // Set minimum threshold for heatmap (all data below this value is transparent)
    

    Mat magnitudeFrame(magnitudeHeight, magnitudeWidth, CV_32FC1, Scalar(0)); // Single channel, magnitude matrix, initialized to 0

    //==============================================================================================

    // Setup recording if enabled
    VideoWriter vide0;                                   // Initialize recording vide0
    cap >> testframe;                                    // Capture a single test frame
    int codec = VideoWriter::fourcc('M', 'J', 'P', 'G'); // Set codec of video recording
    string videoFileName = "./testoutput.avi";           // Set video file output file name
    
    // Open recording if enabled, report error if it doesn't work
    if (recording == 1) 
    {
        vide0.open(videoFileName, codec, FRAME_RATE, testframe.size(), 1);
        if (!vide0.isOpened()) 
        {
            cerr << "Could not open the output video file for write\n";
            return -1;
        }
    }

    //==============================================================================================
      
    // Loop for capturing frames, heatmap, displaying, and recording
    while (1) 
    {
        cap >> frame; //capture the frame
        if(!frame.empty()) 
        {
            cout << "Frame Captured!";
        } 
        
        else 
        {
            cout << "Frame is empty ;(";
        }
        
        //==============================================================================================

        
        // Read the shared memory to obtain magnitude data
        if (!audioData.read2D(magnitudeInput)) 
        { 
            cerr << "2. read2D Failed\n";
        } 
        else 
        {
            //cout << "Data read from shared memory";
        }

        // Nested loops for converting vector vector into an OpenCV matrix
        for(int rows = 0; rows < NUM_ANGLES; rows++) 
        { 
            for(int columns = 0; columns < NUM_ANGLES; columns++)
            {
                magnitudeFrame.at<int>(rows, columns) = magnitudeInput[rows][columns];
            }
        }      
        

        //==============================================================================================

        // Magnitude Data Proccessing
        resize(magnitudeFrame, heatMapData, Size(width, height), 0, 0, INTER_LINEAR);       //Scaling and interpolating into camera resolution
        //threshold(heatMapData, heatMapData, thresholdValue, thresholdPeak, THRESH_TOZERO);  //Apply a threshold to the magnitude data
        normalize(heatMapData, heatMapDataNormal, 0, 255, NORM_MINMAX);                     //Normalize the data into a (0-255) range
        heatMapData.convertTo(heatMapData, CV_8UC1);                                        //Convert heat map data data type
        heatMapDataNormal.convertTo(heatMapDataNormal, CV_8UC1);                            //Convert normalized heat map data data type

        //==============================================================================================
        
        // Colormap creation
        applyColorMap(heatMapDataNormal, heatMapRGB, COLORMAP_INFERNO);                     //Convert the normalized heat map data into a colormap
        cvtColor(heatMapRGB, heatMapRGBA, COLOR_RGB2RGBA);                                  //Convert heat map from RGB to RGBA
        cvtColor(frame, frameRGBA, COLOR_RGB2RGBA);                                         //Convert video frame from RGB to RGBA
        
        //==============================================================================================

        // Scary nested for loops to iterate through each value and set to clear if 0
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

        //==============================================================================================

        // Write configs to shared memory
        if(!userConfig.write(USER_CONFIGS))
        {
            cerr << "2. write failed.\n";
        }
        
        //==============================================================================================

        // Break loop if key is pressed
        if (waitKey(30) >= 0) break;

    } // end loop

    //==============================================================================================

    cap.release();
    vide0.release();
    //destroyAllWindows(); 

    return 0;
} // end main
