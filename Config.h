// Kyle Williams and Thomas Franceschi
// CSE 30341-01
// Professor Thain
// Project IV
// Due: 3/24/17

// Configuration File Class

#include <fstream>		// fstream
#include <string>		// stoi
#include <iostream>		// cout
#include <unistd.h> 	// access
#include <cstring> 		// strerror
#include <cerrno> 		// errno
#include <cstdlib>		// exit
#include <exception> 	// exception

class Config{

public:
	Config(std::string filename);

	int get_period_fetch() 			{return period_fetch;}
	int get_num_fetch()				{return num_fetch;}
	int get_num_parse()				{return num_parse;}
	std::string get_search_file()	{return search_file;}
	std::string get_site_file()		{return site_file;}

private:
	int period_fetch;
	int num_fetch;
	int num_parse;
	std::string search_file;
	std::string site_file;

};

Config::Config(std::string filename){

	// set default values
	period_fetch = 180;
	num_fetch = 1;
	num_parse = 1;
	search_file = "Search.txt";
	site_file = "Sites.txt";

	// open input configuration file
	std::ifstream inputFile;
	inputFile.open(filename.c_str());
	std::string line;
	std::string param;
	std::string value;

	// loop trough lines of file and set parameters respectively
	while(!inputFile.eof()){

		inputFile >> line;

		// find string position of '='
		int delim_pos = line.find("=");

		// set left of '=' to parameter and right of '=' to value
		param = line.substr(0, delim_pos);
		value = line.substr(delim_pos+1, line.length());

		if(param == "PERIOD_FETCH"){
			try{
				period_fetch = std::stoi(value);
			}
			// catch anything that cannot be converted to integer
			catch(std::exception& e){
				std::cout << "site-tester: PERIOD_FETCH Error: " << e.what() << " of " << value << " to integer" << std::endl;
				exit(EXIT_FAILURE);
			}

			// make sure period time is not negative
			if(period_fetch < 0){
				std::cout << "site-tester: PERIOD_FETCH Error: PERIOD_FETCH time must be positive integer" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		else if(param == "NUM_FETCH"){
			try{
				num_fetch = std::stoi(value);
			}
			// catch anything that cannot be converted to integer
			catch(std::exception& e){
				std::cout << "site-tester: NUM_FETCH Error: " << e.what() << " of " << value << " to integer" << std::endl;
				exit(EXIT_FAILURE);
			}

			// check that fetch threads are within range
			if(num_fetch < 1){
				std::cout << "site-tester: NUM_FETCH Error: Not enough fetch threads in config file. MIN threads is 1" << std::endl;
				exit(EXIT_FAILURE);
			}
			if(num_fetch > 8){
				std::cout << "site-tester: NUM_FETCH Error: Too many fetch threads in config file. MAX threads is 8" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		else if(param == "NUM_PARSE"){
			try{
				num_parse = std::stoi(value);
			}
			// catch anything that cannot be converted to integer
			catch(std::exception& e){
				std::cout << "site-tester: NUM_PARSE Error: " << e.what() << " of " << value << " to integer" << std::endl;
				exit(EXIT_FAILURE);
			}

			// check that parse threads are within range
			if(num_parse < 1){
				std::cout << "site-tester: NUM_PARSE Error: Not enough parse threads in config file. MIN threads is 1" << std::endl;
				exit(EXIT_FAILURE);
			}
			if(num_parse > 8){
				std::cout << "site-tester: NUM_PARSE Error: Too many parse threads in config file. MAX threads is 8" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		else if(param == "SEARCH_FILE"){
			search_file = value;

			// check if search file exists
			int search_result = access(search_file.c_str(), R_OK);
			if(search_result < 0){
				std::cout << "site-tester: Search File Error: " << value << ": " << strerror(errno) << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		else if(param == "SITES_FILE"){
			site_file = value;

			// check if site file exists
			int site_result = access(site_file.c_str(), R_OK);
			if(site_result < 0){
				std::cout << "site-tester: Site File Error: " << value << ": " << strerror(errno) << std::endl;
				exit(EXIT_FAILURE);
			}
		} else{
			std::cout << "site-tester: Warning. Unknown parameter found in configuration file." << std::endl;
		}

	}

}
