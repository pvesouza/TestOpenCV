#include "fileHelper.h"

FileHelper::FileHelper() {
    // Constructor
}

FileHelper::~FileHelper() {
    // Desructor
}

void FileHelper::saveFile(Point *points, int nPoints, const char *path) {
    ofstream outputFile(path);                                              // Creates an output file

    // Writes the points in file
    for (int i = 0; i < nPoints; i++) {
        Point p = *(points + i);
        outputFile << "(" << p.x << "," << p.y << ")" << endl;
    }

    // Closes files 
    outputFile.close();
}

