#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <ctime>

#include "PARAMS.h"
#include "sharedMemory.h"
//#include "ALSA.h"
//#include "Beamform.h"
#include "Video.h"

using namespace std;
using namespace cv;

video balls(1);

//==============================================================================================



// double FPSCalculator() {
//  if (FPSCountState == 1) {
//             frameCount++;
//             if (frameCount == FPS_COUNTER_AVERAGE) {
//                 double FPSTimeEnd = getTickCount();
//                 double FPSTimeDifference = (FPSTimeEnd - FPSTimeStart) / getTickFrequency();
//                 FPSTimeStart = FPSTimeEnd;
//                 FPS = frameCount / FPSTimeDifference;
//                 frameCount = 0;
                
//             }
        
//         }

//     return FPS;
//}



int main() 
{
    /*
    - Initialize video class
    - initializeWindow
    - Start while loop
    - cap >> frame
    - perform all frame functions
    - imshow(result from frame functions)
    - Break if close key is pressed
    - ~video() after while loop
    */





    //==============================================================================================
    
    // Video frame and capture settings
                     // Set framerate of capture from PARAMS.h
    int codec = VideoWriter::fourcc('M', 'J', 'P', 'G'); // Set codec of video recording
    string videoFileName = "testoutput";           // Set video file output file name

    Mat frame;
    
    //==============================================================================================
    
    // Initially open window
    balls.initializeWindow();
    

    cout << "2. Starting main loop.\n";

    while                                                                                                                                                                                                                                                                                                                                                                                                                           (true) 
    {
       
        cap >> frame; // Capture the frame
        
        Mat frameRGB = mainLoop(frame, codec, videoFileName);
       
        // Shows frame
        imshow("Heat Map Overlay", frameRGB);
 
        //==============================================================================================                               

        // Record if set to record
        
        //if (recordingState == 1) { 
           // video1.write(frameRGB);
        //}
        
        //==============================================================================================

        // Break loop if key is pressed
        if (waitKey(1) >= 0) break;
        //FPS = FPSCalculator();
    
    }   // end loop


    return 0;
} // end main
