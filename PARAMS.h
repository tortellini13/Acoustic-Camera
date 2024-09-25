#ifndef PARAMS
#define PARAMS

// Maybe chnage math stuf into const int

#define M_AMOUNT 4 // Amount of mics in the M direction
#define N_AMOUNT 4 // Amount of mics in the N direction
#define NUM_CHANNELS (M_AMOUNT * N_AMOUNT) // Total number of channels

#define ANGLE_STEP 2   // Amount of degrees that the beamforming algorithm sweeps in each step
#define MIN_ANGLE -180 // Minimum angle for beamforminig algorithm to sweep from
#define MAX_ANGLE 180  // Minimum angle for beamforminig algorithm to sweep to

#define FFT_SIZE 1024     // Amount of samples in one frame of the FFT
#define SAMPLE_RATE 44100 // Audio sample rate
#define BUFFER_SIZE (FFT_SIZE * NUM_CHANNELS * sizeof(float)) // Size of buffer for all channels

#define FRAME_RATE 30 // Frame rate of the camera
#define RESOLUTION_WIDTH 640 // Width of the camera
#define RESOLUTION_HEIGHT 480 // Height of the camera

#endif