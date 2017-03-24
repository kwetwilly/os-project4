# os-project4

Kyle Williams		(kwilli20)
Thomas Franceschi	(tfrances)

System for Verifying Web Placement

Our main program, site-tester, takes in a conifg.txt file that contains parameters 
for the wait between fetches, the number of threads to fetch html and parse html, as 
well as filenames for text files that contain the URLs to fetch and keywords to search
for in those websites. It parses this config file, then loads the URLs into a queue 
and the search terms intoa vector. We then intitialize the consumer and producer threads 
according to the specified numbersin the config file. The CSV output file for the current 
run is then created and headers are appended to the top line. The program then goes into 
a while(1) loop with interrupt handling and as long as the queue of URLs is not empty, a 
producers thread pops it from the queue and fetches the html and puts it in a dict with 
they key being the URL and the value being the html body and puts it in another queue. 
The consumer threads then pop from that queue and search the html body for the given 
keywords, returning a map of the URL to a vector of ints containing the occurences of 
each search term in the html. The threads are all then joined and the run counter is 
incremented and the program sleeps for the specified amount of time before waking up and 
repeating everything.
