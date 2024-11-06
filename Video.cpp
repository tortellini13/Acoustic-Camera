#include <opencv2/opencv.hpp>
#include <iostream>
#include "PARAMS.h"
#include "sharedMemory.h"
#include <vector>
#include <ctime>

using namespace cv;
using namespace std;

//==============================================================================================
//Configuration of callbacks for buttons
int listMaxMagState = 1;
int markMaxMagState = 1;
int colorScaleState = 1;
int heatMapState = 1;
int FPSCountState = 1;
int resetUIState = 0;
int recordingState = 0;
int recordingStateChangeFlag = 0;
void onListMaxMag(int state, void* userdata) {
    listMaxMagState = state; // Update button state
}
void onMarkMaxMag(int state, void* userdata) {
    markMaxMagState = state; // Update button state
}
void onColorScaleState(int state, void* userdata) {
    colorScaleState = state;
}
void onHeatMap(int state, void* userdata) {
    heatMapState = state;
  
}
void onFPSCount(int state, void* userdata) {
    FPSCountState = state;
}
void onRecording(int state, void* userdata) {
    recordingState = state;
    recordingStateChangeFlag = 1;
}
void onResetUI(int state, void* userdata) {
    //Set initial trackbar positions
    setTrackbarPos("Threshold", "Heat Map Overlay", DEFAULT_THRESHOLD);
    setTrackbarPos("Alpha", "Heat Map Overlay", DEFAULT_ALPHA);   

}
//==============================================================================================

void initializeWindow() {
    // Create trackbar and buttons
    createTrackbar("Threshold", "Heat Map Overlay", nullptr, MAP_THRESHOLD_MAX);    
    createTrackbar("Alpha", "Heat Map Overlay", nullptr, 100);
    createButton("List Maximum Magnitude",onListMaxMag, NULL, QT_CHECKBOX,1);
    createButton("Mark Maximum Magnitude",onMarkMaxMag, NULL, QT_CHECKBOX,1);
    createButton("Show Color Scale",onColorScaleState, NULL, QT_CHECKBOX,1);
    createButton("Show Heat Map",onHeatMap, NULL, QT_CHECKBOX,1);
    createButton("Show FPS", onFPSCount, NULL, QT_CHECKBOX, 1);
    //createButton("ResetUI", onResetUI, NULL, QT_CHECKBOX, 0);
    createButton("Record Video", onRecording, NULL, QT_CHECKBOX, 0);
    
}
Mat makeColorBar() {
 Mat colorBar(SCALE_HEIGHT, SCALE_WIDTH, CV_8UC3);
    Mat scaleColor(SCALE_HEIGHT, SCALE_WIDTH, CV_8UC1);

    for(int y = 0; y < SCALE_HEIGHT; y++) {

        int scaleIntensity = 255 * (static_cast<double>((SCALE_HEIGHT - y)) / static_cast<double>(SCALE_HEIGHT));
        
        for(int x = 0; x < SCALE_WIDTH; x++) {
            scaleColor.at<uchar>(y, x) = scaleIntensity;

        }
    }
    
    applyColorMap(scaleColor, colorBar, COLORMAP_INFERNO);
    return colorBar;
}

