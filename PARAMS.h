#ifndef PARAMS
#define PARAMS

#include "Structs.h"

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
const char* AUDIO_DEVICE_NAME = "hw:2,0"; // arecord -l (type in console to find)
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

enum int_configs: uint8_t
{
    imgui_clamp_min,
    imgui_clamp_max,
    imgui_threshold,
    quality,
    octave_band_value,
    third_band_value,
    NUM_INT_CONFIGS
};

enum float_configs: uint8_t
{
    imgui_alpha,
    NUM_FLOAT_CONFIGS
};

enum bool_configs: uint8_t
{
    mark_max_mag_state,
    color_scale_state,
    heat_map_state,
    data_clamp_state,
    threshold_state,

    auto_save_state,
    record_state,
    capture_image_state,

    random_state,
    static_state,

    options_menu,
    hidden_menu,

    full_range,
    octave_bands,
    NUM_BOOL_CONFIGS
};

enum string_configs: uint8_t
{
    heatmap,
    save_path,
    current_band,
    NUM_STRING_CONFIGS
};

enum FULL_OCTAVE_BANDS: uint8_t
{
    FULL_63,
    FULL_125,
    FULL_250,
    FULL_500,
    FULL_1000,
    FULL_2000,
    FULL_4000,
    FULL_8000,
    FULL_16000,
    NUM_FULL_OCTAVE_BANDS
};

enum THIRD_OCTAVE_BANDS: uint8_t
{
    //THIRD_20,
    //THIRD_25,
    //THIRD_31_5,
    //THIRD_40,
    //THIRD_50,
    THIRD_63,
    THIRD_80,
    THIRD_100,
    THIRD_125,
    THIRD_160,
    THIRD_200,
    THIRD_250,
    THIRD_315,
    THIRD_400,
    THIRD_500,
    THIRD_630,
    THIRD_800,
    THIRD_1000,
    THIRD_1250,
    THIRD_1600,
    THIRD_2000,
    THIRD_2500,
    THIRD_3150,
    THIRD_4000,
    THIRD_5000,
    THIRD_6300,
    THIRD_8000,
    THIRD_10000,
    THIRD_12500,
    THIRD_16000,
    //THIRD_20000,
    NUM_THIRD_OCTAVE_BANDS
};

extern CONFIG configs;

// For debugging. Uncomment to enable
// #define PROFILE_MAIN
// #define PROFILE_BEAMFORM
// #define PROFILE_VIDEO
// #define PRINT_AUDIO
// #define PRINT_BEAMFORM
#define PRINT_FFT
// #define PRINT_FFT_COLLAPSE
// #define PRINT_POST_PROCESS
// #define PRINT_OUTPUT
//#define ENABLE_AUDIO
#define ENABLE_VIDEO
#define ENABLE_IMGUI
#define AVG_SAMPLES 10
#define PI // Set for usage on Pi
//#define UBUNTU // Set for usage on ubuntu

#endif