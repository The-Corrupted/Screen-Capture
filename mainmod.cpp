#include "opencv2/opencv.hpp"
#include "./server_inspector.cpp"
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <stdexcept>

class ImageData {
private:
	int white = 0;
	int other = 0;
public:
	float doGetRes();
	int doGetWhite();
	int doGetOther();
	void incrementWhite();
	void incrementOther();
	~ImageData();
};

bool checkColor(int row, int col, cv::Mat Frame);
bool writeFile(std::string status, std::string filename, int hdmiDisplayCount);
bool parseIP(std::string ip);
cv::Mat GetFrame(cv::VideoCapture *cap);
void ShowVideo(cv::Mat *Frame, cv::Mat *Objframe);

static void on_low_h_thresh_trackbar(int, void *);
static void on_high_h_thresh_trackbar(int, void *);
static void on_low_s_thresh_trackbar(int, void *);
static void on_high_s_thresh_trackbar(int, void *);
static void on_low_v_thresh_trackbar(int, void *);
static void on_high_v_thresh_trackbar(int, void *);


// ---------------------------------- Some globals for HSV Colorizer and Adjuster Widget -----------------------------------------------

std::string LIVEFEEDWINDOW = "Live feed";
std::string OBJECTDETECTIONWINDOW = "Objects Detected";
const int MAX_C_VAL = 255;
const int H_MAX = 10;
int low_H = 0, low_S = 177, low_V = 65;
int high_H = H_MAX, high_S = MAX_C_VAL, high_V = MAX_C_VAL;


// -----------------------------------------------------Main Program------------------------------------------

