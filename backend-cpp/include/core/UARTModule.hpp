/**
 * @file UARTModule.hpp
 * @brief UART communication module for ESP32 interface
 * 
 * This module handles serial communication with ESP32 devices, providing
 * automatic port detection, connection management, and data parsing.
 * Designed for high-throughput data streaming with robust error handling.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

namespace izana {
namespace core {

/**
 * @brief Structure to hold parsed signal data from ESP32
 */
struct SignalData {
    uint16_t raw_value;      ///< Raw ADC value (0-511)
    double voltage;          ///< Converted voltage value
    uint64_t timestamp_us;   ///< Timestamp in microseconds
    uint8_t channel;         ///< Channel identifier (for multi-channel support)
    
    SignalData(uint16_t raw = 0, double volt = 0.0, uint64_t ts = 0, uint8_t ch = 0)
        : raw_value(raw), voltage(volt), timestamp_us(ts), channel(ch) {}
};

/**
 * @brief Callback type for signal data reception
 */
using SignalDataCallback = std::function<void(const SignalData&)>;

/**
 * @brief Callback type for connection status changes
 */
using ConnectionStatusCallback = std::function<void(bool connected, const std::string& port)>;

/**
 * @brief UART communication module for ESP32 interface
 * 
 * Provides high-level interface for UART communication with ESP32 devices.
 * Features automatic port detection, reconnection logic, and data parsing.
 * Thread-safe and designed for real-time operation.
 */
class UARTModule {
public:
    /**
     * @brief Configuration structure for UART parameters
     */
    struct Config {
        std::vector<std::string> ports_to_try = {
            "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyACM0", "/dev/ttyACM1",
            "COM3", "COM4", "COM5", "COM6"
        };
        uint32_t baud_rate = 115200;        ///< Baud rate for communication
        uint32_t timeout_ms = 5000;         ///< Connection timeout
        bool auto_reconnect = true;         ///< Enable automatic reconnection
        uint32_t reconnect_delay_ms = 3000; ///< Delay between reconnection attempts
        size_t buffer_size = 4096;          ///< Internal buffer size
    };

    /**
     * @brief Connection status enumeration
     */
    enum class Status {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        ERROR
    };

    /**
     * @brief Constructor
     * @param config Configuration parameters
     */
    explicit UARTModule(const Config& config = Config{});

    /**
     * @brief Destructor
     */
    ~UARTModule();

    /**
     * @brief Start UART communication
     * @return true if initialization successful
     */
    bool start();

    /**
     * @brief Stop UART communication
     */
    void stop();

    /**
     * @brief Check if UART is connected
     * @return true if connected
     */
    bool is_connected() const;

    /**
     * @brief Get current connection status
     * @return Current status
     */
    Status get_status() const;

    /**
     * @brief Get connected port name
     * @return Port name or empty string if disconnected
     */
    std::string get_connected_port() const;

    /**
     * @brief Set signal data callback
     * @param callback Function to call when signal data is received
     */
    void set_signal_callback(SignalDataCallback callback);

    /**
     * @brief Set connection status callback
     * @param callback Function to call when connection status changes
     */
    void set_status_callback(ConnectionStatusCallback callback);

    /**
     * @brief Send command to ESP32
     * @param command Command string to send
     * @return true if sent successfully
     */
    bool send_command(const std::string& command);

    /**
     * @brief Get statistics about data reception
     */
    struct Statistics {
        uint64_t total_packets_received = 0;
        uint64_t total_packets_dropped = 0;
        uint64_t total_bytes_received = 0;
        double packets_per_second = 0.0;
        uint64_t last_packet_timestamp_us = 0;
    };

    /**
     * @brief Get reception statistics
     * @return Current statistics
     */
    Statistics get_statistics() const;

    /**
     * @brief Reset statistics counters
     */
    void reset_statistics();

private:
    /**
     * @brief Internal implementation (PIMPL idiom)
     */
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // Prevent copy construction and assignment
    UARTModule(const UARTModule&) = delete;
    UARTModule& operator=(const UARTModule&) = delete;
};

} // namespace core
} // namespace izana