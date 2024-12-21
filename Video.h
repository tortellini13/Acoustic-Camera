#include <opencv2/opencv.hpp>
#include <atomic>
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>

using namespace std;
using namespace cv;
/*
    - initialize class
    - startCapture()
    - processData() in loop with audio
    - exit key in loop to break
    - stopCapture() outside loop
*/
class video 
{
public:
    video(int frame_width, int frame_height, int frame_rate);

    ~video();

    void startCapture();

    void stopCapture();

    void processFrame(Mat& data_input);

private:
    bool getFrame(Mat& frame);  // Non-blocking frame retrieval
    
    void captureVideo(VideoCapture& cap, atomic<bool>& is_running);

    Mat drawUI();

    Mat createHeatmap(Mat& data_input);

    Mat mergeHeatmap(Mat& frame, Mat& heatmap, double alpha);

    Mat drawColorBar(int scale_width, int scale_height);

    Mat drawUI(Mat& data_input);
    
    // Camera params
    int frame_width;
    int frame_height;
    int frame_rate;

    VideoCapture cap;

    // For multithreading
    atomic<bool> is_running;
    thread video_thread;

    // Frame queue for inter-thread communication
    queue<Mat> frame_queue;
    mutex frame_mutex;  // Protects access to the frame queue

    // Variables for buttons
    int list_max_mag_state = 1;
    int mark_max_mag_state = 1;
    int color_scale_state = 1;
    int heat_map_state = 1;
    int FPS_count_state = 1;
    // Button callbacks
    void onListMaxMag(int state, void* user_data);
    void onMarkMaxMag(int state, void* user_data);
    void onColorScaleState(int state, void* user_data);
    void onHeatMap(int state, void* user_data);
    void onFPSCount(int state, void* user_data);
    void UISetup();

    // For magnitude proccessing
    // Min and max magnitude
    double magnitude_min;
    double magnitude_max;
    // Coords for max magnitude
    Point max_coord;        // Coords from data
    Point max_point_scaled; // Coords scaled to frame

    //Color bar
    Mat static_color_bar;
    

};

//=====================================================================================

video::video(int frame_width, int frame_height, int frame_rate) :
    frame_width(frame_width),
    frame_height(frame_height),
    frame_rate(frame_rate),
    cap(0, CAP_V4L2) {}

video::~video() 
{
    stopCapture();
} // end ~video

//=====================================================================================

void video::startCapture() 
{
    // Configure capture properties
    cap.set(CAP_PROP_FRAME_WIDTH, frame_width);
    cap.set(CAP_PROP_FRAME_HEIGHT, frame_height);
    cap.set(CAP_PROP_FPS, frame_rate);
    cap.set(CAP_PROP_BUFFERSIZE, 1);

    // Record video on separate thread
    is_running = true;
    video_thread = thread(&video::captureVideo, this, ref(cap), ref(is_running));

    UISetup();
} // end startCapture

void video::stopCapture() 
{
    is_running = false;
    if (video_thread.joinable()) 
    {
        video_thread.join();
    }
} // end stopCapture

//=====================================================================================

// Callbacks for UI Buttons

void video::onListMaxMag(int state, void* user_data) 
{
    list_max_mag_state = state;
}

void video::onMarkMaxMag(int state, void* user_data) 
{
    mark_max_mag_state = state;
}

void video::onColorScaleState(int state, void* user_data) 
{
    color_scale_state = state;
}

void video::onHeatMap(int state, void* user_data) 
{
    heat_map_state = state;
}

void video::onFPSCount(int state, void* user_data) 
{
    FPS_count_state = state;
}

//=====================================================================================

// Setup UI Buttons and trackbars

