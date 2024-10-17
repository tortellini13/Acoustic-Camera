#include <opencv2/opencv.hpp>
#include <iostream>
#include "PARAMS.h"
#include "sharedMemory.h"
#include <vector>

using namespace cv;
using namespace std;


void thresholdChange (int thresholdValue, void* userdata) {
//cout << thresholdValue;
}






int main() 
{
    //==============================================================================================
    
    // Initializes array to receive data from Audio
    vector<vector<float>> magnitudeInput(NUM_ANGLES, vector<float>(NUM_ANGLES));   

    // Initializes array to send user config to Audio. Filled with default settings
    vector<int> userConfigs(NUM_CONFIGS, 0);     // ***Might need to manually set default settings     
    
    // Initialize shared memory class
   // sharedMemory shm(AUDIO_SHM, CONFIG_SHM, SEM_1, SEM_2, NUM_ANGLES, NUM_ANGLES, NUM_CONFIGS);   


    
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
   // if (!shm.shmStart2()) 
    //{                                                             
      //  cerr << "2. shmStart2 Failed\n";
    //}
    //cout << "Shared Memory Configured.\n"; // For debugging

    //==============================================================================================

    // Setup recording if enabled
    int recording = 0; // Recording setting
    VideoCapture cap(0, CAP_V4L2);
    VideoWriter vide0;
    int codec = VideoWriter::fourcc('M', 'J', 'P', 'G'); // Set codec of video recording
    Mat testframe;                                   // Initialize recording vide0
    cap >> testframe;                                    // Capture a single test frame
    string videoFileName = "./testoutput.avi";           // Set video file output file name
    imshow("Heat Map Overlay", testframe); 
     // Open recording if enabled, report error if it doesn't work
    if (recording == 1) 
    {
        vide0.open(videoFileName, codec, FRAME_RATE, testframe.size(), 1);
        
        if (!vide0.isOpened()) 
        {
            cerr << "Could not open the output video file for write.\n";
            return 1;
        }
    }

    Mat heatMapData, heatMapDataNormal, heatMapRGB, heatMapRGBA, blended, frameRGBA, frame;  // Establish matricies for 
    heatMapData = Mat(frame.size(), CV_32FC1);                                                          // Make heat map data matrix
    Mat magnitudeFrame(NUM_ANGLES, NUM_ANGLES, CV_32FC1, Scalar(0)); // Single channel, magnitude matrix, initialized to 0

    // Video frame and capture settings
    double alpha = 0.6;                     // Transparency factor
    cap.set(CAP_PROP_FRAME_WIDTH, RESOLUTION_WIDTH);   // Set frame width for capture
    cap.set(CAP_PROP_FRAME_HEIGHT, RESOLUTION_HEIGHT); // Set frame height for capture
    cap.set(CAP_PROP_FPS, FRAME_RATE);      // Set framerate of capture from PARAMS.h

    //==============================================================================================
      
    // Trackbars
    int thresholdValue = MAP_THRESHOLD;    // Set minimum threshold for heatmap (all data below this value is transparent)
    int threshTrackMax = MAP_THRESHOLD_MAX;
    int threshTrackMin = MAP_THRESHOLD_MIN;
    

    //==============================================================================================

    // Loop for capturing frames, heatmap, displaying, and recording
    while                                                                                                                                                                                                                                                                                                                                                                                                                           (true) 
    {
        cap >> frame; // Capture the frame
        if(frame.empty()) 
        {
           cout << "Frame is empty ;(\n";
           //cout << "Frame Captured!\n";
        } 
        //cout << "Frame Captured!\n"; // For debugging
        
        //==============================================================================================

        // Read audio data and write user configs to Audio
       // if (!shm.readWrite2(magnitudeInput, userConfigs)) 
       // { 
         //   cerr << "2. readWrite2 Failed\n";
       // } 

       
        
        for (int rows = 0; rows < NUM_ANGLES; rows++) // Converts vector<vector<float>> into an OpenCV matrix
        { 
            for (int columns = 0; columns < NUM_ANGLES; columns++)
            {
                magnitudeFrame.at<int>(rows, columns) = magnitudeInput[rows][columns];
            }
        }      
        randu(magnitudeFrame, Scalar(0), Scalar(300));
        //==============================================================================================

        // Magnitude Data Proccessing
        resize(magnitudeFrame, heatMapData, Size(RESOLUTION_WIDTH, RESOLUTION_HEIGHT), 0, 0, INTER_LINEAR);       // Scaling and interpolating into camera resolution
        normalize(heatMapData, heatMapDataNormal, 0, 255, NORM_MINMAX);                     // Normalize the data into a (0-255) range
        heatMapData.convertTo(heatMapData, CV_32FC1);                                        // Convert heat map data data type
        heatMapDataNormal.convertTo(heatMapDataNormal, CV_8UC1);                             // Convert normalized heat map data data type

        //==============================================================================================
        
        // Colormap creation
        applyColorMap(heatMapDataNormal, heatMapRGB, COLORMAP_INFERNO);                     // Convert the normalized heat map data into a colormap
        cvtColor(heatMapRGB, heatMapRGBA, COLOR_RGB2RGBA);                                  // Convert heat map from RGB to RGBA
        cvtColor(frame, frameRGBA, COLOR_RGB2RGBA);                                         // Convert video frame from RGB to RGBA
        
        //==============================================================================================

        // Scary nested for loops to iterate through each value and set to clear if 0
        for (int y = 0; y < heatMapData.rows; ++y) 
        {
            for (int x = 0; x < heatMapData.cols; ++x) 
            {
                if (heatMapData.at<uchar>(y, x) > thresholdValue) 
                {
                    //heatMapRGBA.at<Vec4s>(y, x)[0] = 100; // Set alpha channel 0 if the threshold data is zero
                    //heatMapRGBA.at<Vec4s>(y, x)[1] = 100; // Set alpha channel 0 if the threshold data is zero
                    //heatMapRGBA.at<Vec4s>(y, x)[2] = 100; // Set alpha channel 0 if the threshold data is zero
                    //heatMapRGBA.at<Vec4s>(y, x)[3] = 0; // Set alpha channel 0 if the threshold data is zero
                    Vec4b& pixel = frameRGBA.at<Vec4b>(y, x);
                    Vec4b heatMapPixel = heatMapRGBA.at<Vec4b>(y, x);
                    for (int c = 0; c < 3; c++) {

                        pixel[c] = static_cast<uchar>(alpha * heatMapPixel[c] + (1 - alpha) * pixel[c]);
                    } 

                }
                //cout << heatMapRGBA.at<Vec4s>(y, x)[7] << endl;
            }
        }   
        
    
        // Merge the heat map and video frame
        //addWeighted(heatMapRGBA, alpha, frameRGBA, 1.0 - alpha, 0.0, blended);  
        // Display the merged frame
        //cvtColor(blended, blended, COLOR_RGBA2RGB);





        imshow("Heat Map Overlay", frameRGBA);
        createTrackbar("Threshold", "Heat Map Overlay", &thresholdValue, threshTrackMax, thresholdChange);    
        cout << thresholdValue;                                

        // Record if set to record
        if (recording == 1) 
        { 
            
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

        // Break loop if key is pressed
        if (waitKey(30) >= 0) break;

    } // end loop

    //==============================================================================================

    // Close video
    cap.release();
    vide0.release();
    //destroyAllWindows(); 

    // Close shm
   // shm.closeAll();
    //shm.~sharedMemory();

    return 0;
} // end main
