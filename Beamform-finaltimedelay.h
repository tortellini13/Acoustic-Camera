#pragma once

// Libraries
#include <iostream>
#include <complex>
#include <cmath>              // for signbit
#include <fftw3.h>            // FFT
#include <omp.h>              // Multithreading
#include <opencv2/opencv.hpp> // OpenCV
#include <stk/DelayL.h>       // STK Delay

// Headers
#include "PARAMS.h"
#include "Structs.h"
#include "Timer.h"

class beamform
{
public:
    // Constructor
    beamform(const int fft_size, const int sample_rate, const int m_channels, const int n_channels, const int num_taps,
             const float mic_spacing, const float speed_of_sound,
             const int min_theta, const int max_theta, const int step_theta, const int num_theta,
             const int min_phi, const int max_phi, const int step_phi, const int num_phi);

    // Destructor
    ~beamform();

    // Sets up all constants and initialized FFT
    void setup();

    // Performs beamforming
    void processData(cv::Mat &data_output, const int lower_frequency, const int upper_frequency, const uint8_t post_process_type, array3D<float> &data_buffer_1, array3D<float> &data_buffer_2);

private:
    // Converts degrees to radians
    float degtorad(const float angle_deg);

    // Calculates time delays
    void setupDelays();

    // Calculates FIR weights
    void setupFIR();

    // Creates FFT plan
    void setupFFT();

    // Access buffers at specified indicies
    float accessBuffer(const int m, const int n, const int b, array3D<float> &data_buffer_1, array3D<float> &data_buffer_2);

    // Performs beamforming on audio data
    void handleBeamforming(array3D<float> &data_buffer_1, array3D<float> &data_buffer_2);

    // Performs FFT on beamformed data
    void FFT();

    // Combines all bins and band passes data
    void FFTCollapse(const int lower_frequency, const int upper_frequency);

    // Performs designated post processing
    void postProcess(const uint8_t post_process_type);

    // Converts float2D to Mat
    cv::Mat array2DtoMat(const array2D<float> &data);

    // Initial conditions
    int fft_size;     // Size of FFT in samples
    int sample_rate;  // Audio sample rate in Hz
    int m_channels;   // Number of microphones in the M direction
    int n_channels;   // Number of microphones in the N direction
    int num_channels; // Total number of microphones
    int num_taps;     // Number of taps for FIR filter
    int tap_offset;   // To center FIR filter around 0

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

    // Plan for fft to reuse
    fftwf_plan fft_plan;

    // Timers for profiling
    timer beamform_time;
    timer fft_time;
    timer fft_collapse_time;
    timer post_process_time;

    // Arrays
    // array4D<int> delay_time_int;      // (theta, phi, m, n)
    // array4D<float> delay_time_frac;   // (theta, phi, m, n)
    array4D<float> delay_time;    // (theta, phi, m, n)
    // array5D<float> FIR_weights;   // (theta, phi, m, n, num_taps)
    float hamming_weights[FFT_SIZE]; // Hamming window weights
    array3D<float> data_beamform; // (theta, phi, b)
    // array3D<complex<float>> data_fft; // (theta, phi, b / 2 + 1)
    float *fft_input_buffer;          // 1D buffer for input
    fftwf_complex *fft_output_buffer; // 1D buffer for output
    array3D<float> data_fft;          // (theta, phi, b / 2 + 1)
    array2D<float> data_fft_collapse; // (theta, phi)
    array2D<float> data_post_process; // (theta, phi)

    // Delay
    stk::DelayL delay; // STK delay object
};

