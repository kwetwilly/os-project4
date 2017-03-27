// Kyle Williams and Thomas Franceschi
// CSE 30341-01
// Professor Thain
// Project IV
// Due: 3/24/17

#include "Config.h"
#include "QueueSiteList.h"
#include "QueueParseList.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <csignal>
#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <vector>
#include <map>
#include <string>
#include <cstring>

// GLOBAL VARIABLES AND STRUCTURES ------------------------------------------
QueueSiteList  			 sites_queue;
QueueParseList 			 parse_queue;
std::vector<std::string> search_vect;

int PERIOD_FETCH;
int NUM_FETCH;
int NUM_PARSE;
std::string SEARCH_FILE;
std::string SITES_FILE;

std::mutex mtx;

size_t BATCH 	  = 1;
int KEEP_RUNNING  = 1;
int SITES_FETCHED = 0;
int SITES_PARSED  = 0;
bool ALARM_SET    = true;

// memory structure for libcurl
struct MemoryStruct {

	char *memory;
	size_t size;

};

// FUNCTION PROTOTYPES ------------------------------------------------------
void process_search(std::string);
void process_site(std::string);
static size_t WriteMemoryCallback(void *, size_t, size_t, void *);
std::string getinmemory_main(std::string);
void fetch_html();
void parse_write_html();
std::map<std::string, int> findTerms(std::string html);
void alarm_handler(int);
void signal_handler(int);
void usage();

// MAIN ---------------------------------------------------------------------
int main(int argc, char *argv[]){

	// check for proper number of arguments
	if(argc > 2){
		std::cout << "site-tester: Error. Too many arguements" << std::endl;
		usage();
		exit(EXIT_FAILURE);
	}
	if(argc < 2){
		std::cout << "site-tester: Error. Too few arguements" << std::endl;
		usage();
		exit(EXIT_FAILURE);
	}

	// check if config file exists
	int config_result = access(argv[1], R_OK);
	if(config_result < 0){
		std::cout << "site-tester: Config File Error: " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	// create configuration class from file
	Config config_file(argv[1]);

	// set global configuration variables
	PERIOD_FETCH = config_file.get_period_fetch();
	NUM_FETCH 	 = config_file.get_num_fetch();
	NUM_PARSE 	 = config_file.get_num_parse();
	SEARCH_FILE  = config_file.get_search_file();
	SITES_FILE 	 = config_file.get_site_file();

	// initialize threads for consumers and producers
	std::thread *producers = new std::thread[NUM_FETCH];
	std::thread *consumers = new std::thread[NUM_PARSE];

	// write headers to .csv output file
	std::ofstream myfile;
	// open file
	myfile.open(std::to_string(BATCH) + ".csv");
	// set headers
	myfile << "Time,Phrase,Site,Count\n";
	myfile.close();

	// process the search term files from the configuration object
	process_search(SEARCH_FILE);
	// populate the site queue from the configuration object param file
	process_site(SITES_FILE);

	std::cout << "Batch Number: " << BATCH << std::endl;

	// set handler for a ctrl-c
	signal(SIGINT, signal_handler);

	// initialize libcurl
	curl_global_init(CURL_GLOBAL_ALL);

	// create producer threads based on number specified in configuration file
	for(int i = 0; i < NUM_FETCH; i++){
		producers[i] = std::thread(fetch_html);
	}
	// create consumer threads based on number specified in configuration file
	for(int i = 0; i < NUM_PARSE; i++){
		consumers[i] = std::thread(parse_write_html);
	}

	// join threads when keep_running is 0
	for(int i = 0; i < NUM_FETCH; i++){
		producers[i].join();
	}
	for(int i = 0; i < NUM_PARSE; i++){
		consumers[i].join();
	}

	delete [] producers;
	delete [] consumers;

	exit(0);

}

// FUNCTIONS ----------------------------------------------------------------
void process_search(std::string filename){

	// search file processing
	std::ifstream inputFile;
	inputFile.open(filename.c_str());
	std::string line;

	while(getline(inputFile, line)){
		search_vect.push_back(line);
	}

	inputFile.close();

}

void process_site(std::string filename){

	// site file processing
	std::ifstream inputFile;
	inputFile.open(filename.c_str());
	std::string line;

	while(getline(inputFile, line)){
		sites_queue.push(line);
	}

	inputFile.close();

}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){

	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */ 
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;

}
 
