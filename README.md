Screen_Reader

Author: Donovan Loperena<br/>
Last Modified: Aug 12 2019 11:09AM<br/>


******************** Technologies Used ********************

- BrightScript ( autorun.brs )
- libcurl-dev
- opencv2
- pthread

*********************  Instructions  ********************

Compilation:

Move to the directory where both files are located ( both files must be in the same directory).

Once in the directory run ``g++ main2.cpp -o main -lcurl -lpthread `pkg-config --cflags --libs opencv` ``

Usage:

In order to run this, you must connect the device you're scanning and the computer running this program to the same network and you must be able to access the device's IP.

After the initial setup, run
./main --ip <device_ip> --cam <cam_port> 
e.x. ./main --ip 10.0.200.160 --cam 0

( Notice: This program uses c++17 features. As such, please either alias g++ --std=c++17 or set it manually when you go to compile ).

Once the program starts a raw feed of the camera as well as a black and white filtered feed should be displayed to the screen. You must make sure the the screen you are attempting to read takes up at least 80 of the cameras view.

Once the camera is setup, insert the autorun into the test device and let the program run for as long as you need it to.

When you are ready to quit, click ESC.

Results:

Results will be stored in edid.txt on the device's sd as well as log.txt on the computer running this program. To check that everything ran well, open log.txt and do a search for FAIL and MISCFAIL. If FAIL appears, that means the device detected edid but the camera didn't detect that the screen had turned on. MISCFAIL shows up when the device does not detect hdmi and the camera does or doesn't detect the screen.

Resetting:

When you're ready to start a new test/clean your environment; use serial to connect to your device and make sure you remove the registry of previous runs via registry delete counter. This should reset the devices iteration count.

Delete edid from the devices storage and remove log.txt.

Your environment should now be clean and ready for retesting.

********************* Technical Notes ********************


This program starts a capture on the specified camera port and will read the color filtered image for black and white values ( red and not red ).

In tandem to this ( in its own seperate thread to prevent blocking ) a curl instance is started using the specified ip and a shared_ptr of type ServerReader.

ServerReader is used to synchronize communications between the main thread and the threaded curl function. When curl is able to make a connection, ServerReader*->OK_FLAG will be set to true and the body is checked and returned as a true/false value in ServerReader*->edid_found.

On the main thread, ServerReader will trigger an if statement that triggers a color check of the most recent frame capture. Once the check is done, the value of OK_FLAG is set back to false and the main thread will wait until another connection is made.

The color is checked as a collection of black and white values ( i.e. the first value is checked for 255 or 0 ). If the number of white/black pixels is >= 1 then it is assumed the screen is on. If it is lower, it is assumed the screen is off or suffering heavy distortion.

  
