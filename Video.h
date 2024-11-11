#ifndef BEAMFORM
#define BEAMFORM

// Internal Libraries
#include <iostream>

// External Libraries
#include <opencv2/opencv.hpp>

// Header Files
#include "PARAMS.h"

using namespace std;
using namespace cv;

//=====================================================================================

class video
{
public:
    // Initialize video class
    video(int other);

    // Clear memory for all arrays
    ~video();

    // Creates window
    void initializeWindow();

    void displayHeatmap(float data_input[]);

private:
    // Functions

    // Variables
    int thing;

};

//=====================================================================================

video::video(int other):
    thing(other)

    
    // Allocate memory for all arrays
    {

    };

//=====================================================================================

video::~video()
{

} // end ~video

//=====================================================================================

void video::displayHeatmap(float data_input[])
{

} // end displayHeatmap

//=====================================================================================

#endif