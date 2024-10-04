// Default libraries
#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <math.h>
#include <iomanip>
#include <sys/mman.h>

// Additional libraries
#include <alsa/asoundlib.h> // Need to install

// User headers
#include "PARAMS.h"
#include "sharedMemory.h"

using namespace std;

/* Todo:
Audio
	-read data from audio interface (ALSA)
	-buffer data in 1024 sample chunks into DATA array
	-dBZ and dBA
	-calculate spl
	
Gui
	-open cv
	-square camera display
	-draw gain to heatmap
	-overlay gain heatmap and camera
	-user controls
	-user selections on gui for 1/3 octave band, 1 octave band, or BB
	-add ability to click a location and read the spl at that point (take average around point)
	-support to connect multiple screens and duplicate
	
Recording
	-raw data & camera recording and playback
	-record and playback just screen
	-create filepath system like B&K
	
Misc
	-make calibration program
*/

//==============================================================================================

// Custom type declaration for ease of use
typedef complex<float> cfloat;

// Defines amount of angles to sweep through (coordinates)
const int ANGLE_AMOUNT = ((MAX_ANGLE - MIN_ANGLE) / ANGLE_STEP + 1);

// Half of FFT_SIZE
const int HALF_FFT_SIZE = FFT_SIZE / 2;

// Initializes gain array
vector<vector<float>> gain(ANGLE_AMOUNT, vector<float>(ANGLE_AMOUNT));

// Initialize upper and lower frequencies
// Changed by user input
int lowerFrequency;
int upperFrequency; 

//==============================================================================================

// Converts degrees to radians
float degtorad(float angleDEG)
{
	float result = angleDEG * (M_PI / 180);
	return result;
} // end degtorad

//==============================================================================================

// Read audio data from interface

//==============================================================================================

// Calculates Array Factor and outputs gain
void arrayFactor(const vector<vector<float>>& Data)
{
	// Initialize Array Factor array
	vector<vector<cfloat>> AF(ANGLE_AMOUNT, vector<cfloat>(ANGLE_AMOUNT));

	// Calculate phase shift for every angle
	int indexTHETA = 0; // Reset theta index
	for (int THETA = MIN_ANGLE; indexTHETA < ANGLE_AMOUNT; THETA += ANGLE_STEP, ++indexTHETA)
	{
		int indexPHI = 0; // Reset phi index
		for (int PHI = MIN_ANGLE; indexPHI < ANGLE_AMOUNT; PHI += ANGLE_STEP, ++indexPHI)
		{
			cfloat sum = 0;  // To accumulate results
			for (int m = 0; m < M_AMOUNT; m++)
			{
				for (int n = 0; n < N_AMOUNT; n++)
				{
					// Calculate the phase shift for each element
					float angle = degtorad(THETA * m + PHI * n);
					sum += Data[m][n] * cfloat(cos(angle), sin(angle));
				}
			}
			AF[indexTHETA][indexPHI] = sum; // Writes sum to Array Factor array
		}
	}


	// Calculate gain 10log10(signal)
	for (int indexTHETA = 0; indexTHETA < ANGLE_AMOUNT; ++indexTHETA)
	{
		for (int indexPHI = 0; indexPHI < ANGLE_AMOUNT; ++indexPHI)
		{
			gain[indexTHETA][indexPHI] = 10 * log10(norm(AF[indexTHETA][indexPHI]));

		}
	}


	/*
	// Print the result array (for debugging)
	for (int index1 = 0; index1 < ANGLE_AMOUNT; index1++)
	{
		for (int index2 = 0; index2 < ANGLE_AMOUNT; index2++)
		{
			cout << AF[index1][index2] << ", ";
		}
		cout << "\n";
	}

	cout << "\n\n\n\n\n\n";
	*/
} // end arrayFactor

//==============================================================================================

// Calculate all constants and write to arrays to access later
float vk; // (2 * M_PI * k) / FFT_SIZE
vector<cfloat> ev(HALF_FFT_SIZE); // exp((-2 * M_PI * k) / FFT_SIZE)
vector<vector<cfloat>> ev2m(HALF_FFT_SIZE, vector<cfloat>(HALF_FFT_SIZE)); // exp((-2 * M_PI * k * 2 * m) / FFT_SIZE)
void constantCalcs()
{
	for (int k = 0; k < HALF_FFT_SIZE; k++)
	{
		vk = (2 * M_PI * k) / FFT_SIZE;     // some vector as a function of k
		ev[k] = cfloat(cos(vk), -sin(vk)); // some complex vector as a function of k
		for (int m = 0; m < HALF_FFT_SIZE; m++)
		{
			ev2m[k][m] = cfloat(cos(vk * 2 * m), -sin(vk * 2 * m)); // some complex vector as a function of k with a factor of 2m in exponent
		}
	}
} // end constantCalcs

