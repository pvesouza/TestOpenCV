#include <iostream>
#include <string>
#include <math.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/core/utils/logger.hpp>
#include "../../../fileHelper.h"


#define REC_MIN_PERIMETER 50
#define ARC_COS_86 0.05  
#define DEBUG

#ifdef _MSC_VER
    #define _CRT_SECURE_NO_WARNINGS
#endif

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
const char* BASE_PATH = "images_out\\";

// Saves the rectangle points in a txt file
void savePoints(Point* points, int number, char* path) {
    FileHelper helper;

    // Saves the name of the arquive
    char c = '\0', name[100];
    int secondPoint = 0;
    uint8_t count = 0;

    while ((secondPoint < 1) && (count < 100)) {
        c = path[count];
        if (c == '.') {
            secondPoint++;
        }
        name[count++] = c;
    }

    // Fills the name ".txt"
    name[count++] = 't';
    name[count++] = 'x';
    name[count++] = 't';
    name[count] = '\0';

    helper.saveFile(points, number, name);
}

bool saveImage(char* path, const Mat& image) {
    // Taking the name from path argument
    char  c = '\0';
    char name[100];
    int count = 0;
    int secondBar = 0;
    int nBars = 0;

    // Counting the number of / character
    do {
        c = path[count++];
        //cout << c;
        if (c == '\\') {
            nBars++;
        }

    } while (c != '\0');

    //cout << endl;
    count = 0;

    //Scanning for the image path
    do {
        c = path[count];
        name[count] = c;
        //cout << c;
        if (c == '\\') {
            secondBar++;
        }
        count++;

    } while (c != '\0' && secondBar < nBars);

    name[count] = '\0';
    //cout << endl;
    secondBar = 0;

    // Saves the image name
    char imageName[20];
    while (c != '\0') {
        c = path[count++];
        imageName[secondBar++] = c;
        //cout << c;
    }

    //cout << endl;

    // Test if has saved the image name
    if (secondBar == 0) {
        return false;
    }

    // Insere o caminho image_out/
    strcat_s(name, "images_out\\");
    strcat_s(name, imageName);

    //cout << "Filename: " << name << endl;

    return imwrite(name, image);
}

// Trackbar for Canny border algorithm
void on_trackbar_canny(int, void*) {
    Canny(image, border, top_slider, 3 * top_slider);
    //imshow("canny", border);
    //waitKey();
}

