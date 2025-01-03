#ifndef PARAMS
#define PARAMS

// Mic channels
#define M_AMOUNT 4 // Amount of mics in the M direction
#define N_AMOUNT 4 // Amount of mics in the N direction
#define MIC_SPACING 0.030f // ***Change this number to be correct. Also units?
const int NUM_CHANNELS = (M_AMOUNT * N_AMOUNT); // Total number of channels
const int CHANNEL_ORDER[M_AMOUNT][N_AMOUNT] =
{
    {11, 9,  3, 1},
    {12, 10, 4, 2},
    {15, 13, 7, 5},
    {16, 14, 8, 6}
};
#define MIC_GAIN 1.0f

// Angles
#define ANGLE_STEP 6  // Amount of degrees that the beamforming algorithm sweeps in each step
#define MIN_ANGLE -90 // Minimum angle for beamforming algorithm to sweep from
#define MAX_ANGLE  90 // Minimum angle for beamforming algorithm to sweep to
const int NUM_ANGLES = ((MAX_ANGLE - MIN_ANGLE) / ANGLE_STEP) + 1; // Number of angles

// FFT
#define FFT_SIZE 1024                   // Amount of samples in one frame of the FFT

// Audio
const char* AUDIO_DEVICE_NAME = "hw:2,0"; // arecord -l (type in console to find)
#define SAMPLE_RATE 48000                 // Audio sample rate

// Camera
#define FRAME_RATE 30         // Frame rate of the camera
#define RESOLUTION_WIDTH 640  // Width of the camera
#define RESOLUTION_HEIGHT 480 // Height of the camera

// Heatmap
#define MAP_THRESHOLD 100      // Initial threshold for heat map ***SOME OF THESE ARE REDUNDANT/NOT NEEDED< WILL FIX WHEN THRESHOLD IS OVERHAULED***
#define MAP_THRESHOLD_OFFSET -1000 // Minimum threshold for heat map
#define MAP_THRESHOLD_MAX 1300  // Minimum threshold for heat map

#define DEFAULT_ALPHA 60       // Default alpha value (ALPHA * 100)
#define DEFAULT_THRESHOLD 150  // Default threshold value

// Text
#define FONT_TYPE FONT_HERSHEY_PLAIN // Font for overlayed text
#define FONT_THICKNESS 1             // Font thickness for overlayed text
#define FONT_SCALE 1                 // Font scale for overlayed text
#define MAX_LABEL_POS_X 10           // Horizontal location of maximum value overlayed text
#define MAX_LABEL_POS_Y 20           // Vertical location of maximum value overlayed text
#define LABEL_PRECISION 1            // Number of decimal places to be shown on screen

// Scale
#define SCALE_WIDTH 40   // Width of the color scale
#define SCALE_HEIGHT 400 // Height of the color scale
#define SCALE_POS_X 580  // X Position of color scale
#define SCALE_POS_Y 40   // Y Position of color scale
#define SCALE_BORDER 5   // Thickness of border around scale
#define SCALE_POINTS 5   // Quantity of points on the scale to be marked

// Crosshair
#define CROSS_THICKNESS 2 // Thickness for cross at maximum magnitude
#define CROSS_SIZE 20     // Size of cross at maximum magnitude

// FPS counter
#define FPS_COUNTER_AVERAGE 10 // Number of frames to be averaged for calculating FPS

// Post processing types
#define POST_dBFS 0
#define POST_dBZ  1
#define POST_dBA  2
#define POST_dBC  3

// For debugging. Uncomment to enable
// #define PROFILE_MAIN
// #define PROFILE_BEAMFORM
// #define PROFILE_VIDEO
// #define PRINT_AUDIO
// #define PRINT_BEAMFORM
// #define PRINT_FFT
// #define PRINT_FFT_COLLAPSE
// #define PRINT_POST_PROCESS
// #define PRINT_OUTPUT
#define AVG_SAMPLES 20

#endif