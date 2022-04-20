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

#define RET_MIN_AREA 500

using namespace std;
using namespace cv;

cv::Mat image, border;

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
        // Saves the name of the arquive
    char c = '\0', name[100];
    int secondPoint = 0;
    uint8_t count = 0;

    while ((secondPoint < 2) && (count < 100)) {
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

    helper.saveFile(points, number, name);
}

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
   
    } while (c != '\0' && secondBar < 2);

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

    cout << "Filename: " << name << endl;

    return imwrite(name, image);
}

// Trackbar for Canny border algorithm
void on_trackbar_canny(int, void*) {
    Canny(image, border, top_slider, 3 * top_slider);
    imshow("canny", border);
    //waitKey();
}

// Histogram Equalization Function
Mat histogramEqualize(cv::Mat image) {
    // Equalize histogram
    Mat hist_equalized_image;
    equalizeHist(image, hist_equalized_image);

    //Define names for windows
    namedWindow("Histogram", cv::WINDOW_AUTOSIZE);
    imshow("Histogram", hist_equalized_image);
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

    for (int i = 1; i < histSize; i++) {

        line(histImage, Point(binWidth*(i - 1), histHeight - cvRound(grayHistogram.at<float>(i - 1)))
                ,Point(binWidth*(i), histHeight - cvRound(grayHistogram.at<float>(i)) )
                ,Scalar(255, 0, 0), 2, 8, 0);
    }

    //imshow("histogram calculated", histImage);
    //waitKey();

    return grayHistogram;
}

