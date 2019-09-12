# Multi-thread-Web-Server (Ganzorig Battur)

HTTP web server implemented in C using multi-threads (pthread). It can transfer files between clients of the server using HTTP protocol. It should be able to transfer anyfiletype:HTML,GIF,JPEG,TXT,etc. First we dispatch threads will get all request for client and add into the custom queue. From the queue, worker thread will be polling and return response data into client using custom Least Frequently Used(LFU) CACHE.

# How to use it
To compile the program, simply type "make" in the directory containing the makefile and code. To run the program type **"./web_server <port> <path_to_testing>/testing <num_dispatch> <num_worker> <dynamic_flag> <queue_len> <cache_entries>"**
  The server will be configurable in the following ways:
  * port​ number can be specified (you may only use ports 1025 - 65535 by default)
  * path​ is the path to your web root location from where all the files will be served
  * num_dispatcher​ is how many dispatcher threads to start up
  * num_workers​ is how many worker threads to start up
  * dynamic_flag​ indicates whether the worker thread pool size should be static or dynamic. Bydefault, it should be 0.
  * qlen​ is the fixed, bounded length of the request queue
  * cache_entries​ is the number of entries available in the cache (an alternative way to do thiswould be to specify the maximum memory used by the cache, but we are just using a limit on thenumber of entries)
  
  You can try to download a file using this command: **wget http://127.0.0.1:9000/image/jpg/29.jpg** (Please note that 127.0.0.1 means localhost)

# How it works
Our server.c program works by checking to make sure that all arguments entered match the required criteria, then if correct, changes the current working directory to the specified path. Then the specified number of worker and dispatch threads are created to handle the requests. The dispatch threads connect to the client and take in the clients request, and adds it into the queue that we implemented. The worker threads then take the requests from the queue, check to see if the request being handled is in the cache, if not it reads the file requested from disk and the request is added to the cache. During the worker thread's process, the program records how long the process took, then the information from that worker thread and request are logged and printed to the terminal. To stop the server, type "^C" in the terminal where the server was started. 

The caching mechanism used is a dynamic memory block created with malloc. When a new entry is to be added, the memory block is changed to accommodate the size of the new entry. Memory is freed by iterating through the cache freeing all of the entries, then finally freeing the cache itself to make sure there are no memory leaks. The cache implemented LFU.

My contributions included checking command line arguments, creating threads, implementing worker and dispatch threads, implemented the queue, reading from disk function, caching, bug fixing, testing and performance analysis.

