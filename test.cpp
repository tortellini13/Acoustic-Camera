#include <opencv2/opencv.hpp>

int main() {
    cv::VideoCapture cap(0); // 0 is typically for the default camera

    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera" << std::endl;
        return -1;
    }

    cv::Mat frame;
    while (true) {
        cap >> frame;

        if (frame.empty()) {
            break;
        }

        // Process the frame
        cv::imshow("Frame", frame);

        if (cv::waitKey(10) >= 0) {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();

    return 0;
}