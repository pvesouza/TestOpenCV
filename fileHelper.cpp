#include "fileHelper.h"

FileHelper::FileHelper() {
    // Constructor
}

FileHelper::~FileHelper() {
    // Desructor
}

void FileHelper::saveFile(Point *points, int nPoints, const char *path) {
    ofstream outputFile(path);                                              // Creates an output file
    char jsonOut[201];
    char jsonAux[50];
    int pos = 0;

    // Writes the points in file
    for (int i = 0; i < nPoints; i++) {
        Point p = *(points + i);
        int writtenChars = 0;

        if (i == 0) {
            writtenChars = sprintf(jsonAux, "{\n\"c%d\":\"%d\",\"%d\",\n", i + 1, p.x, p.y);
        } else if (i == 3) {
            writtenChars = sprintf(jsonAux, "\"c%d\":\"%d\",\"%d\",\n}", i + 1, p.x, p.y);
        }else {           
            writtenChars = sprintf(jsonAux, "\"c%d\":\"%d\",\"%d\",\n", i + 1, p.x, p.y);
        }

        strcat(jsonOut, jsonAux);

        cout << jsonOut;
    }
    outputFile << jsonOut << endl;
    // Closes files 
    outputFile.close();
}