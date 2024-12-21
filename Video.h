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

    void processFrame(Mat& data_input, const float lower_limit, const float upper_limit);

private:
    bool getFrame(Mat& frame);  // Non-blocking frame retrieval
    
    void captureVideo(VideoCapture& cap, atomic<bool>& is_running);

    Mat drawUI();

    Mat createHeatmap(Mat& data_input, const float lower_limit, const float upper_limit);

    Mat mergeHeatmap(Mat& frame, Mat& heatmap, double alpha);
    
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

Mat video::createHeatmap(Mat& data_input, const float lower_limit, const float upper_limit)
{
    // Ensure data_input is of type CV_32F for proper normalization
    // Mat data_clamped;
    // data_input.convertTo(data_clamped, CV_32F);

    // min(data_clamped, lower_limit);
    // max(data_clamped, upper_limit);

    // Normalizes data to be 0-255
    Mat data_normalized;
    data_input.convertTo(data_normalized, CV_32F);

    // Normalize the input to the range 0-255 (required for color mapping)
    normalize(data_normalized, data_normalized, 0, 255, NORM_MINMAX);

    // Convert to 8-bit format (required for applying color map)
    Mat heatmap_input;
    data_normalized.convertTo(heatmap_input, CV_8U);

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

void video::processFrame(Mat& data_input, const float lower_limit, const float upper_limit)
{
    /*
    - convert input_data to correct range
    - turn input_data into a heatmap
    - merge frame and input_data
    */
    Mat frame;
    if (getFrame(frame)) 
    {
        // Creates heatmap from beamformed audio data
        Mat heatmap = createHeatmap(data_input, lower_limit, upper_limit);

        // Merge frame with heatmap
        Mat frame_merged = mergeHeatmap(frame, heatmap, 0.8f);

        imshow("Window", frame_merged);
    }
} // end processData

//=====================================================================================