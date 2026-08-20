#include <util/ThreadCollection.hpp>
#include <util/Job.hpp>
#include <unistd.h>
#include <util/Worker.hpp>

static int sub_D66E4980() {
	#if defined(__WIN32__) || defined(__PPU__) || defined(__PS3__) || defined(__psl1ght__)
	// En PS3, el PPU (PowerPC Processing Unit) tiene 2 hilos por hardware.
	return 2;
	#else
	int v0; // r0
	v0 = sysconf(97); // 97 = _SC_NPROCESSORS_ONLN en Linux/Android ARM
	return v0 & ~(v0 >> 31);
	#endif
}

ThreadCollection::ThreadCollection(uint32_t maxthreads) {
	this->isStopped = 0;
	if(maxthreads == 0) {
		int v4 = sub_D66E4980();
		if(v4) maxthreads = v4;
		else maxthreads = 1;
	}

	for(unsigned int i = 0; i != maxthreads; ++i) {
		this->threads.emplace_back(std::thread(Worker(*this)));
	}
}

void ThreadCollection::enqueue(std::shared_ptr<Job> a2) {
	std::unique_lock<std::mutex> v11(this->mutex, std::defer_lock);
	v11.lock();
	this->field_C.emplace_back(a2);
	v11.unlock();
	this->field_64.notify_one();
}

void ThreadCollection::processUIThread() {
	for(auto&& it = this->field_34.begin(); it != this->field_34.end();) {
		if(it->get()->status == JS_FINISHED) {
			it->get()->finish();
		}
		it = this->field_34.erase(it); //TODO check
	}
}

ThreadCollection::~ThreadCollection() {
	{
		std::unique_lock<std::mutex> lock1(this->mutex);
		std::unique_lock<std::mutex> lock2(this->field_60);
		this->isStopped = 1;
	}
	this->field_64.notify_all();

	// Volvemos a hacer el join directo ya que psl1ght no soporta joinable()
	for(auto&& t: this->threads) {
		t.join();
	}
	this->threads.clear();
}
