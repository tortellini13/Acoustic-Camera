#ifndef BEAMFORM_H
#define BEAMFORM_H

// Libraries
#include <iostream>
#include <cstring> // Memory manipulation for ring buffer
#include <omp.h>   // Multithreading
#include <fftw3.h> // FFT
#include <opencv2/opencv.hpp> // OpenCV

// Headers
#include "PARAMS.h"
#include "Timer.h"
#include "Structs.h"

class beamform
{
public:
    beamform(int fft_size, int sample_rate, int m_channels, int n_channels,
             float mic_spacing, int min_angle, int max_angle, int angle_step, float speed_of_sound);

    ~beamform();

    void setup();

    void processData(array3D<float>& data_input, cv::Mat& data_output, const int lower_frequency, const int upper_frequency, uint8_t post_process_type);
    
private:
    // Converts degrees to radians
    float degtorad(const float angle_deg);

    // Performs beamforming on audio data
    void handleBeamforming(array3D<float>& data_input);

    // Creates FFT plan
    void setupFFT();

    // Performs FFT on beamformed data
    void FFT();

    // Combines all bins and band passes data
    void FFTCollapse(const int lower_frequency, const int upper_frequency);

    // Performs designated post processing
    void postProcess(uint8_t post_process_type, const int lower_frequency, const int upper_frequency);

    // Converts float2D to Mat
    cv::Mat arraytoMat(const array2D<float>& data);

    // Initial conditions
    int fft_size;
    int half_fft_size;
    int sample_rate;
    int m_channels;
    int n_channels;
    int num_channels;
    float mic_spacing;
    int min_angle;
    int max_angle;
    int angle_step;
    int num_angles;
    float speed_of_sound;
    int data_size;

    // Plan for fft to reuse
    fftwf_plan fft_plan;

    // Arrays
    array4D<complex<float>> steering_array;    // (m, n, theta, phi)
    array3D<complex<float>> data_beamform;     // (theta, phi, fft_size)
    array3D<complex<float>> data_fft; // (theta, phi, fft_size / 2)
    array2D<float> data_fft_collapse; // (theta, phi)
    array2D<float> data_post_process; // (theta, phi)

    // Timers
    timer beamform_time;
    timer fft_time;
    timer fft_collapse_time;
    timer post_process_time;
};

beamform::beamform(int fft_size, int sample_rate, int m_channels, int n_channels,
                   float mic_spacing, int min_angle, int max_angle, int angle_step, float speed_of_sound) :
    fft_size(fft_size),
    half_fft_size(fft_size / 2),
    sample_rate(sample_rate),
    m_channels(m_channels),
    n_channels(n_channels),
    num_channels(m_channels * n_channels),
    mic_spacing(mic_spacing),
    min_angle(min_angle),
    max_angle(max_angle),
    angle_step(angle_step),
    num_angles(((max_angle - min_angle) / angle_step) + 1),
    speed_of_sound(speed_of_sound),
    data_size(m_channels * n_channels * fft_size),

    // Allocate memory for arrays
    steering_array(m_channels, n_channels, num_angles, num_angles),
    data_beamform(num_angles, num_angles, fft_size),
    data_fft(num_angles, num_angles, half_fft_size),
    data_fft_collapse(num_angles, num_angles),
    data_post_process(num_angles, num_angles),

    // Timers
    beamform_time("Beamform"),
    fft_time("FFT"),
    fft_collapse_time("FFT Collapse"),
    post_process_time("Post Process")
    {}

beamform::~beamform()
{

} // end ~beamform

//=====================================================================================

float beamform::degtorad(const float angle_deg)
{
    return angle_deg * M_PI / 180.0f;
} // end degtorad

//=====================================================================================

void beamform::setupFFT()
{
    // Enable FFTW multithreading
    if (fftwf_init_threads() == 0)
    {
        cerr << "Error: FFTW threading initialization failed.\n";
        throw runtime_error("FFTW threading initialization failed");
    }

    // Set the number of threads for FFTW
    fftwf_plan_with_nthreads(omp_get_max_threads()); // Use all available threads

    fftwf_complex* input  = reinterpret_cast<fftwf_complex*>(data_beamform.data); // Allocate input array
    fftwf_complex* output = reinterpret_cast<fftwf_complex*>(data_fft.data);      // Allocate output array

    fft_plan = fftwf_plan_dft_3d(data_beamform.dim_1, data_beamform.dim_2, data_beamform.dim_3, 
                                 input, output, 
                                 FFTW_FORWARD, FFTW_MEASURE);

    if (!fft_plan)
    {
        cerr << "Error: Failed to create FFT plan.\n";
        throw runtime_error("FFTW plan creation failed");
    }
} // end setupFFT

//=====================================================================================

void beamform::setup()
{
    // Creates FFT plan
    setupFFT();

    complex<float> i(0, 1);
    // Loop through all indicies to calculate time delays
    for (int phi = min_angle, phi_index = 0; phi_index < num_angles; phi += angle_step, phi_index++)
    {
        float sin_phi = sinf(degtorad(phi));
        for (int theta = min_angle, theta_index = 0; theta_index < num_angles; theta += angle_step, theta_index++)
        {
            float sin_theta = sinf(degtorad(theta));
            float cos_theta = cosf(degtorad(theta));
            for (int n = 0; n < n_channels; n++)
            {
                for (int m = 0; m < m_channels; m++)
                {
                    float exponent = -2 * M_PI * mic_spacing * (m * sin_theta * sin_phi + n * cos_theta * sin_theta) / speed_of_sound;
                    //cout << "m: " << m << " n: " << n << " theta: " << theta << " phi: " << phi << " exponent: " << exponent << "\n";
                    steering_array.at(m, n, theta, phi) = exp(exponent * i);
                    //cout << "m: " << m << " n: " << n << " theta: " << theta << " phi: " << phi << " steering array: " << exp(exponent * i) << "\n";
                }
            }
        }
    }

    cout << "Finished setting up Beamform.\n"; // (debugging)

    // cout << "Max delay value: " << max_val << "\n";
    // cout << "Min delay value: " << min_val << "\n";
} // end setup

