#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "opencv/cv.h"
#include "opencv/highgui.h"

using namespace std;
using namespace cv;

void help(){
    printf("ihmontage - brings multiple videos of the same game into a single movie.\n");
    printf("usage:\tihmontage [options] <1st video> <2nd video> <output path>\n");
    printf("options: \n");
    printf("\t--help\tdisplay this help\n");
    printf("\t--verbose\toutput more information during video processing\n");
    printf("\t-w\twidth of the output video\n");
    printf("\t-h\theight of the output video\n");
    printf("\t-c\tcodec of the output video, please find list of possible values here http://www.fourcc.org/codecs.php\n");
}

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

void outputFrame(Mat &frame, VideoWriter &writer, double distance, int width, int height){
    if(!frame.empty()){
        Mat output;
        resize(frame, output, Size(width, height));
        writer << output;
        putText(output, "Distance: " + std::to_string(distance), cvPoint(30,30),
                FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
    //    imshow("edges", output);
    }
}

int main (int argc, char *argv[])
{
    MovingAverage movingAvgA(10), movingAvgB(10);
    static int verboseFlag, helpFlag;
    string pathA, pathB, outputPath;
    int outputWidth = 400, outputHeight = 200, codec = CV_FOURCC('m', 'p', '4', 'v');
    
    while (1)
    {
        int c;
        
        static struct option long_options[] =
        {
            {"verbose", no_argument,       &verboseFlag, 1},
            {"help", no_argument,       &helpFlag, 1},
            {"h",    required_argument,       0, 'h'},
            {"w",    required_argument,       0, 'w'},
            {"c",    required_argument,       0, 'c'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        
        c = getopt_long (argc, argv, "h:w:c:",
                         long_options, &option_index);
        
        if (c == -1)
            break;
        
        switch (c)
        {
            case 0:
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;
                
            case 'h':
                outputHeight = stoi(optarg);
                break;
                
            case 'w':
                outputWidth = stoi(optarg);
                break;
                
            case 'c':
                if(strnlen(optarg, 4) < 4){
                    printf("incorrect codec\n");
                    return -1;
                } else {
                    codec = CV_FOURCC(optarg[0], optarg[1], optarg[2], optarg[3]);
                }
                break;
                
            case '?':
                break;
                
            default:
                abort ();
        }
    }
    
    if(helpFlag){
        help();
        return 0;
    }
    
    if (argc - optind < 3 ) {
        printf("Incorrect parameters\n");
        help();
        return -1;
    } else {
        pathA = argv[optind];
        pathB = argv[optind + 1];
        outputPath = argv[optind + 2];
    }
    
    VideoCapture inputA(pathA);
    if(!inputA.isOpened()){
        cerr  << "Could not open [" << pathA << "]" << endl;
        return -1;
    }
    
    inputA.set(CV_CAP_PROP_POS_MSEC, 2000);
    
    if(verboseFlag) printVideoMetadata(inputA);
    
    VideoCapture inputB(pathB);
    if(!inputB.isOpened()){
        cerr  << "Could not open [" << pathB << "]" << endl;
        return -1;
    }
    
    inputB.set(CV_CAP_PROP_POS_MSEC, 2000);
    
    if(verboseFlag) printVideoMetadata(inputB);
    
    VideoWriter output(outputPath, codec, inputA.get(CV_CAP_PROP_FPS), Size(outputWidth, outputHeight), false);
    if (!output.isOpened()){
        cerr  << "Could not open " << outputPath << " for write" << endl;
        return -1;
    }
    
    Mat edges;
    namedWindow("edges", CV_WINDOW_NORMAL);
    resizeWindow("edges", outputWidth, outputHeight);
    
    int framesA = inputA.get(CV_CAP_PROP_FRAME_COUNT) - inputA.get(CV_CAP_PROP_POS_FRAMES);
    int framesB = inputB.get(CV_CAP_PROP_FRAME_COUNT) - inputB.get(CV_CAP_PROP_POS_FRAMES);
    
    bool hasMoreA = true, hasMoreB = true;
    
    do {

        Mat frameA, frameB;
        
        if(hasMoreA) hasMoreA = inputA.read(frameA);
        if(hasMoreB) hasMoreB = inputB.read(frameB);
        
        double distanceA = (hasMoreA)? movingAvgA.nextFrame(frameA) : 0;
        double distanceB = (hasMoreB)? movingAvgB.nextFrame(frameB) : 0;
        
        if(distanceA > distanceB){
            outputFrame(frameA, output, distanceA, outputWidth, outputHeight);
        } else {
            outputFrame(frameB, output, distanceB, outputWidth, outputHeight);
        }
        
        if(waitKey(1) >= 0) break;
        
    } while(hasMoreA || hasMoreB);

    output.release();
    
    // Exit
    return 0;
}