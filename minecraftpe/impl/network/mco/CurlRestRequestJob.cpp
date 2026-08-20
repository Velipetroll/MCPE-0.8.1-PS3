#ifndef ANDROID
#include <network/mco/CurlRestRequestJob.hpp>
#include <network/RestService.hpp>

#ifndef __PS3__
#include <curl/curl.h>
static size_t curl_onWrite(char* contents, size_t size, size_t nmemb, void* userp) {
	std::string* str = (std::string*)userp;
	str->append(contents, size * nmemb);
	return size * nmemb;
}
#endif

CurlRestRequestJob::CurlRestRequestJob() {
	this->field_58 = 0;
	this->field_5C = 0;
	this->started = 0;
}

bool CurlRestRequestJob::isRunning() {
	#ifndef __PS3__
	std::unique_lock<std::mutex> v5(this->mutex, std::defer_lock);
	v5.lock();
	#endif
	return this->started;
}

void CurlRestRequestJob::onRequestComplete(int a2, int a3, const std::string& a4) {
	this->httpStatusOrNegativeError = a3;
	this->content = a4;
	#ifndef __PS3__
	this->field_60.notify_one();
	#endif
}

CurlRestRequestJob::~CurlRestRequestJob() {
	if(this->isRunning()) {
		this->stop();
	}
}

void CurlRestRequestJob::stop() {
	#ifdef __PS3__
	if(this->getStatus() == JS_STOPPED) {
		return;
	}
	this->trySetStatus(JS_STOPPED);
	this->started = 0;
	#else
	{
		std::unique_lock<std::mutex> v5(this->mutex, std::defer_lock);
		if(this->getStatus() == JS_STOPPED) {
			return;
		}
		this->trySetStatus(JS_STOPPED);
		this->started = 0;
		//TODO abortWebRequest
		printf("CurlRestRequestJob::stop - not implemented\n");
	}
	this->field_60.notify_one();
	#endif
}

void CurlRestRequestJob::run() {
	#ifdef __PS3__
	// MODO OFFLINE SEGURO PARA PS3
	this->started = 1;
	this->trySetStatus(JS_STARTED);
	this->httpStatusOrNegativeError = -1;
	this->trySetStatus(JS_FINISHED);
	this->started = 0;
	#else
	// MODO ONLINE PARA PC Y ANDROID
	std::unique_lock<std::mutex> v5(this->mutex, std::defer_lock);
	v5.lock();
	this->started = 1;
	this->trySetStatus(JS_STARTED);

	CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
	if(code != CURLE_OK) {
		printf("curl global init failed: %d\n", code);
		return;
	}
	CURL* curl = curl_easy_init();
	if(!curl) {
		printf("curl_easy_init returned 0\n");
		return;
	}
	struct curl_slist* list = NULL;

	printf("Sending: %s %d\n", this->restService->getSeriveURL()->c_str(), this->requestType);
	curl_easy_setopt(curl, CURLOPT_URL, this->restService->getSeriveURL()->c_str());
	switch(this->requestType) {
		case RRT_GET:
			curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
			break;
		case RRT_POST:
			curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1L);
			if(this->body != "") {
				list = curl_slist_append(list, "Content-Type: application/json");
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, this->body.c_str());
			}
			break;
		case RRT_PUT:
			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
			if(this->body != "") {
				list = curl_slist_append(list, "Content-Type: application/json");
				curl_easy_setopt(curl, CURLOPT_READDATA, this->body.c_str());
			}

			break;
		case RRT_DELETE:
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			break;
	}
	std::string response;
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_onWrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	std::string cookie = this->restService->getCookieDataAsString();
	if(cookie.length() > 0) {
		printf("Setting cookie: (%lu) %s\n", cookie.length(), cookie.c_str());
		list = curl_slist_append(list, ("Cookie: " + cookie).c_str());
	}

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	CURLcode result = curl_easy_perform(curl);
	printf("Req: %s -> %d: (%s)\n", this->restService->getSeriveURL()->c_str(), result, response.c_str());
	if(result == CURLE_OK) {
		int code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
		this->onRequestComplete(0, code, response);
	} else {
		this->httpStatusOrNegativeError = -1;
	}
	this->trySetStatus(JS_FINISHED);
	this->started = 0;
	#endif // __PS3__
}

void CurlRestRequestJob::finish(){
	if(this->getStatus() != JS_STOPPED) {
		if(this->httpStatusOrNegativeError > 0) {
			if(this->httpStatusOrNegativeError < 300) {
				this->field_10(this->httpStatusOrNegativeError, this->content, this->field_44, std::shared_ptr<RestRequestJob>(this->field_8));
			} else {
				this->field_20(0, 0, this->httpStatusOrNegativeError, this->content, this->field_44, std::shared_ptr<RestRequestJob>(this->field_8));
			}
		} else {
			this->field_20(0, 1, this->httpStatusOrNegativeError, this->content, this->field_44, std::shared_ptr<RestRequestJob>(this->field_8));
		}
		return;
	}
	this->field_20(1, 0, 0, "", this->field_44, std::shared_ptr<RestRequestJob>(this->field_8));
	printf("CurlRestRequestJob::finish - not implemented\n");
}
#endif // ANDROID