void video::UISetup() 
{   
    // Make a window for the buttons to go on
    static_color_bar = drawColorBar(SCALE_WIDTH, SCALE_HEIGHT);

    Mat initialFrame(Size(RESOLUTION_HEIGHT, RESOLUTION_WIDTH), CV_32FC1);

    while (!getFrame(initialFrame)) 
    {
        cerr << "Error: Could not retreive frame from capture thread!" << endl;
    }

    imshow("Window",initialFrame); 

    createTrackbar("Threshold", "Window", nullptr, MAP_THRESHOLD_MAX);

    createTrackbar("Alpha", "Window", nullptr, 100);

    setTrackbarPos("Threshold", "Window", DEFAULT_THRESHOLD);
    
    setTrackbarPos("Alpha", "Window", DEFAULT_ALPHA);

    // Maximum magnitude button
    createButton("List Maximum Magnitude", 
        // Lambda function to call onListMaxMag
        [](int state, void* user_data) 
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onListMaxMag(state, user_data);
        }, 
        this, QT_CHECKBOX, 1);
    
    // Mark maximum magnitude button
    createButton("Mark Maximum Magnitude", 
        [](int state, void* user_data)
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onMarkMaxMag(state, user_data);
        },
        this, QT_CHECKBOX, 1);

    // Show color scale button
    createButton("Show Color Scale",
        [](int state, void* user_data)
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onColorScaleState(state, user_data);
        },
        this, QT_CHECKBOX, 1);

    // Show heat map button
    createButton("Show Heat Map",
        [](int state, void* user_data)
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onHeatMap(state, user_data);
        },
        this, QT_CHECKBOX, 1);

    // Show fps button
    createButton("Show fps",
        [](int state, void* user_data)
        {
            auto* vid = static_cast<video*>(user_data);
            vid->onFPSCount(state, user_data);
        }, 
        this, QT_CHECKBOX, 1);
    
}

//=====================================================================================

void video::captureVideo(VideoCapture& cap, atomic<bool>& is_running) 
{
    while (is_running) 
    {
        Mat temp_frame;
        if (cap.read(temp_frame)) 
        {
            // Protect access to the frame queue with a mutex
            {
                lock_guard<mutex> lock(frame_mutex);
                frame_queue.push(temp_frame.clone());  // Store a copy of the frame in the queue
            }
        }
        else 
        {
            cerr << "Error: Could not capture frame from video source!" << endl;
            is_running = false;
        }
    }
    destroyAllWindows();
} // end videoCapture

//=====================================================================================

bool video::getFrame(Mat& frame) 
{
    // Try to fetch a frame without blocking
    lock_guard<mutex> lock(frame_mutex);
    if (!frame_queue.empty()) 
    {
        frame = frame_queue.front(); // Retrieve the frame
        frame_queue.pop();           // Remove the frame from the queue
        return true;
    }
    return false; // No frame available
} // end getFrame

//=====================================================================================

Mat video::createHeatmap(Mat& data_input)
{
    // Ensure data_input is of type CV_32F for proper normalization
    Mat normalized_input;
    data_input.convertTo(normalized_input, CV_32F);

     // Find coord of max and min magnitude
    minMaxLoc(data_input, &magnitude_min, &magnitude_max, NULL, &max_coord);
    
    // Scale max point to video resolution
    max_point_scaled.x = (static_cast<double>(max_coord.x) / static_cast<double>(normalized_input.cols)) * RESOLUTION_WIDTH; 
    max_point_scaled.y = (static_cast<double>(max_coord.y) / static_cast<double>(normalized_input.rows)) * RESOLUTION_HEIGHT;


    // Normalize the input to the range 0-255 (required for color mapping)
    normalize(normalized_input, normalized_input, 0, 255, NORM_MINMAX);

    // Convert to 8-bit format (required for applying color map)
    Mat heatmap_input;
    normalized_input.convertTo(heatmap_input, CV_8U);

    // Apply a colormap to create the heatmap
    Mat heatmap;
    applyColorMap(heatmap_input, heatmap, COLORMAP_INFERNO);

    return heatmap; // Return the generated heatmap
} // end createHeatmap

//=====================================================================================

Mat video::mergeHeatmap(Mat& frame, Mat& heatmap, double alpha)
{
    // Ensure the heatmap is the same size as the frame
    Mat resized_heatmap;
    resize(heatmap, resized_heatmap, frame.size());
    

    // Blend the heatmap and the frame with an alpha value
    Mat frame_merged;
    addWeighted(frame, 1.0, resized_heatmap, alpha, 0.0, frame_merged);

    return frame_merged; // Return the merged frame
} // end mergeHeatmap

//=====================================================================================

