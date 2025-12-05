#ifndef TRACE_PROCESSOR_H
#define TRACE_PROCESSOR_H

#include <string>
#include <vector>
#include <fstream>
#include <memory>

struct TraceEntry {
    std::string key;
    std::string operation; // Typically "GET" for cache traces
    size_t size;          // Object size if available
    
    TraceEntry(const std::string& k, const std::string& op = "GET", size_t s = 0)
        : key(k), operation(op), size(s) {}
};

class TraceProcessor {
private:
    std::string trace_file_path;
    std::ifstream trace_file;
    bool is_compressed;
    
    bool download_trace_file(const std::string& url, const std::string& local_path);
    bool extract_gz_file(const std::string& gz_path, const std::string& output_path);
    
public:
    TraceProcessor(const std::string& trace_url);
    ~TraceProcessor();
    
    bool initialize();
    bool has_next();
    TraceEntry get_next();
    void reset();
    
    // Utility functions
    static bool file_exists(const std::string& filename);
    static std::string get_filename_from_url(const std::string& url);
};

#endif // TRACE_PROCESSOR_H