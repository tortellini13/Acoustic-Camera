// Libraries
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <ctime>
#include <chrono>
#include <iomanip>

#include "PARAMS.h"
#include "ALSA.h"
#include "Beamform.h"
#include "Video.h"
#include "Timer.h"


using namespace std;

CONFIG configs(NUM_INT_CONFIGS, NUM_FLOAT_CONFIGS, NUM_BOOL_CONFIGS, NUM_STRING_CONFIGS);

int main()
{

    Mat frame;

    //=====================================================================================

    // Initialize ALSA and Beamform
    #ifdef ENABLE_AUDIO
    ALSA ALSA(AUDIO_DEVICE_NAME, M_AMOUNT, N_AMOUNT, SAMPLE_RATE, FFT_SIZE);
    beamform beamform(FFT_SIZE, SAMPLE_RATE, M_AMOUNT, N_AMOUNT, NUM_TAPS,
                      MIC_SPACING, 343.0f,
                      MIN_THETA, MAX_THETA, STEP_THETA, NUM_THETA,
                      MIN_PHI, MAX_PHI, STEP_PHI, NUM_PHI);
    #endif

    // Initialize video
    #ifdef ENABLE_VIDEO
    video video(RESOLUTION_WIDTH, RESOLUTION_HEIGHT, FRAME_RATE);
    #endif

    //=====================================================================================

    // Timer for testing
    // timer test("Test");

    // Arrays to store data
    array3D<float> audio_data_buffer_1(M_AMOUNT, N_AMOUNT, FFT_SIZE);
    array3D<float> audio_data_buffer_2(M_AMOUNT, N_AMOUNT, FFT_SIZE);
    cv::Mat processed_data(NUM_THETA, NUM_PHI, CV_32FC1, cv::Scalar(0));

    // Clear buffers
    for (int m = 0; m < audio_data_buffer_1.dim_1; m++)
    {
        for (int n = 0; n < audio_data_buffer_1.dim_2; n++)
        {
            for (int b = 0; b < audio_data_buffer_1.dim_3; b++)
            {
                audio_data_buffer_1.at(m, n, b) = 0.0f;
                audio_data_buffer_2.at(m, n, b) = 0.0f;
            } // end b
        } // end n
    } // end m

    // Send configuration to ALSA and start recording audio
    #ifdef ENABLE_AUDIO
    ALSA.setup();
    ALSA.start();
    // cout << "Audio setup complete.\n"; 

    beamform.setup();
    // cout << "Beamform setup complete.\n";
    #endif

    // Start video capture
    #ifdef ENABLE_VIDEO 
    video.startCapture();
    cout << "Video setup complete.\n";
    #endif
 
    //=====================================================================================

    cout << "Starting main loop.\n";
    while(1)
    {
        
        // test.start();
        // Copy data from ring buffer and process beamforming
        #ifdef ENABLE_AUDIO
        ALSA.copyRingBuffer(audio_data_buffer_1, audio_data_buffer_2);

        // audio_data_buffer_1.print_layer(100);

        beamform.processData(processed_data, 20, 27, POST_dBFS, audio_data_buffer_1, audio_data_buffer_2);
        // cout << "End of processData\n";

        
        #endif
        
        // Generate heatmap and ui then display the frame
        #ifdef ENABLE_VIDEO
        // if (waitKey(1) >= 0) break;
        #ifdef ENABLE_AUDIO
        int pcm_error = ALSA.pcm_error;
        #else
        int pcm_error = 0;
        #endif
        if (video.processFrame(processed_data, pcm_error) == false) break;
        //if (waitKey(1) >= 0) break;
        #endif

        // test.end();
        // test.print();
        // test.print_avg(AVG_SAMPLES);

    } // end loop

    //=====================================================================================

    // Clean up and exit
    #ifdef ENABLE_AUDIO
    ALSA.stop();
    #endif

    #ifdef ENABLE_VIDEO
    video.stopCapture();
    #endif
 
    return 0;
} // end main