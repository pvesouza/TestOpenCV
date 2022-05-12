#include "fileHelper.h"

FileHelper::FileHelper() {
    // Constructor
}

FileHelper::~FileHelper() {
    // Desructor
}

void FileHelper::saveFile(Point *points, int nPoints, const char *path) {
    bool isEmpty = true;
    for (int i = 0; i < nPoints; i++)
    {
        if (points[i].x != 0 || points[i].y != 0) {
            isEmpty = false;
        }
    }

    if (isEmpty)
    {
        cout << "{}" << endl;
        return;
    }

    char jsonOut[201];
    char jsonAux[50];
    int pos = 0;
    jsonAux[0] = '\0';
    jsonOut[0] = '\0';

    // Writes the points in file
    for (int i = 0; i < nPoints; i++) 
    {
        Point p = *(points + i);
        int writtenChars = 0;

        if (i == 0) {
            writtenChars = sprintf(jsonAux, "{\"c%d\":\"%d,%d\",", i + 1, p.x, p.y);
        } else if (i == 3) {
            writtenChars = sprintf(jsonAux, "\"c%d\":\"%d,%d\"}", i + 1, p.x, p.y);
        }else {           
            writtenChars = sprintf(jsonAux, "\"c%d\":\"%d,%d\",", i + 1, p.x, p.y);
        }

        pos = pos + writtenChars;
        if (pos > 201){
            cout << "Name file very large" << endl;
            return;
        }
        cout << jsonAux << endl;
        
        #ifdef OS_Windows
            strcat_s(jsonOut, jsonAux);
        #else
            strcat(jsonOut, jsonAux);
        #endif
       
    }
}