//==============================================================================================

// Calculates Ek (first component of FFT for even samples in buffer as a function of k)
vector<vector<vector<cfloat>>> Ek(const vector<vector<vector<float>>>& rawData, int lowerBound, int upperBound)
{
	vector<vector<vector<cfloat>>> result(HALF_FFT_SIZE, vector<vector<cfloat>>(M_AMOUNT, vector<cfloat>(N_AMOUNT, 0)));

	for (int x = 0; x < M_AMOUNT; x++) // Loop through M-axis
	{
		for (int y = 0; y < N_AMOUNT; y++) // Loop through N-axis
		{
			for (int k = 0; k < HALF_FFT_SIZE; k++) // k (frequency bin)
			{
				// Set result = 0 for all k lower than lowerBound
				if (k < lowerBound)
				{
					result[x][y][k] = 0;
				}
					
				// Set result = 0 for all k greater than upperBound
				else if (k > upperBound)
				{
					result[x][y][k] = 0;
				}
					
				// Calculates Ek array
				else
				{
					cfloat sum = 0;
					for (int m = 0; m < HALF_FFT_SIZE; m += 2) // m (buffered sample index)
					{
						sum += rawData[x][y][m] * ev2m[k][m]; // Calculates Ek array
					}
					result[x][y][k] = sum;	
				} // end Ek calc
			} // end k
		} // end y
	} // end x
	return result;
} // end Ek

//==============================================================================================

// Calculates Ok (second component of FFT for odd samples in buffer as a function of k)
vector<vector<vector<cfloat>>> Ok(const vector<vector<vector<float>>>& rawData, int lowerBound, int upperBound)
{
	vector<vector<vector<cfloat>>> result(HALF_FFT_SIZE, vector<vector<cfloat>>(M_AMOUNT, vector<cfloat>(N_AMOUNT, 0)));

	for (int x = 0; x < M_AMOUNT; x++) // Loop through M-axis
	{
		for (int y = 0; y < N_AMOUNT; y++) // Loop through N-axis
		{
			for (int k = 0; k < HALF_FFT_SIZE; k++) // k (frequency bin)
			{
				// Set result = 0 for all k lower than lowerBound
				if (k < lowerBound - HALF_FFT_SIZE)
				{
					result[x][y][k] = 0;
				}
					
				// Set result = 0 for all k greater than upperBound
				else if (k > upperBound - HALF_FFT_SIZE)
				{
					result[x][y][k] = 0;
				}
					
				// Calculates Ok array
				else
				{
					cfloat sum = 0;
					for (int m = 1; m < HALF_FFT_SIZE; m += 2) // m (buffered sample index)
					{
						sum += rawData[x][y][m] * ev2m[k][m]; // Calculates Ok array
					}
					result[x][y][k] = sum;	
				} // end Ok calc
			} // end k
		} // end y
	} // end x
	return result;
} // end Ok

//==============================================================================================

