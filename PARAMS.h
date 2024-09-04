#pragma once

#define M_PI 3.14159265359 // Pi approximation

#define M_AMOUNT 4         // Amount of mics in the M direction
#define N_AMOUNT 4         // Amount of mics in the N direction

#define ANGLE_STEP 2       // Amount of degrees that the beamforming algorithm sweeps in each step
#define MIN_ANGLE -180     // Minimum angle for beamforminig algorithm to sweep from
#define MAX_ANGLE 180      // Minimum angle for beamforminig algorithm to sweep to

#define FFT_SIZE 1024      // Amount of samples in one frame of the FFT

#define FRAME_RATE 30      // Frame rate of the camera