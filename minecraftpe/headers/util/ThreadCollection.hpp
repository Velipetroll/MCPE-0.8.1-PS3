#pragma once
#include <_types.h>
#include <deque>
#include <vector>
#include <memory>

#ifndef __PS3__
#include <mutex>
#include <thread>
#include <condition_variable>
#endif

struct Job;
struct ThreadCollection
{
	std::vector<std::thread> threads;
	std::deque<std::shared_ptr<Job>> field_C;
	std::deque<std::shared_ptr<Job>> field_34;
	std::mutex mutex;
	std::mutex field_60;
	std::condition_variable field_64;
	bool isStopped;

	ThreadCollection(uint32_t maxthreads);
	void enqueue(std::shared_ptr<Job>);
	void processUIThread();
	~ThreadCollection();
};
