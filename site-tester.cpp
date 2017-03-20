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

// function prototypes
void process_search(std::vector<std::string> &, std::string);
void process_site(std::vector<std::string> &, std::string);
void usage();

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

	// std::cout << config_file.get_period_fetch() << std::endl;
	// std::cout << config_file.get_num_fetch() << std::endl;
	// std::cout << config_file.get_num_parse() << std::endl;
	// std::cout << config_file.get_search_file() << std::endl;
	// std::cout << config_file.get_site_file() << std::endl;

	std::vector<std::string> searchVect;
	std::vector<std::string> siteVect;

	// process the search and site files by populating vectors, respectively
	process_search(searchVect, config_file.get_search_file());
	process_site(siteVect, config_file.get_site_file());

	// for(int i = 0; i < siteVect.size(); i++){
	// 	std::cout << searchVect[i] << std::endl;
	// 	std::cout << siteVect[i] << std::endl;
	// }

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