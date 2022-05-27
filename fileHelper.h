#ifndef FILEHELPER_H
#define FILEHELPER_H

#ifdef __unix__         

#elif defined(_WIN32) || defined(WIN32) 
   #define OS_Windows
#endif

#include <opencv2/opencv.hpp>
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