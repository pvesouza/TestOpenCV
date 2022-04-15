#ifndef FILEHELPER_H
#define FILEHELPER_H

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <string>
#include <cstdlib>
#include <iostream>
#include <fstream>

using namespace cv;
using namespace std;

class FileHelper 
{

    public:
        FileHelper();                                                       //Constructor
        virtual ~FileHelper();                                              // Destructor
        void saveFile(Point *points, int nPoints, const char *path);        // Saves a txt file

};

#endif //FILEHELPER_H