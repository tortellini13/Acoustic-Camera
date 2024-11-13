#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <ctime>

#include "PARAMS.h"
#include "sharedMemory.h"
//#include "ALSA.h"
//#include "Beamform.h"

using namespace std;
using namespace cv;

//==============================================================================================

// Initialize video capture and writer
    
    VideoWriter video1;

// Initialize Variables





//UI Funcitons

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
Mat DrawUI(Mat frameRGBA, string maximumText, Point maxPointScaled, Mat colorBar, double magnitudeMax, double magnitudeMin) {

        // Graphics and Text
        Mat frameRGB;

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

    return frameRGB;
}
//Heat Map Functions
Mat HeatMapAlphaMerge(Mat heatMapData, Mat heatMapRGBA, Mat frameRGBA, int thresholdValue, double alpha) {
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
    return frameRGBA;
}
double FPSCalculator() {
 if (FPSCountState == 1) {
            frameCount++;
            if (frameCount == FPS_COUNTER_AVERAGE) {
                double FPSTimeEnd = getTickCount();
                double FPSTimeDifference = (FPSTimeEnd - FPSTimeStart) / getTickFrequency();
                FPSTimeStart = FPSTimeEnd;
                FPS = frameCount / FPSTimeDifference;
                frameCount = 0;
                
            }
        
        }

    return FPS;
}
Mat mainLoop(Mat frame, int codec, string videoFileName) {
    double magnitudeMin;
    double magnitudeMax;
    double alpha;
    int thresholdValue;
    Point maxPoint;
    Mat heatMapDataNormal, heatMapRGB, heatMapRGBA, blended, frameRGBA, frameRGB;  // Establish matricies for 
    Mat heatMapData(Size(RESOLUTION_HEIGHT, RESOLUTION_WIDTH), CV_32FC1);                                                          // Make heat map data matrix
    Mat magnitudeFrame(NUM_ANGLES, NUM_ANGLES, CV_32FC1, Scalar(0)); // Single channel, magnitude matrix, initialized to 0
    Mat colorBar(SCALE_HEIGHT, SCALE_WIDTH, CV_8UC3);
    
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
            frameRGBA =  HeatMapAlphaMerge(heatMapData, heatMapRGBA, frameRGBA, thresholdValue, alpha);
        }
        
        colorBar = makeColorBar();
        frameRGB = DrawUI(frameRGBA, maximumText, maxPointScaled, colorBar, magnitudeMax, magnitudeMin);
        
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

    return frameRGB;

}


int main() 
{
    //==============================================================================================
    
    // Video frame and capture settings
                     // Set framerate of capture from PARAMS.h
    int codec = VideoWriter::fourcc('M', 'J', 'P', 'G'); // Set codec of video recording
    string videoFileName = "testoutput";           // Set video file output file name

    Mat frame;
    
    //==============================================================================================
    
    // Initially open window
    initializeWindow();
    onResetUI(1,0);

    cout << "2. Starting main loop.\n";

    while                                                                                                                                                                                                                                                                                                                                                                                                                           (true) 
    {
       
        cap >> frame; // Capture the frame
        
        Mat frameRGB = mainLoop(frame, codec, videoFileName);
       
        // Shows frame
        imshow("Heat Map Overlay", frameRGB);
 
        //==============================================================================================                               

        // Record if set to record
        
        if (recordingState == 1) { 
            video1.write(frameRGB);
        }
        
        //==============================================================================================

        // Break loop if key is pressed
        if (waitKey(1) >= 0) break;
        FPS = FPSCalculator();
    
    }   // end loop

    //==============================================================================================

    // Close video
    cap.release();
    video1.release();

    return 0;
} // end main
