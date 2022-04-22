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

#define REC_MIN_PERIMETER 500

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

    //cout << "Filename: " << name << endl;

    return imwrite(name, image);
}

// Trackbar for Canny border algorithm
void on_trackbar_canny(int, void*) {
    Canny(image, border, top_slider, 3 * top_slider);
    imshow("canny", border);
    //waitKey();
}

// Histogram Equalization Function
Mat histogramEqualize(Mat image) {
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
void drawSquare(Mat &image, Point *points) {

    for (int i = 0; i < 4; i++) {
        if (i != 3) {
            line(image, *(points + i), *(points + i + 1), Scalar(128,255,255),3);
        }else {
            line(image, *(points + i), *(points), Scalar(128,255,255),3);
        }
    }

    imshow("Squares", image);
}

void _drawContours(Mat image, vector< vector<Point> > contours, vector<Vec4i> hierarchy) {
    int levels = -1;
    Mat dest = Mat::zeros(image.rows, image.cols, CV_8UC3);

    drawContours(dest, contours, levels, Scalar(128,255,255), 2, LINE_AA);
    namedWindow("contours", WINDOW_AUTOSIZE);
    imshow("contours", dest);
}

// Get rectangle points
void getRectanglePoints(Mat &image, Point *pt_out) {
   
    vector<Point> contours;
    vector<Vec4i> hierarchy_out;
    vector<Vec4i> hierarchy;                //[Next, Previous, First_Child, Parent]
    vector<vector<Point>> contoursSet;
    vector<vector<Point>> contoursOut;

    findContours(image, contoursSet, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    size_t numberOfContours = contoursSet.size();
    //cout << "Size: " << numberOfContours << endl;

    for (size_t k = 0; k < numberOfContours; k++) { 
        double perimeter = arcLength(contoursSet[k], true);

        approxPolyDP(Mat(contoursSet[k]), contours, perimeter * 0.02, true);

        // Testa a quantidade de lados de contoursOut[k]
        if (contours.size() == 4) {
            double perimeterRec = arcLength(contours, true);

            if (perimeterRec >= REC_MIN_PERIMETER) {
                contoursOut.push_back(contours);
                hierarchy_out.push_back(hierarchy[k]);

                //cout << "PR = " << perimeterRec << endl;
                //for (int h = 0; h < 4; h++) {
                    //cout << "Hierarchy: " << hierarchy[k][h] << " Pos: " << k << endl;
                //}
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
            // Retrieves the Points and corrects the rectangle
            points_vector = contoursOut[0];

            point[0] = {(points_vector[0].x + points_vector[1].x) / 2, (points_vector[0].y + points_vector[3].y) / 2};
            point[1] = {(points_vector[0].x + points_vector[1].x) / 2, (points_vector[1].y + points_vector[2].y) / 2};
            point[2] = {(points_vector[2].x + points_vector[3].x) / 2, (points_vector[1].y + points_vector[2].y) / 2};
            point[3] = {(points_vector[2].x + points_vector[3].x) / 2, (points_vector[0].y + points_vector[3].y) / 2};

            for (int i = 0; i < points_vector.size(); i++) {
                *(pt_out + i) = point[i];
                //cout << "(" << point[i].x << "," << point[i].y << ")" << endl;
            }

        } else {

            double greatest = 0;
            int indexGreatest = 0;

            for (int i = 0; i < contoursOut.size(); i++) {

                double perimeterRec1 = arcLength(contoursOut[i], true);
                //cout << "Perimeter: " << perimeterRec1 << endl;

                // Discards the edge of the image
                if (hierarchy_out[i][0] == -1 && hierarchy_out[i][1] == -1 && hierarchy_out[i][2] != -1 && hierarchy_out[i][3] == -1) {

                }else {
                    if (greatest < perimeterRec1) {
                        greatest = perimeterRec1;
                        indexGreatest = i;
                    }
                }

                for (int h = 0; h < 4; h++) {
                    //cout << "Hierarchy: " << hierarchy_out[i][h] << " Pos: " << i << endl;
                }
            }

            points_vector = contoursOut[indexGreatest];

            point[0] = {(points_vector[0].x + points_vector[1].x) / 2, (points_vector[0].y + points_vector[3].y) / 2};
            point[1] = {(points_vector[0].x + points_vector[1].x) / 2, (points_vector[1].y + points_vector[2].y) / 2};
            point[2] = {(points_vector[2].x + points_vector[3].x) / 2, (points_vector[1].y + points_vector[2].y) / 2};
            point[3] = {(points_vector[2].x + points_vector[3].x) / 2, (points_vector[0].y + points_vector[3].y) / 2};

            for (int i = 0; i < points_vector.size(); i++) {
                *(pt_out + i) = point[i];
                //cout << "(" << point[i].x << "," << point[i].y << ")" << endl;
            }
        }
    }else {
        // Fills with zeros 
            for (int i = 0; i < 4; i++) {
                *(pt_out + i) = {0,0};
            }
        //cout << "Sem quadriláteros" << endl;
        return ;
    }
           
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

    // Limiarization
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

    // Median Filter to remove noise
    Mat timg(image);
    medianBlur(image, timg, 3);
    imshow("MedianBlur", timg);
    
    // Saving rectangle's points
    Point pointsToSave[4];
    getRectanglePoints(timg, pointsToSave);
    savePoints(pointsToSave, 4, argv[1]);

    drawSquare(timg, pointsToSave);

    if (saveImage(argv[1], timg)) {
        //cout << "Saved Image" << endl;
    }

    waitKey();

    return 0;
}



