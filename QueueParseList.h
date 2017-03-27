// Kyle Williams and Thomas Franceschi
// CSE 30341-01
// Professor Thain
// Project IV
// Due: 3/24/17

// Wrapper for Parse List Queue

#include <queue>
#include <mutex>
#include <condition_variable>

struct Pair {

	std::string url;
	std::string html;

};

class QueueParseList{

public:
	QueueParseList() {};

	void push(std::string, std::string);
	Pair pop();
	int is_empty() {return parse_queue.empty();}

private:
	std::queue<Pair> parse_queue;
	std::mutex mtx;
	std::condition_variable cv;

	bool empty = true;

};

void QueueParseList::push(std::string url, std::string html){

	// lock before push, unique_lock takes care of unlock
	std::unique_lock<std::mutex> lck(mtx);

	Pair pair;
	pair.url = url;
	pair.html = html;

	parse_queue.push(pair);

	// update empty status and notify thread
	empty = false;
	cv.notify_one();

}

Pair QueueParseList::pop(){

	// lock before pop and wait on queue if it's empty
	std::unique_lock<std::mutex> lck(mtx);
	while(empty) cv.wait(lck);

	Pair pair = parse_queue.front();
	parse_queue.pop();

	if(parse_queue.empty()){
		empty = true;
	}
	
	return pair;

}