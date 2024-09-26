#include <opencv2/opencv.hpp>
#include <iostream>
#include "PARAMS.h"
#include "sharedMemory.h"
#include <vector>

using namespace cv;
using namespace std;



int main() {

    vector<vector<float>> magnitudeInput;
    sharedMemory audioData(AUDIO_SHM, AUDIO_SHM_1, AUDIO_SEM_2, NUM_ANGLES, NUM_ANGLES); 
    audioData.openAll();
    audioData.read2D(magnitudeInput);



    //set this to 1 to record video
    int recording = 0;
  
   Mat heatMapData, heatMapDataNormal, heatMapRGB, heatMapRGBA, blended, frameRGBA;
   VideoCapture cap(0, CAP_V4L2); //open camera

//Video frame and capture settings

        int width = RESOLUTION_WIDTH;
        int height = RESOLUTION_HEIGHT;
        double alpha = 0.5; //transparency factor

        cap.set(CAP_PROP_FRAME_WIDTH, width);  //set frame width
        cap.set(CAP_PROP_FRAME_HEIGHT, height); //set frame height
        cap.set(CAP_PROP_FPS, FRAME_RATE);

        Mat frame;
//Heat Map settings

        int mangnitudeWidth = (MAX_ANGLE - MIN_ANGLE)/ANGLE_STEP;
        int magnitudeHeight = (MAX_ANGLE - MIN_ANGLE)/ANGLE_STEP;
        double thresholdValue = 100;
        double thresholdPeak = 255;


        Mat mangitudeFrame(magnitudeHeight, mangnitudeWidth, CV_8UC1, Scalar(0)); //single channel, magnitude matrix, initialized to 0

        //int testcycle = 0;
    
//setup recording if enabled
    VideoWriter vide0;
    Mat testframe;
    cap >> testframe;
    int codec = VideoWriter::fourcc('M', 'J', 'P', 'G');
    string videoFileName = "./testoutput.avi";
    
    if (recording == 1) {
    vide0.open(videoFileName, codec, FRAME_RATE, testframe.size(), 1);
        if (!vide0.isOpened()) {
        cerr << "Could not open the output video file for write\n";
        return -1;
    }
    }

    if (recording == 0) {
        vide0.release();
    }


    
    
    
    
    
    while (true) {
       
       
        cap >> frame; //capture the frame
        while (frame.empty()) {
            cap >> frame;
        }

        
        heatMapData = Mat(frame.size(), CV_32FC1); //make heat map data matrix
        //randu(heatMapData, Scalar(0), Scalar(255)); 
        //generate random heat map data every 10th frame
        //if (testcycle == 0 ) {
            randu(mangitudeFrame, Scalar(0), Scalar(255)); //random input of magnitude data  
        //}
        ////++testcycle;
        //if (testcycle == 10) {
           // testcycle = 0;
            //thresholdValue = thresholdValue + 1;
        //} 
       
        

        //scaling and interpolating
        resize(mangitudeFrame, heatMapData, Size(width, height), 0, 0, INTER_LINEAR);

        //apply threshold
        threshold(heatMapData, heatMapData, thresholdValue, thresholdPeak, THRESH_TOZERO);



        //normalize and convert to 8bit
        normalize(heatMapData, heatMapDataNormal, 0, 255, NORM_MINMAX);
        heatMapData.convertTo(heatMapData, CV_8UC1);
        heatMapDataNormal.convertTo(heatMapDataNormal, CV_8UC1);

        //convert to color map
        applyColorMap(heatMapDataNormal, heatMapRGB, COLORMAP_INFERNO);

        //convert colormap from rgb to rgba
        cvtColor(heatMapRGB, heatMapRGBA, COLOR_RGB2RGBA);
        cvtColor(frame, frameRGBA, COLOR_RGB2RGBA);

        //scary nested for loops to iterate through each value and set to clear if 0
        for (int y = 0; y < heatMapData.rows; ++y) {
            
            for (int x = 0; x < heatMapData.cols; ++x) {
            
                if (heatMapData.at<uchar>(y, x) == 0) {

                    heatMapRGBA.at<Vec4b>(y, x)[4] = 0; //set alpha channel 0
            
            }
        }
    }


        // combined color map and original frame
        addWeighted(heatMapRGBA, alpha, frameRGBA, 1.0 - alpha, 0.0, blended);

        //display the merged frame
        imshow("Heat Map Overlay", blended);

        //record if set to record
        if (recording == 1) {
        cvtColor(blended, blended, COLOR_RGBA2RGB);
           vide0.write(blended);

        }
        
        //if (getWindowProperty("Heat Map Overlay", WND_PROP_VISIBLE) < 1) {
            // Exit the loop if the window is closed
          //  break;

        //}


        //break loop if key is pressed
        if (waitKey(30) >= 0) break;

    }

    cap.release();
    vide0.release();
    //destroyAllWindows(); 
    return 0;
}
