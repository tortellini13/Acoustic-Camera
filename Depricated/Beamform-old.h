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
    beamform(int fft_size, int sample_rate, int m_channels, int n_channels, int num_taps,
             float mic_spacing, float speed_of_sound,
             int min_theta, int max_theta, int step_theta, int num_theta,
             int min_phi, int max_phi, int step_phi, int num_phi);

    ~beamform();

    void setup();

    void processData(array3D<float>& data_buffer_1, array3D<float>& data_buffer_2, cv::Mat& data_output, const int lower_frequency, const int upper_frequency, uint8_t post_process_type);
    
private:
    // Calculates time delays
    void setupDelays();

    // Calculates FIR weights
    void setupFIR();

    // Creates FFT plan
    void setupFFT();

    // Converts degrees to radians
    float degtorad(const float angle_deg);

    // Access buffers at specified indicies
    float accessBuffer(int m_index, int n_index, int b, array3D<float>& data_buffer_1, array3D<float>& data_buffer_2);

    // Performs beamforming on audio data
    void handleBeamforming(array3D<float>& data_buffer_1, array3D<float>& data_buffer_2);

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

    int min_theta;
    int max_theta;
    int step_theta;
    int num_theta;

    int min_phi;
    int max_phi;
    int step_phi;
    int num_phi;

    float speed_of_sound;
    int data_size;
    int num_taps; // move this into constructor
    int tap_offset;

    int beamform_data_size;

    // Plan for fft to reuse
    fftwf_plan fft_plan;

    // Arrays
    array4D<int>   time_delay_int;       // (m, n, theta, phi)
    array4D<float> time_delay_frac;      // (m, n, theta, phi)
    array5D<float> FIR_weights;          // (m, n, theta, phi, num_taps)
    array3D<float> data_beamform;        // (theta, phi, fft_size)
    array3D<complex<float>> data_fft;    // (theta, phi, fft_size / 2)
    array2D<float> data_fft_collapse;    // (theta, phi)
    array2D<float> data_post_process;    // (theta, phi)

    // Timers
    timer beamform_time;
    timer fft_time;
    timer fft_collapse_time;
    timer post_process_time;
};

beamform::beamform(int fft_size, int sample_rate, int m_channels, int n_channels, int num_taps,
                   float mic_spacing, float speed_of_sound,
                   int min_theta, int max_theta, int step_theta, int num_theta,
                   int min_phi, int max_phi, int step_phi, int num_phi) :
    fft_size(fft_size),
    half_fft_size(fft_size / 2),
    sample_rate(sample_rate),
    m_channels(m_channels),
    n_channels(n_channels),
    num_taps(num_taps),
    num_channels(m_channels * n_channels),
    mic_spacing(mic_spacing),

    min_theta(min_theta),
    max_theta(max_theta),
    step_theta(step_theta),
    num_theta(num_theta),

    min_phi(min_phi),
    max_phi(max_phi),
    step_phi(step_phi),
    num_phi(num_phi),

    speed_of_sound(speed_of_sound),
    data_size(m_channels * n_channels * fft_size),
    tap_offset(-floor(num_taps / 2)),

    beamform_data_size(num_theta * num_phi * fft_size),

    // Allocate memory for arrays
    time_delay_int(m_channels, n_channels, num_theta, num_phi),
    time_delay_frac(m_channels, n_channels, num_theta, num_phi),
    FIR_weights(m_channels, n_channels, num_theta, num_phi, num_taps),
    data_beamform(num_theta, num_phi, fft_size),
    data_fft(num_theta, num_phi, half_fft_size),
    data_fft_collapse(num_theta, num_phi),
    data_post_process(num_theta, num_phi),

    // Timers
    beamform_time("Beamform"),
    fft_time("FFT"),
    fft_collapse_time("FFT Collapse"),
    post_process_time("Post Process")
    {
        // Fill arrays with zeros
        time_delay_int.fill(0);
        time_delay_frac.fill(0.0f);
        FIR_weights.fill(0.0f);
        data_beamform.fill(0.0f);
        data_fft.fill(complex<float>(0.0f, 0.0f));
        data_fft_collapse.fill(0.0f);
        data_post_process.fill(0.0f);
    }

beamform::~beamform()
{

} // end ~beamform

