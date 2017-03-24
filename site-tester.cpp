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
#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <thread>
#include <csignal>

// GLOBAL VARIABLES AND STRUCTURES
QueueSiteList  sites;
QueueParseList raw_html;
std::vector<std::string> searchVect;
size_t runCount = 1;
bool interrupt = false;

// memory structure for libcurl
struct MemoryStruct {

	char *memory;
	size_t size;

};

// FUNCTION PROTOTYPES
// site URLs/search term processing
void process_search(std::string);
void process_site(std::string);

// function for getting raw html via libcurl
static size_t WriteMemoryCallback(void *, size_t, size_t, void *);
std::string getinmemory_main(std::string);
std::string body_strip(std::string html);

void usage();

// functions for producer/consumer threads
void fetch_html();
void parse_write_html();

std::map<std::string, int> findTerms(std::string html);

void signal_handler(int);

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

	// process the sites and search term files from the configuration object
	process_search(config_file.get_search_file());

	while(1){

		void (*prev_handler)(int);
		prev_handler = signal(SIGINT, signal_handler);

		process_site(config_file.get_site_file());

		while(!sites.is_empty()){

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

		runCount++;

		// wait for next period to fetch
		sleep(config_file.get_period_fetch());

	}

	delete [] producers;
	delete [] consumers;

	exit(0);

}

void process_search(std::string filename){

	// search file processing
	std::ifstream inputFile;
	inputFile.open(filename.c_str());
	std::string line;

	while(!inputFile.eof()){
		inputFile >> line;
		searchVect.push_back(line);
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
		sites.push(line);
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

	/* send all data to this function  */ 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */ 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	/* some servers don't like requests that are made without a user-agent field, so we provide one */ 
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* get it! */ 
	res = curl_easy_perform(curl_handle);

	/* check for errors */ 
	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
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
	//get start and end indeces of body content
	size_t start = html.find("<body");
	if(start == std::string::npos) return html;
	size_t end = html.find("</body>");
	if(end == std::string::npos) return html;
	size_t length = end - start - 5;
	//get substring between those values
	std::string just_the_body = html.substr(start + 5, length);
	return just_the_body;

}

void usage(){
	std::cout << "usage: ./site-tester <configuration file>" << std::endl;
}

std::map<std::string, int> findTerms( std::string html){
	
	std::map<std::string, int> counts;

	for ( size_t i = 0; i < searchVect.size(); i++){
		size_t pos = 0;
		size_t count = 0;

		while(pos != std::string::npos){

			pos = html.find(searchVect[i], pos);
			if( pos != std::string::npos) {
				count++;
				pos++;
			}
			else continue;
		}

		counts.insert({searchVect[i], count});

	}

	return counts;

}

void fetch_html(){

	std::string url = sites.pop();
	std::string html = getinmemory_main(url);
	//strip to body only
	html = body_strip(html);

	raw_html.push(url, html);

}

void parse_write_html(){

	// pair of url and html
	Pair pair = raw_html.pop();

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

void signal_handler(int x){

	interrupt = true;

}