/**
 * @file Config.hpp
 * @brief Configuration management utility
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace izana {
namespace utils {

/**
 * @brief Configuration management class
 */
class Config {
public:
    Config() = default;
    ~Config() = default;
    
    bool load(const std::string& filename);
    bool save(const std::string& filename) const;
    
    template<typename T>
    T get(const std::string& key, const T& default_value) const;
    
    template<typename T>
    void set(const std::string& key, const T& value);
    
    std::map<std::string, std::string> get_object(const std::string& prefix) const;
    
private:
    std::map<std::string, std::string> config_data_;
};

// Template specializations for common types
template<>
int Config::get<int>(const std::string& key, const int& default_value) const;

template<>
uint32_t Config::get<uint32_t>(const std::string& key, const uint32_t& default_value) const;

template<>
uint16_t Config::get<uint16_t>(const std::string& key, const uint16_t& default_value) const;

template<>
uint8_t Config::get<uint8_t>(const std::string& key, const uint8_t& default_value) const;

template<>
bool Config::get<bool>(const std::string& key, const bool& default_value) const;

template<>
double Config::get<double>(const std::string& key, const double& default_value) const;

template<>
std::string Config::get<std::string>(const std::string& key, const std::string& default_value) const;

template<>
std::vector<std::string> Config::get<std::vector<std::string>>(const std::string& key, const std::vector<std::string>& default_value) const;

} // namespace utils
} // namespace izana