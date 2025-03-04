#ifndef PARAMS
#define PARAMS

// Mic channels
#define M_AMOUNT 4 // Amount of mics in the M direction
#define N_AMOUNT 4 // Amount of mics in the N direction
#define MIC_SPACING 0.030f // Meters
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
#define MIN_THETA -30
#define MAX_THETA  30
#define STEP_THETA 5 // 2 is correct
const int NUM_THETA = ((MAX_THETA - MIN_THETA) / STEP_THETA) + 1;

#define MIN_PHI -18
#define MAX_PHI  18
#define STEP_PHI 4 // 1 is correct
const int NUM_PHI = ((MAX_PHI - MIN_PHI) / STEP_PHI) + 1;

#define NUM_TAPS 5

// FFT
#define FFT_SIZE 1024                   // Amount of samples in one frame of the FFT

// Audio
const char* AUDIO_DEVICE_NAME = "hw:3,0"; // arecord -l (type in console to find)
#define SAMPLE_RATE 48000                 // Audio sample rate

// Camera
#define FRAME_RATE 30         // Frame rate of the camera
#define RESOLUTION_WIDTH 640  // Width of the camera
#define RESOLUTION_HEIGHT 480 // Height of the camera

// Heatmap
#define MAP_THRESHOLD_TRACKBAR_VAL 0      // Initial threshold for heat map 
#define MAP_THRESHOLD_OFFSET 100 // Offset for trackbar position relative to threshold value (trackbar pos - offset = threshold)
#define MAP_THRESHOLD_MAX 0  // Maximum threshold for heat map

#define DEFAULT_ALPHA 60       // Default alpha value (ALPHA * 100)

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
#define SCALE_POINTS 5   // Quantity of points on the scale to be marked`

// Crosshair
#define CROSS_THICKNESS 2 // Thickness for cross at maximum magnitude
#define CROSS_SIZE 20     // Size of cross at maximum magnitude

// FPS counter
#define FPS_COUNTER_AVERAGE 10 // Number of frames to be averaged for calculating FPS

// Post processing types
enum post_processing: uint8_t
{
    POST_dBFS,
    POST_dBZ,
    POST_dBA,
    POST_dBC
};

// For debugging. Uncomment to enable
// #define PROFILE_MAIN
#define PROFILE_BEAMFORM
// #define PROFILE_VIDEO
// #define PRINT_AUDIO
// #define PRINT_BEAMFORM
// #define PRINT_FFT
// #define PRINT_FFT_COLLAPSE
// #define PRINT_POST_PROCESS
// #define PRINT_OUTPUT
#define ENABLE_AUDIO
#define ENABLE_VIDEO
#define ENABLE_IMGUI
// #define ENABLE_RANDOM_DATA
// #define ENABLE_STATIC_DATA
#define AVG_SAMPLES 10
#define PI // Set for usage on Pi
//#define UBUNTU // Set for usage on ubuntu

#endif