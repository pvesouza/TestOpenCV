#include <iostream>
#include <string>
#include <math.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include "fileHelper.h"

#define REC_MIN_PERIMETER 50
#define ARC_COS_86 0.05  
//#define DEBUG1

using namespace std;
using namespace cv;

cv::Mat image, border;
int imagePerimeter;

// Variables used for Canny transformation
int top_slider = 10;
int top_slider_max = 200;
char TrackbarName[50];

// Variables used for filters
int DELAY_CAPTION = 1500;
int DELAY_BLUR = 100;   
int MAX_KERNEL_LENGTH = 12;

// Path for saved images
const char *BASE_PATH = "images_out/";

// Saves the rectangle points in a txt file
void savePoints(Point *points, int number, char *path) {
    FileHelper helper;

    helper.saveFile(points, number, path);
}

// Save an image file with the output format
bool saveImage(char *path, const Mat &image) {
    // Taking the name from path argument
    char  c = '\0';
    char name[100];
    int count = 0;
    int secondBar = 0;
    int nBars = 0;

    // Counting the number of / character
        do {
        c = path[count++];
        if (c == '/') {
            nBars++;
        }
   
    } while (c != '\0');

    count = 0;

    //Scanning for the image path
    do {
        c = path[count];
        name[count] = c;
        if (c == '/') {
            secondBar++;
        }
        count++;
   
    } while (c != '\0' && secondBar < nBars);

    name[count] = '\0';
    secondBar = 0;

    // Saves the image name
    char imageName[20];
    while (c != '\0') {
        c = path[count++];
        imageName[secondBar++] = c;
   }

    // Test if has saved the image name
    if (secondBar == 0) {
        return false;
    }

    // Insere o caminho image_out/
    strcat(name, BASE_PATH);
    strcat(name, imageName);

    return imwrite(name, image);
}


// Histogram Equalization Function
Mat histogramEqualize(Mat image) {
    // Equalize histogram
    Mat hist_equalized_image;
    equalizeHist(image, hist_equalized_image);

    //Define names for windows
    namedWindow("Histogram", cv::WINDOW_AUTOSIZE);
    //imshow("Histogram", hist_equalized_image);
    //waitKey();
    return hist_equalized_image;
}

// Plots the histogram of an image
Mat plotHistogram(const Mat &image) {
    int histSize = 256;
    float range[] = {0,256};
    const float *histRange[] = {range};

    bool uniform = true, accumulate = false;

    Mat grayHistogram;
    calcHist(&image, 1, 0, Mat(), grayHistogram, 1, &histSize, histRange, uniform, accumulate);

    int histWidth = 512, histHeight = 400;
    int binWidth = cvRound((double) histWidth / histSize);

    Mat histImage(histHeight, histWidth, CV_8UC3, Scalar(0,0,0));

    normalize(grayHistogram, grayHistogram, 0, histImage.rows, NORM_MINMAX, -1, Mat());

    for (int i = 1; i < histSize; i++) 
    {
        line(histImage, Point(binWidth*(i - 1), histHeight - cvRound(grayHistogram.at<float>(i - 1)))
                ,Point(binWidth*(i), histHeight - cvRound(grayHistogram.at<float>(i)) )
                ,Scalar(255, 0, 0), 2, 8, 0);
    }

    #ifdef DEBUG
         imshow("Histogram", histImage);
         waitKey();
    #endif
   

    return grayHistogram;
}

// Dilation used for make bright regions larger
Mat dilateImage(const Mat &image, int dilationSize = 2) {

    int dilation_type = MORPH_RECT;
    int dilation_size = dilationSize;
    Mat dilation_dst;

    Mat element = getStructuringElement( 
            dilation_type,
            Size( 2*dilation_size + 1, 2*dilation_size+1 ),
            Point( dilation_size, dilation_size ) );

    dilate( image, dilation_dst, element );

    #ifdef DEBUG
        imshow( "Dilation Demo", dilation_dst );
        waitKey();
    #endif

    return dilation_dst;
}

// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2
double angle(Point pt1, Point pt2, Point pt0) {
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    // Product intern divided by the norm
    return (dx1*dx2 + dy1*dy2) / sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
} 

// the function draws all the squares in the image
void drawSquare(Mat &image, Point *points) 
{
    for (int i = 0; i < 4; i++) 
    {
        if (i != 3) {
            line(image, *(points + i), *(points + i + 1), Scalar(128,255,255),3);
        }else {
            line(image, *(points + i), *(points), Scalar(128,255,255),3);
        }
    }

    #ifdef DEBUG1
        imshow("Squares", image);
        waitKey();
    #endif
}

void _drawContours(Mat image, vector< vector<Point> > contours, vector<Vec4i> hierarchy) 
{
    int levels = -1;
    Mat dest = Mat::zeros(image.rows, image.cols, CV_8UC3);

    drawContours(dest, contours, levels, Scalar(128,255,255), 2, LINE_AA);
    namedWindow("contours", WINDOW_AUTOSIZE);
    #ifdef DEBUG
        imshow("contours", dest);
        waitKey();
    #endif
   
}

void getCoodinates(Point *pt_out, vector<Point> points_vector) 
{

    int xSmall = 0;
    int xGreat = 0;
    int ySmall = 0;
    int yGreat = 0;
    Point point[4];

    // Finding the greatest x and y
    for (int k = 0; k < 4; k++) 
    {
        if (xGreat < points_vector[k].x) 
        {
            xGreat = points_vector[k].x;
        }

        if (yGreat < points_vector[k].y) 
        {
            yGreat = points_vector[k].y;
        }
    }

            
    xSmall = xGreat;
    ySmall = yGreat;
    // Finding the smallest x and y
    for (int k = 0; k < 4; k++) 
    {
        if (xSmall > points_vector[k].x) {
            xSmall = points_vector[k].x;
        }

        if (ySmall > points_vector[k].y) {
            ySmall = points_vector[k].y;
        }
    }

    point[0] = {xSmall, yGreat};
    point[1] = {xSmall, ySmall};
    point[2] = {xGreat, ySmall};
    point[3] = {xGreat, yGreat};

    for (int k = 0; k < points_vector.size(); k++) {
        *(pt_out + k) = point[k];
    }
}

