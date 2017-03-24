// Kyle Williams and Thomas Franceschi
// CSE 30341-01
// Professor Thain
// Project IV
// Due: 3/24/17

// Wrapper for Site List Queue

#include <queue>
#include <mutex>
#include <condition_variable>

class QueueSiteList{

public:
	QueueSiteList() {};

	void push(std::string s);
	std::string pop();
	int is_empty() {return sites_queue.empty();}

private:
	std::queue<std::string> sites_queue;
	std::mutex mtx;
	std::condition_variable cv;

	bool empty = true;

};

void QueueSiteList::push(std::string s){

	// lock before push, unique_lock takes care of unlock
	std::unique_lock<std::mutex> lck(mtx);
	sites_queue.push(s);

	// update empty status and notify thread
	empty = false;
	cv.notify_one();

}

std::string QueueSiteList::pop(){

	if(!sites_queue.empty()){
		// lock before pop and wait on queue if it's empty
		std::unique_lock<std::mutex> lck(mtx);
		// while(sites_queue.empty()) cv.wait(lck);
		std::string s = sites_queue.front();
		sites_queue.pop();

		if(sites_queue.empty()){
			empty = true;
		}

		return s;
	}

	return "";

}