#include <opencv2/opencv.hpp>
#include "opencv2/highgui.hpp"
#include "opencv2/tracking.hpp"

#include <time.h>

#include <iostream>
#include <sstream>
#include <thread>
#include <string>
#include <unistd.h>

#include "Serial.h"

std::chrono::steady_clock::time_point last_time_point;
std::chrono::steady_clock::time_point new_time_point;

cv::Mat image;
cv::Mat imageToShow;

cv::VideoCapture cap;

int width_trackbox = 20;

cv::Point left_top;
cv::Point right_down;

cv::Rect trackingBox;

bool track_turn_off = false;

int width;
int height;

cv::Point center;

//-------------------------------------------------------------------------------------------------------------------------------
//Sending trough serial port + tolerance setings
std::vector<uint8_t> vector1;
serial::Serial port;

bool armed = false; //track(false = autoguided / true = manual) manual(false = disarmed / true = armed)
bool manual_auto = false;

long in_min = 0; long out_min = 0; long out_max = 180;
int smerovka = 90; int zadni = 90; int predni_r = 90; int predni_l = 90;

int tolerance = 2; //tolerance of offset in degrees

std::string mod = "";
//--------------------------------------------------------------------------------------------------------------------------------

int pom_x = 0;
int pom_y = 0;

int key;
bool key_off = false;

int fps = 0;
float delta_time;

void Fps()
{
    new_time_point = std::chrono::steady_clock::now();
    using FpMilliseconds = std::chrono::duration<float, std::chrono::milliseconds::period>;
    delta_time = std::chrono::duration_cast<FpMilliseconds>(new_time_point - last_time_point).count() * 0.001f;
    last_time_point = new_time_point;
    fps = 1000 / (delta_time * 1000);
}

void display()
{
    if(height > 480)
    {
        cv::resize(image, imageToShow, cv::Size(width*2,height*2));
    }
    else if(height <= 480)
    {
        cv::resize(image, imageToShow, cv::Size(width*3,height*3));
    }
    else
    {
        imageToShow = image;
    }
    cv::imshow("Arax", imageToShow);
}

void serialout(cv::Rect trackingBox)
{
    vector1 = {};

    if(mod == "track")
    {
        pom_x = trackingBox.x + trackingBox.width/2;
        pom_y = trackingBox.y - trackingBox.height/2;

        //kód pro převod na úhly(čísla od O - 240)
        //pom_x = odchylka od středu (x)
        //pom_y = odchylka od středu (y)

        if(port.isOpen()) 
        {
            //0,1023,13,180
            //(x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

            smerovka = (pom_x - in_min) * (out_max - out_min) / (2160 - in_min) + out_min;
            zadni = (pom_y - in_min) * (out_max - out_min) / (1440 - in_min) + out_min;

            vector1.push_back(255);//start
            if(smerovka > 90 + tolerance || smerovka < 90 - tolerance)
            {
                vector1.push_back(smerovka);//servo 1
            }
            else
            {
                vector1.push_back(90);//servo 1   
            }
            if(smerovka > 90 + tolerance || smerovka < 90 - tolerance)
            {
                vector1.push_back(zadni);//servo 2
            }
            else
            {
                vector1.push_back(90);//servo 2
            }
            vector1.push_back(predni_l);//servo 3
            vector1.push_back(predni_r);//servo 4
            vector1.push_back(manual_auto); //manual_auto = 1,0 (track is guided or just tracking)
            vector1.push_back(armed);//armed = 1,0
            vector1.push_back(253);//mod = track = 253
            vector1.push_back(254);//end

            for(int j=0; j < int(vector1.size()); j++)
            {
                port.transmitAsync(vector1);
            }   
            std::cout << "\33[2K\r" << std::to_string(vector1[1]) << " s1 " << std::to_string(vector1[2]) << " s2 " << std::to_string(vector1[3]) << " s3 " << std::to_string(vector1[4]) << " s4 " << std::to_string(vector1[5]) << " autoguided or just tracking " << std::to_string(vector1[6]) << " armed " << std::to_string(vector1[7]) << " mod --- message sent ---" << std::flush;          
        }
    }
    if(mod == "manual")
    {
        if(port.isOpen()) 
        {
            vector1.push_back(255);//start
            vector1.push_back(1);//servo1
            vector1.push_back(2);//servo2
            vector1.push_back(3);//servo3
            vector1.push_back(4);//servo4
            vector1.push_back(250);//free
            vector1.push_back(armed);//armed = 1,0
            vector1.push_back(252);//mod = manual = 252
            vector1.push_back(254);//end

            for(int j=0; j < int(vector1.size()); j++)
            {
                port.transmitAsync(vector1);
            }    
            std::cout << "\33[2K\r" << std::to_string(vector1[1]) << " s1 " << std::to_string(vector1[2]) << " s2 " << std::to_string(vector1[3]) << " s3 " << std::to_string(vector1[4]) << " s4 " << std::to_string(vector1[5]) << " free " << std::to_string(vector1[6]) << " armed " << std::to_string(vector1[7]) << " mod --- message sent ---" << std::flush;          
        }
    }
}

