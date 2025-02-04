// Libraries
#include <iostream>
#include <opencv2/opencv.hpp>

// Headers
#include "PARAMS.h"
#include "Structs.h"
#include "Beamform.h"
#include "ALSA.h"


using namespace std;

int main()
{
    ALSA ALSA(AUDIO_DEVICE_NAME, NUM_CHANNELS, SAMPLE_RATE, FFT_SIZE);
    beamform beamform(FFT_SIZE, SAMPLE_RATE, M_AMOUNT, N_AMOUNT,
                      MIC_SPACING, 343.0f,
                      MIN_THETA, MAX_THETA, STEP_THETA, NUM_THETA,
                      MIN_PHI, MAX_PHI, STEP_PHI, NUM_PHI);
    timer test("Test");

    array3D<float> audio_data(M_AMOUNT, N_AMOUNT, FFT_SIZE);
    cv::Mat processed_data(NUM_THETA, NUM_PHI, CV_32FC1, cv::Scalar(0));

    ALSA.setup();
    beamform.setup();

    while(1)
    {
        ALSA.recordAudio(audio_data);

        #ifdef PRINT_AUDIO
        audio_data.print_layer(0);
        #endif

        beamform.processData(audio_data, processed_data, 1, 511, POST_dBFS);
    }
    
    return 0;
}