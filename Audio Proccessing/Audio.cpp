#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <math.h>
#include <iomanip>

#include <alsa/asoundlib.h> // Need to install
#include <sys/mman.h> // Should only have this on Linux

#include "PARAMS.h"

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
typedef complex<double> cdouble;

// Defines amount of angles to sweep through (coordinates)
const int ANGLE_AMOUNT = ((MAX_ANGLE - MIN_ANGLE) / ANGLE_STEP + 1);

// Half of FFT_SIZE
const int HALF_FFT_SIZE = FFT_SIZE / 2;

// Initializes gain array
vector<vector<double>> gain(ANGLE_AMOUNT, vector<double>(ANGLE_AMOUNT));

// Initialize upper and lower frequencies
// Changed by user input
int lowerFrequency;
int upperFrequency; 

//==============================================================================================

// Converts degrees to radians
double degtorad(double angleDEG)
{
	double result = angleDEG * (M_PI / 180);
	return result;
} // end degtorad

//==============================================================================================

// Needs to be checked on Linux. Currently untested***
// Read data from interface and put into FFT_SIZE buffer
bool captureAudioData(const char* device, int16_t*** audio_data)
{
	snd_pcm_t* handle;
	snd_pcm_hw_params_t* params;
	int err;

	// Open PCM device for recording (capture)
	if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) 
	{
		cerr << "Cannot open audio device " << device << ": " << snd_strerror(err) << std::endl;
		return false;
	}

	// Allocate a hardware parameters object
	snd_pcm_hw_params_alloca(&params);

	// Fill it in with default values
	snd_pcm_hw_params_any(handle, params);

	// Set the desired hardware parameters
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
	
	snd_pcm_hw_params_set_rate_near(handle, params, &SAMPLE_RATE, 0);
	snd_pcm_hw_params_set_channels(handle, params, M_AMOUNT * N_AMOUNT); // Total number of channels

	// Write the parameters to the driver
	if ((err = snd_pcm_hw_params(handle, params)) < 0) 
	{
		cerr << "Unable to set HW parameters: " << snd_strerror(err) << std::endl;
		return false;
	}

	// Allocate buffer to hold captured data
	int buffer_frames = FFT_SIZE;
	int buffer_size = buffer_frames * M_AMOUNT * N_AMOUNT * sizeof(int16_t); // 2 bytes/sample, M*N channels
	int16_t* buffer = new int16_t[M_AMOUNT * N_AMOUNT * buffer_frames];

	// Capture data
	err = snd_pcm_readi(handle, buffer, buffer_frames);
	if (err == -EPIPE) 
	{
		std::cerr << "Overrun occurred" << std::endl;
		snd_pcm_prepare(handle);
	}
	else if (err < 0) 
	{
		std::cerr << "Error from read: " << snd_strerror(err) << std::endl;
		delete[] buffer;
		snd_pcm_close(handle);
		return false;
	}
	else if (err != buffer_frames) 
	{
		std::cerr << "Short read, read " << err << " frames" << std::endl;
	}

	// Store captured data into the 3D array
	for (int m = 0; m < M_AMOUNT; ++m) 
	{
		for (int n = 0; n < N_AMOUNT; ++n) 
		{
			for (int u = 0; u < buffer_frames; ++u) 
			{
				audio_data[m][n][u] = buffer[(m * N_AMOUNT + n) * buffer_frames + u];
			}
		}
	}

	// Clean up
	delete[] buffer;
	snd_pcm_drain(handle);
	snd_pcm_close(handle);

	return true;
} // end captureAudioData

//==============================================================================================

