// Default libraries
#include <iostream>
#include <cmath>
#include <complex>
#include <math.h>
#include <iomanip>
#include <sys/mman.h>

// User headers
#include "PARAMS.h"
#include "sharedMemory.h"
#include "Audio.h"

using namespace std;

//==============================================================================================

// Initialize upper and lower frequencies
int lowerFrequency;
int upperFrequency; 

// Audio Data
float AUDIO_DATA[BUFFER_SIZE];
cfloat AUDIO_DATA_BEAMFORM[NUM_ANGLES * NUM_ANGLES * FFT_SIZE];
double AUDIO_DATA_FFT[DATA_SIZE_BUFFER_HALF];
float AUDIO_DATA_FFTCOLLAPSE[TOTAL_ANGLES];
float AUDIO_DATA_dBfs[TOTAL_ANGLES];

// Constants
cfloat CONSTANT_1[HALF_FFT_SIZE];
cfloat CONSTANT_2[HALF_FFT_SIZE * HALF_FFT_SIZE];
cfloat ANGLES[NUM_ANGLES * NUM_ANGLES];

//==============================================================================================

int main()
{
	//==============================================================================================

	// Initialize shared memory class
	sharedMemory shm(AUDIO_SHM, CONFIG_SHM, SEM_1, SEM_2, NUM_ANGLES, NUM_ANGLES, NUM_CONFIGS);

	if(!shm.shmStart1())
	{
		cerr << "1. shmStart1 failed.\n";
	}

	//==============================================================================================

	// Initialize variables for audio handling
    snd_pcm_t *pcm_handle;
    snd_pcm_uframes_t frames;
    int pcm;

	//==============================================================================================

    // Setup audio
    pcm = setupAudio(&pcm_handle, &frames, AUDIO_INPUT_NAME);
    if (pcm < 0) 
    {
        return 1;  // Exit if setup fails
    }
	cout << "1. setupAudio done.\n";

	// Initializes constants for later use
	setupConstants(CONSTANT_1, CONSTANT_2, ANGLES);
	cout << "1. setupCalcs done.\n";
	
	// Initializes variables for user configs
	int bandTypeSelection = 0;
	int thirdOctaveBandSelection = 0;
	int fullOctaveBandSelection = 0;
	bool isRecording = 0;

	int userConfigs[NUM_CONFIGS]; // Main user config array

	//==============================================================================================

	/* Switch case legend
	dont change legend if removing frequencies. just limit user input
	potentially remove lower frequencies due to too big bins***
	potentially remove higher frequencies due to mic response***
	
	Full Octave:
	0 - 63 Hz
	1 - 125 Hz
	2 - 250 Hz
	3 - 500 Hz
	4 - 1000 Hz
	5 - 2000 Hz
	6 - 4000 Hz
	7 - 8000 Hz
	8 - 16000 Hz
	
	Third Octave:
	0  - 50 Hz
	1  - 63 Hz
	2  - 80 Hz
	3  - 100 Hz
	4  - 125 Hz
	5  - 160 Hz
	6  - 200 Hz
	7  - 250 Hz
	8  - 315 Hz
	9  - 400 Hz
	10 - 500 Hz
	11 - 630 Hz
	12 - 800 Hz
	13 - 1000 Hz
	14 - 1250 Hz
	15 - 1600 Hz
	16 - 2000 Hz
	17 - 2500 Hz
	18 - 3150 Hz
	19 - 4000 Hz
	20 - 5000 Hz
	21 - 6300 Hz
	22 - 8000 Hz
	23 - 10000 Hz
	24 - 12500 Hz
	25 - 16000 Hz
	26 - 20000 Hz	
	*/
	
	//==============================================================================================
	
	cout << "1. Main loop starting.\n";
	// Main loop
	while (1)
	{
		/* Configs Legend
		0. 0 = Broadband
		   1 = Full octave
		   2 = Third Octave 
		1. 0 - 8 Full octave band selection
		2. 0 - 26 Third octave band selection
		3. 0 = Is not recording
		   1 = Is recording
		4. 
		5. 
		*/

		bandTypeSelection = userConfigs[0];
		fullOctaveBandSelection = userConfigs[1];
		thirdOctaveBandSelection = userConfigs[2];
		isRecording = userConfigs[3];

		//==============================================================================================

		//cout << "1. captureAudio start.\n";
		// Read data from microphones
        pcm = captureAudio(AUDIO_DATA, pcm_handle);
        if (pcm < 0) 
        {
            cerr << "Error capturing audio" << endl;
            // Decide whether to break or continue
            break;
        }
		//cout << "1. captureAudio done.\n";

		/*
		// Print out first frame of audio from buffer on each channel
		for (int m = 0; m < M_AMOUNT; m++)
		{
			for (int n = 0; n < N_AMOUNT; n++)
			{
				cout << AUDIO_DATA[m * N_AMOUNT * FFT_SIZE + n * FFT_SIZE] << " ";
			}
			cout << endl;
		}
		cout << endl;
		*/

		//==============================================================================================

		// Change input to audioDataIn*******
		// User Configs
		//cout << "1. Selecting band type - ";
		switch (bandTypeSelection)
		{
			case 0:
				//arrayFactor(DATA, audioDataOut); // *** for testing only with 2-D array

				lowerFrequency = 1; upperFrequency = 511;
				//cout << "BroadBand" << endl;
			break;
			
			//==============================================================================================
			
			// Full Octave Band
			case 1:
			
				switch (fullOctaveBandSelection)
				{
					// 63 Hz
					case 0:	lowerFrequency = 1;   upperFrequency = 3;   break;
						
					// 125 Hz
					case 1:	lowerFrequency = 2;   upperFrequency = 5;   break;
						
					// 250 Hz
					case 2:	lowerFrequency = 4;   upperFrequency = 9;   break;
					
					// 500 Hz
					case 3:	lowerFrequency = 8;   upperFrequency = 17;  break;

					// 1000 Hz
					case 4:	lowerFrequency = 16;  upperFrequency = 33;  break;
					
					// 2000 Hz
					case 5:	lowerFrequency = 32;  upperFrequency = 66;  break;

					// 4000 Hz
					case 6:	lowerFrequency = 65;  upperFrequency = 132; break;

					// 8000 Hz
					case 7:	lowerFrequency = 131; upperFrequency = 264; break;

					// 16000 Hz
					case 8:	lowerFrequency = 263; upperFrequency = 511; break;	
				} // end Full Octave Band
				
			break;
			
			//==============================================================================================
			
			// Third Octave Band
			case 2:
			
				switch (thirdOctaveBandSelection)
				{
					// 50 Hz
					case 0: lowerFrequency = 1;   upperFrequency = 1;   break; // idk

					// 63 Hz
					case 1: lowerFrequency = 2;   upperFrequency = 2;   break; // idk

					// 80 Hz
					case 2: lowerFrequency = 1;   upperFrequency = 3;   break; // idk
				
					// 100 Hz
					case 3: lowerFrequency = 2;   upperFrequency = 3;   break;

					// 125 Hz
					case 4: lowerFrequency = 2;   upperFrequency = 4;   break;

					// 160 Hz
					case 5: lowerFrequency = 3;   upperFrequency = 5;   break;

					// 200 Hz
					case 6: lowerFrequency = 4;   upperFrequency = 6;   break;

					// 250 Hz
					case 7: lowerFrequency = 5;   upperFrequency = 7;   break;

					// 315 Hz
					case 8: lowerFrequency = 6;   upperFrequency = 9;   break;

					// 400 Hz
					case 9: lowerFrequency = 8;   upperFrequency = 11;  break;

					// 500 Hz
					case 10: lowerFrequency = 10;  upperFrequency = 14;  break;

					// 630 Hz
					case 11: lowerFrequency = 13;  upperFrequency = 17;  break;

					// 800 Hz
					case 12: lowerFrequency = 16;  upperFrequency = 21;  break;

					// 1000 Hz
					case 13: lowerFrequency = 20;  upperFrequency = 27;  break;

					// 1250 Hz
					case 14: lowerFrequency = 26;  upperFrequency = 33;  break;

					// 1600 Hz
					case 16: lowerFrequency = 32;  upperFrequency = 42;  break;

					// 2000 Hz
					case 17: lowerFrequency = 41;  upperFrequency = 52;  break;

					// 2500 Hz
					case 18: lowerFrequency = 51;  upperFrequency = 66;  break;
	
					// 3150 Hz
					case 19: lowerFrequency = 65;  upperFrequency = 83;  break;

					// 4000 Hz
					case 20: lowerFrequency = 82;  upperFrequency = 104; break;

					// 5000 Hz
					case 21: lowerFrequency = 103; upperFrequency = 131; break;

					// 6300 Hz
					case 22: lowerFrequency = 130; upperFrequency = 165; break;

					// 8000 Hz
					case 23: lowerFrequency = 164; upperFrequency = 207; break;

					// 10000 Hz
					case 24: lowerFrequency = 206; upperFrequency = 261; break;

					// 12500 Hz
					case 25: lowerFrequency = 260; upperFrequency = 329; break;
					
					// 16000 Hz
					case 26: lowerFrequency = 328; upperFrequency = 413; break;
					
					// 20000 Hz
					case 27: lowerFrequency = 412; upperFrequency = 511; break;			
				} // end Third Octave Band		
			
			//==============================================================================================
			
			cout << "1. FFT bounds: " << lowerFrequency << " - " << upperFrequency << endl;
			// Filters data to remove unneeded frequencies and sums all frequency bins
			// Applies per-band calibration?
    		handleData(AUDIO_DATA, AUDIO_DATA_BEAMFORM, AUDIO_DATA_FFT,
          			   AUDIO_DATA_FFTCOLLAPSE, AUDIO_DATA_dBfs, 257, 511, ANGLES);
			cout << "1. arrayFactor done.\n";
			
			break; // end frequency selection
		} // end main loop
		
		//==============================================================================================
		
		// Write audio data to Video and read user configs
		if(!shm.handleshm1(AUDIO_DATA_dBfs, userConfigs))
		{
			cerr << "1. writeRead1 failed.\n";
		}
		//cout << "1. handleshm1 done.\n";
		
	} // end loop
	
	/*
	// Print gain array in .csv format (debugging)
	for (int index1 = 0; index1 < ANGLE_AMOUNT; ++index1)
	{
		for (int index2 = 0; index2 < ANGLE_AMOUNT; ++index2)
		{
			// Output gain value followed by comma
			cout << gain[index1][index2];
			if (index2 < ANGLE_AMOUNT - 1)
			{
				cout << ",";
			}
		}
		cout << endl;
	} // end print gain
	*/

    // Clean up ALSA
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);

	// Clean up shm
	shm.closeAll();
	shm.~sharedMemory();

} // end main