//=====================================================================================

void beamform::handleBeamforming(array3D<float>& data_input)
{
    #pragma omp parallel for schedule(dynamic, 2)
    for (int phi_index = 0; phi_index < data_beamform.dim_2; phi_index++) 
    {
        for (int theta_index = 0; theta_index < data_beamform.dim_1; theta_index++) 
        {
            for (int b = 0; b < data_beamform.dim_3; b++) 
            {
                complex<float> sum(0, 0);
                for (int n = 0; n < n_channels; n++) 
                {
                    for (int m = 0; m < m_channels; m++) 
                    {
                        sum += data_input.at(m, n, b) * steering_array.at(m, n, theta_index, phi_index);
                    }
                }
                data_beamform.at(theta_index, phi_index, b) = sum;
            }
        }
    }
} // end handleBeamforming

//=====================================================================================

// Performs FFT on audio data, real-to-complex
void beamform::FFT()
{
    // Execute the FFT
    fftwf_execute(fft_plan);
} // end FFT

//=====================================================================================

// Sums all frequency bands in desired range. Result is num_angles x num_angles array of complex values
void beamform::FFTCollapse(const int lower_frequency, const int upper_frequency)
{
    // cout << "FFTCollapse start\n"; // (debugging)
    int num_bins = upper_frequency - lower_frequency + 1;
    for (int theta_index = 0; theta_index < data_fft.dim_1; theta_index++)
    {
        for (int phi_index = 0; phi_index < data_fft.dim_2; phi_index++)
        {
            complex<float> sum = 0;
            for (int k = lower_frequency; k <= upper_frequency; k++)
            {
                sum += data_fft.at(theta_index, phi_index, k);
            }

            // Write data to array
            // divide by num_bins to normalize?
            data_fft_collapse.at(theta_index, phi_index) = abs(sum) / num_bins;
        }
    }
} // end FFTCollapse

//=====================================================================================

// Performs the desired post processing (ie. dBFS, dBA, etc.)
void beamform::postProcess(uint8_t post_process_type, const int lower_frequency, const int upper_frequency)
{
    // cout << "postProcess start\n"; // (debugging)
    for (int theta = 0; theta < data_post_process.dim_1; theta++)
    {
        for (int phi = 0; phi < data_post_process.dim_2; phi++)
        {
            switch (post_process_type) 
            {
            case POST_dBFS:
                // cout << "Post Process Type: dBFS\n"; // (debugging)
                // 20 * log10(abs(signal) / max_possible_value)
                float max_signal_value = 1.0f;
                data_post_process.at(theta, phi) = 20 * log10(data_fft_collapse.at(theta, phi) / max_signal_value); // Check the max value***
            break;
            }
        }
    }
    // cout << "postProcess end\n"; // (debugging)
} // end postProcess

//=====================================================================================

cv::Mat beamform::arraytoMat(const array2D<float>& data)
{
    // Create a Mat and reassign data
    cv::Mat mat(data.dim_1, data.dim_2, CV_32FC1, data.data);
    return mat;
} // end arraytoMat

//=====================================================================================

void beamform::processData(array3D<float>& data_input, cv::Mat& data_output, const int lower_frequency, const int upper_frequency, uint8_t post_process_type)
{
    // Beamforming
    beamform_time.start();
    handleBeamforming(data_input);
    beamform_time.end();

    #ifdef PRINT_BEAMFORM 
    data_beamform.print_layer(0); 
    #endif

    // FFT
    fft_time.start();
    FFT();
    fft_time.end();

    #ifdef PRINT_FFT
    data_fft.print_layer(23);
    #endif

    // FFT Collapse
    fft_collapse_time.start();
    FFTCollapse(lower_frequency, upper_frequency);
    fft_collapse_time.end();

    #ifdef PRINT_FFT_COLLAPSE
    data_fft_collapse.print();
    #endif

    // Post Process
    post_process_time.start();
    postProcess(post_process_type, lower_frequency, upper_frequency);
    post_process_time.end();

    #ifdef PRINT_POST_PROCESS
    data_post_process.print();
    #endif

    // Convert output to cv::Mat
    data_output = arraytoMat(data_post_process);

    #ifdef PROFILE_BEAMFORM
    beamform_time.print_avg(AVG_SAMPLES);
    fft_time.print_avg(AVG_SAMPLES);
    fft_collapse_time.print_avg(AVG_SAMPLES);
    post_process_time.print_avg(AVG_SAMPLES);
    if (beamform_time.getCurrentAvgCount() > AVG_SAMPLES - 1)
    {
        cout << "\n";
    }

    float total_time = beamform_time.time() + fft_time.time() + fft_collapse_time.time() + post_process_time.time();
    //cout << "Total Time: " << total_time << " ms.\n\n";
    #endif

} // end processData

#endif