void text(cv::Rect trackingBox)
{
    if(mod == "track")
    {  
        cv::rectangle(image, cv::Point(0, 60), cv::Point(260,270), CV_RGB(0,0,0),-1,cv::LINE_8);
        if(armed)//false = disarmed, true = armed
        {
            cv::putText(image, "Armed", cv::Point(6,160), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,0,0), 1);

            cv::rectangle(image, cv::Point(trackingBox.x + trackingBox.width/2-1, trackingBox.y + trackingBox.height/2-1),cv::Point (trackingBox.x + trackingBox.width/2+1, trackingBox.y + trackingBox.height/2+1) , CV_RGB(255,0,0), -1, cv::LINE_8);
        }
        else
        {
            cv::putText(image, "Disarmed", cv::Point(6,160), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(0,255,0), 1);

            cv::rectangle(image, cv::Point(trackingBox.x + trackingBox.width/2-1, trackingBox.y + trackingBox.height/2-1),cv::Point (trackingBox.x + trackingBox.width/2+1, trackingBox.y + trackingBox.height/2+1) , CV_RGB(0,255,0), -1, cv::LINE_8);
        }

        if(manual_auto)//false = autoguided, true = tracking
        {
            cv::putText(image, "Mode: Tracking", cv::Point(6,100), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);

            cv::line(image, cv::Point(0,height/2), cv::Point(width,height/2), CV_RGB(255,255,255), 1, cv::LINE_8);
            cv::line(image, cv::Point(width/2,0), cv::Point(width/2,height), CV_RGB(255,255,255), 1, cv::LINE_8);
        }
        else
        {
            cv::putText(image, "Mode: Auto guided", cv::Point(6,100), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);
        }

        cv::putText(image, "Fps: " + std::to_string(fps), cv::Point(6,250), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);

        pom_x = trackingBox.x - width/2 + trackingBox.width/2;
        pom_y = height/2 - trackingBox.y - trackingBox.height/2;

        cv::putText(image, "Offset X:" + std::to_string(pom_x), cv::Point(6,190), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);
        cv::putText(image, "Offset Y:" + std::to_string(pom_y), cv::Point(6,220), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);
    }

    if(mod == "manual")
    {
        cv::rectangle(image, cv::Point(0, 60), cv::Point(200,190), CV_RGB(0,0,0),-1,cv::LINE_8);

        cv::putText(image, "Mode: Manual", cv::Point(6,100), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);

        cv::putText(image, "Fps: " + std::to_string(fps), cv::Point(6,160), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);

        cv::line(image, cv::Point(0,height/2),cv::Point(width,height/2) , CV_RGB(255,255,255), 1, cv::LINE_8);
        cv::line(image, cv::Point(width/2,0),cv::Point(width/2,height) , CV_RGB(255,255,255), 1, cv::LINE_8);

        if(armed)
        {
            cv::putText(image, "Armed", cv::Point(6,130), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,0,0), 2);
            cv::rectangle(image, cv::Point(width/2-2,height/2-2), cv::Point(width/2+2,height/2+2), CV_RGB(255,0,0), 1, cv::LINE_8);
        }
        else
        {
            cv::putText(image, "Disarmed", cv::Point(6,130), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(0,255,0), 2);
            cv::rectangle(image, cv::Point(width/2-2,height/2-2), cv::Point(width/2+2,height/2+2), CV_RGB(0,255,0), 1, cv::LINE_8);
        }
    }

    if(mod == "main")
    {
        cv::rectangle(image, trackingBox, CV_RGB(255, 255, 255),2, 8);

        cv::line(image, cv::Point (trackingBox.x + trackingBox.width/2, trackingBox.y + trackingBox.height/2),cv::Point (trackingBox.x + trackingBox.width/2, trackingBox.y + trackingBox.height/2) , CV_RGB(255,0,0), 5, cv::LINE_8);

        cv::rectangle(image, cv::Point(0,70), cv::Point(120,110), CV_RGB(0,0,0),-1,cv::LINE_8);
        cv::putText(image, "Fps: " + std::to_string(fps), cv::Point(6,100), cv::FONT_HERSHEY_SIMPLEX, .8, CV_RGB(255,255,255), 1);
    }
}

