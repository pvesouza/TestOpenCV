#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

#include "fileHelper.h"

using namespace std;
using namespace cv;

cv::Mat image, border;
int top_slider = 10;
int top_slider_max = 200;
char TrackbarName[50];

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

// Trackbar for Canny border algorithm
void on_trackbar_canny(int, void*) {
    Canny(image, border, top_slider, 3 * top_slider);
    imshow("canny", border);
}

// Histogram Equalization Function
Mat histogramEqualize(cv::Mat image) {
    // Equalize histogram
    Mat hist_equalized_image;
    equalizeHist(image, hist_equalized_image);

    //Define names for windows
    namedWindow("Histogram", cv::WINDOW_AUTOSIZE);
    imshow("Histogram", hist_equalized_image);

    return hist_equalized_image;
}

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

    imshow("histogram calculated", histImage);

    return grayHistogram;
}

Mat dilateImage(const Mat &image) {

    int dilation_type = MORPH_RECT;
    int dilation_size = 3;
    Mat dilation_dst;

    Mat element = getStructuringElement( 
            dilation_type,
            Size( 2*dilation_size + 1, 2*dilation_size+1 ),
            Point( dilation_size, dilation_size ) );

  dilate( image, dilation_dst, element );

  imshow( "Dilation Demo", dilation_dst );
  return dilation_dst;
}

//***********************************************************************
/////////MAIN
//***********************************************************************

int main(int argc, char** argv) {

    Point rectangleCorners[4] = {{10,0}, {20,0}, {10,200}, {20,200}};
    // Reads an image from arquive
    image = cv::imread(argv[1],cv::IMREAD_GRAYSCALE);

    if (image.empty()) {
        return EXIT_FAILURE;
    }

    savePoints(rectangleCorners, 4, argv[1]);
    imshow("image", image);

    Mat hist = plotHistogram(image);


    float great = 0.0;
    int index = 0;

    for (int j = 0; j < hist.rows; j++) {
        cout << "(" << j << "," << 0 << ") = " << hist.at<float>(j,0) << endl;
        if (great < hist.at<float>(j,0)) {
            great = hist.at<float>(j,0);
            index = j;
        }
    }

    cout << "index: " << index << endl;
    cout << "maior: " << great << endl;

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

    image = dilateImage(image);

    //border = dilateImage(border);

    waitKey();

    return 0;
}



