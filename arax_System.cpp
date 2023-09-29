#include <opencv2/opencv.hpp>
#include "opencv2/highgui.hpp"
#include "opencv2/tracking.hpp"

#include <time.h>

#include <iostream>
#include <sstream>
#include <thread>
#include <string>

#include "/Users/adamkapsa/Documents/Python_bruh/Arax_tracking/serialib-master/lib/serialib.cpp"

#include "Serial.h"

std::chrono::steady_clock::time_point last_time_point;
std::chrono::steady_clock::time_point new_time_point;

cv::Mat image;

cv::VideoCapture cap("/Users/adamkapsa/Documents/Python_bruh/Arax_tracking/Dron_videa/jed.MP4");//"/Users/adamkapsa/Documents/all_desktop/dron_videa/tren05.mp4"

int width_trackbox = 20;

cv::Point left_top;
cv::Point right_down;

cv::Rect trackingBox;

bool track_turn_off = false;

std::string out = "";

int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

cv::Point center(width/2,height/2);

//-------------------------------------------------------------------------------------------------------------------------------
//Sending trough serial port

std::string comport = "/dev/cu.usbmodem101";
unsigned int baud = 115200;
std::vector<uint8_t> vector1;
serial::Serial port;

std::string message = "";

bool armed = false; //track(false = autoguided / true = manual) manual(false = disarmed / true = armed)

std::string mod = "";

//--------------------------------------------------------------------------------------------------------------------------------

int pom1_i = 0;
int pom2_i = 0;
int pom3_i = 0;
int pom4_i = 0;

int a = 2; //velikost zaměřovače v manuálu

int key;
bool key_off = false;

int fps = 0;

void Fps()
{
    using FpMilliseconds = std::chrono::duration<float, std::chrono::milliseconds::period>;
    float delta_time = std::chrono::duration_cast<FpMilliseconds>(new_time_point - last_time_point).count() * 0.001f;
    last_time_point = new_time_point;
    fps = 1000 / (delta_time * 1000);
}

void display()
{
    cv::imshow("Arax", image);
}

void serialout(cv::Rect trackingBox)
{
    serialib serial;
    const char *SERIAL_PORT = "/dev/cu.usbmodem101";

    // Connection to serial port
    char errorOpening = serial.openDevice(SERIAL_PORT, 115200);

    // If connection fails, return the error code otherwise, display a success message
    if (errorOpening != 1)
    {
        printf ("Successful connection to %s\n",SERIAL_PORT);
    }
    else
    {
        std::cout << "Error with opening port" << std::endl;
        return;
    }
    // Create an array of bytes
    unsigned char prime[8] = {1, 3, 5, 7, 11, 13, 17, 19};

    // Write the array on the serial device
    serial.writeBytes(prime, 8);
    printf ("Data sent\n");


    // vector1 = {};

    // pom3_i = trackingBox.x - width/2 + trackingBox.width/2;
    // pom4_i = height/2 - trackingBox.y - trackingBox.height/2;

    // vector1.push_back(pom3_i);
    // vector1.push_back(pom4_i);
    
	// if (port.isOpen()) 
    // {
    //     for(int j=0; j < int(vector1.size()); j++)
    //     {
    //         port.transmitAsync(vector1);

    //     }
    //     std::cout << "\33[2K\r" << pom3_i << " " << pom4_i << std::flush;
        
        // std::cout << "\33[2K\r" << "----Message sent----" << out << std::flush;
        // out = "";
	// }
}

