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

int main()
{
    ALSA ALSA(AUDIO_DEVICE_NAME, NUM_CHANNELS, SAMPLE_RATE, FFT_SIZE);
    beamform beamform(FFT_SIZE, SAMPLE_RATE, M_AMOUNT, N_AMOUNT,
                      MIC_SPACING, 343.0f,
                      MIN_THETA, MAX_THETA, STEP_THETA, NUM_THETA,
                      MIN_PHI, MAX_PHI, STEP_PHI, NUM_PHI);
    video video(RESOLUTION_WIDTH, RESOLUTION_HEIGHT, FRAME_RATE);
    timer test("Test");

    array3D<float> audio_data(M_AMOUNT, N_AMOUNT, FFT_SIZE);
    cv::Mat processed_data(NUM_THETA, NUM_PHI, CV_32FC1, cv::Scalar(0));

    ALSA.setup();
    beamform.setup();
    video.startCapture();

    while(1)
    {
        ALSA.recordAudio(audio_data);

        #ifdef PRINT_AUDIO
        audio_data.print_layer(0);
        #endif

        //beamform.processData(audio_data, processed_data, 1, 511, POST_dBFS);

        video.processFrame(processed_data, -70.0f, -30.0f);
        

    }
    
    video.stopCapture();
    
    return 0;
}