/*
 *  Test program SEEK Thermal CompactPRO
 *  Author: Maarten Vandersteegen
 */
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "seek.h"
#include <iostream>
#include <ros/ros.h>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>

using namespace std;

cv::Mat makebinary(cv::Mat image, int threshold) {
    cv::Mat result(image.size(), CV_8UC1);
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
            if (image.at<uchar>(i, j) > threshold)
                result.at<uchar>(i, j) = 255; //Make pixel white
            else
                result.at<uchar>(i, j) = 0;   //Make pixel black
        }
    }
    return result;
}

cv::Rect find_largest_region(cv::Mat imgOriginal, cv::Mat imgThresholded) {
/*
 * Input: imgOriginal is the source image which never been processed
 *        imgThresholded is binary image
 *
 * Output: the initial location of the danger marker
 * */

    cv::Mat bwImg;
    vector<vector<cv::Point>> contours;
    bwImg = imgThresholded;

    // 查找轮廓，对应连通域
    cv::findContours(bwImg, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

    // 寻找最大连通域
    double maxArea = 5000;
    vector<cv::Point> maxContour;
    std::vector<double> areas;
    for (size_t i = 0; i < contours.size(); i++) {
        double area = cv::contourArea(contours[i]);
        areas.push_back(area);

    }


    std::vector<double>::iterator biggest = std::max_element(std::begin(areas), std::end(areas));

    int index = std::distance(std::begin(areas), biggest);


    std::cout << "Max element is " << *biggest << " at position " << index << std::endl;

    maxContour = contours[index];


    // 将轮廓转为矩形框
    cv::Rect maxRect = cv::boundingRect(maxContour);
    std::cout << maxRect.x << " " << maxRect.y << " " << maxRect.height << " " << maxRect.width << std::endl;
    // 显示连通域
    cv::Mat result1, result2;

    bwImg.copyTo(result1);
    bwImg.copyTo(result2);

    cv::rectangle(imgOriginal, maxRect, cv::Scalar(0, 255, 0), 5);

    //std::cout<<maxRect. <<std::endl;

    cv::Mat image_roi = imgOriginal(maxRect);

    return maxRect;
}


int main(int argc, char **argv) {
    // seek image
    LibSeek::SeekThermalPro seek("");
    cv::Mat frame, gray_frame;
    cv::Mat image;
    string fname;

    // ros node
    ros::init(argc, argv, "seek_camera");
    ros::NodeHandle nh;
    image_transport::ImageTransport it(nh);
    image_transport::Publisher pub = it.advertise("seek_camera/image", 1);

    if (!seek.open()) {
        std::cout << "failed to open seek cam" << std::endl;
        return -1;
    }

    ros::Rate loop_rate(5);
    while (nh.ok()) {
        if (!seek.read(frame)) {
            std::cout << "no more LWIR img" << std::endl;
            return -1;
        }

        cv::normalize(frame, gray_frame, 0, 65535, cv::NORM_MINMAX);

        sensor_msgs::ImagePtr msg = cv_bridge::CvImage(std_msgs::Header(), "mono16", gray_frame).toImageMsg();

        cv_bridge::CvImagePtr cv_ptr;
        try {
            cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
        }
        catch (cv_bridge::Exception& e) {
            ROS_ERROR("cv_bridge exception: %s", e.what());
            return 0;
        }

        char szName[200] = {'\0'};

        cv::Mat gray;
        image = cv_ptr->image;

        int threshold = 121;
        cv::cvtColor(image, gray, CV_BGR2GRAY);

        cv::Mat binary = makebinary(gray, threshold);

        cv::Rect maxRect = find_largest_region(image, binary);

//        cv::imshow("image", image);
//        cv::imwrite("/home/kuanghf/workspace/Robotics/ROS/seek_ws/src/seek_camera_ros/seek_ros/test.jpg",image);
//        cv::waitKey(1);

        sensor_msgs::ImagePtr msg_detected = cv_bridge::CvImage(std_msgs::Header(), "bgr8", image).toImageMsg();

        double count = 0;
        double num = maxRect.width * maxRect.height;
        for (int i = maxRect.x; i < maxRect.x + maxRect.width; ++i)
        {
            for (int j = maxRect.y; j < maxRect.y + maxRect.height; ++j) {
               count += gray.at<uchar>(i, j);
            }
        }

        double mean_value = count / num;

        std::cout << mean_value << std::endl;

        if (mean_value > 115)
        {
            pub.publish(msg_detected);
        }

        ros::spinOnce();
        loop_rate.sleep();
    }
}