beamform::beamform(const int fft_size, const int sample_rate, const int m_channels, const int n_channels, const int num_taps,
                   const float mic_spacing, const float speed_of_sound,
                   const int min_theta, const int max_theta, const int step_theta, const int num_theta,
                   const int min_phi, const int max_phi, const int step_phi, const int num_phi) : fft_size(fft_size),
                                                                                                  sample_rate(sample_rate),
                                                                                                  m_channels(m_channels),
                                                                                                  n_channels(n_channels),
                                                                                                  num_channels(m_channels * n_channels),
                                                                                                  num_taps(num_taps),
                                                                                                  tap_offset(-floor(num_taps / 2)),

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

                                                                                                  beamform_time("Beamform"),
                                                                                                  fft_time("FFT"),
                                                                                                  fft_collapse_time("FFT Collapse"),
                                                                                                  post_process_time("Post Process"),

                                                                                                  // delay_time_int(num_theta, num_phi, m_channels, n_channels),
                                                                                                  // delay_time_frac(num_theta, num_phi, m_channels, n_channels),
                                                                                                  delay_time(num_theta, num_phi, m_channels, n_channels),
                                                                                                  // FIR_weights(num_theta, num_phi, m_channels, n_channels, num_taps),
                                                                                                  data_beamform(num_theta, num_phi, fft_size),
                                                                                                  data_fft(num_theta, num_phi, fft_size / 2 + 1),
                                                                                                  data_fft_collapse(num_theta, num_phi),
                                                                                                  data_post_process(num_theta, num_phi),
                                                                                                  delay()
{
}

beamform::~beamform()
{

} // end ~beamform

//=====================================================================================

float beamform::degtorad(const float angle_deg)
{
    return angle_deg * (M_PI / 180.0f);
} // end degtorad

//=====================================================================================

void beamform::setupDelays()
{
    // Temporary variables
    // array4D<float> delay_time(num_theta, num_phi, m_channels, n_channels);
    float min_delay = MAXFLOAT;  // initialize with a big value and update later
    float max_delay = -MAXFLOAT; // initialize with a small value and update later

    // Calculate full delay time and keep track of lowest delay value
    for (int theta = min_theta, theta_index = 0; theta_index < delay_time.dim_1; theta += step_theta, theta_index++)
    {
        for (int phi = min_phi, phi_index = 0; phi_index < delay_time.dim_2; phi += step_phi, phi_index++)
        {
            // Convert theta and phi to radians
            // cout << "Theta: " << theta << " Phi: " << phi << "\n";
            float theta_r = degtorad(theta);
            float phi_r = degtorad(phi);

            // Normal vector to incoming plane wave
            vec3<float> normal;
            normal.x = sinf(theta_r) * cosf(phi_r);
            normal.y = sinf(theta_r) * sinf(phi_r);
            normal.z = cosf(theta_r);

            for (int m = 0; m < delay_time.dim_3; m++)
            {
                for (int n = 0; n < delay_time.dim_4; n++)
                {
                    // Distance between the origin and a normal vector from (m, n)
                    float numerator = normal.x * m * mic_spacing + normal.y * n * mic_spacing;
                    float denominator = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
                    float distance = numerator / denominator;

                    // Coordinate of the point from the normal vector, projected onto the plane
                    vec3<float> plane;
                    plane.x = m * mic_spacing - distance * normal.x;
                    plane.y = n * mic_spacing - distance * normal.y;
                    plane.z = -distance * normal.z;

                    // Magnitude of the plane vector
                    vec3<float> magnitude_comp;
                    magnitude_comp.x = m * mic_spacing - plane.x;
                    magnitude_comp.y = n * mic_spacing - plane.y;
                    magnitude_comp.z = -plane.z;
                    int sign = 1;
                    if (signbit(plane.z))
                    {
                        sign = -1;
                    }
                    float magnitude = sign * sqrtf(magnitude_comp.x * magnitude_comp.x + magnitude_comp.y * magnitude_comp.y + magnitude_comp.z * magnitude_comp.z); // signbit to account for if delay is ahead or behind (0,0) element

                    // Delay time in samples
                    float delay_samples = (magnitude / speed_of_sound) * sample_rate;
                    delay_time.at(theta_index, phi_index, m, n) = delay_samples;

                    // Keep track of lowest delay value
                    if (delay_samples < min_delay)
                    {
                        min_delay = delay_samples;
                    }

                    if (delay_samples > max_delay)
                    {
                        max_delay = delay_samples;
                    }
                } // end n
            } // end m
        } // end phi
    } // end theta

    // delay_time.print_layer(0, 0);

    // For debugging
    cout << "Min Delay: " << min_delay << "\n";
    cout << "Max Delay: " << max_delay << "\n";

    // Account for time travel by offsetting all delays by the minimum delay
    for (int theta = 0; theta < delay_time.dim_1; theta++)
    {
        for (int phi = 0; phi < delay_time.dim_2; phi++)
        {
            for (int m = 0; m < delay_time.dim_3; m++)
            {
                for (int n = 0; n < delay_time.dim_4; n++)
                {
                    delay_time.at(theta, phi, m, n) -= min_delay; // Offset all delays by the minimum delay
                } // end n
            } // end m
        } // end phi
    } // end theta

} // end setupDelays

