/*
 *  Test program SEEK Thermal CompactPRO
 *  Author: Maarten Vandersteegen
 */
#include <opencv2/highgui/highgui.hpp>
#include "seek.h"
#include <iostream>
#include <ros/ros.h>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>


int main(int argc, char** argv)
{
    // seek image
    LibSeek::SeekThermalPro seek("");
    cv::Mat frame, gray_frame;

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
    while(nh.ok()) {
        if (!seek.read(frame)) {
            std::cout << "no more LWIR img" << std::endl;
            return -1;
        }

        cv::normalize(frame, gray_frame, 0, 65535, cv::NORM_MINMAX);
        //cv::GaussianBlur(grey_frame, grey_frame, cv::Size(7,7), 0);

        sensor_msgs::ImagePtr msg = cv_bridge::CvImage(std_msgs::Header(), "mono16", gray_frame).toImageMsg();

//        ros::Rate loop_rate(5);
//        while (nh.ok())
//        {
//            pub.publish(msg);
//            ros::spinOnce();
//            loop_rate.sleep();
//        }
        pub.publish(msg);
        ros::spinOnce();
        loop_rate.sleep();
    }
}