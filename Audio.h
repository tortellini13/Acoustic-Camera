#ifndef AUDIO
#define AUDIO

// Internal Libraries
#include <iostream>
#include <complex>
#include <cmath>
#include <omp.h> // For OpenMP parallelization if available

// External Libraries
#include <fftw3.h> // FFT library
#include <alsa/asoundlib.h> // For audio input

// Header Files
#include "PARAMS.h"

using namespace std;
typedef complex<float> cfloat;

//=====================================================================================

// Converts degrees to radians
float degtorad(float angleDEG)
{
	float result = angleDEG * (M_PI / 180);
	return result;
} // end degtorad

//=====================================================================================

// Calculates constants 
void FFTConstants(cfloat constant1[HALF_FFT_SIZE], cfloat constant2[HALF_FFT_SIZE * HALF_FFT_SIZE])
{
    // Iterates through frequency mins
    for (int k = 0; k < HALF_FFT_SIZE; k++)
    {
        float exponent1 = (-2 * M_PI * k) / HALF_FFT_SIZE;
        constant1[k] = cfloat(cos(exponent1), sin(exponent1)); // e^(i * 2 * pi * k / (N / 2))

        // Produces constants for HALF_FFT_SIZE x HALF_BUFFER
        for (int b = 0; b < HALF_FFT_SIZE; b++)
        {
            float exponent2 = exponent1 * b;
            constant2[k * HALF_FFT_SIZE + b] = cfloat(cos(exponent2), sin(exponent2)); // e^(i * 2 * pi * k * b / (N / 2))
        }
    }
} // end FFTConstants

//=====================================================================================

// Precompute cos/sin values
void FFTAngles(cfloat angles[TOTAL_ANGLES])
{
    for (int theta = MIN_ANGLE, thetaIndex = 0; theta < NUM_ANGLES; theta += ANGLE_STEP, thetaIndex++) 
    {
        for (int phi = MIN_ANGLE, phiIndex = 0; phi < NUM_ANGLES; phi += ANGLE_STEP, phiIndex++) 
        {
            float angle = degtorad(theta * M_AMOUNT + phi * N_AMOUNT);
            angles[thetaIndex * NUM_ANGLES + phiIndex] = cfloat(cos(angle), sin(angle));
        }
    }
}

//=====================================================================================

void handleBeamforming(float dataInput[BUFFER_SIZE], cfloat dataOutput[DATA_SIZE_BUFFER],
                       cfloat angles[TOTAL_ANGLES]) 
{
    #pragma omp parallel for collapse(2) // Parallelize theta and phi loops
    for (int thetaIndex = 0; thetaIndex < NUM_ANGLES; thetaIndex++) 
    {
        for (int phiIndex = 0; phiIndex < NUM_ANGLES; phiIndex++) 
        {
            for (int b = 0; b < FFT_SIZE; b++) 
            {
                cfloat sum = {0.0f, 0.0f}; // Accumulate results

                // Use precomputed angles
                cfloat angleComplex = angles[thetaIndex * NUM_ANGLES + phiIndex];
                for (int m = 0; m < M_AMOUNT; m++) 
                {
                    for (int n = 0; n < N_AMOUNT; n++) 
                    {
                        sum += dataInput[m * N_AMOUNT * FFT_SIZE + n * FFT_SIZE + b] * angleComplex;
                    }
                }
                dataOutput[thetaIndex * NUM_ANGLES * FFT_SIZE + phiIndex * FFT_SIZE + b] = sum;
            } // end b
        } // end phi
    } // end theta
} // end handleBeamforming

//=====================================================================================

void FFTCalc(const cfloat inputData[DATA_SIZE_BUFFER], double outputData[DATA_SIZE_BUFFER_HALF]) 
{    
    // Create a plan for the complex-to-real 3D FFT
    fftw_plan plan = fftw_plan_dft_c2r_3d(NUM_ANGLES, NUM_ANGLES, FFT_SIZE, 
                                           reinterpret_cast<fftw_complex*>(const_cast<cfloat*>(inputData)), 
                                           outputData, 
                                           FFTW_ESTIMATE);
    
    // Execute the 3D FFT
    fftw_execute(plan);
    
    // Cleanup
    fftw_destroy_plan(plan);
}

//=====================================================================================

void FFTCollapse(const double inputData[DATA_SIZE_BUFFER_HALF], float dataOutput[TOTAL_ANGLES],
                int lowerBound, int upperBound)
{
    int k_amount = upperBound - lowerBound;
    for (int theta = 0; theta < NUM_ANGLES; theta++)
    {
        for (int phi = 0; phi < NUM_ANGLES; phi++)
        {
            float sum = 0;
            for (int k = lowerBound; k <= upperBound; k++)
            {
                sum += abs(inputData[theta * NUM_ANGLES * HALF_FFT_SIZE + phi * HALF_FFT_SIZE + k]);
            } // end k
            dataOutput[theta * NUM_ANGLES + phi] = sum / k_amount;
        } // end phi
    } // end theta
} // end FFTCollapse

//=====================================================================================

