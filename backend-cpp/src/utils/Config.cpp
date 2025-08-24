/**
 * @file Config.cpp
 * @brief Implementation of configuration management
 */

#include "utils/Config.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace izana {
namespace utils {

bool Config::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Simple key=value parser (not full JSON for this minimal implementation)
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            config_data_[key] = value;
        }
    }
    
    return true;
}

bool Config::save(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& pair : config_data_) {
        file << pair.first << "=" << pair.second << std::endl;
    }
    
    return true;
}

std::map<std::string, std::string> Config::get_object(const std::string& prefix) const {
    std::map<std::string, std::string> result;
    std::string prefix_dot = prefix + ".";
    
    for (const auto& pair : config_data_) {
        if (pair.first.find(prefix_dot) == 0) {
            std::string key = pair.first.substr(prefix_dot.length());
            result[key] = pair.second;
        }
    }
    
    return result;
}

// Template specializations

template<>
int Config::get<int>(const std::string& key, const int& default_value) const {
    auto it = config_data_.find(key);
    if (it == config_data_.end()) {
        return default_value;
    }
    
    try {
        return std::stoi(it->second);
    } catch (const std::exception&) {
        return default_value;
    }
}

template<>
uint32_t Config::get<uint32_t>(const std::string& key, const uint32_t& default_value) const {
    auto it = config_data_.find(key);
    if (it == config_data_.end()) {
        return default_value;
    }
    
    try {
        return static_cast<uint32_t>(std::stoul(it->second));
    } catch (const std::exception&) {
        return default_value;
    }
}

template<>
uint16_t Config::get<uint16_t>(const std::string& key, const uint16_t& default_value) const {
    auto it = config_data_.find(key);
    if (it == config_data_.end()) {
        return default_value;
    }
    
    try {
        return static_cast<uint16_t>(std::stoul(it->second));
    } catch (const std::exception&) {
        return default_value;
    }
}

template<>
uint8_t Config::get<uint8_t>(const std::string& key, const uint8_t& default_value) const {
    auto it = config_data_.find(key);
    if (it == config_data_.end()) {
        return default_value;
    }
    
    try {
        return static_cast<uint8_t>(std::stoul(it->second));
    } catch (const std::exception&) {
        return default_value;
    }
}

template<>
bool Config::get<bool>(const std::string& key, const bool& default_value) const {
    auto it = config_data_.find(key);
    if (it == config_data_.end()) {
        return default_value;
    }
    
    std::string value = it->second;
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    
    return (value == "true" || value == "1" || value == "yes" || value == "on");
}

template<>
double Config::get<double>(const std::string& key, const double& default_value) const {
    auto it = config_data_.find(key);
    if (it == config_data_.end()) {
        return default_value;
    }
    
    try {
        return std::stod(it->second);
    } catch (const std::exception&) {
        return default_value;
    }
}

template<>
std::string Config::get<std::string>(const std::string& key, const std::string& default_value) const {
    auto it = config_data_.find(key);
    if (it == config_data_.end()) {
        return default_value;
    }
    
    return it->second;
}

template<>
std::vector<std::string> Config::get<std::vector<std::string>>(const std::string& key, const std::vector<std::string>& default_value) const {
    auto it = config_data_.find(key);
    if (it == config_data_.end()) {
        return default_value;
    }
    
    std::vector<std::string> result;
    std::stringstream ss(it->second);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        // Trim whitespace
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    
    return result.empty() ? default_value : result;
}

} // namespace utils
} // namespace izana