//=====================================================================================

float beamform::degtorad(const float angle_deg)
{
    return angle_deg * M_PI / 180.0f;
} // end degtorad

//=====================================================================================

void beamform::setupDelays()
{
    // Calculates delay times
    array4D<float> time_delay_float(m_channels, n_channels, num_theta, num_phi); 

    // Initialize time delay
    float delay_time;

    // Initialize the minimum value to an arbitrary large value
    float min_val = MAXFLOAT;

    // Loop through all indicies to calculate time delays
    for (int phi = min_phi, phi_index = 0; phi_index < num_phi; phi += step_phi, phi_index++)
    {
        float sin_phi = sinf(phi);
        float cos_phi = cosf(phi);
        for (int theta = min_theta, theta_index = 0; theta_index < num_theta; theta += step_theta, theta_index++)
        {
            float sin_theta = sinf(theta);
            float cos_theta = cosf(theta);
            for (int n = 0; n < n_channels; n++)
            {
                for (int m = 0; m < m_channels; m++)
                {
                    delay_time = (mic_spacing * (n * sin_theta * sin_phi + m * sin_theta * cos_phi)) / speed_of_sound;

                    // Keep track of minimum value
                    if (delay_time < min_val)
                    {
                        min_val = delay_time;
                    }
                    time_delay_float.at(m, n, theta_index, phi_index) = delay_time;
                } // end m
            } // end n
        } // end theta_index
    } // end phi_index

    // Track max value for testing
    int max_val = -INT32_MAX;

    // Shift all delays so there are no negative values and convert to samples
    for (int phi_index = 0; phi_index < time_delay_int.dim_4; phi_index++)
    {
        for (int theta_index = 0; theta_index < time_delay_int.dim_3; theta_index++)
        {
            for (int n = 0; n < time_delay_int.dim_2; n++)
            {
                for (int m = 0; m < time_delay_int.dim_1; m++)
                {
                    float time_delay_int_temp;

                    // Extract fractional delay (0 - <1)
                    time_delay_frac.at(m, n, theta_index, phi_index) = modf(time_delay_float.at(m, n, theta_index, phi_index) * sample_rate, &time_delay_int_temp);
                    
                    // Integer part of delay, shifted right by half of taps for FIR filter
                    time_delay_int.at(m, n, theta_index, phi_index) = static_cast<int>(time_delay_int_temp) - tap_offset;
                    
                    // Track max value for testing
                    if (time_delay_int.at(m, n, theta_index, phi_index) > max_val)
                    {
                        max_val = time_delay_int.at(m, n, theta_index, phi_index);
                    }
                } // end m
            } // end n
        } // end theta_index
    } // end phi_index
} // end setupDelays

//=====================================================================================

void beamform::setupFIR()
{
    // Calculate taps from -M/2. Index from 0
    for (int phi_index = 0; phi_index < FIR_weights.dim_4; phi_index++)
    {
        for (int theta_index = 0; theta_index < FIR_weights.dim_3; theta_index++)
        {
            for (int n = 0; n < FIR_weights.dim_2; n++)
            {
                for (int m = 0; m < FIR_weights.dim_1; m++)
                {
                    float weight_sum = 0.0f;

                    // Compute filter taps and calculate sum
                    for (int tap_index = 0; tap_index < FIR_weights.dim_5; tap_index++)
                    {
                        int tap_value = tap_offset + tap_index;

                        // Hamming window
                        float a0 = (25.0f / 46.0f);
                        float window = a0 + (1.0f - a0) * cosf((2 * M_PI * static_cast<float>(tap_value)) / static_cast<float>(num_taps));

                        float x = static_cast<float>(tap_value) - time_delay_frac.at(m, n, theta_index, phi_index);

                        // Compute filter tap value
                        float weight = (x == 0) ? (1.0f * window) : ((sinf(M_PI * x) / (M_PI * x)) * window);

                        FIR_weights.at(m, n, theta_index, phi_index, tap_index) = weight;

                        weight_sum += weight; // Accumulate the sum
                    } // end tap

                    // Normalize the weights
                    for (int tap_index = 0; tap_index < FIR_weights.dim_1; tap_index++)
                    {
                        FIR_weights.at(m, n, theta_index, phi_index, tap_index) /= weight_sum;
                    }
                } // end m
            } // end n
        } // end theta_index
    } // end phi_index
} // setupFIR

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
    // Calculates all time delays
    setupDelays();

    // Calculates all weights for windowed FIR filter
    setupFIR();

    // Creates FFT plan
    setupFFT();

    cout << "Finished setting up Beamform.\n"; // (debugging)

    // cout << "Max delay value: " << max_val << "\n";
    // cout << "Min delay value: " << min_val << "\n";
} // end setup