//=====================================================================================

void beamform::setupFIR()
{
    /* // Calculate taps from -num_taps / 2. Index from 0
    for (int theta = 0; theta < FIR_weights.dim_1; theta++)
    {
        for (int phi = 0; phi < FIR_weights.dim_2; phi++)
        {
            for (int m = 0; m < FIR_weights.dim_3; m++)
            {
                for (int n = 0; n < FIR_weights.dim_4; n++)
                {
                    // Track sum of weights for normalization
                    float weight_sum = 0.0f;
                    for (int tap = 0; tap < FIR_weights.dim_5; tap++)
                    {
                        // Center tap around 0
                        int tap_value = tap + tap_offset;

                        // Hamming window
                        float a0 = (25.0f / 46.0f); // Magic numbers
                        float window = a0 + (1.0f - a0) * cosf((2 * M_PI * static_cast<float>(tap_value)) / static_cast<float>(num_taps));

                        // Compute filter tap value
                        float x = static_cast<float>(tap_value) - delay_time_frac.at(theta, phi, m, n);
                        float weight = (x == 0) ? (1.0f * window) : ((sinf(M_PI * x) / (M_PI * x)) * window); // Limit as x -> 0 = 1
                        FIR_weights.at(theta, phi, m, n, tap) = weight;

                        // Accumulate sum of weights
                        weight_sum += weight;
                    } // end tap

                    // Normalize weights to not add any gain to output
                    for (int tap = 0; tap < FIR_weights.dim_5; tap++)
                    {
                        FIR_weights.at(theta, phi, m, n, tap) /= weight_sum;
                    } // end tap
                } // end n
            } // end m
        } // end phi
    } // end theta  */
} // end setupFIR

//=====================================================================================

void beamform::setupFFT()
{
    /*
        // Enable FFTW multithreading
        if (fftwf_init_threads() == 0)
        {
            cerr << "Error: FFTW threading initialization failed.\n";
            throw runtime_error("FFTW threading initialization failed");
        }

        // Set the number of threads for FFTW
        fftwf_plan_with_nthreads(omp_get_max_threads()); // Use all available threads

        float *input = reinterpret_cast<float *>(data_beamform.data);             // Allocate real input array
        fftwf_complex *output = reinterpret_cast<fftwf_complex *>(data_fft.data); // Allocate complex output array

        fft_plan = fftwf_plan_dft_r2c_3d(data_beamform.dim_1, data_beamform.dim_2, data_beamform.dim_3,
                                         input, output,
                                         FFTW_ESTIMATE);

        if (!fft_plan)
        {
            cerr << "Error: Failed to create FFT plan.\n";
            throw runtime_error("FFTW plan creation failed");
        }
     */

    // Allocate all arrays
    fft_input_buffer = new float[FFT_SIZE];                                                        // Allocate buffer for FFT input
    fft_output_buffer = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * (FFT_SIZE / 2 + 1)); // Allocate buffer for FFT output

    // Create FFT plan
    fft_plan = fftwf_plan_dft_r2c_1d(FFT_SIZE, fft_input_buffer, fft_output_buffer, FFTW_ESTIMATE);

} // end setupFFT

