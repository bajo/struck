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
    cv::Rect rectOfInterest(roi.XCentre()-roi.Width()/2,roi.YCentre()-roi.Height()/2,roi.Width(),roi.Height());//transform to integer
    Mat moi = img(rectOfInterest);
    
    //scale:
    Mat work;
    resize(moi,work,cv::Size(m_HD*m_cellWidth,m_VD*m_cellHeight));
    int k=0;
    //calc data vector
    for(int m=0;m<m_VD;m++){
        for(int n=0;n<m_HD;n++){
            cv::Rect coi(n*m_cellWidth,m*m_cellHeight,m_cellWidth,m_cellHeight);
            Mat cell=work(coi);
            if(cell.type()!=CV_16UC1){
                cout << "shit" << endl;//at least this seems right
            }
            for(int i=0;i<m_cellHeight*m_cellWidth;i++){
                for(int j=i+1;j<m_cellHeight*m_cellWidth;j++){
                    float data=(float)cell.at<uint16_t>(i)-(float)cell.at<uint16_t>(j);
                    Features::m_featVec[k]=data;
                    k++;
                }
                
            }
            
        }
    }
}