void text(std::string mod, cv::Rect trackingBox)
{
    if(mod == "track")
    {  
        cv::rectangle(image, cv::Point(0, 60), cv::Point(260,270), CV_RGB(20,20,20),-1,cv::LINE_8);
        if(armed)//false = autoguided, true = tracking
        {
            cv::putText(image, "Mode: Tracking", cv::Point(6,100), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);
            cv::line(image, cv::Point(0,height/2), cv::Point(width,height/2), CV_RGB(255,255,255), 1, cv::LINE_8);
            cv::line(image, cv::Point(width/2,0), cv::Point(width/2,height), CV_RGB(255,255,255), 1, cv::LINE_8);
        }
        else
        {
            cv::putText(image, "Mode: Auto guided", cv::Point(6,100), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);
        }

        cv::putText(image, "Armed", cv::Point(6,160), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,0,0), 1);
        cv::putText(image, "Fps: " + std::to_string(fps), cv::Point(6,250), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);

        pom1_i = trackingBox.x - width/2 + trackingBox.width/2;
        pom2_i = height/2 - trackingBox.y - trackingBox.height/2;

        cv::line(image, cv::Point(trackingBox.x + trackingBox.width/2, trackingBox.y + trackingBox.height/2),cv::Point (trackingBox.x + trackingBox.width/2, trackingBox.y + trackingBox.height/2) , CV_RGB(255,0,0), 3, cv::LINE_8);

        cv::putText(image, "Offset X:" + std::to_string(pom1_i), cv::Point(6,190), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);
        cv::putText(image, "Offset Y:" + std::to_string(pom2_i), cv::Point(6,220), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);
    }

    if(mod == "manuald")
    {
        cv::rectangle(image, cv::Point(0, 60), cv::Point(200,190), CV_RGB(20,20,20),-1,cv::LINE_8);
        cv::putText(image, "Mode: Manual", cv::Point(6,100), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);
        cv::putText(image, "Disarmed", cv::Point(6,130), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(0,255,0), 2);
        cv::rectangle(image, cv::Point(width/2-a,height/2-a), cv::Point(width/2+a,height/2+a), CV_RGB(0,255,0), 1, cv::LINE_8);
        cv::putText(image, "Fps: " + std::to_string(fps), cv::Point(6,160), cv::FONT_HERSHEY_SIMPLEX, 1.3, CV_RGB(255,255,255), 1);
    }

    else if(mod == "manuala")
    {
        cv::rectangle(image, cv::Point(0, 60), cv::Point(200,190), CV_RGB(20,20,20),-1,cv::LINE_8);
        cv::putText(image, "Mode: Manual", cv::Point(6,100), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);
        cv::putText(image, "Armed", cv::Point(6,130), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,0,0), 2);
        cv::rectangle(image, cv::Point(width/2-a,height/2-a), cv::Point(width/2+a,height/2+a), CV_RGB(255,0,0), 1, cv::LINE_8);
        cv::putText(image, "Fps: " + std::to_string(fps), cv::Point(6,160), cv::FONT_HERSHEY_SIMPLEX, 1.3, CV_RGB(255,255,255), 1);
    }
}

void manual(std::string& mod)
{
    while(true)
    {
        last_time_point = std::chrono::steady_clock::now();

        cap >> image;

        cv::line(image, cv::Point(0,height/2),cv::Point(width,height/2) , CV_RGB(255,255,255), 1, cv::LINE_8);
        cv::line(image, cv::Point(width/2,0),cv::Point(width/2,height) , CV_RGB(255,255,255), 1, cv::LINE_8);
        try
        {
            key = cv::waitKey(1);
        }
        catch(...)
        {
            std::cout << "key error" << std::endl;
        }
        
        if(key == 97)
        {
            if(armed == false)
            {
                mod = "manuald";
                armed = true;
            }

            else if(armed == true)
            {
                mod = "manuala";
                armed = false;
            }
        }

        if(key == 109)
        {
            cv::destroyAllWindows();
            return;
        }
        if(key == 27)
        {
            key_off = true;
            return;
        }
        text(mod, trackingBox);
        display();

        new_time_point = std::chrono::steady_clock::now();
        Fps();
    }
}

void track_update(cv::Mat image, cv::Rect& trackingBox, cv::Ptr<cv::Tracker> tracker, bool& track_turn_off)
{
    if(tracker->update(image, trackingBox))
    {
		cv::rectangle(image, trackingBox, CV_RGB(255, 0, 0), 2, 8);
	}
    else
    {
        track_turn_off = true;
    }
    //std::cout << trackingBox << std::endl;
}

