#pragma once
#ifndef ANDROID
#include <network/mco/RestRequestJob.hpp>
#include <android/AppPlatform_android.hpp>

// Ocultamos las librerias modernas de internet a la PS3
#ifndef __PS3__
#include <mutex>
#include <condition_variable>
#endif

struct CurlRestRequestJob: RestRequestJob
{
	int field_54, field_58, field_5C;

	#ifdef __PS3__
	// Variables falsas para que la PS3 no intente usar std::mutex
	int field_60;
	int field_64;
	int mutex;
	#else
	// Variables reales de PC
	std::condition_variable field_60;
	std::mutex field_64;
	std::mutex mutex;
	#endif

	int field_6C;
	int httpStatusOrNegativeError;
	std::string content;
	bool started;
	char _align[3];

	CurlRestRequestJob();
	bool isRunning();
	void onRequestComplete(int, int, const std::string&);

	virtual ~CurlRestRequestJob();
	virtual void stop();
	virtual void run();
	virtual void finish();
};
#endif
