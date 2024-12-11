#ifndef BEAMFORM_H
#define BEAMFORM_H

#include <iostream>
#include <complex>
#include <fftw3.h>
#include <omp.h>
#include <opencv2/opencv.hpp>
#include <iomanip>

#include "PARAMS.h"
#include "Structs.h"
#include "Timer.h"

using namespace std;

//=====================================================================================

class beamform
{
public:
    beamform(int fft_size, int sample_rate, int m_channels, int n_channels, int mic_spacing,
             int num_angles, int min_angle, int max_angle, int angle_step);

    ~beamform();

    void setup(const float3D& data_input);

    void processData(const float3D& data_input, cv::Mat& data_output, int lower_frequency, int upper_frequency, uint8_t post_process_type);

private:
    // Computes directivity factor
    void directivity();

    // Creates FFT plan
    void setupFFT(const float3D& data_input);

    // Performs FFT on audio data
    void FFT(const float3D& data_input);

    // Performs beamformng algorithm in frequency domain
    void handleBeamforming(int lower_frequency, int upper_frequency);

    // Band passes FFT and combines all bands
    void FFTCollapse(int lower_frequency, int upper_frequency);

    // Performs designated post-processing to data
    void postProcess(uint8_t post_process_type);

    // Converts float2D to Mat
    cv::Mat float2DToMat(const float2D& data);
    
    // Converts degrees to radians
    float degtorad(const float input_degrees);

    // Variables
    int fft_size;
    int half_fft_size;
    int sample_rate;
    int m_channels;
    int n_channels;
    int mic_spacing;
    int num_angles;
    int total_angles;
    int min_angle;
    int max_angle;
    int angle_step;
    float speed_of_sound = 343; // m/s

    // Plan for fft to reuse
    fftwf_plan fft_plan;

    // Initialize data arrays
    cfloat5D directivity_factor;
    cfloat3D data_fft;
    cfloat3D data_beamform;
    cfloat2D data_fft_collapse;
    float2D  data_post_process;

    // Initialize timers
    timer FFT_time;
    timer handleBeamforming_time;
    timer FFTCollapse_time;
    timer postProcess_time;
};

//=====================================================================================

beamform::beamform(int fft_size, int sample_rate, int m_channels, int n_channels, int mic_spacing,
                   int num_angles, int min_angle, int max_angle, int angle_step) :
    // Assign values to variables
    fft_size(fft_size),
    half_fft_size(fft_size / 2),
    sample_rate(sample_rate),
    m_channels(m_channels),
    n_channels(n_channels),
    mic_spacing(mic_spacing),
    num_angles(num_angles),
    total_angles(num_angles * num_angles),
    min_angle(min_angle),
    max_angle(max_angle),
    angle_step(angle_step),

    // Allocate memory to arrays
    directivity_factor(m_channels, n_channels, num_angles, num_angles, half_fft_size + 1),
    data_fft(m_channels, n_channels, half_fft_size + 1),
    data_beamform(num_angles, num_angles, half_fft_size + 1),
    data_fft_collapse(num_angles, num_angles),
    data_post_process(num_angles, num_angles),

    // Initialize timers
    handleBeamforming_time("handleBeamforming"),
    FFT_time("FFT"),
    FFTCollapse_time("FFTCollapse"),
    postProcess_time("postProcess")
{} // end beamform

//=====================================================================================

// Deconstructor for beamform class
beamform::~beamform()
{
    // Destroy the FFT plan
    fftwf_destroy_plan(fft_plan);
    fftwf_cleanup_threads();
    fftwf_cleanup();
} // end ~beamform

//=====================================================================================

void beamform::directivity()
{
    // Precompute directivity factor for all angles
    for (int theta = min_angle, theta_index = 0; theta_index < num_angles; theta += angle_step, theta_index++)
    {
        for (int phi = min_angle, phi_index = 0; phi_index < num_angles; phi += angle_step, phi_index++)
        {
            for (int m = 0; m < m_channels; m++)
            {
                for (int n = 0; n < n_channels; n++)
                {
                    for (int b = 0; b < half_fft_size + 1; b++)
                    {
                        // m * d * k * sin(theta) * cos(phi) + n * d * k * sin(theta) * sin(phi) // Original exponent
                        // d * k * sin(theta) * (m * cos(phi) + n * sin(phi))                    // Simplified exponent
                        float frequency = (sample_rate * b) / fft_size;
                        float wave_number = (2 * M_PI * frequency) / speed_of_sound;
                        float exponent = mic_spacing * wave_number * sin(degtorad(theta)) * (m * cos(degtorad(phi)) + n * sin(degtorad(phi)));
                        directivity_factor.at(m, n, theta_index, phi_index, b) = polar(1.0f, exponent);
                    }
                }
            }
        }
    }
} // end directivity

