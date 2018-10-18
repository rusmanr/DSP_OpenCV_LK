#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <math.h>
#include <iostream>

using namespace cv;
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
//    setWindowTitle(tr("DSP - Demo - HoughLine"));
    // Set the widget to be a scaled content widget
//        ui->label1  ->setScaledContents(true);
//        ui->label2  ->setScaledContents(true);
//        ui->label3  ->setScaledContents(true);
//        ui->label4  ->setScaledContents(true);
//        ui->label5  ->setScaledContents(true);
//        ui->label6  ->setScaledContents(true);
        ui->display1->setScaledContents(true);
        ui->display2->setScaledContents(true);
        ui->display3->setScaledContents(true);
        ui->display4->setScaledContents(true);
        ui->display5->setScaledContents(true);
        ui->display6->setScaledContents(true);

        // Set size of the widget
//        ui->label1  ->setFixedSize( QLWidth, 15 );
//        ui->label2  ->setFixedSize( QLWidth, 15 );
//        ui->label3  ->setFixedSize( QLWidth, 15 );
//        ui->label4  ->setFixedSize( QLWidth, 15 );
//        ui->label5  ->setFixedSize( QLWidth, 15 );
//        ui->label6  ->setFixedSize( QLWidth, 15 );
        ui->display1->setFixedSize( QLWidth, QLHeight );
        ui->display2->setFixedSize( QLWidth, QLHeight );
        ui->display3->setFixedSize( QLWidth, QLHeight );
        ui->display4->setFixedSize( QLWidth, QLHeight );
        ui->display5->setFixedSize( QLWidth, QLHeight );
        ui->display6->setFixedSize( QLWidth, QLHeight );

    cap.open(0);

    // Call update every 100ms
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(100);

}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::update(){
    //************************************************************************************* Capture image

    // Capture image from camera, return if fail to load the camera
    TermCriteria termcrit(TermCriteria::COUNT|TermCriteria::EPS,20,0.03);
    Size subPixWinSize(10,10), winSize(31,31);

    if( !cap.isOpened() ) return;

    cap >> capture;

    if(!BGFlag)
    {
        capture.copyTo(BackGround);
        BGFlag=true;
    }

    if (ui->BackGroundButtom->isChecked())
        absdiff(capture, BackGround, capture);

    //************************************************************************************** Preprocessor

    // Camera calibration, make the image undistort.

    // Blur the image so that edges detected by Canny due to noises will reduced
    medianBlur(capture, smooth, 5);
    for( int i = 0; i < 10; i++)
        medianBlur(smooth, smooth, 5);


    // Main Algorithm ********************************************

    //contour detection
    capture.copyTo(img);

    cvtColor(img, gray2, COLOR_BGR2GRAY);
    Canny(gray2, edge2, 50, 100, 3);
    findContours( edge2, contours0, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    contours.resize(contours0.size());
    for( size_t k = 0; k < contours0.size(); k++ )
        approxPolyDP(Mat(contours0[k]), contours[k], 3, true);

    Mat cnt_img = Mat::zeros(w, w, CV_8UC3);
    int _levels = levels - 3;
    drawContours( cnt_img, contours, _levels <= 0 ? 3 : -1, Scalar(128,255,255),
                  3, LINE_AA, hierarchy, std::abs(_levels) );

    //lk detection
    capture.copyTo(image);
    cvtColor(image, gray, COLOR_BGR2GRAY);

    if( nightMode )
    image = Scalar::all(0);

    if( needToInit)
    {
    // automatic initialization
    goodFeaturesToTrack(gray, points[1], MAX_COUNT, 0.01, 10, Mat(), 3, 0, 0.04);
    cornerSubPix(gray, points[1], subPixWinSize, Size(-1,-1), termcrit);
    addRemovePt = false;
    }

    else if( !points[0].empty() )
    {
    vector<uchar> status;
    vector<float> err;
    if(prevGray.empty())
    gray.copyTo(prevGray);
    calcOpticalFlowPyrLK(prevGray, gray, points[0], points[1], status, err, winSize,3, termcrit, 0, 0.001);
    size_t i, k;

              for( i = k = 0; i < points[1].size(); i++ )
              {
                 if( addRemovePt )
                 {
                     if( norm(point - points[1][i]) <= 5 )
                     {
                     addRemovePt = false;
                     continue;
                     }
                 }

                 if( !status[i] )
                 continue;

                 points[1][k++] = points[1][i];
                 circle( image, points[1][i], 3, Scalar(0,255,0), -1, 8);
               }

     points[1].resize(k);
     }

            qDebug() << points[1].size();
            qDebug() << (size_t)MAX_COUNT;
            qDebug() << addRemovePt ;
            if( addRemovePt && points[1].size() < (size_t)MAX_COUNT )
            {

                vector<Point2f> tmp;
                tmp.push_back(point);
                cornerSubPix( gray, tmp, winSize, Size(-1,-1), termcrit);
                points[1].push_back(tmp[0]);
                addRemovePt = false;
                qDebug() << "okokok";
            }

            needToInit = false;
         //  imshow("LK Demo", image);

         //   char c = (char)waitKey(10);
//            if( c == 27 )
//                break;
//            switch( c )
//            {
//            case 'r':
//                needToInit = true;
//                break;
//            case 'c':
//                points[0].clear();
//                points[1].clear();
//                break;
//            case 'n':
//                nightMode = !nightMode;
//                break;
//            }

            std::swap(points[1], points[0]);
            cv::swap(prevGray, gray);


    // Edge detection
    Canny(smooth, edge, 50, 100, 3);

    // Hough Line
    std::vector<Vec4i> lines;
    HoughLinesP(edge, lines, 1, CV_PI/180, 100, 50, 10 );

    // Draw Hough Line
    capture.copyTo( hline );
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        line( hline, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 1, CV_AA);
    }

    // Search red line from all detected line
    std::vector<Vec4i> redLine;
//    for( unsigned int lineIdx = 0; lineIdx < lines.size(); lineIdx++ ){
//        int total = 0, x, y;

//        // calculate color of 2 pixel of each line
//        x = lines[lineIdx][0];
//        y = lines[lineIdx][1];
//        pixel = capture.at<int>( x,y );
//        total += pixel[2] - pixel[1] - pixel[0];

//        x = lines[lineIdx][2];
//        y = lines[lineIdx][3];
//        pixel = capture.at<int>( x,y );
//        total += pixel[2] - pixel[1] - pixel[0];

//        // if it is red, copy the coordinate to redLine
//        if( total > 200 )
//            redLine.push_back( lines[lineIdx] );
//    }

    // Draw Red Lines
    capture.copyTo(hline2);
    for( size_t i = 0; i < redLine.size(); i++ ){
        Vec4i l = redLine[i];
        line( hline2, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255,0,0), 2, CV_AA);
    }

    // Convert cv::Mat to QImage
    QImage img1 = MatToQImage( capture );
    QImage img2 = MatToQImage( image  );
    QImage img3 = MatToQImage( smooth    );
    QImage img4 = MatToQImage( edge   );
    QImage img5 = MatToQImage( hline  );
    QImage img6 = MatToQImage( cnt_img );

    // Display QImage
    ui->display1->setPixmap(QPixmap::fromImage(img1));
    ui->display4->setPixmap(QPixmap::fromImage(img4));
    ui->display2->setPixmap(QPixmap::fromImage(img2));
    ui->display3->setPixmap(QPixmap::fromImage(img3));
    ui->display5->setPixmap(QPixmap::fromImage(img5));
    ui->display6->setPixmap(QPixmap::fromImage(img6));
}

void MainWindow::on_BackGroundButtom_toggled(bool checked)
{
    BGFlag=checked;
}

void MainWindow::on_pushButton_clicked()
{
    needToInit=true;
}

void MainWindow::on_Nightmode_clicked()
{
    nightMode=true;
}