Mat video::drawColorBar(int scale_width, int scale_height) 
{
    Mat color_bar(scale_width, scale_height, CV_8UC3);
    Mat scaleColor(scale_width, scale_height, CV_8UC1, Scalar(0));

    for (int y = 0; y < scale_height; y++) 
    {
        int scaleIntensity = 255 * (static_cast<double>((scale_height- y)) / static_cast<double>(scale_height));
        for(int x = 0; x < scale_width; x++) 
        {
            //scaleColor.at<uchar>(y, x) = scaleIntensity;
        }
    }
    
    applyColorMap(scaleColor, color_bar, COLORMAP_INFERNO);
    return color_bar;
}


Mat video::drawUI(Mat& data_input)
{
    //Color Bar Scale

    if (color_scale_state == 1) 
    {   
        Rect color_bar_location(SCALE_POS_X, SCALE_POS_Y, SCALE_WIDTH, SCALE_HEIGHT);
        rectangle(data_input, Point(SCALE_POS_X, SCALE_POS_Y) + Point(-SCALE_BORDER, -SCALE_BORDER - 10), Point(SCALE_POS_X, SCALE_POS_Y) + Point(SCALE_WIDTH + SCALE_BORDER - 1, SCALE_HEIGHT + SCALE_BORDER + 5), Scalar(0, 0, 0), FILLED); //Draw rectangle behind scale to make a border
        
        //static_color_bar.copyTo(data_input(color_bar_location)); //Copy the scale onto the image

         //Draw text indicating various points on the scale
        float scaleTextRatio = (1 / static_cast<float>(SCALE_POINTS ));
        for (int i = 0; i < (SCALE_POINTS + 1); i++) 
        {
            Point scaleTextStart(SCALE_POS_X, (SCALE_POS_Y + ((1 - static_cast<double>(i) * scaleTextRatio) * SCALE_HEIGHT) - 3)); //Starting point for the text
            double scaleTextValue = ((static_cast<double>(magnitude_max - magnitude_min) * scaleTextRatio * static_cast<double>(i)) + magnitude_min); //Value of text for each point

            ostringstream scaleTextStream;
            scaleTextStream << fixed << setprecision(LABEL_PRECISION) << scaleTextValue;
            String scaleTextString = scaleTextStream.str() + " ";

            putText(data_input, scaleTextString, scaleTextStart, FONT_TYPE, FONT_SCALE - 0.2, Scalar(255, 255, 255), FONT_THICKNESS);
            line(data_input, scaleTextStart + Point(0,3), scaleTextStart + Point(SCALE_WIDTH, 3), Scalar(255, 255, 255), 1, 8, 0);
        }
    }
    
    
    // Mark maximum location
    if (mark_max_mag_state == 1) 
    {
        drawMarker(data_input, max_point_scaled, Scalar(255, 255, 255), MARKER_CROSS, CROSS_SIZE, CROSS_THICKNESS, 8); //Mark the maximum magnitude point
    }
    

    
    if (list_max_mag_state == 1)
    {
        Point max_text_location(MAX_LABEL_POS_X, MAX_LABEL_POS_Y);
        ostringstream max_magnitude_stream;
        String max_magnitude_string;
        int text_baseline = 0;
    
        max_magnitude_stream << fixed << setprecision(LABEL_PRECISION) << magnitude_max;
        max_magnitude_string = "Maximum = " + max_magnitude_stream.str();

        Size textSize = getTextSize(max_magnitude_string, FONT_TYPE, FONT_SCALE, FONT_THICKNESS, &text_baseline);
        
        rectangle(data_input, max_text_location + Point(0, text_baseline), max_text_location + Point(textSize.width, -textSize.height), Scalar(0, 0, 0), FILLED); //Draw rectangle for text
        putText(data_input, max_magnitude_string, max_text_location + Point(0, +5), FONT_TYPE, FONT_SCALE, Scalar(255, 255, 255), FONT_THICKNESS); //Write text for maximum magnitude
    }
    



return data_input;

}

//=====================================================================================

void video::processFrame(Mat& data_input)
{
    /*
    - convert input_data to correct range
    - turn input_data into a heatmap
    - merge frame and input_data
    - draw UI
    */
    Mat frame;
    if (getFrame(frame)) 
    {
        // Creates heatmap from beamformed audio data
        Mat heatmap = createHeatmap(data_input);
        
        // Merge frame with heatmap
        Mat frame_merged = mergeHeatmap(frame, heatmap, 0.8f);
        
        Mat frame_merged_UI = drawUI(frame_merged);
        
        imshow("Window", frame_merged_UI);
    }
} // end processData

//=====================================================================================