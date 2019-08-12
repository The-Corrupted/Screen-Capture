#include <iostream>
#include <string>
#include <curl/curl.h>
#include <vector>
#include <mutex>
#include <memory>
#include <thread>
#include <chrono>

struct ServerReader {
	bool OK_FLAG;
	bool edid_found;
	std::mutex mu;
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string* userp) {
	userp->append((char*) contents, size * nmemb);
	return size * nmemb;
}

std::vector<std::string> checkpage(std::string http_addr, std::shared_ptr<ServerReader> sr) {
	CURL *curl;
	CURLcode res;
	std::string readBuffer;

	for (; ;) {
		curl = curl_easy_init();
		if(curl) {
			curl_easy_setopt(curl, CURLOPT_URL, http_addr.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1);
		}
		std::cout<<"In Curl code"<<std::endl;
		res = curl_easy_perform(curl);
		std::cout<<"Connection attempt made."<<std::endl;
		if(res != CURLE_OK) {
			std::cout<<"Curl failed to connect to " <<http_addr<<". It's probably not setup yet."<<std::endl;
			std::cout<<curl_easy_strerror(res)<<std::endl;
			curl_easy_cleanup(curl);
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;
		}
		int httpCode=0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
		if(httpCode != 200) {
			std::cout<<"Bad response from server."<<std::endl;
			curl_easy_cleanup(curl);
			continue;
		} else {
			ServerReader* src = sr.get();
			src->mu.lock();
			src->OK_FLAG = true;
			if ( readBuffer == "true" ) {
				src->edid_found = true;
				readBuffer.clear();
			} else {
				std::cout<< "Read buffer: " << readBuffer << std::endl;
				src->edid_found = false;
				readBuffer.clear();
			}
			src->mu.unlock();
			curl_easy_cleanup(curl);
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
	}
}
