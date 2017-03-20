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

void findTerms( std::string html, std::vector<std::string> searchVect);

////////////////////////////////////
//GETINMEMOORY//

struct MemoryStruct {
  char *memory;
  size_t size;
};
 
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
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
/*
    //Write to output file
	int front_pos = url.find("//");
	int end_pos = url.find("/", front_pos + 2);
	std::string filename = url.substr( front_pos + 2, end_pos - front_pos - 2);
	const char * myfilename = filename.c_str();

    FILE *f = fopen( myfilename, "w");
    if ( f == NULL){
      printf("you don' goofed\n");
      return 1;
    }
    fwrite(chunk.memory, sizeof(char), chunk.size, f);
    fclose(f);
  */
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

	// process the search and site files by populating vectors, respectively
	process_search(searchVect, config_file.get_search_file());
	process_site(siteVect, config_file.get_site_file());

	//Get site html
	for( int i = 0; i < siteVect.size(); i++){
		std::string html = getinmemory_main(siteVect[i]);
		findTerms(html, searchVect);
	}
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
void findTerms( std::string html, std::vector<std::string> searchVect){
	size_t pos = 0;
	size_t count = 0;
	std::vector<int> counts;
	std::string str(html);

	for ( int i = 0; i < searchVect.size(); i++){
		while(pos != std::string::npos){
			pos = html.find(searchVect[i], pos);
			if( pos != std::string::npos) count++;
		}
		counts.push_back(count);
	}
}