//=====================================================================================

void beamform::setupFFT(const float3D& data_input)
{
    // Enable FFTW multithreading
    if (fftwf_init_threads() == 0)
    {
        cerr << "Error: FFTW threading initialization failed.\n";
        throw runtime_error("FFTW threading initialization failed");
    }

    // Set the number of threads for FFTW
    fftwf_plan_with_nthreads(omp_get_max_threads()); // Use all available threads

    float* input = reinterpret_cast<float*>(data_input.data); // Allocate real input array
    fftwf_complex* output = reinterpret_cast<fftwf_complex*>(data_fft.data); // Allocate complex output array

    fft_plan = fftwf_plan_dft_r2c_3d(m_channels, n_channels, fft_size, 
                                     input, output, 
                                     FFTW_ESTIMATE);

    if (!fft_plan)
    {
        cerr << "Error: Failed to create FFT plan.\n";
        throw runtime_error("FFTW plan creation failed");
    }
} // end setupFFT

//=====================================================================================

void beamform::setup(const float3D& data_input)
{
    directivity();
    setupFFT(data_input);
} // end setup

//=====================================================================================

// Performs FFT on audio data, real-to-complex
void beamform::FFT(const float3D& data_input)
{
    // Execute the FFT
    fftwf_execute(fft_plan);
} // end FFT

//=====================================================================================

// Performs beamforming algorithm and outputs complex result
void beamform::handleBeamforming(int lower_frequency, int upper_frequency)
{
    // Parallelize the outer loops over theta and phi
    #pragma omp parallel for collapse(2) schedule(dynamic, 2) // Limit each thread to 2 loops at a time to avoid bad scheduling
    for (int theta_index = 0; theta_index < num_angles; theta_index++) 
    {
        for (int phi_index = 0; phi_index < num_angles; phi_index++) 
        {
            // Iterate over the frequency bins. (0,513) is max allowed
            for (int b = lower_frequency; b < upper_frequency; b++) 
            {
                cfloat sum = {0.0f, 0.0f};

                // Calculate and sum correlated samples
                #pragma omp simd
                for (int m = 0; m < m_channels; m++) 
                {
                    for (int n = 0; n < n_channels; n++) 
                    {
                        sum += data_fft.at(m, n, b) * directivity_factor.at(m, n, theta_index, phi_index, b);
                    }
                }

                // Write the result to the output
                data_beamform.at(theta_index, phi_index, b) = sum;
            } // end b
        } // end phi_index
    } // end theta_index
} // end handleBeamforming

//=====================================================================================

// Sums all frequency bands in desired range. Result is num_angles x num_angles array of complex values
void beamform::FFTCollapse(int lower_frequency, int upper_frequency)
{
    // cout << "FFTCollapse start\n"; // (debugging)
    int num_bins = upper_frequency - lower_frequency;
    for (int theta_index = 0; theta_index < num_angles; theta_index++)
    {
        for (int phi_index = 0; phi_index < num_angles; phi_index++)
        {
            cfloat sum = 0;
            for (int k = lower_frequency; k <= upper_frequency; k++)
            {
                sum += data_beamform.at(theta_index, phi_index, k);
            }
            // Write data to array and normalize
            data_fft_collapse.at(theta_index, phi_index) = sum / static_cast<float>(num_bins);
        }
    }
    // cout << "FFTCollapse end\n"; // (debugging)
} // end FFTCollapse

//=====================================================================================

