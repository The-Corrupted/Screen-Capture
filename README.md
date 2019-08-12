Screen_Reader

Author: Donovan Loperena<br/>
Email: dmloperena@brightsign.biz<br/>
Last Modified: Aug 12 2019 11:09AM<br/>


*********************Technologies Used********************

- BrightScript ( autorun.brs )
- libcurl-dev
- opencv2
- pthread

*********************  Instructions  ********************

Compilation:

Move to the directory where both files are located ( both files must be in the same directory).

Once in the directory run `g++ main2.cpp -o main -lcurl -lpthread `pkg-config --cflags --libs opencv``

Usage:

In order to run this, you must connect the device your scanning and the computer running this program to the same network and you must be able to access the devices IP.

Once the initial setup is started, run
./main --ip <device_ip> --cam <cam_port> 
e.x. ./main --ip 10.0.200.160 --cam 0

Once the program starts a live feed should be displayed on your screen with a small purple half-square in the bottom left corner.

Align the purple arrow to one of the edge of the screen and make sure that the edge of the purple arrow does not land on any bright reflections ( i.e. light, any white surfaces. These will be picked up by the camera and the color checker will misinterpret this as the screen being on).

Once the camera is setup, insert the autorun into the device and let the program run for as long as you need to.

When you are ready to quit, click ESC.

Results:

Results will be stored in edid.txt on the devices sd as well as log.txt on the computer running this program. To check that everything ran well, open log.txt and do a search for FAIL and MISCFAIL. If FAIL appears, that means the device detected edid but the camera didn't detect that the screen had turned on. MISCFAIL shows up when the device does not detect hdmi and the camera does or doesn't detect the screen.

Resetting:

When your ready to start a new test/clean your environment; use serial to connect to your device and make sure you remove the registry of previous runs via registry delete counter. This should reset the devices iteration count.

Delete edid from the devices storage and remove log.txt.

Your environment should now be clean and ready for retesting.

********************* Technical Notes ********************


This program starts a capture on the specified camera port and will read the color value from the pixel located at the intersection of the purple half-square (before it is edited of course). The frame will be passed to ModImage where it is displayed on a qtwidget created through opencv.

In tandem to this ( in its own seperate thread to prevent blocking ) a curl instance is started using the specified ip and a shared_ptr of type ServerReader.

ServerReader is used to synchronize communications between the main thread and the threaded curl function. When curl is able to make a connection, ServerReader*->OK_FLAG will be set to true and the body is checked and returned as a true/false value in ServerReader*->edid_found.

On the main thread, ServerReader will trigger an if statement that triggers a color check of the most recent frame capture. Once the check is done, the value of OK_FLAG is set back to false and the main thread will wait until another connection is made.

The color is checked as an rgb value with a fault tolerance of r{130, 255}, g{159,255}, b{175, 255}. This helps to deal with unusual lighting and differences between screens. However it may be necessary to further adjust the tolerance if you find a lot of failures where there shouldn't be any.

  
