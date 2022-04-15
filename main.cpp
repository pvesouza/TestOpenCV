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

void savePoints(Point *points, int number, char *path) {
    FileHelper helper;
        // Saves the name of the arquive
    char c = '\0', name[100];
    uint8_t count = 0;

    while ((c != '.') && (count < 100)) {
        c = path[count];
        name[count++] = c;
    }

    // Fills the name ".txt"
    name[count++] = 't';
    name[count++] = 'x';
    name[count++] = 't';
    
    helper.saveFile(points, number, name);
}

void on_trackbar_canny(int, void*) {
    Canny(image, border, top_slider, 3 * top_slider);
    imshow("canny", border);
}

// Histogram Equalization Function
Mat plotHistogram(cv::Mat image) {
    // Equalize histogram
    Mat hist_equalized_image;
    equalizeHist(image, hist_equalized_image);

    //Define names for windows
    namedWindow("Histogram", cv::WINDOW_AUTOSIZE);
    imshow("Histogram", hist_equalized_image);

    return hist_equalized_image;
}

int main(int argc, char** argv) {

    Point rectangleCorners[4] = {{10,0}, {20,0}, {10,200}, {20,200}};

    // Reads an image from arquive
    image = cv::imread(argv[1],cv::IMREAD_GRAYSCALE);

    if (image.empty()) {
        return 1;
    }

    savePoints(rectangleCorners, 4, argv[1]);

    cv::imshow("image", image);

    sprintf( TrackbarName, "Threshold inferior", top_slider_max);

    namedWindow("canny",1);

    createTrackbar( TrackbarName, 
                "canny",
                &top_slider,
                top_slider_max,
                on_trackbar_canny);

    on_trackbar_canny(top_slider, &top_slider);

    imwrite("cannyborders.png", border);

    imshow("Canny", border);

    //Storing the histogram
    Mat histogram;
    histogram = Mat::zeros(1, 256, CV_8U);

    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
            uchar pixelValue = image.at<uchar>(i,j);
            // Incrementa a quantidade daquela posição
            histogram.at<uchar>(1, pixelValue) += 1;
        }
    }

    //Calculando a média ponderada dos pixels pelo histograma
    int sum = 0;
    int index = 0;
    for (int i = 0; i < histogram.cols; i++) {
        if (sum < histogram.at<uchar>(0,i)) {
            sum = histogram.at<uchar>(0,i);
            index = i;
        }
    }

    // Making Image binary
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < border.cols ; j++) {
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
    waitKey();

    return 0;
}