// Dilation used for make bright regions larger
Mat dilateImage(const Mat &image) {

    int dilation_type = MORPH_RECT;
    int dilation_size = 2;
    Mat dilation_dst;

    Mat element = getStructuringElement( 
            dilation_type,
            Size( 2*dilation_size + 1, 2*dilation_size+1 ),
            Point( dilation_size, dilation_size ) );

  dilate( image, dilation_dst, element );

  imshow( "Dilation Demo", dilation_dst );
  //waitKey();

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
void drawSquares(const Mat &image, const vector<vector<Point> >& squares) {

    for (size_t i = 0; i < squares.size(); i++) {
        const Point* p = &squares[i][0];

        int n = (int)squares[i].size();
        //dont detect the border
        if (p->x > 3 && p->y > 3) {
            polylines(image, &p, &n, 1, true, Scalar(0, 255, 0), 3);
        }
            
    }

    imshow("Squares", image);
    //waitKey();
}

// Get rectangle points
void getRectanglePoints(const Mat &image, vector<vector<Point>> &contoursSet, int n = 300) {

    contoursSet.clear();
   
    vector<Point> contours;

    findContours(image, contoursSet, RETR_LIST, CHAIN_APPROX_SIMPLE);

    cout << "Size: " << contoursSet.size() << endl;

    // Scans all found borders in order to test them
    for (size_t border = 0 ; border < contoursSet.size() ; border++) {
        // Contour approximation taking the perimeter
        approxPolyDP(Mat(contoursSet[border]), contours, arcLength(Mat(contoursSet[border]), true) * 0.02, true);

        cout << "Rectangle count: " << border << endl;

        // A rectangle should have 4 vertices with angles of approximately 90 degrees
        // Greate area to avoid noisy
        // Be convex
        // Area must be different from zero
        Mat rectangle = Mat(contours);

        if (contours.size() == 4) {
            double area = fabs(contourArea(rectangle));

            if (area >= RET_MIN_AREA) {
                
                if (isContourConvex(rectangle)) {
                    // Test all angles of the polygon
                    double maxCosine = 0.0;

                    for (int j = 2; j < 5; j++) {
                        // find the max cosine between edges
                        double cosine = fabs(angle(contours[j % 4], contours[j - 2], contours[j - 1]));
                        maxCosine = MAX(maxCosine, cosine);
                    }

                    // if cosines of all angles are small
                    // (all angles are ~90 degree) then write quandrange
                    // vertices to resultant sequence
                    if (maxCosine < 0.3) {
                        contoursSet.push_back(contours);
                    }
                        
                }
            }
        }
    }

    drawSquares(image, contoursSet);
}

// Mostra a imagem de destino
int display_dst( int delay, const char *window_name, Mat &image)
{
    imshow( window_name, image );
    int c = waitKey(delay);
    if( c >= 0 ) { return -1; }
    return 0;
}

int display_caption( const char* caption, Mat &dst, const Mat &image)
{
    dst = Mat::zeros( image.size(), image.type() );
    putText( dst, caption,
             Point( image.cols/4, image.rows/2),
             FONT_HERSHEY_COMPLEX, 1, Scalar(255, 255, 255) );

    return display_dst(DELAY_CAPTION, " ", dst);
}

int filters(const Mat &image) {

    Mat destiny = image.clone();

    if( display_caption( "Homogeneous Blur", destiny, image) != 0 )
    {
        return 0;
    }

    for ( int i = 1; i < MAX_KERNEL_LENGTH; i = i + 2 )
    {
        blur( image, destiny, Size( i, i ), Point(-1,-1) );
        if( display_dst( DELAY_BLUR, "Homogeneous Blur", destiny) != 0 )
        {
            return 0;
        }
    }

    if( display_caption( "Gaussian Blur" , destiny, image) != 0 )
    {
        return 0;
    }

    for ( int i = 1; i < MAX_KERNEL_LENGTH; i = i + 2 )
    {
        GaussianBlur( image, destiny, Size( i, i ), 0, 0 );
        if( display_dst( DELAY_BLUR, "Gaussian Blur" , destiny) != 0 )
        {
            return 0;
        }
    }

    if( display_caption( "Median Blur" , destiny, image) != 0 )
    {
        return 0;
    }

    for ( int i = 1; i < MAX_KERNEL_LENGTH; i = i + 2 )
    {
        medianBlur ( image, destiny, i );
        if( display_dst( DELAY_BLUR, "Median Blur" , destiny) != 0 )
        {
            return 0;
        }
    }

    if( display_caption( "Bilateral Blur" , destiny, image) != 0 )
    {
        return 0;
    }

    for ( int i = 1; i < MAX_KERNEL_LENGTH; i = i + 2 )
    {
        bilateralFilter ( image, destiny, i, i*2, i/2 );
        if( display_dst( DELAY_BLUR, "Bilateral Blur" , destiny) != 0 )
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

    vector<vector<Point>> rectangleCorners;
    // Reads an image from arquive
    image = cv::imread(argv[1],cv::IMREAD_GRAYSCALE);

    if (image.empty()) {
        return EXIT_FAILURE;
    }

    imshow("image", image);
    //waitKey();

    Mat hist = plotHistogram(image);

    float great = 0.0;
    int index = 0;

    for (int j = 0; j < hist.rows; j++) {
        //cout << "(" << j << "," << 0 << ") = " << hist.at<float>(j,0) << endl;
        if (great < hist.at<float>(j,0)) {
            great = hist.at<float>(j,0);
            index = j;
        }
    }

    //cout << "index: " << index << endl;
    //cout << "maior: " << great << endl;

    //Mat thresh1;
    //adaptiveThreshold(image, thresh1, (double) index, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 27, 5);
    //threshold(image, thresh1, (double) index, 255.0, THRESH_BINARY + THRESH_OTSU);

    //imshow("Threshold", thresh1);
    //waitKey();

    //sprintf( TrackbarName, "Threshold inferior", top_slider_max);

    //namedWindow("canny",1);

    //createTrackbar( TrackbarName, 
      //          "canny",
        //        &top_slider,
          //      top_slider_max,
            //    on_trackbar_canny);

    //on_trackbar_canny(top_slider, &top_slider);

    //imshow("Canny", border);

    if (index == 255) {
        index -= 100;
    }else {
        int dist = (255 - index) / 3;
        index += dist;
    }

    // Making Image binary
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols ; j++) {
            uchar value = 0;
            value = image.at<uchar>(i,j);

            if (value  <= index) {
                image.at<uchar>(i,j) = 0;
            } else {
                image.at<uchar>(i,j) = 255;
            }
        }
    }

    imshow("Binarization", image);
    //waitKey();

    image = dilateImage(image);
    //waitKey();

    Mat timg(image);

    medianBlur(image, timg, 3);

    imshow("MedianBlur", timg);

    if (saveImage(argv[1], timg)) {
        cout << "Saved Image" << endl;
    }

    waitKey();

    return 0;
}