std::string getinmemory_main( std::string url ){

	// if the sites_queue is empty and there is a thread waiting, give the thread and empty string
	if(url == ""){
		return "";
	}

	CURL *curl_handle;
	CURLcode res;

	struct MemoryStruct chunk;

	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */ 

	/* init the curl session */ 
	curl_handle = curl_easy_init();

	/* specify URL to get */ 
	curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

	// set timeout in case site does not respond
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10);

	// make a thread safe
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);

	/* send all data to this function  */ 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */ 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	/* some servers don't like requests that are made without a user-agent field, so we provide one */ 
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* get it! */ 
	res = curl_easy_perform(curl_handle);

	if(res == CURLE_OPERATION_TIMEDOUT){
		std::cout << "site-tester: fetching timed out: " << url << std::endl;
		return "";
	}

	/* check for errors */ 
	if(res != CURLE_OK && res != CURLE_OPERATION_TIMEDOUT) {
		fprintf(stderr, "curl_easy_perform(): %s failed: %s\n", url.c_str(), curl_easy_strerror(res));
		return "";
	}
	else {

		printf("URL: %s\n", url.c_str());

		printf("%lu bytes retrieved\n", (long)chunk.size);

		std::string output = chunk.memory;
		return output;

	}

	/* cleanup curl stuff */ 
	curl_easy_cleanup(curl_handle);

	free(chunk.memory);

	/* we're done with libcurl, so clean it up */ 
	curl_global_cleanup();

	return 0;

}

void fetch_html(){

	while(KEEP_RUNNING){
		std::string url = sites_queue.pop();
		std::string html = getinmemory_main(url);

		if(html != ""){
			SITES_FETCHED++;
		}

		parse_queue.push(url, html);
	}

}

void parse_write_html(){

	while(KEEP_RUNNING){
		// pair of url and html
		Pair pair = parse_queue.pop();

		// check if the html is not an empty string, if the html was empty, then the fetch to that url
		//  timed out in the fetch_html() function, so we are not going to write this to the file, since
		//  there is not data
		if(pair.html != ""){
			// call the find/count function, returns mapping of words and counts for given html
			std::map<std::string, int> map;
			map = findTerms(pair.html);

			mtx.lock();

			if(SITES_PARSED == 0){
				// write headers to .csv output file
				std::ofstream myfile;
				// open file
				myfile.open(std::to_string(BATCH) + ".csv");
				// set headers
				myfile << "Time,Phrase,Site,Count\n";
				myfile.close();
			}

			// save output to csv
			std::fstream myfile;
			// open file in append mode
			myfile.open(std::to_string(BATCH) + ".csv", std::fstream::app);

			// populate data
			for(auto it = map.begin(); it != map.end(); ++it){
				// date
				time_t timev;
				time(&timev);
				myfile << timev << ',';

				// phrase
				myfile << it->first << ',';

				// site
				myfile << pair.url << ',';

				// count
				myfile << it->second << '\n';
			}

			myfile.close();

			SITES_PARSED++;

			mtx.unlock();
		}

		// set alarm after the sites have all been fetched and parse and the alarm
		//  has not been set yet
		if((SITES_FETCHED == SITES_PARSED) && ALARM_SET){
			SITES_FETCHED = 0;
			SITES_PARSED  = 0;
			ALARM_SET 	  = false;
			signal(SIGALRM, alarm_handler);
			alarm(PERIOD_FETCH);
		}

	}

}

std::map<std::string, int> findTerms(std::string html){
	
	std::map<std::string, int> counts;

	for (size_t i = 0; i < search_vect.size(); i++){
		size_t pos = 0;
		size_t count = 0;

		while(pos != std::string::npos){

			pos = html.find(search_vect[i], pos);
			if( pos != std::string::npos) {
				count++;
				pos++;
			}
			else continue;
		}

		counts.insert({search_vect[i], count});

	}

	return counts;

}

void alarm_handler(int x){

	std::cout << "-------------" << std::endl;
	BATCH++;
	std::cout << "Batch Number: " << BATCH << std::endl;
	ALARM_SET = true;
	// populate the site queue from the configuration object param file
	process_site(SITES_FILE);

}

void signal_handler(int x){

	std::cout << "site-tester: Exiting..." << std::endl;
	KEEP_RUNNING = 0;

}

void usage(){

	std::cout << "usage: ./site-tester <configuration file>" << std::endl;

}