void manual(std::string& mod)
{
    while(true)
    {
        last_time_point = std::chrono::steady_clock::now();

        cap >> image;

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
                armed = true;
            }

            else if(armed == true)
            {   
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
        serialout(trackingBox);
        text(trackingBox);
        display();

        new_time_point = std::chrono::steady_clock::now();
        Fps();
    }
}

void track_update(cv::Rect& trackingBox, cv::Ptr<cv::Tracker> tracker, bool& track_turn_off)
{
    if(tracker->update(image, trackingBox))
    {
		cv::rectangle(image, trackingBox, CV_RGB(255, 0, 0), 2, 8);
	}
    else
    {
        track_turn_off = true;
    }
}

void track(cv::Rect& trackingBox, std::string& mod)
{
    cv::Ptr<cv::Tracker> tracker = cv::TrackerCSRT::create();

    tracker->init(image, trackingBox);

    while (true)
    {
        last_time_point = std::chrono::steady_clock::now();

        cap >> image;

        std::thread t1(text, trackingBox);
        std::thread t2(serialout, trackingBox);
        std::thread t3(track_update, std::ref(trackingBox), tracker, std::ref(track_turn_off));

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

        if(key == 114)
        {
            if(manual_auto == false)
            {
                manual_auto = true;
            }

            else if(manual_auto == true)
            {
                manual_auto = false;
            }
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
        if(height > 480)
        {
            center.x = x/2;
            center.y = y/2;
        }
        else if(height <= 480)
        {
            center.x = x/3;
            center.y = y/3;
        }
        else
        {
            center.x = x;
            center.y = y;
        }
    }
    if(key == 116)
    {
        mod = "track";
        armed = false;
        manual_auto = false;
        cv::setMouseCallback("Arax",NULL);
    }
}

int main() 
{
    //---------------------------------------------------------------------------------
    //Capture address
    try
    {
        cap = cv::VideoCapture(0); //"../Dron_videa/jed.MP4"
    }
    catch(...)
    {
        cap = cv::VideoCapture(1);
    }
    //---------------------------------------------------------------------------------
    
    width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    center.x = width/2;
    center.y = height/2;

    //---------------------------------------------------------------------------------
    //Serial boud, port
    std::string comport = "/dev/cu.usbmodem101";//"/dev/cu.usbserial-10" "/dev/cu.usbmodem101"
    unsigned int baud = 115200;

    try
    {
        port.open(comport, baud);
        std::cout << "Port otevřen" << std::endl;
    }
    catch(...)
    {
        std::cout << "Port nelze otevřít" << std::endl;
    }
    //--------------------------------------------------------------------------------

    std::cout << "Cap fps: " << cap.get(cv::CAP_PROP_FPS) << std::endl;

    while (true)
    {   
        last_time_point = std::chrono::steady_clock::now();
        mod = "main";

        cap >> image;

        left_top.x = center.x - width_trackbox;
        left_top.y = center.y - width_trackbox;

        right_down.x = width_trackbox + center.x;
        right_down.y = width_trackbox + center.y;
        
        cv::Rect trackingBox(left_top, right_down);

        text(trackingBox);

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
        
        if(key == 116 || mod == "track")
        {   
            cv::setMouseCallback("Arax",NULL);
            mod = "track";
            armed = false;
            manual_auto = false;
            track(trackingBox, mod);
        }

        if(key == 101)
        {
            cv::setMouseCallback("Arax", track_box_mouse_movement, NULL);
        }
    
        if(key == 109)
        {
            cv::setMouseCallback("Arax",NULL);
            mod = "manual";
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