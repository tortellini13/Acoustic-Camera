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
#define SAMPLE_RATE 48000 // Audio sample rate
#define BUFFER_SIZE (FFT_SIZE * NUM_CHANNELS * sizeof(float)) // Size of buffer for all channels
const char* AUDIO_INPUT_NAME = "hw:3,0"; // arecord -l (type in console to find)

#define FRAME_RATE 30         // Frame rate of the camera
#define RESOLUTION_WIDTH 640  // Width of the camera
#define RESOLUTION_HEIGHT 480 // Height of the camera

#define MAP_THRESHOLD 100      // Initial threshold for heat map ***SOME OF THESE ARE REDUNDANT/NOT NEEDED< WILL FIX WHEN THRESHOLD IS OVERHAULED***
#define MAP_THRESHOLD_MIN -100 // Minimum threshold for heat map
#define MAP_THRESHOLD_MAX 300  // Minimum threshold for heat map
#define ALPHA 0.6              // Transparency of heatmap
#define DEFAULT_ALPHA 60       // Default alpha value (ALPHA * 100)
#define DEFAULT_THRESHOLD 150  // Default threshold value

#define FONT_TYPE FONT_HERSHEY_PLAIN   // Font for overlayed text
#define FONT_THICKNESS 1               // Font thickness for overlayed text
#define FONT_SCALE 1                   // Font scale for overlayed text
#define MAX_LABEL_POS_X 10             // Horizontal location of maximum value overlayed text
#define MAX_LABEL_POS_Y 20             // Vertical location of maximum value overlayed text
#define LABEL_PRECISION 1              // Number of decimal places to be shown on screen

#define SCALE_WIDTH 40      // Width of the color scale
#define SCALE_HEIGHT 400    // Height of the color scale
#define SCALE_POS_X 580     // X Position of color scale
#define SCALE_POS_Y 40      // Y Position of color scale
#define SCALE_BORDER 5      // Thicnkess of border around scale
#define SCALE_POINTS 5      // Quantity of points on the scale to be marked

#define CROSS_THICKNESS 2   // Thicnkess for cross at maximum magnitude
#define CROSS_SIZE 20       // Size of cross at maximum magnitude

#define FPS_COUNTER_AVERAGE 5 //Number of frames to be averaged for calculating FPS

const char* AUDIO_SHM = "/AUDIO_SHM";   // Shared memory name for audio data
const char* CONFIG_SHM = "/CONFIG_SHM"; // Shared memory name for user config
#define SEM_1 "/SEM_1"                  // Semaphore
#define SEM_2 "/SEM_2"                  // Semaphore
#define NUM_CONFIGS 5                   // Number of user configs

#endif