// Get rectangle points
bool getRectanglePoints(Mat &image, Point *pt_out) 
{
   
    vector<Point> contours;
    vector<Vec4i> hierarchy_out;
    vector<Vec4i> hierarchy;                //[Next, Previous, First_Child, Parent]
    vector<vector<Point>> contoursSet;
    vector<vector<Point>> contoursOut;
    bool result = false;

    findContours(image, contoursSet, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    #ifdef DEBUG
        _drawContours(image, contoursSet, hierarchy);
    #endif

    size_t numberOfContours = contoursSet.size();

    for (size_t k = 0; k < numberOfContours; k++) 
    { 
        double perimeter = arcLength(contoursSet[k], true);
  

        approxPolyDP(Mat(contoursSet[k]), contours, perimeter * 0.03, true);

        // Testa a quantidade de lados de contoursOut[k] tem que ser 4
        if (contours.size() == 4) 
        {
            double perimeterRec = arcLength(contours, true);

        // Debug
        #ifdef DEBUG
            cout << "Perimeter: " << perimeterRec << endl;
        #endif

            // Checks minimal perimeter and less than image perimeter
            if (perimeterRec >= REC_MIN_PERIMETER && perimeterRec <= 0.95 * imagePerimeter) 
            {
                // Checks the angles between the points
                double cosineAlpha[4];
                cosineAlpha[0] = fabs(angle(contours[3], contours[1], contours[0]));
                cosineAlpha[1] = fabs(angle(contours[0], contours[2], contours[1]));
                cosineAlpha[2] = fabs(angle(contours[1], contours[3], contours[2]));
                cosineAlpha[3] = fabs(angle(contours[2], contours[0], contours[3]));

                // If 2 or more angles are approx 90degrees consider a rectangle
                int result = 0;

                for (int i = 0; i < 4; i++) 
                {
                    #ifdef DEBUG
                        cout << "Cosine: " << cosineAlpha[i] << endl;
                    #endif

                    if (cosineAlpha[i] < ARC_COS_86) 
                    {
                        result++;
                    }
                }

                if (result >= 2) 
                {
                    #ifdef DEBUG

                        for (int i = 0; i < contours.size(); i++) 
                        {
                            Point p1 = contours[i];
                            cout << "(" << p1.x << "," << p1.y << ")" << endl;
                        }
                    #endif

                    contoursOut.push_back(contours);
                    hierarchy_out.push_back(hierarchy[k]);
                    #ifdef DEBUG
                        cout << hierarchy[k] << endl;
                    #endif
                }
            }
        }
    }

    size_t secondContourSize = contoursOut.size();

    // Check if we have rectangles              
    if (secondContourSize > 0) {

        #ifdef DEBUG
            _drawContours(image, contoursOut, hierarchy_out);
        #endif

        vector<Point> points_vector;
        Point point[4];

        if (secondContourSize == 1) {
            points_vector = contoursOut[0];
            getCoodinates(pt_out, points_vector);
            result = true;
        } 
        else 
        {
        int indexGreatest = 0;

            for (int i = 0; i < secondContourSize; i++) 
            {
                // Discards the edge of the image
                //[Next, Previous, First_Child, Parent]
                if (hierarchy_out[i][0] == -1 && hierarchy_out[i][1] == -1 && hierarchy_out[i][2] != -1 && hierarchy_out[i][3] == -1) 
                {
                    
                }else {
                    if (i != 0) {
                        int dif = hierarchy_out[i - 1][2] - hierarchy_out[i][3];
                    
                        if (dif == 1) {
                            indexGreatest = i - 1;
                            break;
                        }
                    }
                }
            }

            points_vector = contoursOut[indexGreatest];
            getCoodinates(pt_out, points_vector); 
            result = true;       
        }
    } 
    else
    {
        // Fills with zeros 
        for (int i = 0; i < 4; i++) 
        {
            *(pt_out + i) = {0,0};
        }
    }
    return result;          
}

//***********************************************************************
/////////MAIN
//***********************************************************************

int main(int argc, char** argv) {

    //Disabling the logs
    #ifdef OS_Windows
        cv::utils::logging::setLogLevel(utils::logging::LOG_LEVEL_SILENT);
    #endif

    image = imread(argv[1], IMREAD_GRAYSCALE);
    Mat colorImage = imread(argv[1], IMREAD_COLOR);
    imagePerimeter = 2 * (image.cols + image.rows);

    #ifdef DEBUG
        cout << "ImagePerimeter = " << imagePerimeter << endl;
    #endif

    // Reads an image from arquive
    if (image.empty()) {
        #ifdef DEBUG
            cout << "Image not read " << endl;
        #endif

        return EXIT_FAILURE;
    }

    #ifdef DEBUG
        imshow("Image", image);
        waitKey();
    #endif

    Mat hist = plotHistogram(image);

    double great = 0.0;
    double derivate = 1;
    bool signal = false;                // Neg = false ; Pos = true
    bool signal_anterior = false;       // Neg = false ; Pos = true
    int index = 0;
    int index_aux = 0;
    int indexVector[256];

    for (int j = 0; j < hist.rows - 1; j++) 
    {
        derivate = hist.at<float>(j + 1, 0) - hist.at<float>(j, 0);

        if (fabs(derivate) > 1.0) {
            if (derivate < 0.0) 
            {
                signal = false;
                // Signal has changed
                if (signal != signal_anterior) {
                    signal_anterior = signal;
                    indexVector[index++] = j;
                }
            } else 
            {
                signal = true;
                // Signal has changed
                if (signal != signal_anterior) {
                    signal_anterior = signal;
                    indexVector[index++] = j;
                }
            }
        }
    }

    if (index > 1) 
    {
        index_aux = (indexVector[index - 2] + 255) / 2;

        if (index_aux >= 250) 
        {
            index_aux = 245;
        }
    } 
    else{ index_aux = 253; }

    // Making Image binary
    for (int i = 0; i < image.rows; i++) 
    {
        for (int j = 0; j < image.cols ; j++) 
        {
            uchar value = 0;
            value = image.at<uchar>(i,j);

            if (value  <= index_aux)
            {
                image.at<uchar>(i,j) = 0;
            } 
            else 
            {
                image.at<uchar>(i,j) = 255;
            }
        }
    }

    #ifdef DEBUG
        imshow("Binarization", image);
        waitKey();
    #endif


    // Dilate image in order to accentuate the contours
    Mat dilated_image = dilateImage(image);

    // Median Filter to remove noise
    Mat timg(dilated_image);
    medianBlur(dilated_image, timg, 3);

    #ifdef DEBUG
        imshow("MedianBlur", timg);
        waitKey();
    #endif

    // Saving rectangle's points
    Point pointsToSave[4];
    bool haveRectangle = getRectanglePoints(timg, pointsToSave);
    // Has a white rectangle ?
    if (haveRectangle)
    {
        savePoints(pointsToSave, 4, argv[1]);
        #ifdef DEBUG1
            drawSquare(colorImage, pointsToSave);
        #endif

    }else {
        // At this point we analize the green component to see if we have a green rectangle 
        #ifdef DEBUG 
            imshow("Green Image", colorImage);
            waitKey();
        #endif

        Mat channels[3];
        split(colorImage, channels);

        #ifdef DEBUG
            imshow("0", channels[0]);
            imshow("1", channels[1]);
            imshow("2", channels[2]);
            waitKey();
        #endif

        Mat resultimg = channels[2] - channels[0];

        #ifdef DEBUG
            imshow("Sub", resultimg);
            waitKey();
        #endif

        // Binarization

        for (int i = 0; i < resultimg.rows; ++i)
        {
            for (int j = 0; j < resultimg.cols; ++j)
            {
                uchar pixel = resultimg.at<uchar>(i,j);

                if (pixel > 5) 
                {
                    pixel = 255;
                }
                resultimg.at<uchar>(i,j) = pixel;
            }
        }

        #ifdef DEBUG
            imshow("Result", resultimg);
            waitKey();
        #endif

        // Median Filter to remove noise
        medianBlur(resultimg, timg, 25);

        #ifdef DEBUG
            imshow("Median", timg);
            waitKey();
        #endif
            

        //Finding the contours in an image
        vector<Point> contours;
        vector<vector<Point>> contourSet;
        vector<vector<Point>> contourSetOut;
        vector<Vec4i> hierarchy;                //[Next, Previous, First_Child, Parent]
        vector<Vec4i> hierarchy_out;                //[Next, Previous, First_Child, Parent]

        findContours(timg, contourSet, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
        #ifdef DEBUG
            _drawContours(timg, contourSet, hierarchy);
        #endif
        size_t numberOfContours = contourSet.size();

        // The marjority of cases we only have 1 rectangle
        if (numberOfContours == 1) 
        {
            // Poligonal approximation
            double perimeter = arcLength(contourSet[0], true);
            approxPolyDP(Mat(contourSet[0]), contours, perimeter * 0.02, true);
            
            // Getting the minimal rectangle that enclosures the courve
            Point2f vtx[4];
            RotatedRect recBox = minAreaRect(contours);
            recBox.points(vtx);
            contours.clear();

            // Converting the points
            for (int i = 0; i < 4; i++) 
            {
                
                #ifdef DEBUG 
                    cout << "Points: " << vtx[i] << " - ";
                #endif

                Point p = vtx[i];
                pointsToSave[i] = p;

                #ifdef DEBUG 
                    cout << "Points: " << p << " - ";
                #endif
            }

            savePoints(pointsToSave, 4, argv[1]);

            #ifdef DEBUG1
                drawSquare(colorImage, pointsToSave);
            #endif
            return 0;
        }

        for (size_t k = 0; k < numberOfContours; k++) 
        {
            // Poligonal approximation
            double perimeter = arcLength(contourSet[k], true);
            approxPolyDP(Mat(contourSet[k]), contours, perimeter * 0.02, true);
            
            // Getting the minimal rectangle that enclosures the courve
            Point2f vtx[4];
            RotatedRect recBox = minAreaRect(contours);
            recBox.points(vtx);
            contours.clear();

            // Converting the points
            for (int i = 0; i < 4; i++) 
            {
                
                #ifdef DEBUG 
                    cout << "Points: " << vtx[i] << " - ";
                #endif
                Point p = vtx[i];

                #ifdef DEBUG 
                    cout << "Points: " << p << " - ";
                #endif

                contours.push_back(p);
            }

            #ifdef DEBUG 
                cout << endl;
            #endif

            // Checks minimal perimeter and less than image perimeter
            if (perimeter >= REC_MIN_PERIMETER && perimeter <= 0.95 * imagePerimeter) 
            {
                // Checks the angles between the points
                double cosineAlpha[4];
                cosineAlpha[0] = fabs(angle(contours[3], contours[1], contours[0]));
                cosineAlpha[1] = fabs(angle(contours[0], contours[2], contours[1]));
                cosineAlpha[2] = fabs(angle(contours[1], contours[3], contours[2]));
                cosineAlpha[3] = fabs(angle(contours[2], contours[0], contours[3]));

                // If 2 or more angles are approx 90degrees consider a rectangle
                int result = 0;

                for (int i = 0; i < 4; i++) 
                {
                    #ifdef DEBUG
                        cout << "Cosine: " << cosineAlpha[i] << endl;
                    #endif

                    if (cosineAlpha[i] < ARC_COS_86) 
                    {
                        result++;
                    }
                }

                if (result >= 2) 
                {
                    #ifdef DEBUG

                        for (int i = 0; i < contours.size(); i++) 
                        {
                            Point p1 = contours[i];
                            cout << "(" << p1.x << "," << p1.y << ")" << endl;
                        }
                    #endif

                    contourSetOut.push_back(contours);
                    hierarchy_out.push_back(hierarchy[k]);

                    #ifdef DEBUG
                        cout << hierarchy[k] << endl;
                    #endif
                }
            }
        }

        size_t secondContourSize = contourSetOut.size();

        // Check if we have rectangles              
        if (secondContourSize > 0) 
        {

            #ifdef DEBUG
                _drawContours(image, contourSetOut, hierarchy_out);
            #endif

            vector<Point> points_vector;
            Point point[4];

            if (secondContourSize == 1) 
            {
                points_vector = contourSetOut[0];
                getCoodinates(pointsToSave, points_vector);
            } 
            else {
                int indexGreatest = 0;

                for (int i = 0; i < secondContourSize; i++) 
                {
                    // Discards the edge of the image
                    //[Next, Previous, First_Child, Parent]
                    if (hierarchy_out[i][0] == -1 && hierarchy_out[i][1] == -1 && hierarchy_out[i][2] != -1 && hierarchy_out[i][3] == -1) 
                    {
                    
                    } else {
                        if (i != 0) 
                        {
                            int dif = hierarchy_out[i - 1][2] - hierarchy_out[i][3];
                    
                            if (dif == 1) 
                            {
                                indexGreatest = i - 1;
                                break;
                            }
                        }
                    }
                }

                points_vector = contourSetOut[indexGreatest];
                getCoodinates(pointsToSave, points_vector);     
            }
        } else {
            // Fills with zeros 
            for (int i = 0; i < 4; i++) 
            {
                *(pointsToSave + i) = {0,0};
            }
        }

        savePoints(pointsToSave, 4, argv[1]);

        #ifdef DEBUG1
            drawSquare(colorImage, pointsToSave);
        #endif
    }

    return 0;
}



