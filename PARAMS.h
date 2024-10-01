#ifndef PARAMS
#define PARAMS

#define M_AMOUNT 4 // Amount of mics in the M direction
#define N_AMOUNT 4 // Amount of mics in the N direction
#define NUM_CHANNELS (M_AMOUNT * N_AMOUNT) // Total number of channels

#define ANGLE_STEP 2  // Amount of degrees that the beamforming algorithm sweeps in each step
#define MIN_ANGLE -90 // Minimum angle for beamforming algorithm to sweep from
#define MAX_ANGLE 90  // Minimum angle for beamforming algorithm to sweep to
#define NUM_ANGLES ((MAX_ANGLE - MIN_ANGLE) / ANGLE_STEP)

#define FFT_SIZE 1024     // Amount of samples in one frame of the FFT
#define SAMPLE_RATE 44100 // Audio sample rate
#define BUFFER_SIZE (FFT_SIZE * NUM_CHANNELS * sizeof(float)) // Size of buffer for all channels

#define FRAME_RATE 30         // Frame rate of the camera
#define RESOLUTION_WIDTH 640  // Width of the camera
#define RESOLUTION_HEIGHT 480 // Height of the camera

const char* AUDIO_SHM = "/AUDIO_SHM"; // Shared memory name for audio data
#define AUDIO_SEM_1 "/AUDIO_SEM_1"    // Semaphore for audio data
#define AUDIO_SEM_2 "/AUDIO_SEM_2"    // Semaphore for audio data

const char* CONFIG_SHM = "/CONFIG_SHM"; // Shared memory name for user config
#define CONFIG_SEM_1 "/CONFIG_SEM_1"    // Semaphore for user config
#define CONFIG_SEM_2 "/CONFIG_SEM_2"    // Semaphore for user config


#endif