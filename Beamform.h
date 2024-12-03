#ifndef BEAMFORM_H
#define BEAMFORM_H

#include <iostream>
#include <complex>
#include <fftw3.h>
#include <omp.h>
#include <opencv2/opencv.hpp>

#include "PARAMS.h"
#include "Structs.h"
#include "Timer.h"

using namespace std;

//=====================================================================================

class beamform
{
public:
    beamform(int fft_size, int m_channels, int n_channels, int mic_spacing,
             int num_angles, int min_angle, int max_angle, int angle_step);

    ~beamform();

    void processData(const float3D& data_input, cv::Mat& data_output, int lower_frequency, int upper_frequency, uint8_t post_process_type);

private:
    // Performs beamformng algorithm
    void handleBeamforming(const float3D& data_input);

    // Performs FFT on beamformed data
    void FFT();

    // Band passes FFT and combines all bands
    void FFTCollapse(int lower_frequency, int upper_frequency);

    // Performs designated post-processing to data
    void postProcess(uint8_t post_process_type);

    // Converts float2D to Mat
    cv::Mat float2DToMat(const float2D& data);

    // Variables
    int fft_size;
    int m_channels;
    int n_channels;
    int mic_spacing;
    int num_angles;
    int total_angles;
    int min_angle;
    int max_angle;
    int angle_step;

    // Plan for fft to reuse
    fftwf_plan fft_plan;

    // Initialize data arrays
    const cfloat4D directivity_factor;
    cfloat3D data_beamform;
    cfloat3D data_fft;
    cfloat2D data_fft_collapse;
    float2D  data_post_process;

    // Initialize timers
    timer handleBeamforming_time;
    timer FFT_time;
    timer FFTCollapse_time;
    timer postProcess_time;
};

//=====================================================================================

beamform::beamform(int fft_size, int m_channels, int n_channels, int mic_spacing,
                   int num_angles, int min_angle, int max_angle, int angle_step) :
    // Assign values to variables
    fft_size(fft_size),
    m_channels(m_channels),
    n_channels(n_channels),
    mic_spacing(mic_spacing),
    num_angles(num_angles),
    total_angles(num_angles * num_angles),
    min_angle(min_angle),
    max_angle(max_angle),
    angle_step(angle_step),

    // Allocate memory to arrays
    angles(num_angles, num_angles),
    directivity_factor(m_channels, n_channels, num_angles, num_angles),
    data_beamform(num_angles, num_angles, fft_size),
    data_fft(num_angles, num_angles, fft_size),
    data_fft_collapse(num_angles, num_angles),
    data_post_process(num_angles, num_angles),

    // Initialize timers
    handleBeamforming_time("handleBeamforming"),
    FFT_time("FFT"),
    FFTCollapse_time("FFTCollapse"),
    postProcess_time("postProcess")
{
    // Precompute directivity factor for all angles
    for (int theta = min_angle, theta_index = 0; theta_index < num_angles; theta += angle_step, theta_index++)
    {
        for (int phi = min_angle, phi_index = 0; phi_index < num_angles; phi += angle_step, phi_index++)
        {
            for (int m = 1; m <= m_channels; m++)
            {
                for (int n = 1; n <= n_channels; n++)
                {
                    // m * d * sin(theta) * cos(phi) + n * d * sin(theta) * sin(phi) // Original exponent
                    // d * sin(theta) * (m * cos(phi) + n * sin(phi))                // Simplified exponent
                    float exponent = mic_spacing * sin(theta) * (m * cos(phi) + n * sin(phi));
                    directivity_factor.at(m - 1, n - 1, theta_index, phi_index) = (cos(exponent), sin(exponent));
                }
            }
        }
    }

    // Enable FFTW multithreading
    if (fftwf_init_threads() == 0)
    {
        cerr << "Error: FFTW threading initialization failed.\n";
        throw runtime_error("FFTW threading initialization failed");
    }

    // Set the number of threads for FFTW
    fftwf_plan_with_nthreads(omp_get_max_threads()); // Use all available threads
    // Create FFT plan
    fftwf_complex* input = reinterpret_cast<fftwf_complex*>(data_beamform.data);
    fftwf_complex* output = reinterpret_cast<fftwf_complex*>(data_fft.data);

    fft_plan = fftwf_plan_dft_3d(num_angles, num_angles, fft_size,
                                 input, output,
                                 FFTW_FORWARD,
                                 FFTW_ESTIMATE);

    if (!fft_plan)
    {
        cerr << "Error: Failed to create FFT plan.\n";
        throw runtime_error("FFTW plan creation failed");
    }

    /*
     // Precomputes all angles
    for (int theta = num_angles, theta_index = 0; theta < num_angles; theta += angle_step, theta_index++) 
    {
        for (int phi = min_angle, phi_index = 0; phi < num_angles; phi += angle_step, phi_index++) 
        {
            float angle = (theta * m_channels + phi * n_channels) * (M_PI / 180);
            angles.at(theta_index, phi_index) = cfloat(cos(angle), sin(angle));
        }
    } 
    */

} // end beamform

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