// Calculates dB Full Scale (-inf, 0)
void dBfs(const float dataInput[TOTAL_ANGLES], float dataOutput[TOTAL_ANGLES])
{
    for (int theta = 0; theta < NUM_ANGLES; theta++)
    {
        for (int phi = 0; phi < NUM_ANGLES; phi++)
        {
            // ***Not sure if need to abs() before adding signals***
            // 20 * log10(abs(signal) / num_signals)
            dataOutput[theta * NUM_ANGLES + phi] = 20 * log10(abs(dataInput[theta * NUM_ANGLES + phi]) / TOTAL_ANGLES);
        }
    }
} // end dBfs

//=====================================================================================

// Sets up all constants required for beamforming and FFT
void setupConstants(cfloat constant1[HALF_FFT_SIZE], cfloat constant2[HALF_FFT_SIZE * HALF_FFT_SIZE],
                    cfloat angles[TOTAL_ANGLES])
{
    FFTConstants(constant1, constant2);
    FFTAngles(angles);
} // end setupConstants

//=====================================================================================

// Performs all functions needed to go from raw data to fully processed beamformed and FFT data
void handleData(float dataRaw[BUFFER_SIZE], cfloat dataBeamform[DATA_SIZE_BUFFER],
                double dataFFT[DATA_SIZE_BUFFER_HALF], float dataFFTCollapse[TOTAL_ANGLES],
                float datadBfs[TOTAL_ANGLES],
                int lowerBound, int upperBound, cfloat angles[TOTAL_ANGLES])
{
    handleBeamforming(dataRaw, dataBeamform, angles);
    FFTCalc(dataBeamform, dataFFT);
    FFTCollapse(dataFFT, dataFFTCollapse, lowerBound, upperBound);
    dBfs(dataFFTCollapse, datadBfs);
} // end handleData
//==============================================================================================

// Set up parameters for audio interface
int setupAudio(snd_pcm_t **pcm_handle, snd_pcm_uframes_t *frames, const char* audio_input) 
{
    snd_pcm_hw_params_t *params;
    unsigned int rate = SAMPLE_RATE;
    int dir, pcm;

    // Open the PCM device in capture mode
    pcm = snd_pcm_open(pcm_handle, audio_input, SND_PCM_STREAM_CAPTURE, 0);
    if (pcm < 0) 
    {
        cerr << "Unable to open PCM device: " << snd_strerror(pcm) << endl;
        return pcm;
    }
    //cout << "PCM device opened in capture mode.\n"; // Debugging

    // Allocate a hardware parameters object
    snd_pcm_hw_params_alloca(&params);
    //cout << "Hardware parameter object allocated.\n"; // Debugging

    // Set the desired hardware parameters
    snd_pcm_hw_params_any(*pcm_handle, params);
    snd_pcm_hw_params_set_access(*pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(*pcm_handle, params, SND_PCM_FORMAT_FLOAT_LE);  // 32-bit float PCM
    snd_pcm_hw_params_set_channels(*pcm_handle, params, NUM_CHANNELS); 
    snd_pcm_hw_params_set_rate_near(*pcm_handle, params, &rate, &dir);
    cout << "Hardware parameters set.\n";

    // Write the parameters to the PCM device
    pcm = snd_pcm_hw_params(*pcm_handle, params);
    if (pcm < 0) 
    {
        cerr << "Unable to set HW parameters: " << snd_strerror(pcm) << endl;
        return pcm;
    }
    //cout << "Parameters written to device.\n"; // Debugging

    // Get the period size (frames per buffer)
    snd_pcm_hw_params_get_period_size(params, frames, &dir);

    return 0;
} // end setupAudio

//==============================================================================================

// Function to capture audio into a 3D vector (M_AMOUNT x N_AMOUNT x FFT_SIZE)
int captureAudio(float output[], snd_pcm_t *pcm_handle) 
{
    float buffer[FFT_SIZE * NUM_CHANNELS];  // Flat buffer for data (1 sample deep)
    int pcm;

    // Capture PCM audio
    while (1) 
    {
        pcm = snd_pcm_readi(pcm_handle, buffer, FFT_SIZE);
        if (pcm == -EPIPE) 
        {
            // Buffer overrun
            snd_pcm_prepare(pcm_handle);
            cerr << "Buffer overrun occurred, PCM prepared.\n";
            continue;  // Try reading again
        } 
        else if (pcm < 0) 
        {
            cerr << "Error reading PCM data: " << snd_strerror(pcm) << endl;
            return pcm;
        }
        else if (pcm != FFT_SIZE)
        {
            cerr << "Short read, read " << pcm << " frames\n";
            continue;  // Try reading again
        }
        else
        {
            break; // Successful read
        }
    }
    // cout << "Data read from device.\n"; // Debugging

    // Populate the 3D vector with captured data (Unflatten the buffer)
    for (int k = 0; k < FFT_SIZE; k++) 
    {
        for (int m = 0; m < M_AMOUNT; m++) 
        {
            for (int n = 0; n < N_AMOUNT; n++) 
            {
                output[m * N_AMOUNT * FFT_SIZE + n * FFT_SIZE + k] = buffer[k * NUM_CHANNELS + m * N_AMOUNT + n];
            }
        }
    }
    // cout << "3D data vector populated with audio data.\n"; // Optional

    return 0;
} // end captureAudio

//==============================================================================================

#endif