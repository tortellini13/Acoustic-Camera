#ifndef AUDIO
#define AUDIO

// Libraries
#include <iostream>
#include <cmath>
#include <vector>
#include <math.h>
#include <complex>
#include <alsa/asoundlib.h>

// User files
#include "PARAMS.h"

using namespace std;

//==============================================================================================

// Definitions
typedef complex<float> cfloat;

// Defines amount of angles to sweep through (coordinates)
// CHANGE THIS TO USE THE ONE IN PARAMS*******
const int ANGLE_AMOUNT = ((MAX_ANGLE - MIN_ANGLE) / ANGLE_STEP + 1);

// CHANGE THIS TO USE THE ONE IN PARAMS*******
// Half of FFT_SIZE
const int HALF_FFT_SIZE = FFT_SIZE / 2;

//==============================================================================================

// Converts degrees to radians
float degtorad(float angleDEG)
{
	float result = angleDEG * (M_PI / 180);
	return result;
} // end degtorad

//==============================================================================================

// Calculates Array Factor
void arrayFactor(const vector<vector<float>>& AUDIO_DATA, vector<vector<float>> output)
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
					sum += AUDIO_DATA[m][n] * cfloat(cos(angle), sin(angle));
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
			output[indexTHETA][indexPHI] = 10 * log10(norm(AF[indexTHETA][indexPHI]));

		}
	}

} // end arrayFactor

//==============================================================================================

// Calculate all constants and write to arrays to access later
//float vectork; // (2 * M_PI * k) / FFT_SIZE
//vector<cfloat> complexVectorFirst(HALF_FFT_SIZE); // exp((-2 * M_PI * k) / FFT_SIZE)
//vector<vector<cfloat>> complexVectorSecond(HALF_FFT_SIZE, vector<cfloat>(HALF_FFT_SIZE)); // exp((-2 * M_PI * k * 2 * m) / FFT_SIZE)
void constantCalcs(vector<cfloat> complexVectorFirst, vector<vector<cfloat>> complexVectorSecond)
{
	for (int k = 0; k < HALF_FFT_SIZE; k++)
	{
		float vectork = (2 * M_PI * k) / FFT_SIZE;    // some vector as a function of k
		complexVectorFirst[k] = cfloat(cos(vectork), -sin(vectork)); // some complex vector as a function of k
		for (int b = 0; b < HALF_FFT_SIZE; b++)
		{
			complexVectorSecond[k][b] = cfloat(cos(vectork * 2 * b), -sin(vectork * 2 * b)); // some complex vector as a function of k with a factor of 2m in exponent
		}
	}
} // end constantCalcs

//==============================================================================================

// Calculates Ek (first component of FFT for even samples in buffer as a function of k)
void evenConstants(const vector<vector<vector<float>>>& AUDIO_DATA, vector<vector<vector<cfloat>>> output, vector<vector<cfloat>> complexVectorSecond, int lowerBound, int upperBound)
{
	//vector<vector<vector<cfloat>>> result(HALF_FFT_SIZE, vector<vector<cfloat>>(M_AMOUNT, vector<cfloat>(N_AMOUNT, 0)));

    // Loop through M-axis
	for (int m = 0; m < M_AMOUNT; m++) 
	{
        // Loop through N-axis
		for (int n = 0; n < N_AMOUNT; n++) 
		{
            // k (frequency bin)
			for (int k = 0; k < HALF_FFT_SIZE; k++) 
			{
				// Set result = 0 for all k lower than lowerBound
				if (k < lowerBound)
				{
					output[m][n][k] = 0;
				}
					
				// Set result = 0 for all k greater than upperBound
				else if (k > upperBound)
				{
					output[m][n][k] = 0;
				}
					
				// Calculates evenConstant array
				else
				{
					cfloat sum = 0;

                    // b (buffered sample index)
					for (int b = 0; b < HALF_FFT_SIZE; b += 2) 
					{
						sum += AUDIO_DATA[m][n][b] * complexVectorSecond[k][b];
					}
					output[m][n][k] = sum;	
				} // end evenConstant calcs
			} // end k
		} // end n
	} // end m
} // end evenConstants

//==============================================================================================

// Calculates oddConstants (second component of FFT for odd samples in buffer as a function of k)
void oddConstants(const vector<vector<vector<float>>>& AUDIO_DATA, vector<vector<vector<cfloat>>> output, vector<vector<cfloat>> complexVectorSecond, int lowerBound, int upperBound)
{
	//vector<vector<vector<cfloat>>> result(HALF_FFT_SIZE, vector<vector<cfloat>>(M_AMOUNT, vector<cfloat>(N_AMOUNT, 0)));

	for (int m = 0; m < M_AMOUNT; m++) // Loop through M-axis
	{
		for (int n = 0; n < N_AMOUNT; n++) // Loop through N-axis
		{
			for (int k = 0; k < HALF_FFT_SIZE; k++) // k (frequency bin)
			{
				// Set result = 0 for all k lower than lowerBound
				if (k < lowerBound - HALF_FFT_SIZE)
				{
					output[m][n][k] = 0;
				}
					
				// Set result = 0 for all k greater than upperBound
				else if (k > upperBound - HALF_FFT_SIZE)
				{
					output[m][n][k] = 0;
				}
					
				// Calculates Ok array
				else
				{
					cfloat sum = 0;
					for (int b = 1; b < HALF_FFT_SIZE; b += 2) // b (buffered sample index)
					{
						sum += AUDIO_DATA[m][n][b] * complexVectorSecond[k][b]; // Calculates oddConstants array
					}
					output[m][n][k] = sum;	
				} // end odConstant calc
			} // end k
		} // end n
	} // end m
} // end oddConstants

