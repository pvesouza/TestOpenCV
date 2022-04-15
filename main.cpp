#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

using namespace std;

cv::Mat image, border;
int top_slider = 10;
int top_slider_max = 200;
char TrackbarName[50];

void savePoints(cv::Point *points, int number, char *path) {


}

void on_trackbar_canny(int, void*) {
  cv::Canny(image, border, top_slider, 3 * top_slider);
  cv::imshow("canny", border);
}

// Histogram Equalization
cv::Mat plotHistogram(cv::Mat image) {
    // Equalize histogram
    cv::Mat hist_equalized_image;
    cv::equalizeHist(image, hist_equalized_image);

    //Define names for windows
    cv::namedWindow("Histogram", cv::WINDOW_AUTOSIZE);
    cv::imshow("Histogram", hist_equalized_image);

    return hist_equalized_image;
}

int main(int argc, char** argv) {
    cv::Point p = {0,0};

    // Reads an image from arquive
    image = cv::imread(argv[1],cv::IMREAD_GRAYSCALE);

    if (image.empty()) {
        return 0;
    }

    // Saves the name of the arquive
    char c = '\0', name[100];
    uint8_t count = 0;

    while ((c != '.') && (count < 100)) {
        c = argv[1][count];
        name[count++] = c;
    }

    // Fills the name ".txt"
    name[count++] = '.';
    name[count++] = 't';
    name[count++] = 'x';
    name[count++] = 't';

    cv::imshow("image", image);

    //image = plotHistogram(image);

    sprintf( TrackbarName, "Threshold inferior", top_slider_max);

    cv::namedWindow("canny",1);

    cv::createTrackbar( TrackbarName, 
                "canny",
                &top_slider,
                top_slider_max,
                on_trackbar_canny);

    on_trackbar_canny(top_slider, &top_slider);

    cv::imwrite("cannyborders.png", border);

    cv::imshow("Canny", border);

    //Storing the histogram
    cv::Mat histogram;
    histogram = cv::Mat::zeros(1, 256, CV_8U);

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

    cv::imshow("Binarization", image);
    cv::waitKey();

    return 0;
}