//=====================================================================================

void beamform::setup()
{
    // Setup delays
    setupDelays();
    // cout << "setupDelays\n";

    // Setup FIR weights
    // setupFIR();
    // cout << "setupFIR\n";

    // Setup Hamming window
    for (int b = 0; b < FFT_SIZE; b++)
    {
        float a0 = (25.0f / 46.0f); // Magic numbers
        hamming_weights[b] = a0 - (1.0f - a0) * cosf((2 * M_PI * static_cast<float>(b)) / static_cast<float>(fft_size));
    } // end b

    // Setup FFT
    setupFFT();
    // cout << "setupFFT\n";

    // delay_time_frac.print_layer(0, 0);
    // FIR_weights.print_layer(0, 0, 0);
} // end setup

//=====================================================================================

float beamform::accessBuffer(const int m, const int n, const int b, array3D<float> &data_buffer_1, array3D<float> &data_buffer_2)
{
    if (b < fft_size)
    {
        return data_buffer_1.at(m, n, b);
    }

    else if (b >= fft_size)
    {
        return data_buffer_2.at(m, n, (b - fft_size));
    }

    else
    {
        cerr << "Out of bounds buffer access: " << b << "\n";
        return 1;
    }
} // end accessBuffer

//=====================================================================================

void beamform::handleBeamforming(array3D<float> &data_buffer_1, array3D<float> &data_buffer_2)
{
    /*
    // #pragma omp for collapse(3) schedule(static, 4)
    for (int theta = 0; theta < data_beamform.dim_1; theta++)
    {
        for (int phi = 0; phi < data_beamform.dim_2; phi++)
        {
            for (int b = 0; b < data_beamform.dim_3; b++)
            {
                // Accumulator for beamforming result
                float result = 0.0f;
                for (int m = 0; m < m_channels; m++)
                {
                    for (int n = 0; n < n_channels; n++)
                    {
                        // Retrieve integer delay
                        int integer_delay = delay_time_int.at(theta, phi, m, n);

                        // #pragma omp simd reduction(+:result)
                        for (int tap = 0; tap < num_taps; tap++)
                        {
                            // Retrieve weight for FIR filter
                            float weight = FIR_weights.at(theta, phi, m, n, tap);

                            // Apply offest for taps to integer delay
                            int total_integer_delay = integer_delay + tap_offset + tap;

                            // Access buffer at integer delay and tap index
                            result += accessBuffer(m, n, total_integer_delay + b, data_buffer_1, data_buffer_2) * weight; // b is to apply to b coordinate in data
                        } // end tap
                    } // end n
                } // end m

                // Normalize and write result
                data_beamform.at(theta, phi, b) = result / num_channels;
            } // end b
        } // end phi
    } // end theta
    */

    /*
    -
    */

    for (int theta = 0; theta < data_beamform.dim_1; theta++)
    {
        for (int phi = 0; phi < data_beamform.dim_2; phi++)
        {
            for (int b = 0; b < data_beamform.dim_3; b++)
            {
                // Accumulator for beamforming result
                float result = 0.0f;
                for (int m = 0; m < m_channels; m++)
                {
                    for (int n = 0; n < n_channels; n++)
                    {
                        delay.setDelay(delay_time.at(theta, phi, m, n));                           // Set delay in STK delay object
                        result += delay.tick(accessBuffer(m, n, b, data_buffer_1, data_buffer_2)); // Apply delay to input signal

                    } // end n
                } // end m

                // Normalize and write result
                data_beamform.at(theta, phi, b) = (result / num_channels) * hamming_weights[b]; // Apply Hamming window

            } // end b
        } // end phi
    } // end theta
} // end handleBeamforming

