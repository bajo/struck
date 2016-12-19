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

#include "LBPFeatures.h"
#include "Config.h"
#include "Sample.h"

#include <iostream>

#include <opencv/highgui.h>

using namespace std;
using namespace cv;

int factorial(int n)
{
    return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
}

LBPFeatures::LBPFeatures(const Config& conf)
{
    m_HD=3;
    m_VD=4;
    m_cellWidth=3;
    m_cellHeight=3;
    int length =m_HD*m_VD* factorial(m_cellWidth*m_cellHeight)/(2*factorial(m_cellWidth*m_cellHeight-2));
    SetCount(length);
    
}

LBPFeatures::~LBPFeatures(){
    
    
}

void LBPFeatures::UpdateFeatureVector(const Sample& s)
{
    //get frame rect
    //TODO: float readout
    const ImageRep& image = s.GetImage();
    const FloatRect& roi = s.GetROI();
    const Mat &img = image.GetImage();
    cv::Rect rectOfInterest(roi.XMin(),roi.YMin(),roi.Width(),roi.Height());//transform to integer
    Mat moi = img(rectOfInterest);
    int subCellWidth=1;
    while(subCellWidth*m_HD*m_cellWidth<roi.Width()){
        subCellWidth++;
    }
    
    
    int subCellHeight=1;
    while(subCellHeight*m_VD*m_cellHeight<roi.Height()){
        subCellHeight++;
    }
    float scale=1.0f/((float)subCellWidth*(float)subCellHeight);
    
    
    
    
    //scale:
    Mat work;
    resize(moi,work,cv::Size(subCellWidth*m_HD*m_cellWidth,subCellHeight*m_VD*m_cellHeight));
    Mat scaled(m_VD*m_cellHeight,m_HD*m_cellWidth,CV_32FC1);
    scaled.setTo(cv::Scalar(0));
    
    for(int m=0;m<m_VD*m_cellHeight;m++){
        for(int n=0;n<m_HD*m_cellWidth;n++){
            for(int i=0;i<subCellHeight;i++){
                for(int j=0;j<subCellWidth;j++){
                    scaled.at<float>(m,n)+=scale*(float)work.at<uint16_t>(m*subCellHeight+i,n*subCellWidth+j);
                }
            }
        }
    }
    imshow("rectOfInterest",moi*10);
    imshow("rectOfInterestScaled",work*10);
    imshow("rectOfInterestScaledDown",scaled*1.0f/25500.0f);
    waitKey(1);
    int k=0;
    //calc data vector
    for(int m=0;m<m_VD;m++){
        for(int n=0;n<m_HD;n++){
            cv::Rect coi(n*m_cellWidth,m*m_cellHeight,m_cellWidth,m_cellHeight);//cell of interest
            Mat cell=scaled(coi);
            for(int i=0;i<m_cellHeight*m_cellWidth;i++){
                for(int j=i+1;j<m_cellHeight*m_cellWidth;j++){
                    float data=fabs((float)cell.at<float>(i)-(float)cell.at<float>(j));
                    Features::m_featVec[k]=data;
                    k++;
                }
            }
        }
    }
}
