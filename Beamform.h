#pragma once

// Libraries
#include <iostream>
#include <complex>            // Complex numbers
#include <fftw3.h>            // FFT  
#include <opencv2/opencv.hpp> // For outputting cv::Mat

// Headers
#include "PARAMS.h"  // Various hardcoded parameters
#include "Structs.h" // Data structs (array2D, array3D, etc.)
#include "Timer.h"   // Profiling timers

class beamform
{
public:

    // Constructor
    beamform(const int fft_size, const int sample_rate, const int m_channels, const int n_channels,
             const float mic_spacing, const float speed_of_sound,
             const int min_theta, const int max_theta, const int step_theta, const int num_theta,
             const int min_phi, const int max_phi, const int step_phi, const int num_phi);

    // Destructor
    ~beamform();

    // Sets up all constants and initialized FFT
    void setup();

    // Performs beamforming
    void processData(cv::Mat &data_output, const int lower_frequency, const int upper_frequency,
                     array3D<float> &data_buffer_1, array3D<float> &data_buffer_2);

private:
    // Setup functions
    void setupDirectivity(); // Computes directivity factor for phased array
    void setupFFT();         // Creates FFT plan

    // Beamforming functions
    void handleBeamforming(array3D<float> &data_input, const uint8_t lower_frequency, const uint8_t upper_frequency);
    void FFT();
    void FFTCollapse(const uint8_t lower_frequency, const uint8_t upper_frequency);

    // Helper functions
    float degtorad(const float angle_deg);            // Converts degrees to radians
    cv::Mat array2DtoMat(const array2D<float> &data); // Converts array2D to cv::Mat

    // Initial conditions
    int fft_size;     // Size of FFT in samples
    int sample_rate;  // Audio sample rate in Hz

    float mic_spacing;    // Distance between microphones in meters
    float speed_of_sound; // Speed of sound in meters per second

    int min_theta;  // Minimum theta angle in degrees
    int max_theta;  // Maximum theta angle in degrees
    int step_theta; // Step size for theta in degrees
    int num_theta;  // Total number of theta angles
    int min_phi;    // Minimum phi angle in degrees
    int max_phi;    // Maximum phi angle in degrees
    int step_phi;   // Step size for phi in degrees
    int num_phi;    // Total number of phi angles 

    // FFT variables
    fftwf_plan fft_plan;              // Plan for fft to reuse
    fftwf_complex *fft_input_buffer;  // 1D buffer for input
    fftwf_complex *fft_output_buffer; // 1D buffer for output

    // Arrays
    array5D<complex<float>> directivity_factor; // (theta, phi, m, n, bin)
    array3D<complex<float>> data_beamform;      // (theta, phi, buffer) need to add bin dim later
    array3D<float> data_fft;                    // (theta, phi, bin)
    array2D<float> data_fft_collapse;           // (theta, phi)

    // Timers for profiling
    timer fft_time;
    timer beamform_time;
    timer fft_collapse_time;
    timer post_process_time;

}; // end class def

beamform::beamform(const int fft_size, const int sample_rate, const int m_channels, const int n_channels,
    const float mic_spacing, const float speed_of_sound,
    const int min_theta, const int max_theta, const int step_theta, const int num_theta,
    const int min_phi, const int max_phi, const int step_phi, const int num_phi) :
    
    // Assign values to variables
    fft_size(fft_size),
    sample_rate(sample_rate),

    mic_spacing(mic_spacing),
    speed_of_sound(speed_of_sound),

    min_theta(min_theta),
    max_theta(max_theta),
    step_theta(step_theta),
    num_theta(num_theta),
    min_phi(min_phi),
    max_phi(max_phi),
    step_phi(step_phi),
    num_phi(num_phi),

    // Initialize timer names
    fft_time("FFT"),
    beamform_time("Beamform"),
    fft_collapse_time("FFT Collapse"),
    post_process_time("Post Process"),

    // Allocate memory to arrays
    directivity_factor(num_theta, num_phi, m_channels, n_channels, fft_size),
    data_beamform(num_theta, num_phi, fft_size),
    data_fft(m_channels, n_channels, fft_size),
    data_fft_collapse(num_theta, num_phi)
    {} // end beamform

// Destructor
beamform::~beamform()
{
    // Free FFTW plan
    fftwf_destroy_plan(fft_plan);
} // end ~beamform

//=====================================================================================

// Converts degrees to radians
float beamform::degtorad(const float input_degrees)
{
    return input_degrees * M_PI / 180.0f;
} // end degtorad

// Converts array2D to cv::Mat
cv::Mat beamform::array2DtoMat(const array2D<float> &data)
{
    // Create a Mat and reassign data
    cv::Mat mat(data.dim_1, data.dim_2, CV_32FC1, data.data);
    return mat;
} // end array2DtoMat

