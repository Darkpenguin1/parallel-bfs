#include <iostream>
#include <string>
#include <queue>
#include <unordered_set>
#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <stdexcept>
#include "rapidjson/error/error.h"
#include "rapidjson/reader.h"
#include <thread>
#include <mutex>
#include <algorithm>



struct ParseException : std::runtime_error, rapidjson::ParseResult {
    ParseException(rapidjson::ParseErrorCode code, const char* msg, size_t offset) : 
        std::runtime_error(msg), 
        rapidjson::ParseResult(code, offset) {}
};

#define RAPIDJSON_PARSE_ERROR_NORETURN(code, offset) \
    throw ParseException(code, #code, offset)

#include <rapidjson/document.h>
#include <chrono>


/*
  Threading NOTES


  You must never share the same handle in multiple threads. You can pass the handles around among threads, 
  but you must never use a single handle from more than one thread at any given time.

*/

bool debug = false;

// Updated service URL
const std::string SERVICE_URL = "http://hollywood-graph-crawler.bridgesuncc.org/neighbors/";

// Function to HTTP ecnode parts of URLs. for instance, replace spaces with '%20' for URLs
std::string url_encode(CURL* curl, std::string input) {
  char* out = curl_easy_escape(curl, input.c_str(), input.size());
  std::string s = out;
  curl_free(out);
  return s;
}

// Callback function for writing response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// Function to fetch neighbors using libcurl with debugging
std::string fetch_neighbors(CURL* curl, const std::string& node) {

  std::string url = SERVICE_URL + url_encode(curl, node);
  std::string response;

    if (debug)
      std::cout << "Sending request to: " << url << std::endl;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L); // Verbose Logging

    // Set a User-Agent header to avoid potential blocking by the server
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "User-Agent: C++-Client/1.0");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
    } else {
      if (debug)
        std::cout << "CURL request successful!" << std::endl;
    }

    // Cleanup
    curl_slist_free_all(headers);

    if (debug) 
      std::cout << "Response received: " << response << std::endl;  // Debug log

    return (res == CURLE_OK) ? response : "{}";

    
}

// Function to parse JSON and extract neighbors
std::vector<std::string> get_neighbors(const std::string& json_str) {
    std::vector<std::string> neighbors;
    try {
      rapidjson::Document doc;
      doc.Parse(json_str.c_str());
      
      if (doc.HasMember("neighbors") && doc["neighbors"].IsArray()) {
        for (const auto& neighbor : doc["neighbors"].GetArray())
	      neighbors.push_back(neighbor.GetString());
      }
    } catch (const ParseException& e) {
      std::cerr<<"Error while parsing JSON: "<<json_str<<std::endl;
      throw e;
    }
    return neighbors;
}


void worker(const std::vector<std::string> &frontier, size_t begin, size_t end, 
  std::unordered_set<std::string> &visited, std::vector<std::string> &nextFrontier, std::mutex &mu
) 
{
  CURL *curl = curl_easy_init();
  if (!curl) return;
  std::vector<std::string> localNext;

  for (size_t i = begin; i < end; i++){
    const std::string &currNode = frontier[i]; 

    // instead of locking the mutex each time we add one value we will batch process later when have all the values
    for (auto neighbor : get_neighbors(fetch_neighbors(curl, currNode))) {
      localNext.push_back(std::move(neighbor)); // without move we lose the str value ??? 
    }
    

  }
  {
      std::lock_guard<std::mutex> lock(mu);
      for (auto& node : localNext){
        if (visited.insert(node).second) {  // if insertion was succesful add to the next level to be processed next
          nextFrontier.push_back(std::move(node));
        }
      }
  }
  curl_easy_cleanup(curl);

}



// BFS Traversal Function
std::vector<std::vector<std::string>> bfs(const std::string& start, int depth, std::mutex &mu) {
  constexpr size_t MAX_THREADS = 8;
  std::vector<std::vector<std::string>> levels;
  std::unordered_set<std::string> visited;
  
  levels.push_back({start});
  visited.insert(start);

  // First initialize an empty vector for each level
  for (int d = 0;  d < depth; d++) {
    if (debug)
      std::cout<<"starting level: "<<d<<"\n";
    levels.push_back({});
    const auto& frontier = levels[d];
    
    auto& nextFrontier = levels[d+1];

    if (frontier.empty()) break;

    size_t N = frontier.size();
    size_t T = std::min(MAX_THREADS, N);

    std::vector<std::thread> threads;
    threads.reserve(T);
    for (size_t t = 0; t < T; t++){
      size_t begin = (t * N) / T;
      size_t end = ((t + 1) * N) / T;
      
      threads.emplace_back(
          worker,
          std::cref(frontier),
          begin, end,
          std::ref(visited),
          std::ref(nextFrontier),
          std::ref(mu)
      );

    }
    for (auto& th : threads) th.join();
    
  }
  
  return levels;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <node_name> <depth>\n";
        return 1;
    }

    std::string start_node = argv[1];     // example "Tom%20Hanks"
    int depth;
    try {
        depth = std::stoi(argv[2]);
    } catch (const std::exception& e) {
        std::cerr << "Error: Depth must be an integer.\n";
        return 1;
    }
    std::mutex mu;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    


    const auto start{std::chrono::steady_clock::now()};

    size_t level_idx = 0;
    for (const auto& n : bfs(start_node, depth, mu)) {
      for (const auto& node : n)
        if (debug) {
          for (const auto& node : n)
            std::cout << "- " << node << "\n";
        }
    }
    
    const auto finish{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{finish - start};
    std::cout << "Time to crawl: "<<elapsed_seconds.count() << "s\n";
    
    curl_global_cleanup();

    
    return 0;
}