// Performs beamforming algorithm and outputs complex result
void beamform::handleBeamforming(const float3D& data_input)
{
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int theta_index = 0; theta_index < num_angles; theta_index++) 
    {
        for (int phi_index = 0; phi_index < num_angles; phi_index++) 
        {
            for (int b = 0; b < fft_size; b++) 
            {
                cfloat sum = {0.0f, 0.0f}; // Initialize accumulator for complex values

                for (int m = 0; m < m_channels; m++) 
                {
                    for (int n = 0; n < n_channels; n++) 
                    {
                        sum += data_input.at(m, n, b) * directivity_factor.at(m, n, theta_index, phi_index);
                    }
                }
                // Write to the output buffer
                data_beamform.at(theta_index, phi_index, b) = sum;
            } // end b
        } // end phi_index
    } // end theta_index
} // end handleBeamforming

//=====================================================================================

// Performs FFT on beamformed data, complex-to-complex
void beamform::FFT()
{
    // Execute the 3D FFT
    fftwf_execute(fft_plan);
    // cout << "FFT executed.\n"; // (debugging)

} // end FFT

//=====================================================================================

// Sums all frequency bands in desired range. Result is num_angles x num_angles array of complex values
void beamform::FFTCollapse(int lower_frequency, int upper_frequency)
{
    // cout << "FFTCollapse start\n"; // (debugging)
    for (int theta_index = 0; theta_index < num_angles; theta_index++)
    {
        for (int phi_index = 0; phi_index < num_angles; phi_index++)
        {
            cfloat sum = 0;
            for (int k = lower_frequency; k <= upper_frequency; k++)
            {
                sum += data_fft.at(theta_index, phi_index, k);
            }
            data_fft_collapse.at(theta_index, phi_index) = sum;
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
            case POST_dBFS:
                // cout << "dBFS\n"; // (debugging)
                // ***Not sure if need to abs() before adding signals***
                // 20 * log10(abs(signal) / num_signals)
                data_post_process.at(theta, phi) = 20 * log10(abs(data_fft_collapse.at(theta, phi)) / total_angles);
            break;
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

void beamform::processData(const float3D& data_input, cv::Mat& data_output, int lower_frequency, int upper_frequency, uint8_t post_process_type)
{
    #ifdef PROFILE_BEAMFORM
    handleBeamforming_time.start();
    #endif
    handleBeamforming(data_input);
    
    #ifdef PRINT_BEAMFORM
    // Print first frame of data
    for (int phi = 0; phi < NUM_ANGLES; phi++)
    {
        for (int theta = 0; theata < NUM_ANGLES;theta++)
        {
            cout << data_beamform.at(theta, phi, 0) << " ";
        }
        cout << endl << endl;
    }
    #endif

    #ifdef PROFILE_BEAMFORM
    handleBeamforming_time.end();

    FFT_time.start();
    #endif
    FFT();

    #ifdef PRINT_FFT
    // Print first frame of data
    for (int phi = 0; phi < NUM_ANGLES; phi++)
    {
        for (int theta = 0; theata < NUM_ANGLES;theta++)
        {
            cout << data_fft.at(theta, phi, 0) << " ";
        }
        cout << endl << endl;
    }
    #endif

    #ifdef PROFILE_BEAMFORM
    FFT_time.end();

    FFTCollapse_time.start();
    #endif
    FFTCollapse(lower_frequency, upper_frequency);

    #ifdef PRINT_FFT_COLLAPSE
    // Print first frame of data
    for (int phi = 0; phi < NUM_ANGLES; phi++)
    {
        for (int theta = 0; theata < NUM_ANGLES;theta++)
        {
            cout << data_fft_collapse.at(theta, phi, 0) << " ";
        }
        cout << endl << endl;
    }
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
        for (int theta = 0; theata < NUM_ANGLES;theta++)
        {
            cout << data_post_process.at(theta, phi, 0) << " ";
        }
        cout << endl << endl;
    }
    #endif

    #ifdef PROFILE_BEAMFORM
    postProcess_time.end();

    handleBeamforming_time.print();
    FFT_time.print();
    FFTCollapse_time.print();
    postProcess_time.print();
    cout << endl;
    #endif

    data_output = float2DToMat(data_post_process);

    #ifdef PRINT_OUTPUT
    // Print first frame of data
    for (int phi = 0; phi < NUM_ANGLES; phi++)
    {
        for (int theta = 0; theata < NUM_ANGLES;theta++)
        {
            cout << data_output.at(theta, phi, 0) << " ";
        }
        cout << endl << endl;
    }
    #endif

} // end processData

//=====================================================================================

#endif