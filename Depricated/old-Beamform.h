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

    void setup();

    // Beamforming > FFT > FFTCollapse > Post-Process
    void processData(float* data_input, float* data_output,
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
    void postProcessData(float* data_output, uint8_t post_process_type);

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
    cfloat constant_1[HALF_FFT_SIZE];
    cfloat constant_2[HALF_FFT_SIZE * HALF_FFT_SIZE];
    cfloat angles[TOTAL_ANGLES];

    // Data arrays
    float data_raw[NUM_CHANNELS * FFT_SIZE];
    cfloat data_beamform[TOTAL_ANGLES * FFT_SIZE];
    fftw_complex data_beamform_fftw[TOTAL_ANGLES * FFT_SIZE];
    //double* data_fft;
    fftw_complex data_fft[TOTAL_ANGLES * FFT_SIZE];
    float data_fft_collapse[TOTAL_ANGLES];
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
    num_angles((max_angle - min_angle) / angle_step),
    total_num_angles(((max_angle - min_angle) / angle_step) * ((max_angle - min_angle) / angle_step))
    
    // Allocate memory for all arrays
    {
/*         cout << "m_channels: " << m_channels << endl;
        cout << "n_channels: " << n_channels << endl;
        cout << "num_channels: " << num_channels << endl;
        cout << "fft_size: " << fft_size << endl;
        cout << "half_fft_size: " << half_fft_size << endl;
        cout << "min_angle: " << min_angle << endl;
        cout << "max_angle: " << max_angle << endl;
        cout << "num_angles: " << num_angles << endl;
        cout << "total_num_angles: " << total_num_angles << endl;
        
        // cout << sizeof(total_num_angles) << endl;
        // Constants
        constant_1 = new cfloat[half_fft_size];
        constant_2 = new cfloat[half_fft_size * half_fft_size];
        angles = new cfloat[num_angles * num_angles];

        // cout << "constant_1: " << sizeof(constant_1) << endl;
        // cout << "constant_2: " << sizeof(constant_2) << endl;
        // cout << "angles: " << sizeof(angles) << endl;

        // Data
        data_raw = new float[num_channels * fft_size * sizeof(float)];
        data_beamform = new cfloat[total_num_angles * fft_size];
        //data_fft = new double[total_num_angles * half_fft_size];
        data_fft = new fftw_complex[total_num_angles * fft_size];
        data_fft_collapse = new float[total_num_angles]; */

        // cout << "data_raw: " << sizeof(data_raw) << endl;
        // cout << "data_beamform: " << sizeof(data_beamform) << endl;
        // cout << "data_fft: " << sizeof(data_fft) << endl;
        // cout << "data_fft_collapse: " << sizeof(data_fft_collapse) << endl;

        // Precomputes all angles
/*         for (int theta = min_angle, thetaIndex = 0; theta < num_angles; theta += angle_step, thetaIndex++) 
        {
            for (int phi = min_angle, phiIndex = 0; phi < num_angles; phi += angle_step, phiIndex++) 
            {
                float angle = (theta * m_channels + phi * m_channels) * (M_PI / 180);
                angles[thetaIndex * NUM_ANGLES + phiIndex] = cfloat(cos(angle), sin(angle));
            }
        }  */
        
    };

//=====================================================================================

beamform::~beamform()
{
/*     // Constants
    delete[] constant_1;
    delete[] constant_2;
    delete[] angles;

    // Data
    delete[] data_raw;
    delete[] data_beamform;
    delete[] data_fft;
    delete[] data_fft_collapse; */
}

//=====================================================================================

void beamform::setup()
{
    /*
    cout << "Starting setup.\n";
    // Constants
    constant_1 = (cfloat*) malloc(half_fft_size * sizeof(cfloat));
    constant_2 = (cfloat*) malloc(half_fft_size * half_fft_size * sizeof(cfloat));
    angles = (cfloat*) malloc(num_angles * num_angles * sizeof(cfloat));

    cout << "Constants done.\n";
    // cout << "constant_1: " << sizeof(constant_1) << endl;
    // cout << "constant_2: " << sizeof(constant_2) << endl;
    // cout << "angles: " << sizeof(angles) << endl;

    // Data
    data_raw = (float*) malloc(num_channels * fft_size * sizeof(float));
    data_beamform = (cfloat*) malloc(total_num_angles * fft_size * sizeof(cfloat));
    data_beamform_fftw = (fftw_complex*) malloc(total_num_angles * fft_size * sizeof(fftw_complex));
    //data_fft = new double[total_num_angles * half_fft_size];
    data_fft = (fftw_complex*) malloc(total_num_angles * fft_size * sizeof(fftw_complex));
    data_fft_collapse = (float*) malloc(total_num_angles * sizeof(float));
    cout << "Data done.\n";

    // cout << "data_raw: " << sizeof(data_raw) << endl;
    // cout << "data_beamform: " << sizeof(data_beamform) << endl;
    // cout << "data_fft: " << sizeof(data_fft) << endl;
    // cout << "data_fft_collapse: " << sizeof(data_fft_collapse) << endl;
    */

/*     // Precomputes all angles
    for (int theta = min_angle, thetaIndex = 0; theta < num_angles; theta += angle_step, thetaIndex++) 
    {
        for (int phi = min_angle, phiIndex = 0; phi < num_angles; phi += angle_step, phiIndex++) 
        {
            float angle = (theta * m_channels + phi * m_channels) * (M_PI / 180);
            angles[thetaIndex * NUM_ANGLES + phiIndex] = cfloat(cos(angle), sin(angle));
        }
    }
    cout << "Finished setting up beamform.\n"; */
} // end setupBeamforming

//=====================================================================================

void beamform::handleBeamforming(float* data_raw)
{
    //#pragma omp parallel for collapse(2) // Parallelize theta and phi loops
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
    cout << "Start fft.\n";
    // Convert cfloat to fftw_complex
    for (int i = 0; i < num_angles * num_angles * fft_size; i++) 
    {
        data_beamform_fftw[i][0] = static_cast<double>(real(data_beamform[i])); // Real part
        data_beamform_fftw[i][1] = static_cast<double>(imag(data_beamform[i])); // Imaginary part
    }
    cout << "Converted.";

    // Create a plan for the complex-to-complex 3D FFT
    fftw_plan plan = fftw_plan_dft_3d(num_angles, num_angles, fft_size, 
                                      data_beamform_fftw, 
                                      data_fft, 
                                      FFTW_FORWARD, FFTW_ESTIMATE);
    cout << "Plan created.\n";    
    
    // Execute the 3D FFT
    fftw_execute(plan);
    cout << "Plan executed.\n";
    
    // Cleanup
    fftw_destroy_plan(plan);
    fftw_cleanup();
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
                // Calculate the index
                int index = theta * NUM_ANGLES * HALF_FFT_SIZE + phi * HALF_FFT_SIZE + k;

                // Access fftw_complex data (real and imaginary)
                double real_part = data_fft[index][0];  // Real part
                double imag_part = data_fft[index][1];  // Imaginary part

                // Calculate the magnitude (absolute value)
                sum += sqrt(real_part * real_part + imag_part * imag_part);
            } // end k
            data_fft_collapse[theta * NUM_ANGLES + phi] = sum / k_amount;
        } // end phi
    } // end theta
} // end FFTCollapse

//=====================================================================================

void beamform::postProcessData(float* data_output, uint8_t post_process_type)
{
    for (int theta = 0; theta < num_angles; theta++)
    {
        for (int phi = 0; phi < num_angles; phi++)
        {
            switch (post_process_type)
            case POST_dBFS:
                // ***Not sure if need to abs() before adding signals***
                // 20 * log10(abs(signal) / num_signals)
                data_output[theta * NUM_ANGLES + phi] = 20 * log10(abs(data_fft_collapse[theta * num_angles + phi]) / total_num_angles);
            break;
        }
    }
} // end postProcessData

//=====================================================================================

void beamform::processData(float* data_input, float* data_output,
                          int lower_frequency, int upper_frequency, 
                          uint8_t post_process_type)
{
    handleBeamforming(data_input);
    cout << "handleBeamforming.\n";
    FFT();
    cout << "FFT\n";
    FFTCollapse(lower_frequency, upper_frequency);
    cout << "FFTCollapse.\n";
    postProcessData(data_output, post_process_type);
    cout << "postProcessData.\n";
} // end handleData

//=====================================================================================

#endif