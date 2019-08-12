#include "opencv2/opencv.hpp"
#include "./server_inspector.cpp"
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <stdexcept>

bool writeFile(std::string filename, int hdmiDisplayCount) {
	std::ofstream file (filename, std::ios::app);
	file << "*************** HDMI Display Detected. ***************\n\n";
	file << "***************       Iter "<<hdmiDisplayCount<<"            **********\n\n";
	file.close();
	return true;
}


bool isInRange(int r[2], int g[2], int b[2], cv::Vec3b pixColor) {
	int b_color = pixColor[0];
	int g_color = pixColor[1];
	int r_color = pixColor[2];
	if ( b_color < b[0] || b_color > b[1] ) {
		std::cout<<"B_Range not met: "<< b_color<<std::endl;
		return false;
	} else if ( g_color < g[0] || g_color > g[1] ) {
		std::cout<<"G_Range not met: "<< g_color<<std::endl;
		return false;
	} else if ( r_color < r[0] || r_color > r[1] ) {
		std::cout<<"R_Range not met: "<< r_color<<std::endl;
		return false;
	} else {
		return true;
	}
}

bool parseIP(std::string ip) {
	if ( ip == "localhost" ) {
		return true;
	}
	std::vector<std::string> sectioning; 
	std::string buff;
	//Break apart the string
	for (int x=0;x<ip.length();++x) {
		if ( ip[x] == '.' ) {
			if ( buff.length() == 0 ) {
				std::cout<<"Invalid IPv4. Seperator found before integer"<<std::endl;
				return false;
			}
			sectioning.push_back(buff);
			buff.clear();
			continue;
		}
		buff += ip[x];
	}
	if ( buff.length() < 1 ) {
		std::cout<<"Invalid IPv4: Ended with a period"<<std::endl;
		return false;
	}
	sectioning.push_back(buff);
	buff.clear();
	if ( sectioning.size() != 4 ) {
		std::cout<<"Invalid IPv4 sections. Wanted 4, got: "<<sectioning.size()<<std::endl;
		return false;
	}
	for(int x=0; x<sectioning.size(); ++x) {
		std::string sec = sectioning[x];
		for(int y=0; y<sec.length();++y) {
			if ( sec[y] - '0' > 9 || sec[y] - '0' < 0 ) {
				std::cout<<"Invalid character found in IPv4 address"<<std::endl;
				std::cout<<"Invalid character: "<<sec[y]<<std::endl;
				return false;
			}
		}
	}
	return true;

}

cv::Mat GetFrame(cv::VideoCapture *cap) {
	static cv::Mat frame;
	*cap >> frame;
	return frame;
}

cv::Vec3b ModVideo(cv::Mat *frame) {
	int point_x = frame->rows/4;
	int point_y = frame->cols/2;
	static cv::Vec3b new_color(153,102,204);
	static cv::Mat edges;
	cv::Vec3b color = frame->at<cv::Vec3b>(cv::Point(point_x, point_y));
	for(int x=0;x<20;++x) {
		frame->at<cv::Vec3b>(cv::Point(point_x+x, point_y)) = new_color;
		for(int y=0;y<20;++y) {
			frame->at<cv::Vec3b>(cv::Point(point_x, point_y+y)) = new_color;
		}
	}
	cv::cvtColor(*frame, edges, cv::COLOR_BGR2BGRA);
	cv::imshow("Live Feed", edges);
	return color;
}

int main(int argc, char *argv[]) {
	const int CON_FAIL_MAX = 10;
	int r_range[2] = {130, 255};
	int g_range[2] = {159, 255};
	int b_range[2] = {175, 255};
	const std::string filename = "log.txt";
	uint8_t consecutive_false = 0;
	bool colorFound = false;
	bool suspectedReboot = true;
	uint32_t hdmiDisplayCount = 0;
	std::string ip = "";
	std::string cam_port = "0";
	std::string arg = "";
	if ( argc < 3 ) {
		std::cout<<"Not enough arguments found."<<std::endl;
		std::cout<<"Usage: ./main --ip <ip_addr> --cam <cam_val>"<<std::endl;
		return -1;
	}
	for ( int i = 1; i<argc; ++i ) {
		arg = argv[i];
		if ( arg != "--cam" && arg != "--ip" ) {
			std::cout<<"Usage: ./main --ip <ip_addr> --cam <cam_val>"<<std::endl;
			return -1;
		} else {
			if(arg == "--ip") {
				if(i+1 > argc) {
					std::cout<<"Missing value after argument flag: "<<arg<<std::endl;;
					return -1;
				} else {
					std::string arg2 = argv[++i];
					bool isValid = parseIP(arg2);
					if (!(isValid)) return -1;
					ip = arg2;
				}
				continue;
			} else if(arg == "--cam") {
				if(i+1 > argc) {
					std::cout<<"Missing value after argument flag: "<<arg<<std::endl;
					continue;
				} else {
					cam_port = argv[++i];
				}
				continue;
			} else {
				std::cout<<"Invalid flag found: "<<arg<<std::endl;
				std::cout<<"Usage: ./main --ip <ip_addr> --cam <cam_port>"<<std::endl;
				return -1;
			}
		}
	}
	if (ip == "") {
		std::cout<<"Usage: ./main --ip <ip_addr> --cam <cam_val>"<<std::endl;
		return -1;
	}
	int cam;
	try {
		cam = std::stoi(cam_port);
	}catch(std::invalid_argument& ia) {
		std::cout<<"Unable to parse cam_port. Is it not a number?"<<std::endl;
		return -1;
	}
	std::string http_addr = ip + ":8089/hdmistatus";
	std::cout<<"Communicating with: "<<http_addr<<std::endl;
	std::cout<<"Capturing on "<<cam<<std::endl;
	cv::VideoCapture cap(cam);
	if (!cap.isOpened()) {
		return -1;
	}
	cvNamedWindow("Live Feed", 1);
	cv::Mat edges;
	cv::Mat Frame;
	std::shared_ptr<ServerReader> sr = std::make_shared<ServerReader>();
	std::thread t(checkpage, http_addr, sr);
	while(cv::waitKey(1) != 27) {
		Frame = GetFrame(&cap);
		cv::Vec3b color = ModVideo(&Frame);
		colorFound = isInRange(r_range, g_range, b_range, color);
		ServerReader* src = sr.get();
		if ( src->OK_FLAG ) { 
			src->mu.lock();
			src->OK_FLAG = false;
			if (colorFound && src->edid_found) {
				std::cout<<"Color is in range." << std::endl;
				hdmiDisplayCount += 1;
				writeFile(filename, hdmiDisplayCount);
			} else if ( colorFound != true && src->edid_found ) {
				std::cout << colorFound <<  " " << src->edid_found << std::endl;
				std::cout<<"Color not found but server reported the device has an hdmi signal."<<std::endl;
				// suspectedReboot = false;
			} else {
				std::cout<<colorFound << " " << src->edid_found << std::endl;
				std::cout<<"Something went wrong."<<std::endl;
			}
			src->mu.unlock();
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
	t.join();
	return 0;
}