// Histogram Equalization Function
Mat histogramEqualize(cv::Mat image) {
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
Mat plotHistogram(const Mat& image) {
    int histSize = 256;
    float range[] = { 0,256 };
    const float* histRange[] = { range };

    bool uniform = true, accumulate = false;

    Mat grayHistogram;
    calcHist(&image, 1, 0, Mat(), grayHistogram, 1, &histSize, histRange, uniform, accumulate);

    int histWidth = 512, histHeight = 400;
    int binWidth = cvRound((double)histWidth / histSize);

    Mat histImage(histHeight, histWidth, CV_8UC3, Scalar(0, 0, 0));

    normalize(grayHistogram, grayHistogram, 0, histImage.rows, NORM_MINMAX, -1, Mat());

    for (int i = 1; i < histSize; i++) {
        line(histImage, Point(binWidth * (i - 1), histHeight - cvRound(grayHistogram.at<float>(i - 1)))
            , Point(binWidth * (i), histHeight - cvRound(grayHistogram.at<float>(i)))
            , Scalar(255, 0, 0), 2, 8, 0);
    }

    //imshow("histogram calculated", histImage);
    //waitKey();

    return grayHistogram;
}

// Dilation used for make bright regions larger
Mat dilateImage(const Mat& image) {

    int dilation_type = MORPH_RECT;
    int dilation_size = 2;
    Mat dilation_dst;

    Mat element = getStructuringElement(
        dilation_type,
        Size(2 * dilation_size + 1, 2 * dilation_size + 1),
        Point(dilation_size, dilation_size));

    dilate(image, dilation_dst, element);

    //imshow("Dilation Demo", dilation_dst);
    //waitKey();

    return dilation_dst;
}

// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2
double angle(Point pt1, Point pt2, Point pt0) {

    double dx1 = (double)pt1.x - pt0.x;
    double dy1 = (double)pt1.y - pt0.y;
    double dx2 = (double)pt2.x - pt0.x;
    double dy2 = (double)pt2.y - pt0.y;
    // Product intern divided by the norm
    return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
}

// the function draws all the squares in the image
void drawSquare(Mat& image, Point* points) {

    for (int i = 0; i < 4; i++) {
        if (i != 3) {
            line(image, *(points + i), *(points + i + 1), Scalar(128, 255, 255), 3);
        }
        else {
            line(image, *(points + i), *(points), Scalar(128, 255, 255), 3);
        }
    }

    imshow("Squares", image);
}

void _drawContours(Mat image, vector< vector<Point> > contours, vector<Vec4i> hierarchy) {
    int levels = -1;
    Mat dest = Mat::zeros(image.rows, image.cols, CV_8UC3);

    drawContours(dest, contours, levels, Scalar(128, 255, 255), 2, LINE_AA);
    //namedWindow("contours", WINDOW_AUTOSIZE);
    //imshow("contours", dest);
}

void getCoodinates(Point* pt_out, vector<Point> points_vector) {

    int xSmall = 0;
    int xGreat = 0;
    int ySmall = 0;
    int yGreat = 0;
    Point point[4];

    // Finding the greatest x and y
    for (int k = 0; k < 4; k++) {
        if (xGreat < points_vector[k].x) {
            xGreat = points_vector[k].x;
        }

        if (yGreat < points_vector[k].y) {
            yGreat = points_vector[k].y;
        }
    }


    xSmall = xGreat;
    ySmall = yGreat;
    // Finding the smallest x and y
    for (int k = 0; k < 4; k++) {
        if (xSmall > points_vector[k].x) {
            xSmall = points_vector[k].x;
        }

        if (ySmall > points_vector[k].y) {
            ySmall = points_vector[k].y;
        }
    }

    point[0] = { xSmall, yGreat };
    point[1] = { xSmall, ySmall };
    point[2] = { xGreat, ySmall };
    point[3] = { xGreat, yGreat };

    for (int k = 0; k < points_vector.size(); k++) {
        *(pt_out + k) = point[k];
        //cout << "(" << point[k].x << "," << point[k].y << ")" << endl;
    }
}

// Get rectangle points
void getRectanglePoints(Mat& image, Point* pt_out) {

    vector<Point> contours;
    vector<Vec4i> hierarchy_out;
    vector<Vec4i> hierarchy;                //[Next, Previous, First_Child, Parent]
    vector<vector<Point>> contoursSet;
    vector<vector<Point>> contoursOut;

    findContours(image, contoursSet, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    #ifdef DEBUG
    _drawContours(image, contoursSet, hierarchy);
    #endif

    size_t numberOfContours = contoursSet.size();

    // Scans all found borders in order to test them
    for (size_t k = 0; k < numberOfContours; k++) 
    {
        double perimeter = arcLength(contoursSet[k], true);
        // Debug    
        #ifdef DEBUG
            cout << "Perimeter: " << perimeter << endl;
        #endif

        approxPolyDP(Mat(contoursSet[k]), contours, perimeter * 0.03, true);

        // Testa a quantidade de lados de contoursOut[k]
        if (contours.size() == 4) {
            double perimeterRec = arcLength(contours, true);

            // Checks minimal perimeter 
            if (perimeterRec >= REC_MIN_PERIMETER && perimeterRec <= 0.95 * imagePerimeter) {

                // Checks the angles between the points
                double cosineAlpha[4];
                cosineAlpha[0] = fabs(angle(contours[3], contours[1], contours[0]));
                cosineAlpha[1] = fabs(angle(contours[0], contours[2], contours[1]));
                cosineAlpha[2] = fabs(angle(contours[1], contours[3], contours[2]));
                cosineAlpha[3] = fabs(angle(contours[2], contours[0], contours[3]));

                int result = 0;
                for (int i = 0; i < 4; i++) {
                    //cout << "Cosine: " << cosineAlpha[i] << endl;
                    if (cosineAlpha[i] < ARC_COS_86) {
                        result++;
                    }
                }

                if (result >= 2) {
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
    //cout << "Size 2: " << secondContourSize << endl;

    if (secondContourSize > 0) {
        _drawContours(image, contoursOut, hierarchy_out);
        vector<Point> points_vector;
        Point point[4];

        if (secondContourSize == 1) {
            points_vector = contoursOut[0];
            getCoodinates(pt_out, points_vector);
        }
        else {
            int indexGreatest = 0;

            for (int i = 0; i < secondContourSize; i++) {

                // Discards the edge of the image
                //[Next, Previous, First_Child, Parent]
                if (hierarchy_out[i][0] == -1 && hierarchy_out[i][1] == -1 && hierarchy_out[i][2] != -1 && hierarchy_out[i][3] == -1) {

                }
                else {
                    if (i != 0) {
                        int dif = hierarchy_out[i - 1][2] - hierarchy_out[i][3];
                        //cout << "DIFF " << dif << " - " << i << endl;
                        if (dif == 1) {
                            indexGreatest = i - 1;
                            break;
                        }
                    }
                }
            }

            points_vector = contoursOut[indexGreatest];
            getCoodinates(pt_out, points_vector);
        }
    }
    else {
        // Fills with zeros 
        for (int i = 0; i < 4; i++) {
            *(pt_out + i) = { 0,0 };
        }
   
        return;
    }
}

// Mostra a imagem de destino
int display_dst(int delay, const char* window_name, Mat& image)
{
    //imshow(window_name, image);
    int c = waitKey(delay);
    if (c >= 0) { return -1; }
    return 0;
}

int display_caption(const char* caption, Mat& dst, const Mat& image)
{
    dst = Mat::zeros(image.size(), image.type());
    putText(dst, caption,
        Point(image.cols / 4, image.rows / 2),
        FONT_HERSHEY_COMPLEX, 1, Scalar(255, 255, 255));

    return display_dst(DELAY_CAPTION, " ", dst);
}

int filters(const Mat& image) {

    Mat destiny = image.clone();

    if (display_caption("Homogeneous Blur", destiny, image) != 0)
    {
        return 0;
    }

    for (int i = 1; i < MAX_KERNEL_LENGTH; i = i + 2)
    {
        blur(image, destiny, Size(i, i), Point(-1, -1));
        if (display_dst(DELAY_BLUR, "Homogeneous Blur", destiny) != 0)
        {
            return 0;
        }
    }

    if (display_caption("Gaussian Blur", destiny, image) != 0)
    {
        return 0;
    }

    for (int i = 1; i < MAX_KERNEL_LENGTH; i = i + 2)
    {
        GaussianBlur(image, destiny, Size(i, i), 0, 0);
        if (display_dst(DELAY_BLUR, "Gaussian Blur", destiny) != 0)
        {
            return 0;
        }
    }

    if (display_caption("Median Blur", destiny, image) != 0)
    {
        return 0;
    }

    for (int i = 1; i < MAX_KERNEL_LENGTH; i = i + 2)
    {
        medianBlur(image, destiny, i);
        if (display_dst(DELAY_BLUR, "Median Blur", destiny) != 0)
        {
            return 0;
        }
    }

    if (display_caption("Bilateral Blur", destiny, image) != 0)
    {
        return 0;
    }

    for (int i = 1; i < MAX_KERNEL_LENGTH; i = i + 2)
    {
        bilateralFilter(image, destiny, i, i * 2, i / 2);
        if (display_dst(DELAY_BLUR, "Bilateral Blur", destiny) != 0)
        {
            return 0;
        }
    }

    //display_caption( "Done!" );
    return 0;
}

//***********************************************************************
/////////MAIN
//***********************************************************************

int main(int argc, char** argv) {

    //Disabling the logs
    cv::utils::logging::setLogLevel(utils::logging::LOG_LEVEL_SILENT);

    vector<vector<Point>> rectangleCorners;
    // Reads an image from arquive
    //image = cv::imread("C:\\imagens\\biel.png", cv::IMREAD_GRAYSCALE);
    image = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
    Mat colorImage = imread(argv[1], IMREAD_COLOR);
    imagePerimeter = 2 * (image.cols + image.rows);

    if (image.empty()) {
        cout << "Erro ao ler imagem" << endl;
    }


    Mat hist = plotHistogram(image);

    double great = 0.0;
    double derivate = 1;
    bool signal = false;                // Neg = false ; Pos = true
    bool signal_anterior = false;       // Neg = false ; Pos = true
    int index = 0;
    int index_aux = 0;
    int indexVector[256];

    for (int j = 0; j < hist.rows - 1; j++) {

        derivate = hist.at<float>(j + 1, 0) - hist.at<float>(j, 0);

        if (fabs(derivate) > 1.0) {
            if (derivate < 0.0) {
                signal = false;
                // Signal has changed
                if (signal != signal_anterior) {
                    signal_anterior = signal;
                    indexVector[index++] = j;
                }
            }
            else {
                signal = true;
                // Signal has changed
                if (signal != signal_anterior) {
                    signal_anterior = signal;
                    indexVector[index++] = j;
                }
            }
        }
    }

    if (index > 1) {
        index_aux = (indexVector[index - 2] + 255) / 2;
        if (index_aux >= 250) {
            index_aux = 245;
        }
    }
    else {
        index_aux = 253;
    }

    // Making Image binary
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
            uchar value = 0;
            value = image.at<uchar>(i, j);

            if (value <= index_aux) {
                image.at<uchar>(i, j) = 0;
            }
            else {
                image.at<uchar>(i, j) = 255;
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
    getRectanglePoints(timg, pointsToSave);
    savePoints(pointsToSave, 4, argv[1]);

    #ifdef DEBUG
        drawSquare(colorImage, pointsToSave);
        if (saveImage(argv[1], timg))
    #endif

    return 0;
}



