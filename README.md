Screen_Reader

Author: Donovan Loperena<br/>
Email: dmloperena@brightsign.biz<br/>
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

Once the program has started, two feeds should display on screen. One will display the raw video stream, and the other displays the red color checker feed with adjuster slides ( mostly used for testing ).

Setup the camera so that a majority of the feed is taken up by the screen you are inspecting. After you have setup the camera in a semi-permanent and isolated area you may insert the autorun, at which point the test will begin and you can leave it to run for as long as you need.

When you are ready to quit, click ESC.

Results:

Results will be stored in edid.txt on the device's sd as well as log.txt on the computer running this program. To check that everything ran well, open log.txt and do a search for FAIL and MISCFAIL. If FAIL appears, that means the device detected edid but the camera didn't detect that the screen had turned on. MISCFAIL shows up when the device does not detect hdmi and the camera does or doesn't detect the screen.

Resetting:

When you're ready to start a new test/clean your environment; use serial to connect to your device and make sure you remove the registry of previous runs via registry delete counter. This should reset the devices iteration count.

Delete edid from the devices storage and remove log.txt.

Your environment should now be clean and ready for retesting.

********************* Technical Notes ********************


This program starts a capture on the specified camera port and will read a filtered frame when the device is able to make contact with the test device via a threaded instance of curl that communicates back to main via a shared_ptr to the struct ServerReader.

ServerReader is used to synchronize communications between the main thread and the threaded curl function. When curl is able to make a connection, ServerReader*->OK_FLAG will be set to true and the body is checked and returned as a true/false value in ServerReader*->edid_found.

On the main thread, ServerReader will trigger an if statement that triggers a color check of the most recent frame capture. Once the check is done, the value of OK_FLAG is set back to false and the main thread will wait until another connection is made.

The color is checked via black and white pixel values and ultimately. If the number of white pixels ( the color red ) divided by the number of black pixels ( another color ) is greater than or equal to 1, then it is assumed that the screen came on. If it is less than one, it is assumed that the screen is either off; or is seeing some very severe distortion.  
