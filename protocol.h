#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <unordered_map>

enum class CommandType {
    ADD,
    UPDATE,
    GET,
    DELETE,
    INVALID
};

struct Command {
    CommandType type;
    std::string key;
    std::string value;
    
    Command() : type(CommandType::INVALID) {}
};

class ProtocolParser {
public:
    static Command parse_message(const std::string& message);
    static std::string format_response(const std::string& response);
    
private:
    static CommandType string_to_command(const std::string& cmd_str);
    static std::unordered_map<std::string, std::string> parse_fields(const std::string& message);
};

// Response formatting helpers
namespace Response {
    const std::string OK_ADDED = "OK: Key added successfully.";
    const std::string OK_UPDATED = "OK: Key updated successfully.";
    const std::string OK_DELETED = "OK: Key deleted successfully.";
    const std::string ERROR_EXISTS = "ERROR: Key already exists.";
    const std::string ERROR_NOT_FOUND = "ERROR: Key not found.";
    const std::string ERROR_NO_SPACE = "ERROR: Not enough contiguous space.";
    const std::string ERROR_ACCESS_DENIED = "ERROR: Access denied to key.";
    const std::string ERROR_INVALID_COMMAND = "ERROR: Invalid command format.";
    
    std::string ok_value(const std::string& value);
}

#endif // PROTOCOL_H