//==============================================================================================

// Calculates X array (total FFT as a function of k) and sums all of the values in the desired range
// Outputs M x N array of filtered data
void FFTSum(vector<vector<vector<float>>>& AUDIO_DATA, vector<vector<float>> output, vector<cfloat> complexVectorFirst, vector<vector<cfloat>> complexVectorSecond, int lowerBound, int upperBound)
{
	// Calls evenConstants
	vector<vector<vector<cfloat>>> Xcomp1;
    evenConstants(AUDIO_DATA, Xcomp1, complexVectorSecond, lowerBound, upperBound); 

    // Calls oddConstants
	vector<vector<vector<cfloat>>> Xcomp2;
    oddConstants(AUDIO_DATA, Xcomp2, complexVectorSecond, lowerBound, upperBound);
		
	// Xvector
	vector<vector<vector<cfloat>>> Xresult(HALF_FFT_SIZE, vector<vector<cfloat>>(M_AMOUNT, vector<cfloat>(N_AMOUNT, 0)));
	for (int m = 0; m < M_AMOUNT; m++)
	{
		for (int n = 0; n < N_AMOUNT; n++)
		{
			for (int k = 0; k < HALF_FFT_SIZE; k++)
			{
				Xresult[m][n][k] = Xcomp1[m][n][k] + complexVectorFirst[k] * Xcomp2[m][n][k];
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
	//vector<vector<float>> result(M_AMOUNT, vector<float>(N_AMOUNT));
	for (int m = 0; m < M_AMOUNT; m++)
	{
		for (int n = 0; n < N_AMOUNT; n++)
		{
			float sum = 0; // To accumulate results
			for (int k = 0; k < HALF_FFT_SIZE; k++)
			{
				sum += norm(Xresult[m][n][k]); // magnitude of complex number sqrt(a^2 + b^2)
			}
			output[m][n] = sum;
		}
	}
}// end FFTSum

//==============================================================================================

// Set up parameters for audio interface
int setupAudio(snd_pcm_t **pcm_handle, snd_pcm_uframes_t *frames) 
{
    snd_pcm_hw_params_t *params;
    unsigned int rate = SAMPLE_RATE;
    int dir, pcm;

    // Open the PCM device in capture mode
    pcm = snd_pcm_open(pcm_handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (pcm < 0) 
    {
        cerr << "Unable to open PCM device: " << snd_strerror(pcm) << endl;
        return pcm;
    }
    //cout << "PCM device opened in capture mode.\n"; // Debugging

    // Allocate a hardware parameters object
    snd_pcm_hw_params_alloca(&params);
    //cout << "Hardware parameter object allocated.\n"; // Debugging

    // Set the desired hardware parameters
    snd_pcm_hw_params_any(*pcm_handle, params);
    snd_pcm_hw_params_set_access(*pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(*pcm_handle, params, SND_PCM_FORMAT_FLOAT_LE);  // 32-bit float PCM
    snd_pcm_hw_params_set_channels(*pcm_handle, params, NUM_CHANNELS); 
    snd_pcm_hw_params_set_rate_near(*pcm_handle, params, &rate, &dir);
    cout << "Hardware parameters set.\n";

    // Write the parameters to the PCM device
    pcm = snd_pcm_hw_params(*pcm_handle, params);
    if (pcm < 0) 
    {
        cerr << "Unable to set HW parameters: " << snd_strerror(pcm) << endl;
        return pcm;
    }
    //cout << "Parameters written to device.\n"; // Debugging

    // Get the period size (frames per buffer)
    snd_pcm_hw_params_get_period_size(params, frames, &dir);

    return 0;
} // end setupAudio

//==============================================================================================

// Function to capture audio into a 3D vector (M_AMOUNT x N_AMOUNT x FFT_SIZE)
int captureAudio(vector<vector<vector<float>>> &output, snd_pcm_t *pcm_handle) {
    vector<float> buffer(FFT_SIZE * NUM_CHANNELS);  // Flat buffer for data
    int pcm;

    // Capture PCM audio
    while (1) 
    {
        pcm = snd_pcm_readi(pcm_handle, buffer.data(), FFT_SIZE);
        if (pcm == -EPIPE) 
        {
            // Buffer overrun
            snd_pcm_prepare(pcm_handle);
            cerr << "Buffer overrun occurred, PCM prepared.\n";
            continue;  // Try reading again
        } 
        else if (pcm < 0) 
        {
            cerr << "Error reading PCM data: " << snd_strerror(pcm) << endl;
            return pcm;
        }
        else if (pcm != FFT_SIZE)
        {
            cerr << "Short read, read " << pcm << " frames\n";
            continue;  // Try reading again
        }
        else
        {
            break; // Successful read
        }
    }
    // cout << "Data read from device.\n"; // Debugging

    // Populate the 3D vector with captured data (Unflatten the buffer)
    for (int k = 0; k < FFT_SIZE; ++k) 
    {
        for (int m = 0; m < M_AMOUNT; ++m) 
        {
            for (int n = 0; n < N_AMOUNT; ++n) 
            {
                output[m][n][k] = buffer[k * NUM_CHANNELS + (m * N_AMOUNT + n)];
            }
        }
    }
    // cout << "3D data vector populated with audio data.\n"; // Optional

    return 0;
} // end captureAudio

//==============================================================================================
#endif AUDIO