// Calculates X array (total FFT as a function of k) and sums all of the values in the desired range
// Outputs M x N array of filtered data
vector<vector<float>> FFTSum(vector<vector<vector<float>>>& rawData, int lowerBound, int upperBound)
{
	// Calls Ek and Ok
	vector<vector<vector<cfloat>>> Xcomp1 = Ek(rawData, lowerBound, upperBound); 
	vector<vector<vector<cfloat>>> Xcomp2 = Ok(rawData, lowerBound, upperBound);
		
	// Xk
	vector<vector<vector<cfloat>>> Xresult(HALF_FFT_SIZE, vector<vector<cfloat>>(M_AMOUNT, vector<cfloat>(N_AMOUNT, 0)));
	for (int x = 0; x < M_AMOUNT; x++)
	{
		for (int y = 0; y < N_AMOUNT; y++)
		{
			for (int k = 0; k < HALF_FFT_SIZE; k++)
			{
				Xresult[x][y][k] = Xcomp1[x][y][k] + ev[k] * Xcomp2[x][y][k];
			}
		}
	}

	/* Don't need. Produces reflection (-freq)
	// Xk+N/2
	for (int x = 0; x < M_AMOUNT; x++)
	{
		for (int y = 0; y < N_AMOUNT; y++)
		{
			for (int k = 0; k < HALF_FFT_SIZE; k++)
			{
				Xresult[x][y][k + HALF_FFT_SIZE] = Xcomp1[x][y][k + HALF_FFT_SIZE] + ev[k + HALF_FFT_SIZE] * Xcomp1[x][y][k + HALF_FFT_SIZE];
			}
		}
	}
	*/
	
	// Sum all X array values to convert frequency bins into a full band (user defined)
	vector<vector<float>> result(M_AMOUNT, vector<float>(N_AMOUNT));
	for (int x = 0; x < M_AMOUNT; x++)
	{
		for (int y = 0; y < N_AMOUNT; y++)
		{
			float sum = 0; // To accumulate results
			for (int k = 0; k < HALF_FFT_SIZE; k++)
			{
				sum += norm(Xresult[x][y][k]); // magnitude of complex number sqrt(a^2 + b^2)
			}
			result[x][y] = sum;
		}
	}
	return result;
}// end FFTSum

//==============================================================================================

// Prints 3D array of floats to the console
void print3Dfloat(vector<vector<vector<float>>>& inputArray, string& title, int dim1, int dim2, int dim3)
{
	cout << title << endl;
	for (int m = 0; m < dim1; m++)
	{
		for (int n = 0; n < dim2; n++)
		{
			for (int k = 0; k < dim3; k++)
			{
				cout << left << setw(15) << inputArray[m][n][k] << ", ";
			} // end k
			cout << endl;
		} // end n
	} // end m
	cout << "\n\n";
} // end print3Dfloat

//==============================================================================================

int main()
{
	// Needs to be 3D for input***
	// 3rd dimension is buffered data***
	// Will be made 3D after data acquisition is made***
	vector<vector<float>> DATA =
	{
		{1, 1, 1, 1},
		{1, 1, 1, 1},
		{1, 1, 1, 1},
		{1, 1, 1, 1}
	};

	//==============================================================================================

	// Initialize shared memory class for Audio
	sharedMemory audioData(AUDIO_SHM, AUDIO_SEM_1, AUDIO_SEM_2, NUM_ANGLES, NUM_ANGLES);

	if(!audioData.createAll())
	{
		cerr << "1. createAll failed.\n";
	}

	// Initialize shared memory class for userConfigs
	sharedMemory userConfigs(CONFIG_SHM, CONFIG_SEM_1, CONFIG_SEM_2, NUM_CONFIGS, 1);

	if(!userConfigs.openAll())
	{
		cerr << "1. openAll failed.\n";
	}

	//==============================================================================================

	// Initializes constants for later use
	constantCalcs();
	
	// Initializes variables
	int bandTypeSelection = 0;
	int thirdOctaveBandSelection = 0;
	int fullOctaveBandSelection = 0;
	bool isRecording = 0;

	vector<int> USER_CONFIGS(NUM_CONFIGS, 0);
	
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
	
	// Main loop
	while (1)
	{
		// Read USER_CONFIGS and set relevant configs
		if(!userConfigs.read(USER_CONFIGS))
		{
			cerr << "1. read failed.\n";
		}

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

		bandTypeSelection = USER_CONFIGS[0];
		fullOctaveBandSelection = USER_CONFIGS[1];
		thirdOctaveBandSelection = USER_CONFIGS[2];
		isRecording = USER_CONFIGS[3];

		//==============================================================================================

		// User Configs
		switch (bandTypeSelection)
		{
			// Broadband
			case 0:
				arrayFactor(DATA); // *** for testing only with 2-D array

				//lowerFrequency = 1; upperFrequency = 511;
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
			
			// Filters data to remove unneeded frequencies and sums all frequency bins
			// Applies per-band calibration
			//vector<vector<float>> filteredData = FFTSum(DATA); ***disabled until data acquisition is made***

			// Does beamforming algorithm and converts to gain
			//arrayFactor(filteredData);	***disabled until data acquisition is made***	
			
			break; // end frequency selection
		} // end main loop
		
		//==============================================================================================
		
		// Output data to Video script
		if(!audioData.write2D(gain))
		{
			cerr << "1. write2D failed.\n";
		}
	}
	
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
} // end main