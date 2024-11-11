#ifndef BEAMFORM
#define BEAMFORM

// Internal Libraries
#include <iostream>
#include <complex>
#include <cmath>
#include <omp.h> // For multi-threading

// External Libraries
#include <fftw3.h> // FFT library
#include <alsa/asoundlib.h> // For audio input

// Header Files
#include "PARAMS.h"

using namespace std;

//=====================================================================================

class beamform
{
public:
    // Initialize beamform class
    beamform(int m_channels, int n_channels, int fft_size,
             int min_angle, int max_angle, int angle_step);

    // Clear memory for all arrays
    ~beamform();

    // Beamforming > FFT > FFTCollapse > Post-Process
    void processData(float data_input, float data_output,
                    int lower_frequency, int upper_frequency,
                    uint8_t post_process_type);

private:
    typedef complex<float> cfloat;

    // Beamforms input data from microphones
    void handleBeamforming(float* data_raw);

    // Performs FFT on beamformed data
    void FFT();

    // Sums all specifies frequency bins
    void FFTCollapse(int lower_frequency, int upper_frequency);

    // Performs the specified post-processing to the data (dBfs, dbA, etc.)
    void postProcessData(uint8_t post_process_type);

    // Variables
    int m_channels;
    int n_channels;
    int num_channels;
    int fft_size;
    int half_fft_size;
    int min_angle;
    int max_angle;
    int angle_step;
    int num_angles;
    int total_num_angles;

    // Constant Arrays
    cfloat* constant_1;
    cfloat* constant_2;
    cfloat* angles;

    // Data arrays
    float* data_raw;
    cfloat* data_beamform;
    double* data_fft;
    float* data_fft_collapse;
    float* data_post_process;
};

//=====================================================================================

beamform::beamform(int m_channels, int n_channels, int fft_size,
                   int min_angle, int max_angle, int angle_step):
    m_channels(m_channels),
    n_channels(n_channels),
    num_channels(m_channels * n_channels),
    fft_size(fft_size),
    half_fft_size(fft_size / 2),
    min_angle(min_angle),
    max_angle(max_angle),
    num_angles((min_angle - max_angle) / angle_step),
    total_num_angles(num_angles * num_angles)
    
    // Allocate memory for all arrays
    {
        // Constants
        constant_1 = new cfloat[half_fft_size];
        constant_2 = new cfloat[half_fft_size * half_fft_size];
        angles = new cfloat[num_angles * num_angles];

        // Data
        data_raw = new float[num_channels * fft_size * sizeof(float)];
        data_beamform = new cfloat[total_num_angles];
        data_fft = new double[total_num_angles * half_fft_size];
        data_fft_collapse = new float[total_num_angles];
        data_post_process = new float[total_num_angles];

        // Precomputes all angles
        for (int theta = min_angle, thetaIndex = 0; theta < num_angles; theta += angle_step, thetaIndex++) 
        {
            for (int phi = min_angle, phiIndex = 0; phi < num_angles; phi += angle_step, phiIndex++) 
            {
                float angle = (theta * m_channels + phi * m_channels) * (M_PI / 180);
                angles[thetaIndex * NUM_ANGLES + phiIndex] = cfloat(cos(angle), sin(angle));
            }
        }
    };

//=====================================================================================

beamform::~beamform()
{
    // Constants
    delete[] constant_1;
    delete[] constant_2;
    delete[] angles;

    // Data
    delete[] data_raw;
    delete[] data_beamform;
    delete[] data_fft;
    delete[] data_fft_collapse;
    delete[] data_post_process;
}

//=====================================================================================

void beamform::handleBeamforming(float* data_raw)
{
    #pragma omp parallel for collapse(2) // Parallelize theta and phi loops
    for (int thetaIndex = 0; thetaIndex < num_angles; thetaIndex++) 
    {
        for (int phiIndex = 0; phiIndex < num_angles; phiIndex++) 
        {
            for (int b = 0; b < fft_size; b++) 
            {
                cfloat sum = {0.0f, 0.0f}; // Accumulate results

                // Use precomputed angles
                cfloat angleComplex = angles[thetaIndex * num_angles + phiIndex];
                for (int m = 0; m < m_channels; m++) 
                {
                    for (int n = 0; n < m_channels; n++) 
                    {
                        sum += data_raw[m * n_channels * fft_size + n * fft_size + b] * angleComplex;
                    }
                }
                data_beamform[thetaIndex * NUM_ANGLES * fft_size + phiIndex * fft_size + b] = sum;
            } // end b
        } // end phi
    } // end theta
} // end handleBeamforming

//=====================================================================================

void beamform::FFT()
{
    // Create a plan for the complex-to-real 3D FFT
    fftw_plan plan = fftw_plan_dft_c2r_3d(num_angles, num_angles, fft_size, 
                                           reinterpret_cast<fftw_complex*>(const_cast<cfloat*>(data_beamform)), 
                                           data_fft, 
                                           FFTW_ESTIMATE);
    
    // Execute the 3D FFT
    fftw_execute(plan);
    
    // Cleanup
    fftw_destroy_plan(plan);
} // end FFT

//=====================================================================================

void beamform::FFTCollapse(int lower_frequency, int upper_frequency)
{
    int k_amount = upper_frequency - lower_frequency;
    for (int theta = 0; theta < NUM_ANGLES; theta++)
    {
        for (int phi = 0; phi < NUM_ANGLES; phi++)
        {
            float sum = 0;
            for (int k = lower_frequency; k <= upper_frequency; k++)
            {
                sum += abs(data_fft[theta * NUM_ANGLES * HALF_FFT_SIZE + phi * HALF_FFT_SIZE + k]);
            } // end k
            data_fft_collapse[theta * NUM_ANGLES + phi] = sum / k_amount;
        } // end phi
    } // end theta
} // end FFTCollapse

//=====================================================================================

void beamform::postProcessData(uint8_t post_process_type)
{
    for (int theta = 0; theta < num_angles; theta++)
    {
        for (int phi = 0; phi < num_angles; phi++)
        {
            switch (post_process_type)
            case post_dBFS:
                // ***Not sure if need to abs() before adding signals***
                // 20 * log10(abs(signal) / num_signals)
                data_post_process[theta * NUM_ANGLES + phi] = 20 * log10(abs(data_fft_collapse[theta * num_angles + phi]) / total_num_angles);
            break;
        }
    }
} // end postProcessData

//=====================================================================================

void beamform::processData(float data_input, float data_output,
                          int lower_frequency, int upper_frequency, 
                          uint8_t post_process_type)
{
    handleBeamforming(&data_input);
    FFT();
    FFTCollapse(lower_frequency, upper_frequency);
    postProcessData(post_process_type);
} // end handleData

//=====================================================================================

#endif