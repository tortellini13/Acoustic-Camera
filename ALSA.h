#ifndef ALSA_H
#define ALSA_H

// Internal Libraries
#include <iostream>
#include <iomanip>

// External Libraries
#include <alsa/asoundlib.h>

// Header Files
#include "PARAMS.h"
#include "Structs.h"

using namespace std;

//=====================================================================================

class ALSA
{
public:
    // Initialize ALSA class
    ALSA(const char* device_name, int total_channels, int sample_rate, int num_frames);

    // Clear memory for all arrays
    ~ALSA();

    // Send settings to audio device
    bool setup();

    // Call this in a loop to record audio
    bool recordAudio(float3D& data_output);

private:
    // Configs
    snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;        // Set the pcm stream to capture
    snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED; // Stores data where ch1[0], ch2[0], ...ch16[0], ch1[1],...
    snd_pcm_format_t format = SND_PCM_FORMAT_S32_LE;         // Format for input data (32-bit little endian)
    int mode = 0;        // Mode for pcm (0 is default)
    int periods = 2;     // Number of periods. For scheduling inturrupts
    int num_bytes = 4;   // Number of bytes read per sample
    int COLS = M_AMOUNT; // Number of columns
    int ROWS = N_AMOUNT; // Number of rows

    // Variables
    snd_pcm_t *pcm_handle;          // pcm handle
    snd_pcm_hw_params_t *hw_params; // Contains information about pcm configs
    const char *pcm_name;           // Name of pcm device (ie. hw:0,0)
    unsigned int exact_rate;        // Sample rate returned by snd_pcm_hw_params_rate_near
    int dir;                        // Checks if rate and exact_rate are the same
    int num_channels;               // Total number of channels
    int rate;                       // Defined sample rate
    snd_pcm_uframes_t frames;       // Number of frames recorded per period
    int frame_size;                 // Size of one frame
    snd_pcm_uframes_t buffer_size;  // Size of buffer (BUFFER_SIZE * NUM_BYTES * NUM_CHANNELS)
    int32_t* data_buffer;           // Buffer for interlaced data to be written to
    int pcm_return;                 // Return value for pcm reading (for error handling)

}; // end class def

//=====================================================================================

ALSA::ALSA(const char* device_name, int total_channels, int sample_rate, int num_frames):
    pcm_name(device_name),
    num_channels(total_channels),
    rate(sample_rate),
    frames(num_frames),
    frame_size(total_channels * num_frames),
    buffer_size(frames * num_channels)
    {
        // Allocate memory for dataBuffer
        data_buffer = new int32_t[buffer_size];  
    }

//=====================================================================================

ALSA::~ALSA()
{
    delete[] data_buffer;
} // end ~ALSA

//=====================================================================================

bool ALSA::setup()
{
    // Allocate hw_params on the stack
    snd_pcm_hw_params_alloca(&hw_params);

    // Open pcm
    if (snd_pcm_open(&pcm_handle, pcm_name, stream, mode) < 0)
    {
        cerr << "Error opening PCM device." << endl;
        return false;
    }
    
    // Initialize hw_params with full configuration space
    if (snd_pcm_hw_params_any(pcm_handle, hw_params) < 0)
    {
        cerr << "Cannot configure PCM device.\n";
        return false;
    }

    // Set access type
    if (snd_pcm_hw_params_set_access(pcm_handle, hw_params, access) < 0)
    {
        cerr << "Error setting access.\n";
        return false;
    }

    // Set sample format
    if (snd_pcm_hw_params_set_format(pcm_handle, hw_params, format) < 0)
    {
        cerr << "Error setting format.\n";
        return false;
    }

    // Set sample rate
    exact_rate = rate;
    if (snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &exact_rate, &dir) < 0)
    {
        cerr << "Error setting rate.\n";
        return false;
    }

    // If specified rate is not availiable, set to nearest rate
    if (rate != exact_rate)
    {
        cerr << "The rate " << rate << " is not supported. Using " << exact_rate << " instead.\n";
    }

    // Set number of channels
    if (snd_pcm_hw_params_set_channels(pcm_handle, hw_params, num_channels) < 0)
    {
        cerr << "Error setting channels.\n";
        return 0;
    }

    // Set number of periods
    if (snd_pcm_hw_params_set_periods(pcm_handle, hw_params, periods, 0) < 0)
    {
        cerr << "Error setting periods.\n";
        return false;
    }

    // Set buffer size
    if (snd_pcm_hw_params_set_buffer_size(pcm_handle, hw_params, buffer_size) < 0)
    {
        cerr << "Error setting buffer size.\n";
        return false;
    }

    if (snd_pcm_hw_params(pcm_handle, hw_params) < 0)
    {
        cerr << "Error setting HW parameters." << endl;
        return false;
    }

    // snd_pcm_hw_params_get_format(hw_params, &format);
    // cout << "PCM Format: " << snd_pcm_format_name(format) << endl;

    cout << "Finished setting up Audio.\n"; // (debugging)

    return true;
} // end setupAudio

//=====================================================================================

// Data outputs to M * N * FFT_SIZE array
bool ALSA::recordAudio(float3D& data_output)
{
    // Read data from microphones into interlaced buffer
    pcm_return = snd_pcm_readi(pcm_handle, data_buffer, frames);
   
    // Check for error
    if (pcm_return == -EPIPE) 
    { 
        // Buffer overrun/underrun error
        cerr << "Buffer overrun/underrun occurred. Recovering..." << endl;
        snd_pcm_prepare(pcm_handle); // Prepare the device again
        return false;
    } 
    else if (pcm_return < 0) 
    {
        cerr << "Error reading PCM data: " << snd_strerror(pcm_return) << endl;
        return false;
    } 
    else if (pcm_return != frames) 
    {
        cerr << "PCM read returned fewer frames than expected." << endl;
        return false;
    }

    // Remap the data to not-interlaced floats
    for (int b = 0; b < frames; b++)
    {
        for (int n = 0; n < ROWS; n++)
        {
            for (int m = 0; m < COLS; m++)
            {
                data_output.at(m, n, b) = static_cast<float>(data_buffer[b * num_channels + (n * COLS + m)]) / static_cast<float>(1 << 31);
            }
        }
    }
    return true;
} // end recordAudio

//=====================================================================================

#endif