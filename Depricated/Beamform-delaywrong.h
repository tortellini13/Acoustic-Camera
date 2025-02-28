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

    // Writes data into a ring buffer
    void ringBuffer(array3D<float>& data_input);

    // Access buffers at specified indecies
    float accessBuffer(int m, int n, int b);

    // Linear interpolation
    float lerp(const float first_value, const float second_value, const float alpha);

    // Performs beamforming on audio data
    void handleBeamforming();

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
    array4D<int>   time_delay;        // (m, n, theta, phi)
    array4D<float> time_delay_frac;   // (m, n, theta, phi)
    array3D<float> data_buffer_1;     // (m, n, fft_size)
    array3D<float> data_buffer_2;     // (m, n, fft_size)
    array3D<float> data_beamform;     // (theta, phi, fft_size)
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
    time_delay(m_channels, n_channels, num_angles, num_angles),
    time_delay_frac(m_channels, n_channels, num_angles, num_angles),
    data_buffer_1(m_channels, n_channels, fft_size),
    data_buffer_2(m_channels, n_channels, fft_size),
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

    float* input = reinterpret_cast<float*>(data_beamform.data); // Allocate real input array
    fftwf_complex* output = reinterpret_cast<fftwf_complex*>(data_fft.data); // Allocate complex output array

    fft_plan = fftwf_plan_dft_r2c_3d(data_beamform.dim_1, data_beamform.dim_2, data_beamform.dim_3, 
                                     input, output, 
                                     FFTW_ESTIMATE);

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

    // Calculates delay times
    array4D<float> time_delay_float(m_channels, n_channels, num_angles, num_angles); // ***May need to implement interpolation

    // Initialize time delay
    float delay_time;

    // Initialize the minimum value to an arbitrary large value
    float min_val = MAXFLOAT;

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
                    delay_time = (mic_spacing * sin_phi * (n * sin_theta + m * cos_theta)) / speed_of_sound;

                    // Keep track of minimum value
                    if (delay_time < min_val)
                    {
                        min_val = delay_time;
                    }
                    time_delay_float.at(m, n, theta_index, phi_index) = delay_time;
                }
            }
        }
    }

    // Track max value for testing
    int max_val = -INT32_MAX;

    // Shift all delays so there are no negative values and convert to samples
    for (int phi_index = 0; phi_index < time_delay.dim_4; phi_index++)
    {
        for (int theta_index = 0; theta_index < time_delay.dim_3; theta_index++)
        {
            for (int n = 0; n < time_delay.dim_2; n++)
            {
                for (int m = 0; m < time_delay.dim_1; m++)
                {
                    // time_delay_samples = time_delay_float * sample_rate
                    float time_delay_int;
                    time_delay_frac.at(m, n, theta_index, phi_index) = modf(time_delay_float.at(m, n, theta_index, phi_index) * sample_rate, &time_delay_int);
                    time_delay.at(m, n, theta_index, phi_index) = static_cast<int>(time_delay_int);
                    //time_delay.at(m, n, theta_index, phi_index) = round((time_delay_float.at(m, n, theta_index, phi_index) - min_val) * sample_rate);

                    // Track max value for testing
                    if (time_delay.at(m, n, theta_index, phi_index) > max_val)
                    {
                        max_val = time_delay.at(m, n, theta_index, phi_index);
                    }
                }
            }
        }
    }

    // Initialize buffers with 0's
    for (int n = 0; n < data_buffer_1.dim_2; n++)
    {
        for (int m = 0; m < data_buffer_1.dim_1; m++)
        {
            for (int b = 0; b < data_buffer_1.dim_3; b++)
            {
                data_buffer_1.at(m, n, b) = 0.0f;
                data_buffer_2.at(m, n, b) = 0.0f;
            }
        }
    }

    cout << "Finished setting up Beamform.\n"; // (debugging)

    // cout << "Max delay value: " << max_val << "\n";
    // cout << "Min delay value: " << min_val << "\n";
} // end setup

//=====================================================================================

void beamform::ringBuffer(array3D<float>& data_input)
{
    // Move buffer_2 into buffer_1
    swap(data_buffer_1.data, data_buffer_2.data);

    // Read data into buffer_2
    memcpy(data_buffer_2.data, data_input.data, data_size);
} // end ringBuffer

//=====================================================================================

float beamform::accessBuffer(int m, int n, int b)
{
    if (b < fft_size)
    {
        return data_buffer_1.at(m, n, b);
    }

    else if (b >= fft_size)
    {
        return data_buffer_2.at(m, n, b - fft_size);
    }

    else
    {
        cerr << "Out of bounds buffer access.\n";
        return 0;
    }
} // end accessBuffer

float beamform::lerp(const float first_value, const float second_value, const float alpha)
{
    return (1 - alpha) * first_value + alpha * second_value;
} // end lerp

//=====================================================================================

void beamform::handleBeamforming()
{
    #pragma omp parallel for schedule(dynamic, 2)
    for (int phi_index = 0; phi_index < data_beamform.dim_2; phi_index++) 
    {
        for (int theta_index = 0; theta_index < data_beamform.dim_1; theta_index++) 
        {
            for (int b = 0; b < data_beamform.dim_3; b++) 
            {
                float sum = 0;
                for (int n = 0; n < n_channels; n++) 
                {
                    for (int m = 0; m < m_channels; m++) 
                    {
                        int index = b + time_delay.at(m, n, theta_index, phi_index);
                        sum += lerp(accessBuffer(m, n, index), accessBuffer(m, n, b + 1), time_delay_frac.at(m, n, theta_index, phi_index));
                        
                        
                        /* int delay = time_delay.at(m, n, theta_index, phi_index);
                        int index = b + delay;
                        sum += (index < fft_size)
                                ? data_buffer_1.at(m, n, index)
                                : data_buffer_2.at(m, n, index - fft_size); */
                    }
                }
                data_beamform.at(theta_index, phi_index, b) = sum / num_channels;
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
    // Ring buffer
    ringBuffer(data_input);

    // Beamforming
    beamform_time.start();
    handleBeamforming();
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