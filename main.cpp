#include <iostream>
#include <cstring>
#include <cstdio>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <cv.h>

using namespace std;
using namespace cv;

unsigned long GetTickCount()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC,&ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec/(1000*1000) );
}

// shell command
string ShellCmd(const string& cmd)
{
    FILE* ptr;
    char bufPs[1024];
    char result[1024];
    if ((ptr=popen(cmd.c_str(), "r")) != nullptr)
    {
        while(fgets(bufPs, 1024, ptr) != NULL)
        {
            strcat(result, bufPs);
            if(strlen(result) > 1024)
                break;
        }
        pclose(ptr);
        ptr = nullptr;
    }
    else
    {
        cout << "popen error\n" << endl;
    }
    string strResult(result);
    return strResult;
}

// excute shell command like "adb shell input swipe 250 250 250 250 {ms}"
void PressScreen(int ms)
{
    string cmd =  "adb shell input swipe 250 250 250 250 ";
    char str[10];
    sprintf(str, "%d", ms);
    string msStr(str);
    cmd = cmd + msStr;
    auto res = ShellCmd(cmd);
    // cout << res;
}

// get screen shot
void GetScreenShot(Mat& screenShot)
{
    // save screen shot in mobile phone's sdcard.
    string savePng = R"(adb shell /system/bin/screencap -p /sdcard/screenshot.png)";
    auto res = ShellCmd(savePng);
    // pull screen shot from cdcard into current dir.
    string pullPng = R"(adb pull /sdcard/screenshot.png ../screenshot.png)";
    res = ShellCmd(pullPng);
    screenShot = imread(R"(../screenshot.png)");
}

// get body x coordinate
int GetBodyXPos(Mat& screenShot)
{
    int LowerH = 106, HigherH = 136, LowerS = 27, HigherS = 122, LowerV = 46, HigherV = 137;
    Mat roiImage = screenShot(Range(0.5*screenShot.rows, 0.7*screenShot.rows), Range(0, 0.8* screenShot.cols));
    Mat hsvimage, hsvTuple[3], h, s, v, hL, sL, vL, hH, sH, vH;
    cvtColor(roiImage, hsvimage, CV_BGR2HSV);
    split(hsvimage, hsvTuple);
    threshold(hsvTuple[0], hL, LowerH, 255, THRESH_BINARY);
    threshold(hsvTuple[0], hH, HigherH, 255, THRESH_BINARY_INV);
    bitwise_and(hL, hH, h);
    threshold(hsvTuple[1], sL, LowerS, 255, THRESH_BINARY);
    threshold(hsvTuple[1], sH, HigherS, 255, THRESH_BINARY_INV);
    bitwise_and(sL, sH, s);
    threshold(hsvTuple[2], vL, LowerV, 255, THRESH_BINARY);
    threshold(hsvTuple[2], vH, HigherV, 255, THRESH_BINARY_INV);
    bitwise_and(vL, vH, v);

    Mat tmp, result;
    bitwise_and(h, s, tmp);
    bitwise_and(tmp, v, result);


    // dilate to make the person be a whole area.
    Mat dilateKernel = getStructuringElement(MORPH_ELLIPSE,Size(5,5));
    morphologyEx(result, result, MORPH_OPEN, dilateKernel);
    //imshow("src", result);
    // find the person's contours
    vector<vector<Point>> contours;
    findContours(result,contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    int idx = 0;
    if(contours.size() != 1)
    {
        for(auto i = 1; i < contours.size(); i++)
        {
            if(contours[i].size() > contours[idx].size())
                idx = i;
        }
    }
    // fit the contours by rect.
    RotatedRect rc = minAreaRect(contours[idx]);
    Rect br = rc.boundingRect();
    int posX, posY;
    posX = br.x + br.width / 2;
    posY = br.y + br.height - br.height / 6;
    circle(screenShot, Point(posX, posY), 5, Scalar(0, 0, 255), -1);
    return posX;
}

// get square x coordinate
int GetSquarePos(Mat& screenShot, int bodyX)
{
    bool isBodyLeft = bodyX < screenShot.cols / 2 ? true : false;
    Mat roiImage;
    if(isBodyLeft)
        roiImage = screenShot(Range(0.2*screenShot.rows, 0.6*screenShot.rows), Range(screenShot.cols / 2, screenShot.cols));
    else
        roiImage = screenShot(Range(0.2*screenShot.rows, 0.6*screenShot.rows), Range(0, screenShot.cols / 2));
    Mat hsvImage,  result;
    cvtColor(roiImage, hsvImage, CV_BGR2HSV);

    GaussianBlur(hsvImage, result, Size(5,5), 1);
    Canny(result, result, 20, 120);
    threshold(result, result, 12, 255, THRESH_BINARY);

//    Mat dilateKernel = getStructuringElement(MORPH_RECT,Size(3, 3));
//    dilate(tmp2, result, dilateKernel);

    Point squarePos;
    for(auto it = result.begin<uchar>(); it != result.end<uchar>(); ++ it)
    {
        if ((*it) > 0)
        {
            squarePos = it.pos();
            break;
        }
    }

    //imshow("Sobel", result);
    if(isBodyLeft)
        return squarePos.x + screenShot.cols / 2;
    return squarePos.x;
}

int main() {
    Mat screenShot;
    double ratio = 4.9;
    int bodyX, squareX;
    int delay = 0;
    long t0, t1, t2, t3 ;
    while(true)
    {
        t0 = GetTickCount();
        GetScreenShot(screenShot);
        resize(screenShot, screenShot, Size(), 0.5, 0.5);
        t1 = GetTickCount();
        cout << "screenShot:" << t1 - t0<< endl;
        bodyX = GetBodyXPos(screenShot);
        t2 = GetTickCount();
        cout << "body:" << (t2-t1) << endl;
        squareX = GetSquarePos(screenShot, bodyX);
        t3 = GetTickCount();
        cout << "square:" << (t3-t2)  << endl;
        imshow("Screen", screenShot);
        delay = (int)(ratio * abs(squareX - bodyX));
        PressScreen(delay);
        // to make image stable(avoid get the image when little man is still in air )
        waitKey(delay + 600);
    }




}