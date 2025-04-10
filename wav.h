// Libraries
#include <iostream>
#include <cmath>
#include <chrono>

// Headers
#include "Structs.h"
#include "Timer.h"
#include "PARAMS.h"
#include "AudioFile.h"


class WAV
{
public:
    // Constructor
    WAV();

    // Destructor
    ~WAV();

    // Sets up all constants and initialized FFT
    bool setup(const char* file_name);

    // Writes data to wav file
    void readWAV(array3D<float>& data_buffer_1, array3D<float>& data_buffer_2);


private:

timer WAV_timer; // Timer for time travel!?
AudioFile<double> input_audio;

int sampleRate;
int bitDepth;
int numSamplesPerChannel;
double lengthInSeconds;
int numChannels;



int b_file;

array2D<int> channel_order;

array3D<float> data_buffer_1;   // Buffer for audio data
array3D<float> data_buffer_2;   // Buffer for audio data

};

WAV::WAV() :    sampleRate(0),
                bitDepth(0),
                numSamplesPerChannel(0),
                lengthInSeconds(0.0),
                numChannels(0),
                b_file(0),
                channel_order(M_AMOUNT, N_AMOUNT), // Initialize array2D with dimensions
                data_buffer_1(M_AMOUNT, N_AMOUNT, FFT_SIZE), // Initialize array3D with dimensions
                data_buffer_2(M_AMOUNT, N_AMOUNT, FFT_SIZE),  // Initialize array3D with dimensions
                WAV_timer("WAV") // Initialize timer with name
{


}  

 // end WAV

WAV::~WAV()
{

} // end ~WAV



bool WAV::setup(const char* file_name) {

        // Map channel order
    for (int m = 0; m < channel_order.dim_1; m++)
    {
        for (int n = 0; n < channel_order.dim_2; n++)
        {
            channel_order.at(m, n) = CHANNEL_ORDER[m][n];
        }
    }
    
    if (!input_audio.load(file_name)) {
        std::cerr << "Error: Could not load audio file." << std::endl;
        return false;
    } // Load the audio file

    // Read details of the file
    sampleRate = input_audio.getSampleRate(); 
    bitDepth = input_audio.getBitDepth();
    numSamplesPerChannel = input_audio.getNumSamplesPerChannel();
    lengthInSeconds = input_audio.getLengthInSeconds();
    numChannels = input_audio.getNumChannels();

    //Print Details

    std::cout << "Sample Rate: " << sampleRate << std::endl;
    std::cout << "Bit Depth: " << bitDepth << std::endl;
    std::cout << "Number of Samples per Channel: " << numSamplesPerChannel << std::endl;
    std::cout << "Length in Seconds: " << lengthInSeconds << std::endl;
    std::cout << "Number of Channels: " << numChannels << std::endl;

    //Check if the file matches current camera configuration

    if (numChannels != NUM_CHANNELS)
    {
        std::cerr << "Error: Number of channels in the file does not match the camera configuration." << std::endl;
        return false;
    }
    if (sampleRate != SAMPLE_RATE)
    {
        std::cerr << "Error: Sample rate in the file does not match the camera configuration." << std::endl;
        return false;
    }
    if (bitDepth != 32)
    {
        std::cerr << "Error: Bit depth in the file does not match the camera configuration." << std::endl;
        return false;
    }

    std::cout << "File matches camera configuration." << std::endl;
    
    WAV_timer.start(); // Start the timer

    return true;
} // end setup

//=====================================================================================

void WAV::readWAV(array3D<float>& data_buffer_1, array3D<float>& data_buffer_2) {

    //cout << "Reading Wav File..." << endl;

    swap(data_buffer_1.data, data_buffer_2.data);

    //cout << "Buffer swapped..." << endl;

    if((b_file * 1024) + data_buffer_2.dim_3 > numSamplesPerChannel)
    {
        b_file = 0;
        cout << "Repeating Wav File..." << endl;
    }

   // cout << "checked b_file..." << endl;
   
   // std::cout << "data_buffer_2 dimensions: " 
        //   << data_buffer_2.dim_1 << " x " 
        //   << data_buffer_2.dim_2 << " x " 
        //   << data_buffer_2.dim_3 << std::endl;
        //   data_buffer_2.at(0, 0, 0) = 5;
   // cout << "data_buffer_2.at(0, 0, 0): " << data_buffer_2.at(0, 0, 0) << endl;

   //cout << input_audio.samples[1][1] << endl;

    for (int m = 0; m < data_buffer_2.dim_1; m++){
        //cout << "m: " << m << endl;
        for (int n = 0; n < data_buffer_2.dim_2; n++){
            //cout << "n: " << n << endl;
            for (int b = 0; b < data_buffer_2.dim_3; b++){
                //cout << "b: " << b <<  " b_file: " << b_file << endl;
                data_buffer_2.at(m, n, b) = input_audio.samples[channel_order.at(m, n)][b_file * 1024 + b];
               // data_buffer_2.at(m, n, b) = input_audio.samples[2][30];
                
                //cout << "end for loops" << endl;
            }
            
        }
    }

    b_file++;

    WAV_timer.end(); // End the timer
    if (WAV_timer.time() < data_buffer_2.dim_3 * 1000 / sampleRate)
    {
        cout << "yurp, ur time travelin" << endl;
        // this_thread::sleep_for(chrono::seconds((WAV_timer.time() - data_buffer_2.dim_3 * 1 / (sampleRate / 1000))));
    }
    //WAV_timer.print(); // Print the time taken
    WAV_timer.start(); // Restart the timer


}