int main() 
{

    
    //==============================================================================================
    
    // Initializes array to receive data from Audio
    vector<vector<float>> magnitudeInput(NUM_ANGLES, vector<float>(NUM_ANGLES));   

    // Initializes array to send user config to Audio. Filled with default settings
    vector<int> userConfigs(NUM_CONFIGS, 0);     // ***Might need to manually set default settings     
    
    // Initialize shared memory class
    sharedMemory shm(AUDIO_SHM, CONFIG_SHM, SEM_1, SEM_2, NUM_ANGLES, NUM_ANGLES, NUM_CONFIGS);   

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

    //==============================================================================================

    // Open shared memory for Audio
    if (!shm.shmStart2()) 
    {                                                             
        cerr << "2. shmStart2 Failed\n";
    }
    cout << "2. Shared Memory Configured.\n"; // For debugging

    //==============================================================================================

    // Setup video capture
    VideoCapture cap(0, CAP_V4L2);
    VideoWriter video1;
    
    // Video frame and capture settings
    cap.set(CAP_PROP_FRAME_WIDTH, RESOLUTION_WIDTH);   // Set frame width for capture
    cap.set(CAP_PROP_FRAME_HEIGHT, RESOLUTION_HEIGHT); // Set frame height for capture
    cap.set(CAP_PROP_FPS, FRAME_RATE);                 // Set framerate of capture from PARAMS.h

    int codec = VideoWriter::fourcc('M', 'J', 'P', 'G'); // Set codec of video recording
    string videoFileName = "testoutput";           // Set video file output file name

    // Initialize a ton of stuff
    double magnitudeMin;
    double magnitudeMax;
    double alpha;
    double FPS = 0;
    
    int frameCount = 0;
    int thresholdValue;
    int threshTrackMax = MAP_THRESHOLD_MAX;

    Point maxPoint;
    
    Mat heatMapDataNormal, heatMapRGB, heatMapRGBA, blended, frameRGBA, frameRGB, frame;  // Establish matricies for 
    Mat heatMapData(Size(RESOLUTION_HEIGHT, RESOLUTION_WIDTH), CV_32FC1);                                                          // Make heat map data matrix
    Mat magnitudeFrame(NUM_ANGLES, NUM_ANGLES, CV_32FC1, Scalar(0)); // Single channel, magnitude matrix, initialized to 0
    Mat colorBar(SCALE_HEIGHT, SCALE_WIDTH, CV_8UC3);
    Mat UIlayerout(SCALE_HEIGHT, SCALE_WIDTH, CV_8UC4);
    Mat UIMask(RESOLUTION_HEIGHT, RESOLUTION_WIDTH, CV_8UC1);   

    //==============================================================================================
    
    //Initially open window
    cap >> frame;
    imshow("Heat Map Overlay", frame);
    initializeWindow();
    onResetUI(1,0);
    
    colorBar = makeColorBar();
    
    double FPSTimeStart = getTickCount();

    cout << "2. Starting main loop.\n";
    while                                                                                                                                                                                                                                                                                                                                                                                                                           (1) 
    {
        //==============================================================================================
        // Video Recording setup when enabled
        if (recordingStateChangeFlag == 1) {
            if (recordingState == 1) {
                ostringstream videoFileStream;
                videoFileStream << "./" << videoFileName << " " << time(nullptr) << ".avi" ;
                string videoFileFullOutput = videoFileStream.str();
                video1.open(videoFileFullOutput, codec, FRAME_RATE, Size(RESOLUTION_WIDTH, RESOLUTION_HEIGHT), 1);
            }
            if (recordingState == 0) {
                video1.release();
            }
            recordingStateChangeFlag = 0;
        }

        //==============================================================================================

        cap >> frame; // Capture the frame
        
        //==============================================================================================       
        
        /*
        // Reads audio data from shared memory and writes user configs to shared memory
        if (!shm.handleshm2(magnitudeInput, userConfigs))
        {
            cerr << "2. handleshm2 failed.\n";
        }
        */

        for (int rows = 0; rows < NUM_ANGLES; rows++) // Converts vector<vector<float>> into an OpenCV matrix
        { 
            for (int columns = 0; columns < NUM_ANGLES; columns++)
            {
                magnitudeFrame.at<int>(rows, columns) = magnitudeInput[rows][columns];
            }
        } 

        // Generates random data for testing
        randu(magnitudeFrame, Scalar(0), Scalar(300));

        //==============================================================================================

        // Magnitude Data Proccessing
        resize(magnitudeFrame, heatMapData, Size(RESOLUTION_WIDTH, RESOLUTION_HEIGHT), 0, 0, INTER_CUBIC);       // Scaling and interpolating into camera resolution
        normalize(heatMapData, heatMapDataNormal, 0, 255, NORM_MINMAX); // Normalize the data into a (0-255) range
        heatMapDataNormal.convertTo(heatMapDataNormal, CV_8UC1);        // Convert normalized heat map data data type

        //==============================================================================================

        // Finding maximum magnitude in incoming data, scaling location for marking
        minMaxLoc(magnitudeFrame, &magnitudeMin, &magnitudeMax, NULL, &maxPoint); //Find maximum magnitude from incoming data with location
     
        int scaledPointX = (static_cast<double>(maxPoint.x)/static_cast<double>(magnitudeFrame.cols)) * RESOLUTION_WIDTH; //Scale max point to video resolution 
        int scaledPointY = (static_cast<double>(maxPoint.y)/static_cast<double>(magnitudeFrame.rows)) * RESOLUTION_HEIGHT;
        Point maxPointScaled(scaledPointX, scaledPointY);
        ostringstream magnitudeMaxStream;
        magnitudeMaxStream << fixed << setprecision(LABEL_PRECISION) << magnitudeMax;
        String maximumText = "Maximum = " + magnitudeMaxStream.str();
        
        //==============================================================================================
        
        // Colormap creation
        applyColorMap(heatMapDataNormal, heatMapRGB, COLORMAP_INFERNO); // Convert the normalized heat map data into a colormap
        cvtColor(heatMapRGB, heatMapRGBA, COLOR_RGB2RGBA);              // Convert heat map from RGB to RGBA
        cvtColor(frame, frameRGBA, COLOR_RGB2RGBA);                     // Convert video frame from RGB to RGBA
        
        //==============================================================================================

        // Compare value of input array to threshold. Don't render heatmap if below threshold
        thresholdValue = getTrackbarPos("Threshold", "Heat Map Overlay");
        alpha = static_cast<double>(getTrackbarPos("Alpha", "Heat Map Overlay"))/100;
        
        if (heatMapState == 1) {
        for(int y = 0; y < heatMapData.rows; ++y)
        {
            for (int x = 0; x < heatMapData.cols; ++x) 
            {
                if (heatMapData.at<uchar>(y, x) > thresholdValue) 
                {
                    Vec4b& pixel = frameRGBA.at<Vec4b>(y, x);         // Current pixel in camera
                    Vec4b heatMapPixel = heatMapRGBA.at<Vec4b>(y, x); // Current pixel in heatmap

                    // Loops through R G & B channels
                    for (int c = 0; c < 3; c++) 
                    {
                        // Merges heatmap with video and applies alpha value
                        pixel[c] = static_cast<uchar>(alpha * heatMapPixel[c] + (1 - alpha) * pixel[c]);
                    } 
                }
            }
        } // end alphaMerge
    }

        // Graphics and Text
        
        cvtColor(frameRGBA, frameRGB, COLOR_RGBA2RGB);
        
        if(listMaxMagState == 1){
            Point maxTextLocation(MAX_LABEL_POS_X, MAX_LABEL_POS_Y);
    
            int textBaseline=0;
            Size textSize = getTextSize(maximumText, FONT_TYPE, FONT_SCALE, FONT_THICKNESS, &textBaseline);
            
            rectangle(frameRGB, maxTextLocation + Point(0, textBaseline), maxTextLocation + Point(textSize.width, -textSize.height), Scalar(0, 0, 0), FILLED); //Draw rectangle for text
            putText(frameRGB, maximumText, maxTextLocation + Point(0, +5), FONT_TYPE, FONT_SCALE, Scalar(255, 255, 255), FONT_THICKNESS); //Write text for maximum magnitude
        }

        if(markMaxMagState == 1) {
            
            drawMarker(frameRGB, maxPointScaled, Scalar(255, 255, 255), MARKER_CROSS, CROSS_SIZE, CROSS_THICKNESS, 8); //Mark the maximum magnitude point
        }

        if(colorScaleState == 1) {
            
            rectangle(frameRGB, Point(SCALE_POS_X, SCALE_POS_Y) + Point(-SCALE_BORDER, -SCALE_BORDER - 10), Point(SCALE_POS_X, SCALE_POS_Y) + Point(SCALE_WIDTH + SCALE_BORDER - 1, SCALE_HEIGHT + SCALE_BORDER + 5), Scalar(0, 0, 0), FILLED); //Draw rectangle behind scale to make a border
            colorBar.copyTo(frameRGB(Rect(SCALE_POS_X, SCALE_POS_Y, SCALE_WIDTH, SCALE_HEIGHT))); //Copy the scale onto the image

            //Draw text indicating various points on the scale

            float scaleTextRatio = (1 / static_cast<float>(SCALE_POINTS ));
            for (int i = 0; i < (SCALE_POINTS + 1); i++) {
                
                
                Point scaleTextStart(SCALE_POS_X, (SCALE_POS_Y + ((1 - static_cast<double>(i) * scaleTextRatio) * SCALE_HEIGHT) - 3)); //Starting point for the text
                double scaleTextValue = ((static_cast<double>(magnitudeMax - magnitudeMin) * scaleTextRatio * static_cast<double>(i)) + magnitudeMin); //Value of text for each point

                ostringstream scaleTextStream;
                scaleTextStream << fixed << setprecision(LABEL_PRECISION) << scaleTextValue;
                String scaleTextString = scaleTextStream.str() + " ";

                putText(frameRGB, scaleTextString, scaleTextStart, FONT_TYPE, FONT_SCALE - 0.2, Scalar(255, 255, 255), FONT_THICKNESS);
                line(frameRGB, scaleTextStart + Point(0,3), scaleTextStart + Point(SCALE_WIDTH, 3), Scalar(255, 255, 255), 1, 8, 0);
            }

        }
        
        if (FPSCountState == 1) {
            ostringstream FPSStream;
            FPSStream << fixed << setprecision(LABEL_PRECISION) << FPS;
            String FPSString = "FPS: " + FPSStream.str();
            int FPSBaseline = 0;
            Point FPSTextLocation(20, 460);
            Size FPStextSize = getTextSize(FPSString, FONT_TYPE, FONT_SCALE, FONT_THICKNESS, &FPSBaseline);
            
            rectangle(frameRGB, FPSTextLocation + Point(0, 6), FPSTextLocation + Point(80, - 10 - 3), Scalar(0, 0, 0), FILLED); //Draw rectangle for text
            putText(frameRGB, FPSString, FPSTextLocation, FONT_TYPE, FONT_SCALE, Scalar(255, 255, 255), FONT_THICKNESS); //Write text for FPS
            
        }

        
       
        // Shows frame
        imshow("Heat Map Overlay", frameRGB);
 
        //==============================================================================================                               

        // Record if set to record
        
        if (recordingState == 1) { 
            video1.write(frameRGB);
        }
        
        if (getWindowProperty("Heat Map Overlay", WND_PROP_VISIBLE) < 1) {
            //Exit the loop if the window is closed
            break;
        }
        
        //==============================================================================================

        if (FPSCountState == 1) {
            frameCount++;
            if (frameCount == FPS_COUNTER_AVERAGE) {
                double FPSTimeEnd = getTickCount();
                double FPSTimeDifference = (FPSTimeEnd - FPSTimeStart) / getTickFrequency();
                FPSTimeStart = FPSTimeEnd;
                FPS = frameCount / FPSTimeDifference;
                frameCount = 0;
                //cout << FPS << endl;
            }
        
        }

        // Break loop if key is pressed
        if (waitKey(1) >= 0) break;

    } // end loop

    //==============================================================================================

    // Close video
    cap.release();
    video1.release();
    //destroyAllWindows(); 

    // Close shm
    shm.closeAll();
    shm.~sharedMemory();

    return 0;
} // end main
