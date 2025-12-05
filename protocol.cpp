#include "protocol.h"
#include <sstream>
#include <algorithm>
#include <iostream>

CommandType ProtocolParser::string_to_command(const std::string& cmd_str) {
    std::string cmd_upper = cmd_str;
    std::transform(cmd_upper.begin(), cmd_upper.end(), cmd_upper.begin(), ::toupper);
    
    if (cmd_upper == "ADD") return CommandType::ADD;
    if (cmd_upper == "UPDATE") return CommandType::UPDATE;
    if (cmd_upper == "GET") return CommandType::GET;
    if (cmd_upper == "DELETE") return CommandType::DELETE;
    
    return CommandType::INVALID;
}

std::unordered_map<std::string, std::string> ProtocolParser::parse_fields(const std::string& message) {
    std::unordered_map<std::string, std::string> fields;
    std::istringstream stream(message);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Skip empty lines
        if (line.empty()) continue;
        
        // Find colon separator
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) continue;
        
        std::string field_name = line.substr(0, colon_pos);
        std::string field_value = line.substr(colon_pos + 1);
        
        // Trim whitespace
        field_name.erase(0, field_name.find_first_not_of(" \t"));
        field_name.erase(field_name.find_last_not_of(" \t") + 1);
        field_value.erase(0, field_value.find_first_not_of(" \t"));
        field_value.erase(field_value.find_last_not_of(" \t") + 1);
        
        fields[field_name] = field_value;
    }
    
    return fields;
}

Command ProtocolParser::parse_message(const std::string& message) {
    Command cmd;
    
    auto fields = parse_fields(message);
    
    // Parse method
    auto method_it = fields.find("Method");
    if (method_it == fields.end()) {
        std::cerr << "No Method field found in message" << std::endl;
        return cmd;  // Invalid command
    }
    
    cmd.type = string_to_command(method_it->second);
    if (cmd.type == CommandType::INVALID) {
        std::cerr << "Invalid method: " << method_it->second << std::endl;
        return cmd;
    }
    
    // Parse key
    auto key_it = fields.find("Key");
    if (key_it == fields.end()) {
        std::cerr << "No Key field found in message" << std::endl;
        cmd.type = CommandType::INVALID;
        return cmd;
    }
    cmd.key = key_it->second;
    
    // Parse value (optional for GET and DELETE)
    auto value_it = fields.find("Value");
    if (value_it != fields.end()) {
        cmd.value = value_it->second;
    } else if (cmd.type == CommandType::ADD || cmd.type == CommandType::UPDATE) {
        std::cerr << "No Value field found for " << method_it->second << " command" << std::endl;
        cmd.type = CommandType::INVALID;
        return cmd;
    }
    
    return cmd;
}

std::string ProtocolParser::format_response(const std::string& response) {
    return response + "\r\n";
}

std::string Response::ok_value(const std::string& value) {
    return "OK: Value=" + value;
}