void track(cv::Rect& trackingBox, std::string& mod)
{
    cv::Ptr<cv::Tracker> tracker = cv::TrackerCSRT::create();

    tracker->init(image, trackingBox);

    while (true) 
    {
        last_time_point = std::chrono::steady_clock::now();

        cap >> image;

        std::thread t1(text, mod, trackingBox);
        std::thread t2(serialout, trackingBox);
        std::thread t3(track_update, image, std::ref(trackingBox), tracker, std::ref(track_turn_off));

        t3.join();
        t2.join();
        t1.join();

        display();

        try
        {
            key = cv::waitKey(1);
        }
        catch(...)
        {
            std::cout << "key error" << std::endl;
        }

        if(track_turn_off)
        {
            track_turn_off = false;
            cv::destroyAllWindows();
            return;
        }

        if(key == 97)
        {
            if(armed == false)
            {
                armed = true;
            }

            else if(armed == true)
            {
                armed = false;
            }
        }

        if(key == 116)
        {
            cv::destroyAllWindows();
            return;
        }

        if(key == 27)
        {
            key_off = true;
            return;
        }
        new_time_point = std::chrono::steady_clock::now();
        Fps();
	}
}

void track_box_mouse_movement(int event, int x, int y, int flags, void* userdata)
{
    key = cv::waitKey(1);
    if(event == cv::EVENT_MOUSEMOVE && key == 101)
    {
        center.x = x;
        center.y = y;
    }
    if(key == 116)
    {
        mod = "track";
        track(trackingBox, mod);
    }
}

int main() 
{
    try
    {
        port.open(comport, baud);
    }
    catch(...)
    {
        std::cout << "Port nelze otevřít" << std::endl;
    }

    while (true)
    {   
        last_time_point = std::chrono::steady_clock::now();

        text(mod,trackingBox);

        left_top.x = center.x - width_trackbox;
        left_top.y = center.y - width_trackbox;

        right_down.x = width_trackbox + center.x;
        right_down.y = width_trackbox + center.y;

        cap >> image;
        
        cv::Rect trackingBox(left_top, right_down);

        cv::rectangle(image, trackingBox, cv::Scalar(255, 255, 255),2, 8);

        cv::line(image, cv::Point (trackingBox.x + trackingBox.width/2, trackingBox.y + trackingBox.height/2),cv::Point (trackingBox.x + trackingBox.width/2, trackingBox.y + trackingBox.height/2) , CV_RGB(255,0,0), 5, cv::LINE_8);

        cv::rectangle(image, cv::Point(0,70), cv::Point(120,110), CV_RGB(20,20,20),-1,cv::LINE_8);
        cv::putText(image, "Fps: " + std::to_string(fps), cv::Point(6,100), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);

        display();

        try
        {
            key = cv::waitKey(1);
        }
        catch(...)
        {
            std::cout << "key error" << std::endl;
        }

        if(key == 27)
        {
            key_off = true;
        }

        if(key == 233)
        {
            if(width_trackbox < 150)
            {
                width_trackbox += 10;
            }
            else
            {
                std::cout << "nemůžeš zvětšovat" << std::endl; 
            }
        }
        
        if(key == 61)
        {
            if(width_trackbox > 20)
            {
                width_trackbox -= 10;
            }
            else
            {
                std::cout << "nemůžeš zmenšovat" << std::endl;
            }
        }

        if(key == 1)
        {
            right_down.y += 10;
            left_top.y += 10;
            center.y += 10;
        }

        if(key == 0)
        {
            right_down.y -= 10;
            left_top.y -= 10;
            center.y -= 10;
        }

        if(key == 3)
        {
            right_down.x += 10;
            left_top.x += 10;
            center.x += 10;
        }

        if(key == 2)
        {
            right_down.x -= 10;
            left_top.x -= 10;
            center.x -= 10;
        }
        
        if(key == 116)
        {   
            mod = "track";
            track(trackingBox, mod);
        }

        if(key == 101)
        {
            cv::setMouseCallback("Arax", track_box_mouse_movement, NULL);
        }
    
        
        if(key == 109)
        {
            mod = "manuald";
            manual(mod);
        }

        if(key_off)
        {
            cv::destroyAllWindows();
            cap.release();
            port.close();
            return 0;
        }

        new_time_point = std::chrono::steady_clock::now();
        Fps();
    }
}

// // left top corner of monitor [0,0]
// // right down corner of monitor [1200,650]
// //'/dev/cu.usbmodem101'
// //message looks " * message @ message2 # "