//=====================================================================================

// Setup functions

void beamform::setupDirectivity()
{
    // Precompute directivity factor for all angles
    for (int theta = min_theta, theta_index = 0; theta_index < directivity_factor.dim_1; theta += step_theta, theta_index++)
    {
        for (int phi = min_phi, phi_index = 0; phi_index < directivity_factor.dim_2; phi += step_phi, phi_index++)
        {
            for (int m = 0; m < directivity_factor.dim_3; m++)
            {
                for (int n = 0; n < directivity_factor.dim_4; n++)
                {
                    for (int bin = 0; bin < directivity_factor.dim_5; bin++)
                    {
                        float frequency = (sample_rate * bin) / fft_size;
                        float wave_number = (2 * M_PI * frequency) / speed_of_sound; 
                        float exponent = -wave_number * mic_spacing * (m * sinf(degtorad(theta)) + n * sinf(degtorad(phi)));
                        directivity_factor.at(theta_index, phi_index, m, n, bin) = polar(1.0f, exponent);
                    } // end b
                } // end n
            } // end m
        } // end phi
    } // end theta
} // end directivity

void beamform::setupFFT()
{
    // Allocate all arrays
    fft_input_buffer  = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * fft_size); // Allocate buffer for FFT input
    fft_output_buffer = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * fft_size); // Allocate buffer for FFT output

    // Create FFT plan
    fft_plan = fftwf_plan_dft_1d(fft_size, fft_input_buffer, fft_output_buffer, FFTW_FORWARD, FFTW_ESTIMATE);
} // end setupFFT

void beamform::setup()
{
    setupDirectivity();
    setupFFT();
} // end setup

//=====================================================================================

void beamform::handleBeamforming(array3D<float> &data_input, const uint8_t lower_frequency, const uint8_t upper_frequency)
{
    for (int theta = 0; theta < data_beamform.dim_1; theta++)
    {
        for (int phi = 0; phi < data_beamform.dim_2; phi++)
        {
            for (int m = 0; m < data_input.dim_1; m++)
            {
                for (int n = 0; n < data_input.dim_2; n++)
                {
                    // Cache the directivity factor
                    complex<float> directivity = directivity_factor.at(theta, phi, m, n, 21); // 984Hz
                    for (int b = 0; b < data_input.dim_3; b++)
                    {
                        // Calculate and sum correlated samples
                        data_beamform.at(theta, phi, b) += data_input.at(m, n, b) * directivity;
                    } // end b
                } // end n
            } // end m
        } // end phi
    } // end theta
} // end handleBeamforming

//=====================================================================================

void beamform::FFT()
{
    // Loop through each angle and perform 1D FFT on each
    for (int theta = 0; theta < data_beamform.dim_1; theta++)
    {
        for (int phi = 0; phi < data_beamform.dim_2; phi++)
        {
            // Write data to input buffer
            for (int b = 0; b < data_beamform.dim_3; b++)
            {
                fft_input_buffer[b][0] = real(data_beamform.at(theta, phi, b));
                fft_input_buffer[b][1] = imag(data_beamform.at(theta, phi, b));
            } // end b

            // Call fft plan
            fftwf_execute(fft_plan);

            // Convert to dBfs
            for (int bin = 0; bin < data_fft.dim_3; bin++)
            {
                float real = fft_output_buffer[bin][0] / fft_size; // Normalize by FFT size
                float imag = fft_output_buffer[bin][1] / fft_size; // Normalize by FFT size
                float inside = real * real + imag * imag;
                data_fft.at(theta, phi, bin) = 20 * log10f(sqrt(inside));
            } // end b
        } // end n
    } // end m
} // end FFT

//=====================================================================================

void beamform::FFTCollapse(const uint8_t lower_frequency, const uint8_t upper_frequency)
{
    // dB addition
    for (int theta = 0; theta < data_fft.dim_1; theta++)
    {
        for (int phi = 0; phi < data_fft.dim_2; phi++)
        {
            float sum = 0;
            for (int b = lower_frequency; b <= upper_frequency; b++)
            {
                sum += powf(10, data_fft.at(theta, phi, b) / 10);
            } // end b

            data_fft_collapse.at(theta, phi) = 10 * log10f(sum);
            // data_fft_collapse.at(theta, phi) = abs(sum) / (upper_frequency - lower_frequency + 1); // Normalize by number of bins
        } // end phi
    } // end theta
} // end FFTCollapse

//=====================================================================================

void beamform::processData(cv::Mat &data_output, const int lower_frequency, const int upper_frequency,
                           array3D<float> &data_buffer_1, array3D<float> &data_buffer_2)
{

} // end processData