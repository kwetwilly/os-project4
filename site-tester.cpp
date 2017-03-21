// Kyle Williams and Thomas Franceschi
// CSE 30341-01
// Professor Thain
// Project IV
// Due: 3/24/17

#include "Config.h"
#include <iostream>
#include <fstream>
#include <cstdlib>		// exit

#include <vector>

#include <cstdio>
#include <cstring>
#include <curl/curl.h>

// function prototypes
void process_search(std::vector<std::string> &, std::string);
void process_site(std::vector<std::string> &, std::string);
void usage();

std::vector<int> findTerms( std::string html, std::vector<std::string> searchVect);


struct MemoryStruct {
  char *memory;
  size_t size;
};
 
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){
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
 
std::string getinmemory_main( std::string url )
{
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
 
  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */ 
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
 
  /* get it! */ 
  res = curl_easy_perform(curl_handle);
 
  /* check for errors */ 
  if(res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
  }
  else {

    //printf("%s\n", chunk.memory);
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

	std::vector<std::string> searchVect;
	std::vector<std::string> siteVect;
	std::vector<int> currentCount;
	std::vector<std::vector<int> > allCounts;

	// process the search and site files by populating vectors, respectively
	process_search(searchVect, config_file.get_search_file());
	process_site(siteVect, config_file.get_site_file());

	//Loop through all sites
	for( size_t i = 0; i < siteVect.size(); i++){
		//Get html
		std::string html = getinmemory_main(siteVect[i]);
		//std::cout << html << std::endl;
		//Find key terms
		currentCount = findTerms(html, searchVect);
		allCounts.push_back(currentCount);
	}
	//Save output to csv
	//Iterate through 2d vector
	std::ofstream myfile;
	//myfile.open("test.csv");
	for( size_t i; i < allCounts.size(); i++){
		for( size_t j; j < allCounts[i].size(); j++){
			std::cout << allCounts[i][j];
			if(j < allCounts[i].size() - 1){
				std::cout << ',';
			}
		}
		std::cout << '\n';
	}
	//myfile.close();
}

void process_search(std::vector<std::string> &searchVect, std::string filename){

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

void process_site(std::vector<std::string> &siteVect, std::string filename){

	// site file processing
	std::ifstream inputFile;
	inputFile.open(filename.c_str());
	std::string line;

	while(!inputFile.eof()){
		inputFile >> line;
		siteVect.push_back(line);
	}

	inputFile.close();

}

void usage(){
	std::cout << "usage: ./site-tester <configuration file>" << std::endl;
}

//Find shit
std::vector<int> findTerms( std::string html, std::vector<std::string> searchVect){
	
	std::vector<int> counts;

	for ( size_t i = 0; i < searchVect.size(); i++){
		size_t pos = 0;
		size_t count = 0;
		//std::cout << "searching for number " << i << ": " << searchVect[i] << std::endl;
		while(pos != std::string::npos){
			//std::cout << "pos: " << pos << std::endl;
			pos = html.find(searchVect[i], pos);
			if( pos != std::string::npos) {
				count++;
				pos++;
			}
			else continue;
		}
		counts.push_back(count);
		//std::cout << searchVect[i] << ": " << count << std::endl;
	}
	//std::cout << "size of counts: " << counts.size() << std::endl;
	return counts;
}