//=====================================================================================

float beamform::accessBuffer(int m_index, int n_index, int b, array3D<float>& data_buffer_1, array3D<float>& data_buffer_2)
{
    if (b < fft_size)
    {
        return data_buffer_1.data[m_index + n_index + b];
    }

    else if (b >= fft_size)
    {
        return data_buffer_2.data[m_index + n_index + (b - fft_size)];
    }

    else
    {
        cerr << "Out of bounds buffer access: " << b << "\n";
        return 1;
    }
} // end accessBuffer

//=====================================================================================

void beamform::handleBeamforming(array3D<float>& data_buffer_1, array3D<float>& data_buffer_2)
{
    // data_buffer_1.print_layer(100);
    // #pragma omp for collapse(3) schedule(static, 4)
    for (int theta = 0; theta < num_theta; theta++)
    {
        for (int phi = 0; phi < num_phi; phi++)
        {
            for (int b = 0; b < fft_size; b++)
            {
                // To accumulate beamform data
                float result = 0.0f;
                for (int m = 0; m < m_channels; m++)
                {
                    for (int n = 0; n < n_channels; n++)
                    {
                        // Retrieve integer delay and cache it
                        int integer_delay = time_delay_int.at(m, n, theta, phi);

                        for (int tap = 0; tap < num_taps; tap++)
                        {
                            // Adjust integer delay for FIR filter
                            int delay_offset = integer_delay + tap_offset + tap;

                            // Retrieve weight and cache it
                            float weight = FIR_weights.at(m, n, theta, phi, tap);

                            // Perform beamforming
                            result += accessBuffer(m, n, delay_offset + b, data_buffer_1, data_buffer_2) * weight;
                            // if (abs(result) >= 1) {cout << result << "\n";} 
                            // cout << accessBuffer(m, n, delay_offset + b, data_buffer_1, data_buffer_2) << "\n";
                        } // end tap
                    } // end n
                } // end m
                // if (abs(result) >= 1) {cout << theta << " " << phi << " " << b << " " << result << "\n";}

                data_beamform.at(theta, phi, b) = result / static_cast<float>(m_channels * n_channels);
            } // end b
        } // end phi
    } // end theta
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

void beamform::processData(array3D<float>& data_buffer_1, array3D<float>& data_buffer_2, cv::Mat& data_output, const int lower_frequency, const int upper_frequency, uint8_t post_process_type)
{
    #ifdef PRINT_AUDIO
    data_buffer_1.print_layer(100);
    #endif

    // Beamforming
    beamform_time.start();
    handleBeamforming(data_buffer_1, data_buffer_2);
    beamform_time.end();

    #ifdef PRINT_BEAMFORM 
    data_beamform.print_layer(100); 
    #endif

    // cout << "handleBeamforming\n";

    // FFT
    fft_time.start();
    FFT();
    fft_time.end();

    #ifdef PRINT_FFT
    data_fft.print_layer(23);
    #endif

    // cout << "FFT\n";

    // FFT Collapse
    fft_collapse_time.start();
    FFTCollapse(lower_frequency, upper_frequency);
    fft_collapse_time.end();

    #ifdef PRINT_FFT_COLLAPSE
    data_fft_collapse.print();
    #endif

    // cout << "FFTCollapse\n";

    // Post Process
    post_process_time.start();
    postProcess(post_process_type, lower_frequency, upper_frequency);
    post_process_time.end();

    #ifdef PRINT_POST_PROCESS
    data_post_process.print();
    #endif

    // cout << "postProcess\n";

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

    // float total_time = beamform_time.time() + fft_time.time() + fft_collapse_time.time() + post_process_time.time();
    // cout << "Total Time: " << total_time << " ms.\n\n";
    #endif

} // end processData

#endif