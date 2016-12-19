/* 
 * Struck: Structured Output Tracking with Kernels
 * 
 * Code to accompany the paper:
 *   Struck: Structured Output Tracking with Kernels
 *   Sam Hare, Amir Saffari, Philip H. S. Torr
 *   International Conference on Computer Vision (ICCV), 2011
 * 
 * Copyright (C) 2011 Sam Hare, Oxford Brookes University, Oxford, UK
 * 
 * This file is part of Struck.
 * 
 * Struck is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Struck is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Struck.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */
 
#include "Tracker.h"
#include "Config.h"

#include <iostream>
#include <fstream>

#include <opencv/cv.h>
#include <opencv/highgui.h>

//SIMON MÖRDERDEBUG
//#include <cstdlib>

using namespace std;
using namespace cv;

static const int kLiveBoxWidth = 80;
static const int kLiveBoxHeight = 80;

void rectangle(Mat& rMat, const FloatRect& rRect, const Scalar& rColour)
{
	IntRect r(rRect);
	rectangle(rMat, Point(r.XMin(), r.YMin()), Point(r.XMax(), r.YMax()), rColour);
}

int main(int argc, char* argv[])
{
    //SIMONS killerdebug
    //system("pwd");
    //return 0;
    
	// read config file
	string configPath = "config.txt";
	if (argc > 1)
	{
		configPath = argv[1];
	}
	Config conf(configPath);
	cout << conf << endl;
	
	if (conf.features.size() == 0)
	{
		cout << "error: no features specified in config" << endl;
		return EXIT_FAILURE;
	}
	
	ofstream outFile;
	if (conf.resultsPath != "")
	{
		outFile.open(conf.resultsPath.c_str(), ios::out);
		if (!outFile)
		{
			cout << "error: could not open results file: " << conf.resultsPath << endl;
			return EXIT_FAILURE;
		}
	}
	
	// if no sequence specified then use the camera
	bool useCamera = (conf.sequenceName == "");
	
	VideoCapture cap;
	
	int startFrame = -1;
	int endFrame = -1;
	FloatRect initBB;
	string imgFormat;
	float scaleW = 1.f;
	float scaleH = 1.f;
    vector<string> inputFiles;
	bool isPrinceton=false;
	if (useCamera)
	{
		if (!cap.open(0))
		{
			cout << "error: could not start camera capture" << endl;
			return EXIT_FAILURE;
		}
		startFrame = 0;
		endFrame = INT_MAX;
		Mat tmp;
		cap >> tmp;
		scaleW = (float)conf.frameWidth/tmp.cols;
		scaleH = (float)conf.frameHeight/tmp.rows;

		initBB = IntRect(conf.frameWidth/2-kLiveBoxWidth/2, conf.frameHeight/2-kLiveBoxHeight/2, kLiveBoxWidth, kLiveBoxHeight);
		cout << "press 'i' to initialise tracker" << endl;
	}
	else
	{
		// parse frames file
		string framesFilePath = conf.sequenceBasePath+"/"+conf.sequenceName+"/"+conf.sequenceName+"_frames.txt";
		ifstream framesFile(framesFilePath.c_str(), ios::in);
        
		if (!framesFile)
		{
			cout << "Could not open sequence frames file: " << framesFilePath << endl <<
                    "Trying out if dataset is in princeton format" << endl;
            if(conf.features[0].feature == 3){
                framesFilePath = conf.sequenceBasePath+ "/"+conf.sequenceName+"/depthList.txt";
                //ifstream file(framesFilePath, ios::in);
                framesFile.open(framesFilePath,ios::in);
            }else{
                framesFilePath = conf.sequenceBasePath+ "/"+conf.sequenceName+"/rgbList.txt";//DEBUG rgbList.txt
                //ifstream file(framesFilePath, ios::in);
                framesFile.open(framesFilePath,ios::in);
            }
            if(!framesFile){
                cout << "error: could not open sequence frames file: " << framesFilePath << endl;
                return EXIT_FAILURE;
            }
            isPrinceton=true;
		}
        //TODO: Simon here might be a good point to put in the other way to read the files
        if(!isPrinceton){
            string framesLine;
            getline(framesFile, framesLine);
            sscanf(framesLine.c_str(), "%d,%d", &startFrame, &endFrame);
            if (framesFile.fail() || startFrame == -1 || endFrame == -1)
            {
                cout << "error: could not parse sequence frames file" << endl;
                return EXIT_FAILURE;
            }

            if (conf.features[0].feature == 3) // 3 is LBP
            {
                cout << "LBP selected. We need to load depth data instead of RGB" << endl;
                imgFormat = conf.sequenceBasePath+"/"+conf.sequenceName+"/depth/img%05d.png";
            }
            else
            {
                imgFormat = conf.sequenceBasePath+"/"+conf.sequenceName+"/imgs/img%05d.png";
            }

            // read first frame to get size
            char imgPath[256];
            sprintf(imgPath, imgFormat.c_str(), startFrame);
            Mat tmp = cv::imread(imgPath, 0);
            scaleW = (float)conf.frameWidth/tmp.cols;
            scaleH = (float)conf.frameHeight/tmp.rows;
            
            // read init box from ground truth file
            string gtFilePath = conf.sequenceBasePath+"/"+conf.sequenceName+"/"+conf.sequenceName+"_gt.txt";
            ifstream gtFile(gtFilePath.c_str(), ios::in);
            if (!gtFile)
            {
                cout << "error: could not open sequence gt file: " << gtFilePath << endl;
                return EXIT_FAILURE;
            }
            string gtLine;
            getline(gtFile, gtLine);
            float xmin = -1.f;
            float ymin = -1.f;
            float width = -1.f;
            float height = -1.f;
            sscanf(gtLine.c_str(), "%f,%f,%f,%f", &xmin, &ymin, &width, &height);
            if (gtFile.fail() || xmin < 0.f || ymin < 0.f || width < 0.f || height < 0.f)
            {
                cout << "error: could not parse sequence gt file" << endl;
                return EXIT_FAILURE;
            }
            initBB = FloatRect(xmin*scaleW, ymin*scaleH, width*scaleW, height*scaleH);
        }else{
            string firstFrame;
            getline(framesFile,firstFrame);
            string filePath;
            if ( conf.features[0].feature == 3){
                filePath = conf.sequenceBasePath+"/"+conf.sequenceName+"/depth/"+firstFrame;
            }else{
                filePath = conf.sequenceBasePath+"/"+conf.sequenceName+"/rgb/"+firstFrame;//DEBUG: rgb
            }
            startFrame=0;
            inputFiles.push_back(firstFrame);
            endFrame=1;
            while(getline(framesFile,firstFrame)){
                if(firstFrame!=""){
                    endFrame++;
                    inputFiles.push_back(firstFrame);
                }
            }
            Mat tmp = imread(filePath,CV_LOAD_IMAGE_UNCHANGED); //some bytes have to be changed (otherwise they are in mm)
            //"depth image folder : images are in 16 bit png format, with the first 3 bit swap to the last (for visilization purpose Users need to swap them back after reading the image. Values at each pixel are the distance from Kinect to the object in mm."
            scaleW = (float)conf.frameWidth/tmp.cols;
            scaleH = (float)conf.frameHeight/tmp.rows;
            
            //TODO: from here on
            string gtFilePath = conf.sequenceBasePath+"/"+conf.sequenceName+"/"+conf.sequenceName+".txt";
            ifstream gtFile(gtFilePath.c_str(), ios::in);
            if (!gtFile)
            {
                cout << "error: could not open sequence gt file: " << gtFilePath << endl;
                return EXIT_FAILURE;
            }
            string gtLine;
            getline(gtFile, gtLine);
            float xmin = -1.f;
            float ymin = -1.f;
            float width = -1.f;
            float height = -1.f;
            sscanf(gtLine.c_str(), "%f,%f,%f,%f", &xmin, &ymin, &width, &height);
            if(isPrinceton)
            {
                xmin--;
                ymin--;
            }
            if (gtFile.fail() || xmin < 0.f || ymin < 0.f || width < 0.f || height < 0.f)
            {
                cout << "error: could not parse sequence gt file" << endl;
                return EXIT_FAILURE;
            }
            initBB = FloatRect(xmin*scaleW, ymin*scaleH, width*scaleW, height*scaleH);
            
        }
	}
	
	
	
	Tracker tracker(conf);
	if (!conf.quietMode)
	{
		namedWindow("result");
	}
	
	Mat result(conf.frameHeight, conf.frameWidth, CV_8UC3);
	bool paused = false;
	bool doInitialise = false;
	srand(conf.seed);
	for (int frameInd = startFrame; frameInd <= endFrame; ++frameInd)
	{
		Mat frame;
		if (useCamera)
		{
			Mat frameOrig;
			cap >> frameOrig;
			resize(frameOrig, frame, Size(conf.frameWidth, conf.frameHeight));
			flip(frame, frame, 1);
			frame.copyTo(result);
			if (doInitialise)
			{
				if (tracker.IsInitialised())
				{
					tracker.Reset();
				}
				else
				{
					tracker.Initialise(frame, initBB);
				}
				doInitialise = false;
			}
			else if (!tracker.IsInitialised())
			{
				rectangle(result, initBB, CV_RGB(255, 255, 255));
			}
		}
		else
		{
            Mat frameOrig;
            if (isPrinceton){
                
                string imgPath;
                if ( conf.features[0].feature == 3){
                    
                    //read it unchanged: otherwise the depth images get
                    imgPath = conf.sequenceBasePath+"/"+conf.sequenceName+"/depth/"+inputFiles[frameInd];
                    frameOrig = cv::imread(imgPath);//, CV_LOAD_IMAGE_UNCHANGED);
                }else{
                    imgPath = conf.sequenceBasePath+"/"+conf.sequenceName+"/rgb/"+inputFiles[frameInd];//DEBUG: rgb
                    frameOrig = cv::imread(imgPath);
                }
                if (frameOrig.empty())
                {
                    cout << "error: could not read frame: " << imgPath << endl;
                    return EXIT_FAILURE;
                }
            }else{
                char imgPath[256];
                sprintf(imgPath, imgFormat.c_str(), frameInd);
                frameOrig = cv::imread(imgPath, 0);
                if (frameOrig.empty())
                {
                    cout << "error: could not read frame: " << imgPath << endl;
                    return EXIT_FAILURE;
                }
            }
			resize(frameOrig, frame, Size(conf.frameWidth, conf.frameHeight));

            
            if(frame.channels()==1){
                cvtColor(frame, result, CV_GRAY2RGB);
            }else{
                if(frame.channels()==3){
                    result = frame.clone();
                   
                    
                }else{
                    cout << "error: image has unknown format" << endl;
                }
            }
            if ( conf.features[0].feature == 3){
                //get the bits into correct order.
                for(int i=0;i<frame.cols*frame.rows;i++){
                    uint16_t pixel=frame.at<uint16_t>(i);
                    //frame.at<uint16_t>(i)=pixel>>3 | pixel<<13;
                }
                //imshow("frame",frame);
            }
            
			if (frameInd == startFrame)
			{
				tracker.Initialise(frame, initBB);
			}
		}
		
		if (tracker.IsInitialised())
		{
			tracker.Track(frame);
			
			if (!conf.quietMode && conf.debugMode)
			{
				tracker.Debug();
			}
			
			rectangle(result, tracker.GetBB(), CV_RGB(0, 255, 0));
			
			if (outFile)
			{
				const FloatRect& bb = tracker.GetBB();
				outFile << bb.XMin()/scaleW << "," << bb.YMin()/scaleH << "," << bb.Width()/scaleW << "," << bb.Height()/scaleH << endl;
			}
		}
		
		if (!conf.quietMode)
		{
			imshow("result", result);
			int key = waitKey(paused ? 0 : 1);
			if (key != -1)
			{
				if (key == 27 || key == 113) // esc q
				{
					break;
				}
				else if (key == 112) // p
				{
					paused = !paused;
				}
				else if (key == 105 && useCamera)
				{
					doInitialise = true;
				}
			}
			if (conf.debugMode && frameInd == endFrame)
			{
				cout << "\n\nend of sequence, press any key to exit" << endl;
				waitKey();
			}
		}
	}
	
	if (outFile.is_open())
	{
		outFile.close();
	}
	
	return EXIT_SUCCESS;
}
