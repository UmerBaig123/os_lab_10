#include "trace_processor.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

TraceProcessor::TraceProcessor(const std::string& trace_url) 
    : is_compressed(false) {
    
    // Check if it's a file:// URL or just a local file path
    if (trace_url.find("file://") == 0) {
        trace_file_path = trace_url.substr(7); // Remove "file://" prefix
    } else if (trace_url.find("http://") == 0 || trace_url.find("https://") == 0) {
        // It's a URL, download it
        std::string filename = get_filename_from_url(trace_url);
        trace_file_path = "./" + filename;
        
        // Check if it's a compressed file
        if (filename.find(".gz") != std::string::npos) {
            is_compressed = true;
        }
        
        // If file doesn't exist, download it
        if (!file_exists(trace_file_path)) {
            std::cout << "Downloading trace file: " << trace_url << std::endl;
            if (!download_trace_file(trace_url, trace_file_path)) {
                std::cerr << "Failed to download trace file" << std::endl;
                return;
            }
        }
    } else {
        // Assume it's a local file path
        trace_file_path = trace_url;
    }
    
    // If compressed, extract it
    if (is_compressed) {
        std::string extracted_path = trace_file_path;
        extracted_path.erase(extracted_path.length() - 3); // Remove .gz extension
        
        if (!file_exists(extracted_path)) {
            std::cout << "Extracting compressed trace file..." << std::endl;
            if (!extract_gz_file(trace_file_path, extracted_path)) {
                std::cerr << "Failed to extract trace file" << std::endl;
                return;
            }
        }
        trace_file_path = extracted_path;
    }
}

TraceProcessor::~TraceProcessor() {
    if (trace_file.is_open()) {
        trace_file.close();
    }
}

bool TraceProcessor::initialize() {
    trace_file.open(trace_file_path);
    if (!trace_file.is_open()) {
        std::cerr << "Failed to open trace file: " << trace_file_path << std::endl;
        return false;
    }
    
    std::cout << "Trace file loaded: " << trace_file_path << std::endl;
    return true;
}

bool TraceProcessor::has_next() {
    return trace_file.good() && !trace_file.eof();
}

TraceEntry TraceProcessor::get_next() {
    std::string line;
    if (std::getline(trace_file, line)) {
        // Parse the trace line
        // Cache trace format typically contains: key, size, timestamp, etc.
        std::istringstream iss(line);
        std::string key, timestamp, size_str;
        
        // Assuming format: timestamp key size
        if (iss >> timestamp >> key >> size_str) {
            size_t size = 0;
            try {
                size = std::stoull(size_str);
            } catch (const std::exception&) {
                size = 1024; // Default size if parsing fails
            }
            return TraceEntry(key, "GET", size);
        } else {
            // Simple format: just key per line
            return TraceEntry(line, "GET", 1024);
        }
    }
    
    return TraceEntry("", "EOF", 0);
}

void TraceProcessor::reset() {
    if (trace_file.is_open()) {
        trace_file.clear();
        trace_file.seekg(0, std::ios::beg);
    }
}

bool TraceProcessor::download_trace_file(const std::string& url, const std::string& local_path) {
    // Use wget or curl to download the file
    std::string command = "wget -O \"" + local_path + "\" \"" + url + "\"";
    
    // Try wget first
    int result = system(command.c_str());
    if (result == 0) {
        return true;
    }
    
    // If wget fails, try curl
    command = "curl -o \"" + local_path + "\" \"" + url + "\"";
    result = system(command.c_str());
    
    return result == 0;
}

bool TraceProcessor::extract_gz_file(const std::string& gz_path, const std::string& output_path) {
    std::string command = "gunzip -c \"" + gz_path + "\" > \"" + output_path + "\"";
    int result = system(command.c_str());
    return result == 0;
}

bool TraceProcessor::file_exists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

std::string TraceProcessor::get_filename_from_url(const std::string& url) {
    size_t last_slash = url.find_last_of('/');
    if (last_slash != std::string::npos) {
        return url.substr(last_slash + 1);
    }
    return url;
}