/*
  What does the worker thread need access to ? 
  Each thread needs its own curl obj ? 
  Frontier nodes that are known but not yet visited
  int num of nodes to process 
  a ref to the neighboring nodes 
  a ref to levels ? 
  a ref to visited ? 
*/

/* 
        
  For the multi threading portion of the application we can get the number of neighbors within the returned 
  json document. We have a max of 8 threads thus we can divide the number of nodes to the number of threads 
  if we have less nodes than threads we dont have to do any fancy compute just keep going 

  but for ex 36 nodes and 8 threads we do nodeCount / numThreads and write a for loop
  If we have a uneven number we can just front load and use the % operator

*/




/*

try {
        if (debug)
          std::cout<<"Trying to expand"<<s<<"\n";
        
        const auto &neighbors = get_neighbors(fetch_neighbors(curl, s));
        size_t nodeCount = levels[d].size();
        
        
        for (const auto& neighbor : neighbors) {
          if (debug)
            std::cout<<"neighbor "<<neighbor<<"\n";
          // Implement a mutex around this check. Checks if the node is within the current set
          
          std::lock_guard<std::mutex> lk(mu);
          if (!visited.count(neighbor)) {
            visited.insert(neighbor); // adds the neighbor to the visited set. Adds to the next level from within the levels arr
          
            levels[d+1].push_back(neighbor);
            
          }
        }
      } catch (const ParseException& e) {
        std::cerr<<"Error while fetching neighbors of: "<<s<<std::endl;
        throw e;
        }







Skeleton 
const auto &neighbors = get_neighbors(fetch_neighbors(curl, s));
  size_t nodeCount = neighbors.size();
  

	for (const auto& neighbor : neighbors) {
	  if (debug)
	    std::cout<<"neighbor "<<neighbor<<"\n";
    // Implement a mutex around this check. Checks if the node is within the current set
    setMu.lock();
	  if (!visited.count(neighbor)) {
	    visited.insert(neighbor); // adds the neighbor to the visited set. Adds to the next level from within the levels arr
      setMu.unlock();
      levelMu.lock();
	    levels[d+1].push_back(neighbor);
      levelMu.unlock();
	  }
	}


  // BFS Traversal Function
std::vector<std::vector<std::string>> bfs(CURL* curl, const std::string& start, int depth, std::mutex &mu) {
  std::vector<std::vector<std::string>> levels;
  std::unordered_set<std::string> visited;
  
  levels.push_back({start});
  visited.insert(start);

  // First initialize an empty vector for each level
  for (int d = 0;  d < depth; d++) {
    if (debug)
      std::cout<<"starting level: "<<d<<"\n";
    levels.push_back({});
    for (std::string& s : levels[d]) {
      try {
        if (debug)
          std::cout<<"Trying to expand"<<s<<"\n";
        /* 
        
          For the multi threading portion of the application we can get the number of neighbors within the returned 
          json document. We have a max of 8 threads thus we can divide the number of nodes to the number of threads 
          if we have less nodes than threads we dont have to do any fancy compute just keep going 

          but for ex 36 nodes and 8 threads we do nodeCount / numThreads and write a for loop
          If we have a uneven number we can just front load and use the % operator

        
        const auto &neighbors = get_neighbors(fetch_neighbors(curl, s));
        size_t nodeCount = levels[d].size();
        
        
        for (const auto& neighbor : neighbors) {
          if (debug)
            std::cout<<"neighbor "<<neighbor<<"\n";
          // Implement a mutex around this check. Checks if the node is within the current set
          
          std::lock_guard<std::mutex> lk(mu);
          if (!visited.count(neighbor)) {
            visited.insert(neighbor); // adds the neighbor to the visited set. Adds to the next level from within the levels arr
          
            levels[d+1].push_back(neighbor);
            
          }
        }
      } catch (const ParseException& e) {
        std::cerr<<"Error while fetching neighbors of: "<<s<<std::endl;
        throw e;
        }
    }
  }
  
  return levels;
}




  
*/


