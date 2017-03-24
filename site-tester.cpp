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
#include <vector>
#include <map>
#include <string>
#include <cstring>

// GLOBAL VARIABLES AND STRUCTURES ------------------------------------------
QueueSiteList  sites_queue;
QueueParseList parse_queue;
std::vector<std::string> search_vect;
size_t runCount = 1;
bool interrupt = false;
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
std::string body_strip(std::string html);
void fetch_html();
void parse_write_html();
std::map<std::string, int> findTerms(std::string html);
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

	// initialize threads for consumers and producers

	int fetches = config_file.get_num_fetch();
	int parses  = config_file.get_num_parse();
	std::thread *producers = new std::thread[fetches];
	std::thread *consumers = new std::thread[parses];

	// write headers to .csv output file
	std::ofstream myfile;
	// open file
	myfile.open(std::to_string(runCount) + ".csv");
	// set headers
	myfile << "Time,Phrase,Site,Count\n";
	myfile.close();

	// process the search term files from the configuration object
	process_search(config_file.get_search_file());

	while(1){

		signal(SIGINT, signal_handler);

		// populate the site queue from the configuration object param file
		process_site(config_file.get_site_file());

		std::cout << "Batch Number: " << runCount << std::endl;

		while(!sites_queue.is_empty()){

			if(interrupt){
				std::cout << "site-tester: exiting..." << std::endl;

				delete [] producers;
				delete [] consumers;

				exit(0);
			}

			// create producer threads based on number specified in configuration file
			for(int i = 0; i < config_file.get_num_fetch(); i++){
				producers[i] = std::thread(fetch_html);
			}

			// create consumer threads based on number specified in configuration file
			for(int i = 0; i < config_file.get_num_parse(); i++){
				consumers[i] = std::thread(parse_write_html);
			}

			for(int i = 0; i < config_file.get_num_fetch(); i++){
				producers[i].join();
			}

			for(int i = 0; i < config_file.get_num_parse(); i++){
				consumers[i].join();
			}
		}

		std::cout << "-------------" << std::endl;

		runCount++;

		// wait for next period to fetch
		sleep(config_file.get_period_fetch());

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

	while(!inputFile.eof()){
		inputFile >> line;
		search_vect.push_back(line);
	}

	inputFile.close();

}

void process_site(std::string filename){

	// site file processing
	std::ifstream inputFile;
	inputFile.open(filename.c_str());
	std::string line;

	while(!inputFile.eof()){
		inputFile >> line;
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

	CURL *curl_handle;
	CURLcode res;

	struct MemoryStruct chunk;

	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */ 

	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */ 
	curl_handle = curl_easy_init();

	/* specify URL to get */ 
	curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

	// set timeout in case site does not respond
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 1);

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

std::string body_strip(std::string html){

	// get start and end indices of body content
	size_t start = html.find("<body");
	if(start == std::string::npos) return html;
	size_t end = html.find("</body>");
	if(end == std::string::npos) return html;
	size_t length = end - start - 5;

	// get substring between those values
	std::string just_the_body = html.substr(start + 5, length);

	return just_the_body;

}

void fetch_html(){

	std::string url = sites_queue.pop();
	std::string html = getinmemory_main(url);

	// strip to body only
	html = body_strip(html);

	parse_queue.push(url, html);

}

void parse_write_html(){

	// pair of url and html
	Pair pair = parse_queue.pop();

	// check if the html is not an empty string, if the html was empty, then the fetch to that url
	//  timed out in the fetch_html() function, so we are not going to write this to the file, since
	//  there is not data
	if(pair.html != ""){
		// call the find/count function, returns mapping of words and counts for given html
		std::map<std::string, int> map;
		map = findTerms(pair.html);

		// save output to csv
		std::fstream myfile;
		// open file in append mode
		myfile.open(std::to_string(runCount) + ".csv", std::fstream::app);

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

void signal_handler(int x){

	interrupt = true;

}

void usage(){

	std::cout << "usage: ./site-tester <configuration file>" << std::endl;

}