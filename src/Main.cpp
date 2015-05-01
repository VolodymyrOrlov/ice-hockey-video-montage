#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <stdexcept>

#include "opencv/cv.h"
#include "opencv/highgui.h"

using namespace std;
using namespace cv;

void help(){
    printf("ihmontage - brings multiple videos of the same game into a single movie.\n");
    printf("usage:\tihmontage [options] <1st video>:<start_second> <2nd video>:<start_second> <output path>\n");
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

void outputFrame(Mat &frame, VideoWriter &writer, double distance, int width, int height, bool verbose){
    if(!frame.empty()){
        Mat output;
        resize(frame, output, Size(width, height));
        writer << output;
        putText(output, "Distance: " + std::to_string(distance), cvPoint(30,30),
                FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
        if(verbose) imshow("edges", output);
    }
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

std::tuple<std::string, int> parseInputPath(const std::string &line){
    std::vector<std::string> pair = split(line, ':');
    if(pair.size() < 2){
        return std::make_tuple(line, 0);
    } else {
        return std::make_tuple(pair[0], std::stoi(pair[1]));
    }
}

int main (int argc, char *argv[])
{
    MovingAverage movingAvgA(10), movingAvgB(10);
    static int verboseFlag, helpFlag;
    std::tuple<std::string, int> pathA, pathB;
    std::string outputPath;
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
        pathA = parseInputPath(argv[optind]);
        pathB = parseInputPath(argv[optind + 1]);
        outputPath = argv[optind + 2];
    }
    
    VideoCapture inputA(std::get<0>(pathA));
    if(!inputA.isOpened()){
        cerr  << "Could not open [" << std::get<0>(pathA) << "]" << endl;
        return -1;
    }
    
    inputA.set(CV_CAP_PROP_POS_MSEC, std::get<1>(pathA) * 1000);
    
    if(verboseFlag) printVideoMetadata(inputA);
    
    VideoCapture inputB(std::get<0>(pathB));
    if(!inputB.isOpened()){
        cerr  << "Could not open [" << std::get<0>(pathB) << "]" << endl;
        return -1;
    }
    
    inputB.set(CV_CAP_PROP_POS_MSEC, std::get<1>(pathB) * 1000);
    
    if(verboseFlag) printVideoMetadata(inputB);
    
    VideoWriter output(outputPath, codec, inputA.get(CV_CAP_PROP_FPS), Size(outputWidth, outputHeight), false);
    if (!output.isOpened()){
        cerr  << "Could not open " << outputPath << " for write" << endl;
        return -1;
    }
    
    Mat edges;
    namedWindow("edges", CV_WINDOW_NORMAL);
    resizeWindow("edges", outputWidth, outputHeight);
    
    bool hasMoreA = true, hasMoreB = true;
    
    int latestViewNumber = -1;
    
    do {

        Mat frameA, frameB;
        
        if(hasMoreA) hasMoreA = inputA.read(frameA);
        if(hasMoreB) hasMoreB = inputB.read(frameB);
        
        
        double distanceA = (hasMoreA)? movingAvgA.nextFrame(frameA) : 0;
        double distanceB = (hasMoreB)? movingAvgB.nextFrame(frameB) : 0;
        
        int currentViewNumber = (distanceA > distanceB)? 0 : 1;
        
        if(currentViewNumber != latestViewNumber && verboseFlag) {
            printf("Switched to view %d at %d second\n", currentViewNumber, (int)(currentViewNumber == 0 ? inputA : inputB).get(CV_CAP_PROP_POS_MSEC));
            latestViewNumber = currentViewNumber;
        }
        
        if(distanceA > distanceB){
            outputFrame(frameA, output, distanceA, outputWidth, outputHeight, verboseFlag);
        } else {
            outputFrame(frameB, output, distanceB, outputWidth, outputHeight, verboseFlag);
        }
        
        if(waitKey(1) >= 0) break;
        
    } while(hasMoreA || hasMoreB);

    output.release();
    
    return 0;
}