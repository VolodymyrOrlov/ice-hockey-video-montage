#include <iostream>
#include <algorithm>

#include "opencv/cv.h"
#include "opencv/highgui.h"

using namespace std;
using namespace cv;

class MovingAverage {
    Mat previousFrame;
    double avgDistance;
    int movingAvgWindow;
    int currentWindow;
    
    double getAvg(double prev_avg, int x, int n){
        return (prev_avg*n + x)/(n+1);
    }
    
public:
    MovingAverage(int window){
        this->movingAvgWindow = window;
        this->currentWindow = 0;
    }
    
    double nextFrame(Mat &frame, int movingAvgWindow = 10){
        Mat processedFrame;
        
        cvtColor(frame, processedFrame, CV_BGR2GRAY);
        resize(processedFrame, processedFrame, Size(400, 200));
        
        if(!previousFrame.empty() && !processedFrame.empty()){
            double distance = norm(previousFrame, processedFrame, NORM_L2);
            if(avgDistance == 0 ) avgDistance = distance;
            avgDistance = getAvg(avgDistance, distance, movingAvgWindow);
        }
        
        processedFrame.copyTo(previousFrame);
        
        return avgDistance;
    }
    
};

void printVideoMetadata(VideoCapture input){
    cout << "Width [" << input.get(CV_CAP_PROP_FRAME_WIDTH) << "], Height [" << input.get(CV_CAP_PROP_FRAME_HEIGHT)
    << "], Codec [" << input.get(CV_CAP_PROP_FOURCC) << "], # of frames [" << input.get(CV_CAP_PROP_FRAME_COUNT) << "]"<< endl;
}

void outputFrame(Mat &frame, VideoWriter &writer, double distance){
    Mat output;
    resize(frame, output, Size(400, 200));
    writer << output;
    putText(output, "Distance: " + std::to_string(distance), cvPoint(30,30),
            FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
//    imshow("edges", output);
}

int main (int argc, char *argv[])
{
    MovingAverage movingAvgA(10), movingAvgB(10);
    
    VideoCapture inputA(argv[1]);
    if(!inputA.isOpened()){
        cerr  << "Could not open [" << argv[1] << "]" << endl;
        return -1;
    }
    
    printVideoMetadata(inputA);
    
    VideoCapture inputB(argv[2]);
    if(!inputB.isOpened()){
        cerr  << "Could not open [" << argv[2] << "]" << endl;
        return -1;
    }
    
    printVideoMetadata(inputB);
    
    int framesNA = inputA.get(CV_CAP_PROP_FRAME_COUNT), framesNB = inputB.get(CV_CAP_PROP_FRAME_COUNT);
    
    VideoWriter output(argv[3], CV_FOURCC('m', 'p', '4', 'v'), inputA.get(CV_CAP_PROP_FPS), Size(400, 200), false);
    if (!output.isOpened()){
        cerr  << "Could not open " << argv[2] << " for write" << endl;
        return -1;
    }
    
    Mat edges;
    namedWindow("edges", CV_WINDOW_NORMAL);
    resizeWindow("edges", 400, 200);
    for(int i = 0; i < min(framesNA, framesNB); i++)
    {
        Mat frameA, frameB;
        inputA >> frameA;
        inputB >> frameB;

        double distanceA = movingAvgA.nextFrame(frameA);
        double distanceB = movingAvgB.nextFrame(frameB);
        
        if(distanceA > distanceB){
            outputFrame(frameA, output, distanceA);
        } else {
            outputFrame(frameB, output, distanceB);
        }
        
        if(waitKey(1) >= 0) break;
    }

    output.release();
    
    // Exit
    return 0;
}