// Performs the desired post processing (ie. dBFS, dBA, etc.)
void beamform::postProcess(uint8_t post_process_type)
{
    // cout << "postProcess start\n"; // (debugging)
    for (int theta = 0; theta < num_angles; theta++)
    {
        for (int phi = 0; phi < num_angles; phi++)
        {
            switch (post_process_type) 
            {
            case POST_dBFS:
                // cout << "dBFS\n"; // (debugging)
                // ***Not sure if need to abs() before adding signals***
                // 20 * log10(abs(signal) / num_signals)
                float inside = abs(data_fft_collapse.at(theta, phi)) / total_angles;
                data_post_process.at(theta, phi) = 20 * log10(inside);
            break;
            }
        }
    }
    // cout << "postProcess end\n"; // (debugging)
} // end postProcess

//=====================================================================================

// Function to cast float2D to OpenCV Mat
cv::Mat beamform::float2DToMat(const float2D& data) 
{
    // Create a Mat from the raw pointer of data, using the proper dimensions
    cv::Mat mat(data.dim_1, data.dim_2, CV_32FC1, data.data);
    return mat;
}

//=====================================================================================

float beamform::degtorad(const float input_degrees)
{
    return input_degrees * M_PI / 180.0f;
} // end degtorad

//=====================================================================================

void beamform::processData(const float3D& data_input, cv::Mat& data_output, int lower_frequency, int upper_frequency, uint8_t post_process_type)
{
    #ifdef PROFILE_BEAMFORM
    FFT_time.start();
    #endif
    FFT(data_input);

    #ifdef PRINT_FFT
    // Print first frame of data
    for (int n = 0; n < n_channels; n++)
    {
        for (int m = 0; m < m_channels; m++)
        {
            cout << setw(8) << fixed << setprecision(6) << showpos << data_fft.at(m, n, 22) << " ";
        }
        cout << endl;
    }
    cout << endl;
    #endif

    #ifdef PROFILE_BEAMFORM
    FFT_time.end();

    handleBeamforming_time.start();
    #endif
    handleBeamforming(lower_frequency, upper_frequency);
    
    #ifdef PRINT_BEAMFORM
    // Print some of a frame of data
    for (int phi = 0; phi < NUM_ANGLES - 10; phi++)
    {
        for (int theta = 0; theta < NUM_ANGLES - 10; theta++)
        {
            cout << setw(8) << fixed << setprecision(6) << showpos << data_beamform.at(theta, phi, 1) << " ";
        }
        cout << endl;
    }
    cout << endl;
    #endif

    #ifdef PROFILE_BEAMFORM
    handleBeamforming_time.end();

    FFTCollapse_time.start();
    #endif
    FFTCollapse(lower_frequency, upper_frequency);

    #ifdef PRINT_FFT_COLLAPSE
    // Print first frame of data
    for (int phi = 0; phi < NUM_ANGLES; phi++)
    {
        for (int theta = 0; theta < NUM_ANGLES; theta++)
        {
            cout << setw(8) << fixed << setprecision(6) << showpos << data_fft_collapse.at(theta, phi) << " ";
        }
        cout << endl;
    }
    cout << endl;
    #endif
    #ifdef PROFILE_BEAMFORM
    FFTCollapse_time.end();

    postProcess_time.start();
    #endif
    postProcess(post_process_type);

    #ifdef PRINT_POST_PROCESS
    // Print first frame of data
    for (int phi = 0; phi < NUM_ANGLES; phi++)
    {
        for (int theta = 0; theta < NUM_ANGLES; theta++)
        {
            cout << setw(8) << fixed << setprecision(6) << showpos << data_post_process.at(theta, phi) << " ";
        }
        cout << endl;
    }
    cout << endl;
    #endif

    #ifdef PROFILE_BEAMFORM
    postProcess_time.end();

    FFT_time.print();
    handleBeamforming_time.print();
    FFTCollapse_time.print();
    postProcess_time.print();
    cout << endl;
    #endif

    data_output = float2DToMat(data_post_process);

    #ifdef PRINT_OUTPUT
    // Print first frame of data
    for (int phi = 0; phi < NUM_ANGLES; phi++)
    {
        for (int theta = 0; theta < NUM_ANGLES; theta++)
        {
            cout << setw(8) << fixed << setprecision(6) << showpos << data_output.at(theta, phi) << " ";
        }
        cout << endl;
    }
    cout << endl;
    #endif

} // end processData

//=====================================================================================

#endif