// Calculates Array Factor and outputs gain
void arrayFactor(const vector<vector<double>>& Data)
{
	// Initialize Array Factor array
	vector<vector<cdouble>> AF(ANGLE_AMOUNT, vector<cdouble>(ANGLE_AMOUNT));

	// Calculate phase shift for every angle
	int indexTHETA = 0; // Reset theta index
	for (int THETA = MIN_ANGLE; indexTHETA < ANGLE_AMOUNT; THETA += ANGLE_STEP, ++indexTHETA)
	{
		int indexPHI = 0; // Reset phi index
		for (int PHI = MIN_ANGLE; indexPHI < ANGLE_AMOUNT; PHI += ANGLE_STEP, ++indexPHI)
		{
			cdouble sum = 0;  // To accumulate results
			for (int m = 0; m < M_AMOUNT; m++)
			{
				for (int n = 0; n < N_AMOUNT; n++)
				{
					// Calculate the phase shift for each element
					double angle = degtorad(THETA * m + PHI * n);
					sum += Data[m][n] * cdouble(cos(angle), sin(angle));
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
double vk; // (2 * M_PI * k) / FFT_SIZE
vector<cdouble> ev(HALF_FFT_SIZE); // exp((-2 * M_PI * k) / FFT_SIZE)
vector<vector<cdouble>> ev2m(HALF_FFT_SIZE, vector<cdouble>(HALF_FFT_SIZE)); // exp((-2 * M_PI * k * 2 * m) / FFT_SIZE)
void constantCalcs()
{
	for (int k = 0; k < HALF_FFT_SIZE; k++)
	{
		vk = (2 * M_PI * k) / FFT_SIZE;     // some vector as a function of k
		ev[k] = cdouble(cos(vk), -sin(vk)); // some complex vector as a function of k
		for (int m = 0; m < HALF_FFT_SIZE; m++)
		{
			ev2m[k][m] = cdouble(cos(vk * 2 * m), -sin(vk * 2 * m)); // some complex vector as a function of k with a factor of 2m in exponent
		}
	}
} // end constantCalcs

//==============================================================================================

// Calculates Ek (first component of FFT for even samples in buffer as a function of k)
vector<vector<vector<cdouble>>> Ek(const vector<vector<vector<double>>>& rawData, int lowerBound, int upperBound)
{
	vector<vector<vector<cdouble>>> result(HALF_FFT_SIZE, vector<vector<cdouble>>(M_AMOUNT, vector<cdouble>(N_AMOUNT, 0)));

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
					cdouble sum = 0;
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
vector<vector<vector<cdouble>>> Ok(const vector<vector<vector<double>>>& rawData, int lowerBound, int upperBound)
{
	vector<vector<vector<cdouble>>> result(HALF_FFT_SIZE, vector<vector<cdouble>>(M_AMOUNT, vector<cdouble>(N_AMOUNT, 0)));

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
					cdouble sum = 0;
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
vector<vector<double>> FFTSum(vector<vector<vector<double>>>& rawData)
{
	// Calls Ek and Ok
	vector<vector<vector<cdouble>>> Xcomp1 = Ek(rawData, lowerFrequency, upperFrequency); 
	vector<vector<vector<cdouble>>> Xcomp2 = Ok(rawData, lowerFrequency, upperFrequency);
		
	// Xk
	vector<vector<vector<cdouble>>> Xresult(HALF_FFT_SIZE, vector<vector<cdouble>>(M_AMOUNT, vector<cdouble>(N_AMOUNT, 0)));
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
	vector<vector<double>> result(M_AMOUNT, vector<double>(N_AMOUNT));
	for (int x = 0; x < M_AMOUNT; x++)
	{
		for (int y = 0; y < N_AMOUNT; y++)
		{
			double sum = 0; // To accumulate results
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

// Prints 3D array of doubles to the console
void print3DDouble(vector<vector<vector<double>>>& inputArray, string& title, int dim1, int dim2, int dim3)
{
	cout << "title\n";
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
	cout << "\n\n\n";
} // end print3DDouble

//==============================================================================================

int main()
{
	// Needs to be 3D for input***
	// 3rd dimension is buffered data***
	// Will be made 3D after data aquisition is made***
	vector<vector<double>> DATA =
	{
		{1, 1, 1, 1},
		{1, 1, 1, 1},
		{1, 1, 1, 1},
		{1, 1, 1, 1}
	};


	// Initializes constants for later use
	constantCalcs();
	
	// Initializes octave band selections
	int bandTypeSelection = 0;
	int thirdOctaveBandSelection = 0;
	int fullOctaveBandSelection = 0;
	// add user inputs for all***
	
	//==============================================================================================

	/* Switch case legend
	legend might change when removing lower and higher frequencies***
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
	
	switch (bandTypeSelection)
	{
		// BroadBand
		case 0:
			// Writes gain to M x N array to be plotted
			arrayFactor(DATA);
		break;
		
		//==============================================================================================
		
		// Full Octave Band
		case 1:
		
			switch (fullOctaveBandSelection)
			{
				// 63 Hz
				case 0:	
					lowerFrequency = 1;
					upperFrequency = 3;
				break;	
				
				// 125 Hz
				case 1:	
					lowerFrequency = 2;
					upperFrequency = 5;
				break;	
				
				// 250 Hz
				case 2:	
					lowerFrequency = 4;
					upperFrequency = 9;
				break;	
				
				// 500 Hz
				case 3:	
					lowerFrequency = 8;
					upperFrequency = 17;
				break;	
					
				// 1000 Hz
				case 4:	
					lowerFrequency = 16;
					upperFrequency = 33;
				break;	
				
				// 2000 Hz
				case 5:	
					lowerFrequency = 32;
					upperFrequency = 66;
				break;	
				
				// 4000 Hz
				case 6:	
					lowerFrequency = 65;
					upperFrequency = 132;
				break;	
				
				// 8000 Hz
				case 7:	
					lowerFrequency = 131;
					upperFrequency = 264;
				break;	
				
				// 16000 Hz
				case 8:	
					lowerFrequency = 263;
					upperFrequency = 511;
				break;	
			} // end Full Octave Band
			
		break;
		
		//==============================================================================================
		
		// Third Octave Band
		case 2:
		
			switch (thirdOctaveBandSelection)
			{
				// 50 Hz
				case 1:
					lowerFrequency = 1; // idk
					upperFrequency = 1;
				break;

				// 63 Hz
				case 2:
					lowerFrequency = 2; // idk
					upperFrequency = 2;
				break;

				// 80 Hz
				case 3:
					lowerFrequency = 1; // idk
					upperFrequency = 3;
				break;
			
				// 100 Hz
				case 4:
					lowerFrequency = 2;
					upperFrequency = 3;
				break;

				// 125 Hz
				case 5:
					lowerFrequency = 2;
					upperFrequency = 4;
				break;

				// 160 Hz
				case 6:
					lowerFrequency = 3;
					upperFrequency = 5;
				break;

				// 200 Hz
				case 7:
					lowerFrequency = 4;
					upperFrequency = 6;
				break;		
				// 250 Hz
				case 8:
					lowerFrequency = 5;
					upperFrequency = 7;
				break;

				// 315 Hz
				case 9:
					lowerFrequency = 6;
					upperFrequency = 9;
				break;

				// 400 Hz
				case 10:
					lowerFrequency = 8;
					upperFrequency = 11;
				break;

				// 500 Hz
				case 11:
					lowerFrequency = 10;
					upperFrequency = 14;
				break;	
					
				
				// 630 Hz
				case 12:
					lowerFrequency = 13;
					upperFrequency = 17;
				break;	
				
			
				// 800 Hz
				case 13:
					lowerFrequency = 16;
					upperFrequency = 21;
				break;	
					
				
				// 1000 Hz
				case 14:
					lowerFrequency = 20;
					upperFrequency = 27;
				break;	
				

				// 1250 Hz
				case 15:
					lowerFrequency = 26;
					upperFrequency = 33;
				break;	
					
				
				// 1600 Hz
				case 16:
					lowerFrequency = 32;
					upperFrequency = 42;
				break;	
				
			
				// 2000 Hz
				case 17:
					lowerFrequency = 41;
					upperFrequency = 52;
				break;	
					
				
				// 2500 Hz
				case 18:
					lowerFrequency = 51;
					upperFrequency = 66;
				break;	
				
				
				// 3150 Hz
				case 19:
					lowerFrequency = 65;
					upperFrequency = 83;
				break;	
					
				
				// 4000 Hz
				case 20:
					lowerFrequency = 82;
					upperFrequency = 104;
				break;	
				
			
				// 5000 Hz
				case 21:
					lowerFrequency = 103;
					upperFrequency = 131;
				break;	
					
				
				// 6300 Hz
				case 22:
					lowerFrequency = 130;
					upperFrequency = 165;
				break;	
				

				// 8000 Hz
				case 23:
					lowerFrequency = 164;
					upperFrequency = 207;
				break;	
					
				
				// 10000 Hz
				case 24:
					lowerFrequency = 206;
					upperFrequency = 261;
				break;	
				
			
				// 12500 Hz
				case 25:
					lowerFrequency = 260;
					upperFrequency = 329;
				break;	
					
				
				// 16000 Hz
				case 26:
					lowerFrequency = 328;
					upperFrequency = 413;
				break;	
				

				// 20000 Hz
				case 27:
					lowerFrequency = 412;
					upperFrequency = 511;
				break;				
			} // end Third Octave Band		
		
		//==============================================================================================
		
		// Filters data to remove unneeded frequencies and sums all frequency bins
		//vector<vector<double>> filteredData = FFTSum(DATA);	***disabled until data aquisition is made***


		// Writes gain to M x N array to be plotted
		//arrayFactor(filteredData);	***disabled until data aquisition is made***	
		
		
		break;
	} // end Frequency Selection
	
	//==============================================================================================
	
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
} // end main