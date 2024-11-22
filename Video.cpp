#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <ctime>

#include "PARAMS.h"
#include "ALSA.h"
#include "Beamform.h"
#include "Video.h"

using namespace std;
using namespace cv;

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
    string video_file_name = "testoutput";           // Set video file output file name
    
    // Audio data [M * N * FFT_SIZE]
    float audio_data[NUM_CHANNELS * FFT_SIZE] = {0.0f};
    cout << "audio_data\n";

    // Beamformed and post-processed data [NUM_ANGLES * NUMANGLES]
    float processed_data[TOTAL_ANGLES] = {0.0f};
    cout << "processed_data\n";

    // Output from all video processing to show
    Mat frame;
    
    // Initialize classes
    ALSA ALSA(AUDIO_DEVICE_NAME, NUM_CHANNELS, SAMPLE_RATE, FFT_SIZE);
    cout << "ALSA\n";
    beamform beamform(M_AMOUNT, N_AMOUNT, FFT_SIZE, MIN_ANGLE, MAX_ANGLE, ANGLE_STEP);
    cout << "beamform\n";
    //video video(RESOLUTION_WIDTH, RESOLUTION_HEIGHT, FRAME_RATE, MAP_THRESHOLD, ALPHA);

    //==============================================================================================
    
    // Setup audio and data processing
    ALSA.setup();
    cout << "ALSA.setup\n";
    beamform.setup();
    cout << "beamform.setup\n";

    // Open window
    //video.initializeWindow();
    
    cout << "Starting loop.\n";
    // Loop to calculate audio and display video
    while (1) // if this reads while(true) you will die. Source: saw it in a dream after drinking a monster and taking melatonin
    {
        // Recieve audio and process data
        ALSA.recordAudio(audio_data);
        cout << "recordAudio.\n";

        beamform.processData(audio_data, processed_data, 0, 511, POST_dBFS);
        cout << "processData.\n";


        // Does all processing to frame including drawing UI and doing heat map
        //frame = video.processFrame(audio_data, codec, video_file_name);
       
        // Shows frame
        //imshow("Heat Map Overlay", frame);
 
        //==============================================================================================

        // Break loop if key is pressed
        //if (waitKey(1) >= 0) break;

        // Calculates fpr
        // fps = FPSCalculator();
    
    }   // end loop


    return 0;
} // end main