int main(int argc, char *argv[]) {
	const std::string filename = "log.txt";
	bool colorFound = false;
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
	cvNamedWindow(LIVEFEEDWINDOW.c_str());
	cvNamedWindow(OBJECTDETECTIONWINDOW.c_str());
	cv::createTrackbar("Low H", OBJECTDETECTIONWINDOW, &low_H, H_MAX, on_low_h_thresh_trackbar);
	cv::createTrackbar("High H", OBJECTDETECTIONWINDOW, &high_H, H_MAX, on_high_h_thresh_trackbar);
	cv::createTrackbar("Low S", OBJECTDETECTIONWINDOW, &low_S, MAX_C_VAL, on_low_s_thresh_trackbar);
	cv::createTrackbar("High S", OBJECTDETECTIONWINDOW, &high_S, MAX_C_VAL, on_high_s_thresh_trackbar);
	cv::createTrackbar("Low V", OBJECTDETECTIONWINDOW, &low_V, MAX_C_VAL, on_low_v_thresh_trackbar);
	cv::createTrackbar("High V", OBJECTDETECTIONWINDOW, &high_V, MAX_C_VAL, on_high_v_thresh_trackbar);

	cv::Mat edges;
	cv::Mat Frame;
	std::shared_ptr<ServerReader> sr = std::make_shared<ServerReader>();
	std::shared_ptr<Signaler> sig = std::make_shared<Signaler>();

	std::thread t(checkpage, http_addr, sr, sig);
	// Threshold frame
	cv::Mat Frame_Threshold;
	while(cv::waitKey(1) != 27) {
		Frame = GetFrame(&cap);
		// Convert Frame to HSV Colorspace
		cv::Mat Frame_HSV;
		cvtColor(Frame, Frame_HSV, cv::COLOR_BGR2HSV);
		// Get threshold frame
		cv::inRange(Frame_HSV, cv::Scalar(low_H, low_S, low_V), cv::Scalar(high_H, high_S, high_V), Frame_Threshold);
		// Display Original && Obj Frame
		ShowVideo(&Frame, &Frame_Threshold);
		ServerReader* src = sr.get();
		ImageData imagedata;
		int rows = Frame_Threshold.rows;
		int cols = Frame_Threshold.cols;
		int x = 1;
		for ( x; x < rows+1; ++x ) {
			int y = 1;
			bool isRed = checkColor(x, y, Frame_Threshold);
			if ( isRed ) {
				imagedata.incrementWhite();
			} else {
				imagedata.incrementOther();
			}
			for ( y; y < cols; ++y ) {
				isRed = checkColor(x, y, Frame_Threshold);
				if ( isRed ) {
					imagedata.incrementWhite();
				} else {
					imagedata.incrementOther();
				}
			}
		}
		std::cout<<imagedata.doGetWhite() << " " << imagedata.doGetOther() << std::endl;
		int dec = imagedata.doGetRes();
		if ( src->OK_FLAG ) { 
			src->mu.lock();
			src->OK_FLAG = false;
			hdmiDisplayCount += 1;
			ImageData imagedata;
			int rows = Frame_Threshold.rows;
			int cols = Frame_Threshold.cols;
			int x = 1;
			for ( x; x < rows+1; ++x ) {
				int y = 1;
				bool isRed = checkColor(x, y, Frame_Threshold);
				if ( isRed ) {
					imagedata.incrementWhite();
				} else {
					imagedata.incrementOther();
				}
				for ( y; y < cols; ++y ) {
					isRed = checkColor(x, y, Frame_Threshold);
					if ( isRed ) {
						imagedata.incrementWhite();
					} else {
						imagedata.incrementOther();
					}
				}
			}
			std::cout<<imagedata.doGetWhite() << " " << imagedata.doGetOther() << std::endl;
			float res = imagedata.doGetRes();
			bool colorFound;
			if ( dec >= 0.8 ) colorFound = true;
			else {
				colorFound = false;
				std::cout << "Color is missing or a majority of it is distorted." << std::endl;
			}
			std::cout<< "DEC: " << dec << std::endl;
			if (colorFound && src->edid_found) {
				std::cout<<"Color is in range." << std::endl;
				writeFile("PASS", filename, hdmiDisplayCount);
			} else if ( colorFound != true && src->edid_found ) {
				std::cout << colorFound <<  " " << src->edid_found << std::endl;
				std::cout<<"Color not found but server reported the device has an hdmi signal."<<std::endl;
				const std::string iterCountStr = std::to_string(hdmiDisplayCount);
				std::string imageName = "FImg"+iterCountStr+".png";
				bool result = cv::imwrite(imageName, Frame);
				if (!result) {
					std::cout<<"Failed to save the failing frame."<<std::endl;
				}
				writeFile("FAIL", filename, hdmiDisplayCount);
			} else {
				std::cout<<colorFound << " " << src->edid_found << std::endl;
				std::cout<<"Something went wrong."<<std::endl;
				const std::string iterCountStr = std::to_string(hdmiDisplayCount);
				std::string imageName = "FImg"+iterCountStr+".png";
				bool result = cv::imwrite(imageName, Frame);
				if (!result) {
					std::cout<<"Failed to save the failing frame."<<std::endl;
				}
				writeFile("MISCFAIL", filename, hdmiDisplayCount);
			}
			src->mu.unlock();
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	Signaler* sig_ptr = sig.get();
	sig->mu.lock();
	sig->quit = true;
	sig->mu.unlock();
	t.join();
	return 0;
}

// -----------------------------------------------------Results Logging Function-----------------------------------------

bool writeFile(std::string status, std::string filename, int hdmiDisplayCount) {
	if ( status == "PASS" ) {
		std::ofstream file (filename, std::ios::app);
		file << "*************** HDMI Display Detected. ***************\n\n";
		file << "***************       Iter "<<hdmiDisplayCount<<"            **********\n\n";
		file.close();
		return true;
	} else if ( status == "FAIL" ) {
		std::ofstream file(filename, std::ios::app);
		file << "*************** FAIL: HDMI Did Not Display ****************\n\n";
		file << "***************       Iter "<<hdmiDisplayCount<<"            **********\n\n";
		file.close();
		return true;
	} else {
		std::ofstream file(filename, std::ios::app);
		file << "***************  MISCFAIL: Nothing worked  ****************\n\n";
		file << "***************       Iter "<<hdmiDisplayCount<<"            **********\n\n";
		return true;
	}
	std::cout<<"Something went wrong."<<std::endl;
	return false;
}

//---------------------------------------------Command-Line IP validation-----------------------------------------------


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

// -----------------------------------------Captures and returns frames as they come in-----------------------------------------------------------

cv::Mat GetFrame(cv::VideoCapture *cap) {
	static cv::Mat frame;
	*cap >> frame;
	return frame;
}

// ------------------------------------ Create The Display widget and show frames ------------------------------------------------------------

void ShowVideo(cv::Mat *Frame, cv::Mat *Objframe) {
	static cv::Mat edges;
	cv::cvtColor(*Frame, edges, cv::COLOR_BGR2BGRA);
	cv::imshow(LIVEFEEDWINDOW, *Frame);
	cv::imshow(OBJECTDETECTIONWINDOW, *Objframe);
}

bool checkColor(int row, int col, cv::Mat Frame) {
	cv::Vec3b color = Frame.at<cv::Vec3b>(cv::Point(row, col));
	if ( color[0] == 255 ) {
		return true;
	}
	return false;
}

void ImageData::incrementWhite() {
	++white;
}

void ImageData::incrementOther() {
	++other;
}

int ImageData::doGetWhite() {
	return white;
}

int ImageData::doGetOther() {
	return other;
}

float ImageData::doGetRes() {
	if ( other == 0 && white > 0 ) {
		std::cout << "All red found." << std::endl;
		return 1;
	}
	return (float)(white)/(float)(other);
}

ImageData::~ImageData() {
	std::cout<<"Image Data imploding."<<std::endl;
}

// ----------------------------------------- Color Adjuster Functions ------------------------------------
static void on_low_h_thresh_trackbar(int, void *) {
	low_H = cv::min(high_H-1, low_H);
	cv::setTrackbarPos("Low H", OBJECTDETECTIONWINDOW, low_H);
} 

static void on_high_h_thresh_trackbar(int, void *) {
	high_H = cv::max(high_H, low_H+1);
	cv::setTrackbarPos("High H", OBJECTDETECTIONWINDOW, high_H);
}

static void on_low_s_thresh_trackbar(int, void *) {
	low_S = cv::min(high_S-1, low_S);
	cv::setTrackbarPos("Low S", OBJECTDETECTIONWINDOW, low_S);
}

static void on_high_s_thresh_trackbar(int, void *) {
	low_H = cv::max(high_S, low_S+1);
	cv::setTrackbarPos("High S", OBJECTDETECTIONWINDOW, high_S);
}

static void on_low_v_thresh_trackbar(int, void *) {
	low_V = cv::min(high_V-1, low_V);
	cv::setTrackbarPos("Low V", OBJECTDETECTIONWINDOW, low_V);
}

static void on_high_v_thresh_trackbar(int, void *) {
	high_V = cv::max(high_V, low_V+1);
	cv::setTrackbarPos("High V", OBJECTDETECTIONWINDOW, high_V);
}