//=====================================================================================

void beamform::FFT()
{
    // Loop through each mic and perform 1D FFT on each
    for (int theta = 0; theta < data_beamform.dim_1; theta++)
    {
        for (int phi = 0; phi < data_beamform.dim_2; phi++)
        {
            // Write data to input buffer
            for (int b = 0; b < fft_size; b++)
            {
                fft_input_buffer[b] = data_beamform.at(theta, phi, b);
            } // end b

            // Call fft plan
            fftwf_execute(fft_plan);

            // Convert to dBfs
            for (int b = 0; b < FFT_SIZE / 2 + 1; b++)
            {
                float real = fft_output_buffer[b][0] / fft_size; // Normalize by FFT size
                float imag = fft_output_buffer[b][1] / fft_size; // Normalize by FFT size
                float inside = real * real + imag * imag;
                data_fft.at(theta, phi, b) = 20 * log10f(sqrt(inside));

            } // end b
        } // end n
    } // end m
} // end FFT

//=====================================================================================

void beamform::FFTCollapse(const int lower_frequency, const int upper_frequency)
{
    /*
        for (int theta = 0; theta < data_fft.dim_1; theta++)
        {
            for (int phi = 0; phi < data_fft.dim_2; phi++)
            {
                complex<float> sum = 0;
                for (int b = lower_frequency; b <= upper_frequency; b++)
                {
                    sum += data_fft.at(theta, phi, b);
                } // end b

                data_fft_collapse.at(theta, phi) = abs(sum);
            } // end phi
        } // end theta
     */
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

void beamform::postProcess(const uint8_t post_process_type)
{
    float max_signal_value = 1.0f;
    for (int theta = 0; theta < data_fft_collapse.dim_1; theta++)
    {
        for (int phi = 0; phi < data_fft_collapse.dim_2; phi++)
        {
            switch (post_process_type)
            {
            case POST_dBFS:
                // 20 * log10(abs(signal) / max_possible_value)
                data_post_process.at(theta, phi) = 20 * log10f(data_fft_collapse.at(theta, phi) / max_signal_value); // Check the max value***
                break;

            default:
                cerr << "Invalid post processing type.\n";
                break;
            } // end switch
        } // end phi
    } // end theta
} // end postProcess

//=====================================================================================

cv::Mat beamform::array2DtoMat(const array2D<float> &data)
{
    // Create a Mat and reassign data
    cv::Mat mat(data.dim_1, data.dim_2, CV_32FC1, data.data);
    return mat;
} // end array2DtoMat

//=====================================================================================

void beamform::processData(cv::Mat &data_output, const int lower_frequency, const int upper_frequency, const uint8_t post_process_type, array3D<float> &data_buffer_1, array3D<float> &data_buffer_2)
{
    // Beamforming
    // cout << "Handling Beamforming\n";
    beamform_time.start();
    handleBeamforming(data_buffer_1, data_buffer_2);
    beamform_time.end();

#ifdef PRINT_BEAMFORM
    data_beamform.print_layer(100);
#endif

    // FFT
    // cout << "Performing FFT\n";
    fft_time.start();
    FFT();
    fft_time.end();

#ifdef PRINT_FFT
    data_fft.print_layer(23);
#endif

    // FFT Collapse
    // cout << "Collapsing FFT\n";
    fft_collapse_time.start();
    FFTCollapse(lower_frequency, upper_frequency);
    fft_collapse_time.end();

#ifdef PRINT_FFT_COLLAPSE
    data_fft_collapse.print();
#endif

    // Post Process
    // cout << "Post Processing\n";
    // post_process_time.start();
    // postProcess(post_process_type);
    // post_process_time.end();

#ifdef PRINT_POST_PROCESS
    data_post_process.print();
#endif

    // Final output
    data_output = array2DtoMat(data_fft_collapse);

// Profiling
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

//=====================================================================================