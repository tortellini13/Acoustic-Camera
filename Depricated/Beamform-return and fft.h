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

    void processData(array3D<float>& data_input, cv::Mat& data_output, const int lower_frequency, const int upper_frequency, uint8_t post_process_type);
    
private:
    // Calculates time delays
    void setupDelays();

    // Calculates FIR weights
    void setupFIR();

    // Creates FFT plan
    void setupFFT(int dim_1, int dim_2, int dim_3);

    // Deletes FFT
    void cleanupFFT(array3D<float>& data_input, array3D<complex<float>> data_output);

    // Converts degrees to radians
    float degtorad(const float angle_deg);

    // Writes data into a ring buffer
    void ringBuffer(array3D<float>& data_input);

    // Access buffers at specified indicies
    float accessBuffer(int m_index, int n_index, int b);

    // Performs beamforming on audio data
    array3D<float> handleBeamforming();

    // Performs FFT on beamformed data
    void FFT(array3D<float>& data_input);

    // Combines all bins and band passes data
    void FFTCollapse(const int lower_frequency, const int upper_frequency);

    // Performs designated post processing
    void postProcess(uint8_t post_process_type, const int lower_frequency, const int upper_frequency);

    // Converts float2D to Mat
    cv::Mat arraytoMat(const array2D<float>& data);

    // Initial conditions
    int fft_size;      // Size of FFT and buffer
    int half_fft_size; // Half of FFT size
    int sample_rate;   // Audio sample rate
    int m_channels;    // Number of channels in M dimension
    int n_channels;    // Number of channels in N dimension
    int num_channels;  // Total number of channels (N * M)
    float mic_spacing; // Spacing between microphones in meters

    int min_theta;     // Minimum azimuth 
    int max_theta;     // Maximum azimuth
    int step_theta;    // Step size for azimuth
    int num_theta;     // Total number of azimuth angles

    int min_phi;       // Minimum elevation
    int max_phi;       // Maximum elevation
    int step_phi;      // Step size for elevation
    int num_phi;       // Total number of elevation angles

    float speed_of_sound; // Speed of sound in m/s
    int data_size;        // ???
    int num_taps;         // Number of taps for FIR filter
    int tap_offset;       // Number of taps to the left of 0

    int beamform_data_size; // ???

    // Plan for fft to reuse
    fftwf_plan fft_plan;
    //float* fft_input;
    //fftwf_complex* fft_output;

    // Arrays
    array4D<int>   time_delay_int;    // (m, n, theta, phi)
    array4D<float> time_delay_frac;   // (m, n, theta, phi)
    array5D<float> FIR_weights;       // (m, n, theta, phi, num_taps)
    array3D<float> data_buffer_1;     // (m, n, fft_size)
    array3D<float> data_buffer_2;     // (m, n, fft_size)
    array3D<float> data_beamform;     // (theta, phi, fft_size)
    array3D<float> data_fft_input;    // (theta, phi, fft_size)
    array3D<complex<float>> data_fft_output; // (theta, phi, fft_size / 2)
    array2D<float> data_fft_collapse; // (theta, phi)
    array2D<float> data_post_process; // (theta, phi)

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
    data_buffer_1(m_channels, n_channels, fft_size),
    data_buffer_2(m_channels, n_channels, fft_size),
    data_beamform(num_theta, num_phi, fft_size),
    data_fft_input(num_theta, num_phi, fft_size),
    data_fft_output(num_theta, num_phi, half_fft_size + 1),
    data_fft_collapse(num_theta, num_phi),
    data_post_process(num_theta, num_phi),

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
        for (int theta = min_theta, theta_index = 0; theta_index < num_theta; theta += step_theta, theta_index++)
        {
            float sin_theta = sinf(theta);
            float cos_theta = cosf(theta);
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
                    time_delay_int.at(m, n, theta_index, phi_index) = static_cast<int>(time_delay_int_temp) + floor(num_taps / 2);
                    
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
                        int tap_value = -floor(num_taps / 2) + tap_index;

                        // Hamming window
                        float window = 0.53836f - 0.46164f * cosf((2 * M_PI * tap_value) / (num_taps - 1));

                        float x = tap_value - time_delay_frac.at(m, n, theta_index, phi_index);

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

void beamform::setupFFT(int dim_1, int dim_2, int dim_3)
{
    /* // Allocate input and output buffers
    fft_input = (float*)fftwf_malloc(sizeof(float) * num_theta * num_phi * fft_size);
    fft_output = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * num_theta * num_phi * (fft_size / 2 + 1));

    // Batched 1D FFT parameters
    int batch_size = num_theta * num_phi;  // Number of independent 1D FFTs
    int rank = 1;                  // 1D FFTs
    int n[] = { fft_size };         // FFT length per batch
    int howmany = batch_size;       // Number of independent transforms

    int istride = 1, idist = fft_size;         // Input stride
    int ostride = 1, odist = (fft_size / 2 + 1);  // Output stride

    // Create batched 1D real-to-complex FFT plan
    fft_plan = fftwf_plan_many_dft_r2c(rank, n, howmany,
                                   fft_input, nullptr, istride, idist,
                                   fft_output, nullptr, ostride, odist,
                                   FFTW_MEASURE);
 */

    
    // Enable FFTW multithreading
    if (fftwf_init_threads() == 0)
    {
        cerr << "Error: FFTW threading initialization failed.\n";
        throw runtime_error("FFTW threading initialization failed");
    }

    // Set the number of threads for FFTW
    fftwf_plan_with_nthreads(omp_get_max_threads()); // Use all available threads

    float* fft_input = reinterpret_cast<float*>(data_fft_input.data); // Allocate real input array
    fftwf_complex* fft_output = reinterpret_cast<fftwf_complex*>(data_fft_output.data); // Allocate complex output array

    fft_plan = fftwf_plan_dft_r2c_3d(data_fft_input.dim_1, data_fft_input.dim_2, data_fft_input.dim_3, 
                                     fft_input, fft_output, 
                                     FFTW_ESTIMATE);

    if (!fft_plan)
    {
        cerr << "Error: Failed to create FFT plan.\n";
        throw runtime_error("FFTW plan creation failed");
    }
    
} // end setupFFT

void beamform::cleanupFFT(array3D<float>& data_input, array3D<complex<float>> data_output)
{
    if (fft_plan)
    {
        fftwf_destroy_plan(fft_plan);
        fft_plan = nullptr;
    }

    fftwf_free(data_input.data);
    fftwf_free(data_output.data);
    data_input.data = nullptr;
    data_output.data = nullptr;

    fftwf_cleanup_threads();
}

//=====================================================================================

void beamform::setup()
{
    // Calculates all time delays
    setupDelays();

    // Calculates all weights for windowed FIR filter
    setupFIR();

    // Creates FFT plan
    setupFFT(num_theta, num_phi, fft_size);

    // Initialize buffers with 0's
    for (int n = 0; n < data_buffer_1.dim_2; n++)
    {
        for (int m = 0; m < data_buffer_1.dim_1; m++)
        {
            for (int b = 0; b < data_buffer_1.dim_3; b++)
            {
                data_buffer_1.at(m, n, b) = 0.0f;
                data_buffer_2.at(m, n, b) = 0.0f;
            } // end b
        } // end m
    } // end n

    cout << "Finished setting up Beamform.\n"; // (debugging)

    // cout << "Max delay value: " << max_val << "\n";
    // cout << "Min delay value: " << min_val << "\n";
} // end setup

//=====================================================================================

void beamform::ringBuffer(array3D<float>& data_input)
{
    // Move buffer_2 into buffer_1
    swap(data_buffer_1.data, data_buffer_2.data);

    // Read new data into buffer_2
    memcpy(data_buffer_2.data, data_input.data, data_size);
} // end ringBuffer

//=====================================================================================

float beamform::accessBuffer(int m_index, int n_index, int b)
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

array3D<float> beamform::handleBeamforming()
{
    cout << "Beamform start\n";
    array3D<float> data_beamform_temp(num_theta, num_phi, fft_size);
    /*
    - For each coord
    - Every time integer delay is applied, and FIR for fractional delay
    - Delay sample by integer, then FIR at new index (FIR_index = int_delay + tap_value)
    */

    // Calculate beamforming and FIR and store into buffer for parallelization
    //#pragma omp for collapse(3) schedule(static, 4)
    for (int m = 0; m < m_channels; m++)
    {
        // cout << "Start m\n"; 

        for (int n = 0; n < n_channels; n++)
        {
            // cout << "Start n\n";

            for (int theta = 0; theta < num_theta; theta++)
            {
                // cout << "Start theta\n";

                for (int phi = 0; phi < num_phi; phi++)
                {
                    // cout << "Start phi\n";

                    // Calculate contribution to accessBuffer index     
                    int m_index_access = m * data_buffer_1.dim_2 * // Contribution of m to index
                                             data_buffer_1.dim_3;  

                    int n_index_access = n * data_buffer_1.dim_3;  // Contribution of n to index

                    // Calculate contribution to data_beamform_buffer index
                    int theta_index_result = theta * data_beamform.dim_2 * // Contribution of theta to index
                                                     data_beamform.dim_3;  

                    int phi_index_result =     phi * data_beamform.dim_3;  // Contribution of phi to index

                    int result_index = theta_index_result + phi_index_result; // Precalculate result index

                    // Calculate contribution to FIR_weights index
                    int m_index_weight =         m * FIR_weights.dim_2 * // Contribution of m to index
                                                     FIR_weights.dim_3 *
                                                     FIR_weights.dim_4 *
                                                     FIR_weights.dim_5;

                    int n_index_weight =         n * FIR_weights.dim_3 * // Contribution of n to index
                                                     FIR_weights.dim_4 *
                                                     FIR_weights.dim_5;

                    int theta_index_weight = theta * FIR_weights.dim_4 * // Contribution of theta to index
                                                     FIR_weights.dim_5;
                                             
                    int phi_index_weight =     phi * FIR_weights.dim_5;  // Contribution of phi to index

                    // Retrieve integer delay
                    int m_index_integer =         m * time_delay_int.dim_2 * // Contribution of m to index
                                                      time_delay_int.dim_3 *
                                                      time_delay_int.dim_4;

                    int n_index_integer =         n * time_delay_int.dim_3 * // Contribution of n to index
                                                      time_delay_int.dim_4;

                    int theta_index_integer = theta * time_delay_int.dim_4;  // Contribution of theta to index

                    int integer_index = m_index_integer + n_index_integer + theta_index_integer + phi; // Index of data point

                    int integer_delay = time_delay_int.data[integer_index]; // Retrieve integer delay and cache it
 
                    for (int tap = 0; tap < num_taps; tap++)
                    {
                        // cout << "Start tap\n";

                        // Makes tap (-num_taps / 2, num_taps / 2) so it looks backwards by half num_taps
                        int tap_value = tap_offset + tap;

                        // Precompute part of integer delay
                        int delay_offset = integer_delay + tap_value;

                        // Retrieve weight 
                        int weight_index = theta_index_weight + phi_index_weight + tap;
                        float weight = FIR_weights.data[weight_index];

                        // Single Instruction Multiple Data
                        float result = 0.0f;
                        //#pragma omp simd reduction(+:result)
                        for (int b = 0; b < fft_size; b++)
                        {
                            // cout << "Start b\n";

                            result += accessBuffer(m_index_access, n_index_access, delay_offset + b) * weight;
                        } // end b
                        
                        // Accumulate data with atomic variable across threads
                        //#pragma omp atomic
                        data_beamform_temp.data[result_index] += result; // This is the issue***

                    } // end tap
                } // end phi_index
            } // end theta_index
        } // end n
    } // end m    

    cout << "Beamform done\n";

    return data_beamform_temp;

} // end handleBeamforming

//=====================================================================================

// Performs FFT on audio data, real-to-complex
void beamform::FFT(array3D<float>& data_input)
{
    // Copy input data onto the array allocated for the fft
    memcpy(data_fft_input.data, data_input.data, num_theta * num_phi * fft_size);

    //data_fft_input = data_input;

    fftwf_execute(fft_plan);


    /*
    // Copy new data into FFTW input buffer
    std::memcpy(fft_input, new_input, sizeof(float) * data_size);

    // Execute the batched FFT
    fftwf_execute(fft_plan);

    // Flatten the 3D array and copy data directly into array3D.data() (raw memory access)
    for (int t = 0; t < num_theta; ++t) {
        for (int p = 0; p < num_phi; ++p) {
            for (int f = 0; f < fft_size / 2 + 1; ++f) {
                // Calculate the index in the flat array
                int index = (t * num_phi + p) * (fft_size / 2 + 1) + f;
                // Directly copy the complex values into the 3D array's flat memory
                data_fft.at(t, p, f) = std::complex<float>(output[index][0], output[index][1]);
            }
        }
    }
    */

    //fftwf_execute_dft_r2c(fft_plan, fft_input, output);
    
    // Execute the FFT
    //fftwf_execute(fft_plan);
} // end FFT

//=====================================================================================

// Sums all frequency bands in desired range. Result is num_angles x num_angles array of complex values
void beamform::FFTCollapse(const int lower_frequency, const int upper_frequency)
{
    // cout << "FFTCollapse start\n"; // (debugging)
    int num_bins = upper_frequency - lower_frequency + 1;
    for (int theta_index = 0; theta_index < data_fft_output.dim_1; theta_index++)
    {
        for (int phi_index = 0; phi_index < data_fft_output.dim_2; phi_index++)
        {
            complex<float> sum = 0;
            for (int k = lower_frequency; k <= upper_frequency; k++)
            {
                sum += data_fft_output.at(theta_index, phi_index, k);
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

    // cout << "ringBuffer\n";

    // Beamforming
    beamform_time.start();
    //array3D<float> beamform_help(num_theta, num_phi, fft_size);
    //cout << "Help created\n";
    //beamform_help = handleBeamforming();
    array3D<float> beamform_help = handleBeamforming();
    beamform_time.end();

    #ifdef PRINT_BEAMFORM 
    beamform_help.print_layer(0); 
    #endif

    cout << "handleBeamforming\n";

    // FFT

    // cout << beamform_help.dim_1 << " " << beamform_help.dim_2 << " " << beamform_help.dim_3 << "\n";
    fft_time.start();

    //cout << "setupFFT\n";
    //setupFFT(beamform_help, data_fft);
    //cout << "FFT\n";
    FFT(beamform_help);
    fft_time.end();

    #ifdef PRINT_FFT
    data_fft.
    #endif

    cout << "FFT\n";

    // FFT Collapse
    fft_collapse_time.start();
    FFTCollapse(lower_frequency, upper_frequency);
    fft_collapse_time.end();

    #ifdef PRINT_FFT_COLLAPSE
    data_fft_collapse.print();
    #endif

    cout << "FFTCollapse\n";

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

    //cleanupFFT(beamform_help, data_fft_output);

} // end processData

#endif