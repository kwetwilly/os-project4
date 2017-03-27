# os-project4

Kyle Williams		(kwilli20)
Thomas Franceschi	(tfrances)

System for Verifying Web Placement

Our main program, site-tester, takes in a conifg.txt file that contains parameters 
for the wait between fetches, the number of threads to fetch html and parse html, as 
well as filenames for text files that contain the URLs to fetch and keywords to search
for in those websites. It parses this config file, then loads the URLs into a queue 
and the search terms into a vector. We then intitialize the fetch_html and parse_html 
threads according to the specified numbers in the config file. The fetch threads then 
run in a while loop and while the site queue is not empty, it pops a site off and fetches 
the html using curl then places that html body into a queue to be parsed. If there are no 
sites in the queue the threads go to sleep. Whenever we place html in the parse queue we 
send a signal to wake up a parse thread to process the data. The parse threads search the 
html text for the given keywords and populate a  map where the key is the term and the 
value is the number of occurences. The map is then used to write to the output csv file 
for the given batch number. If there are no entries in the html queue the parse threads 
go to sleep. If the number of websites parsed equals the number of total websites read in, 
then an alarm is set to restart the whole process.