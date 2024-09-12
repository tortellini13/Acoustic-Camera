#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;



int main() {
  
   Mat frame, heatMapData, heatMap, blended;
   VideoCapture cap(0, CAP_V4L2); //open camera
    
    while (true) {
       
        int width = 640;
        int height = 480;
        double alpha = 0.8; //transparency factor

        //cap.set(CAP_PROP_FRAME_WIDTH, width);  //set frame width
        //cap.set(CAP_PROP_FRAME_HEIGHT, height); //set frame height
        cap.set(CAP_PROP_FPS, 60);
        cap >> frame; //capture the frame
        //while (frame.empty()) {
        //    cap >> frame;
        //}

        //generate random heat map data
        heatMapData = Mat(frame.size(), CV_32FC1); 
        randu(heatMapData, Scalar(0), Scalar(255)); 

        //normalize and convert to 8bit
        normalize(heatMapData, heatMapData, 0, 255, NORM_MINMAX);
        heatMapData.convertTo(heatMapData, CV_8UC1);

        //convert to color map
        applyColorMap(heatMapData, heatMap, COLORMAP_JET);

        // combined color map and original frame
        addWeighted(heatMap, alpha, frame, 1.0 - alpha, 0.0, blended);

        //display the merged frame
        imshow("Heat Map Overlay", blended);

        //break loop if key is pressed
        if (waitKey(30) >= 0) break;
    }

    cap.release();
    //destroyAllWindows(); 
    return 0;
}
