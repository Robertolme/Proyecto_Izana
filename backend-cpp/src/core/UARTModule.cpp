/**
 * @file UARTModule.cpp
 * @brief Implementation of UART communication module
 * 
 * Provides UART communication with ESP32 devices for real-time signal acquisition.
 * Implements automatic port detection, error recovery, and data parsing.
 */

#include "core/UARTModule.hpp"
#include "utils/Logger.hpp"

#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <regex>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #include <setupapi.h>
#else
    #include <termios.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/select.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <errno.h>
    #include <string.h>
    #include <dirent.h>
#endif

namespace izana {
namespace core {

/**
 * @brief Internal implementation class for UARTModule (PIMPL)
 */
class UARTModule::Impl {
public:
    explicit Impl(const Config& config);
    ~Impl();
    
    bool start();
    void stop();
    bool is_connected() const;
    Status get_status() const;
    std::string get_connected_port() const;
    
    void set_signal_callback(SignalDataCallback callback);
    void set_status_callback(ConnectionStatusCallback callback);
    
    bool send_command(const std::string& command);
    Statistics get_statistics() const;
    void reset_statistics();

private:
    // Configuration
    Config config_;
    
    // Connection state
    std::atomic<Status> status_;
    std::atomic<bool> should_run_;
    std::string connected_port_;
    mutable std::mutex port_mutex_;
    
    // Threads
    std::thread connection_thread_;
    std::thread data_thread_;
    
    // Callbacks
    SignalDataCallback signal_callback_;
    ConnectionStatusCallback status_callback_;
    mutable std::mutex callback_mutex_;
    
    // Serial port handle (platform specific)
#ifdef _WIN32
    HANDLE serial_handle_;
#else
    int serial_fd_;
#endif
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    std::chrono::steady_clock::time_point last_stats_update_;
    
    // Data processing
    std::string data_buffer_;
    std::mutex buffer_mutex_;
    
    // Private methods
    void connection_worker();
    void data_worker();
    bool try_open_port(const std::string& port);
    void close_port();
    bool configure_port();
    bool read_data();
    void parse_data(const std::string& data);
    void update_status(Status new_status, const std::string& port = "");
    void call_status_callback(bool connected, const std::string& port);
    void call_signal_callback(const SignalData& data);
    std::vector<std::string> get_available_ports();
    bool is_port_available(const std::string& port);
    void update_statistics(bool packet_received, size_t bytes = 0);
};

UARTModule::Impl::Impl(const Config& config) 
    : config_(config)
    , status_(Status::DISCONNECTED)
    , should_run_(false)
    , connected_port_("")
#ifdef _WIN32
    , serial_handle_(INVALID_HANDLE_VALUE)
#else
    , serial_fd_(-1)
#endif
    , last_stats_update_(std::chrono::steady_clock::now())
{
    // Initialize statistics
    reset_statistics();
}

UARTModule::Impl::~Impl() {
    stop();
}

bool UARTModule::Impl::start() {
    if (should_run_.load()) {
        return false; // Already running
    }
    
    should_run_ = true;
    update_status(Status::CONNECTING);
    
    // Start connection management thread
    connection_thread_ = std::thread(&Impl::connection_worker, this);
    
    // Start data reading thread
    data_thread_ = std::thread(&Impl::data_worker, this);
    
    utils::Logger::instance().info("UARTModule started");
    return true;
}

void UARTModule::Impl::stop() {
    if (!should_run_.load()) {
        return; // Already stopped
    }
    
    should_run_ = false;
    
    // Close port to unblock threads
    close_port();
    
    // Wait for threads to finish
    if (connection_thread_.joinable()) {
        connection_thread_.join();
    }
    
    if (data_thread_.joinable()) {
        data_thread_.join();
    }
    
    update_status(Status::DISCONNECTED);
    utils::Logger::instance().info("UARTModule stopped");
}

bool UARTModule::Impl::is_connected() const {
    return status_.load() == Status::CONNECTED;
}

UARTModule::Status UARTModule::Impl::get_status() const {
    return status_.load();
}

std::string UARTModule::Impl::get_connected_port() const {
    std::lock_guard<std::mutex> lock(port_mutex_);
    return connected_port_;
}

void UARTModule::Impl::set_signal_callback(SignalDataCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    signal_callback_ = callback;
}

void UARTModule::Impl::set_status_callback(ConnectionStatusCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    status_callback_ = callback;
}

bool UARTModule::Impl::send_command(const std::string& command) {
    if (!is_connected()) {
        return false;
    }
    
    std::string cmd_with_newline = command + "\n";
    
#ifdef _WIN32
    DWORD bytes_written;
    return WriteFile(serial_handle_, cmd_with_newline.c_str(), 
                    cmd_with_newline.length(), &bytes_written, NULL) &&
           bytes_written == cmd_with_newline.length();
#else
    ssize_t bytes_written = write(serial_fd_, cmd_with_newline.c_str(), cmd_with_newline.length());
    return bytes_written == static_cast<ssize_t>(cmd_with_newline.length());
#endif
}

UARTModule::Statistics UARTModule::Impl::get_statistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void UARTModule::Impl::reset_statistics() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = Statistics{};
    last_stats_update_ = std::chrono::steady_clock::now();
}

void UARTModule::Impl::connection_worker() {
    utils::Logger::instance().info("Connection worker started");
    
    while (should_run_.load()) {
        if (!is_connected()) {
            // Try to establish connection
            bool connected = false;
            
            for (const auto& port : config_.ports_to_try) {
                if (!should_run_.load()) break;
                
                if (try_open_port(port)) {
                    connected = true;
                    break;
                }
                
                // Brief delay between port attempts
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            
            if (!connected && config_.auto_reconnect) {
                // Wait before retrying
                std::this_thread::sleep_for(std::chrono::milliseconds(config_.reconnect_delay_ms));
            }
        } else {
            // Check connection health
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    utils::Logger::instance().info("Connection worker stopped");
}

void UARTModule::Impl::data_worker() {
    utils::Logger::instance().info("Data worker started");
    
    while (should_run_.load()) {
        if (is_connected()) {
            if (!read_data()) {
                // Read error, connection likely lost
                update_status(Status::ERROR);
                close_port();
            }
        } else {
            // Wait for connection
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    utils::Logger::instance().info("Data worker stopped");
}

bool UARTModule::Impl::try_open_port(const std::string& port) {
    utils::Logger::instance().info("Trying to open port: " + port);
    
#ifdef _WIN32
    std::string win_port = "\\\\.\\" + port;
    serial_handle_ = CreateFileA(
        win_port.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (serial_handle_ == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    if (!configure_port()) {
        CloseHandle(serial_handle_);
        serial_handle_ = INVALID_HANDLE_VALUE;
        return false;
    }
    
#else
    serial_fd_ = open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    
    if (serial_fd_ == -1) {
        return false;
    }
    
    if (!configure_port()) {
        close(serial_fd_);
        serial_fd_ = -1;
        return false;
    }
#endif
    
    // Update state
    {
        std::lock_guard<std::mutex> lock(port_mutex_);
        connected_port_ = port;
    }
    
    update_status(Status::CONNECTED, port);
    
    utils::Logger::instance().info("Successfully connected to: " + port);
    return true;
}

void UARTModule::Impl::close_port() {
#ifdef _WIN32
    if (serial_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(serial_handle_);
        serial_handle_ = INVALID_HANDLE_VALUE;
    }
#else
    if (serial_fd_ != -1) {
        close(serial_fd_);
        serial_fd_ = -1;
    }
#endif
    
    {
        std::lock_guard<std::mutex> lock(port_mutex_);
        connected_port_.clear();
    }
}

bool UARTModule::Impl::configure_port() {
#ifdef _WIN32
    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    
    if (!GetCommState(serial_handle_, &dcb)) {
        return false;
    }
    
    dcb.BaudRate = config_.baud_rate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fAbortOnError = FALSE;
    
    if (!SetCommState(serial_handle_, &dcb)) {
        return false;
    }
    
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 10;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    
    return SetCommTimeouts(serial_handle_, &timeouts);
    
#else
    struct termios tty;
    
    if (tcgetattr(serial_fd_, &tty) != 0) {
        return false;
    }
    
    // Configure baud rate
    speed_t speed;
    switch (config_.baud_rate) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        case 460800: speed = B460800; break;
        case 921600: speed = B921600; break;
        default: return false;
    }
    
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    
    // 8N1 configuration
    tty.c_cflag &= ~PARENB;   // No parity
    tty.c_cflag &= ~CSTOPB;   // One stop bit
    tty.c_cflag &= ~CSIZE;    // Clear size mask
    tty.c_cflag |= CS8;       // 8 data bits
    tty.c_cflag &= ~CRTSCTS;  // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable reading, ignore control lines
    
    // Configure input processing
    tty.c_lflag &= ~ICANON;   // Raw input
    tty.c_lflag &= ~ECHO;     // No echo
    tty.c_lflag &= ~ECHOE;    // No echo erase
    tty.c_lflag &= ~ECHONL;   // No echo newline
    tty.c_lflag &= ~ISIG;     // No signal handling
    
    // Configure input flags
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // No software flow control
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    
    // Configure output processing
    tty.c_oflag &= ~OPOST;    // Raw output
    tty.c_oflag &= ~ONLCR;    // No CR to NL conversion
    
    // Configure timeouts
    tty.c_cc[VTIME] = 1;      // 0.1 second timeout
    tty.c_cc[VMIN] = 0;       // Non-blocking read
    
    return tcsetattr(serial_fd_, TCSANOW, &tty) == 0;
#endif
}

bool UARTModule::Impl::read_data() {
    char buffer[1024];
    size_t bytes_read = 0;
    
#ifdef _WIN32
    DWORD dwBytesRead;
    if (!ReadFile(serial_handle_, buffer, sizeof(buffer) - 1, &dwBytesRead, NULL)) {
        return false;
    }
    bytes_read = dwBytesRead;
#else
    ssize_t result = read(serial_fd_, buffer, sizeof(buffer) - 1);
    if (result < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available, not an error
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return true;
        }
        return false;
    }
    bytes_read = result;
#endif
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        
        // Add to buffer and process complete lines
        {
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            data_buffer_.append(buffer, bytes_read);
        }
        
        // Process complete lines
        std::string line;
        {
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            size_t pos;
            while ((pos = data_buffer_.find('\n')) != std::string::npos) {
                line = data_buffer_.substr(0, pos);
                data_buffer_.erase(0, pos + 1);
                
                if (!line.empty()) {
                    parse_data(line);
                }
            }
        }
        
        update_statistics(true, bytes_read);
    }
    
    return true;
}

void UARTModule::Impl::parse_data(const std::string& data) {
    // Parse signal data in format "signal:123"
    static const std::regex signal_regex(R"(signal:(\d{1,3}))");
    std::smatch match;
    
    if (std::regex_match(data, match, signal_regex)) {
        try {
            uint16_t raw_value = static_cast<uint16_t>(std::stoi(match[1]));
            
            // Clamp to 9-bit range
            if (raw_value > 511) {
                raw_value = 511;
            }
            
            // Convert to voltage (assuming 3.3V reference)
            double voltage = (static_cast<double>(raw_value) / 511.0) * 3.3;
            
            // Get timestamp
            auto now = std::chrono::steady_clock::now();
            uint64_t timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch()
            ).count();
            
            // Create signal data
            SignalData signal_data(raw_value, voltage, timestamp_us, 0); // Channel 0 by default
            
            call_signal_callback(signal_data);
            
        } catch (const std::exception& e) {
            utils::Logger::instance().warn("Failed to parse signal data: " + data);
            update_statistics(false);
        }
    } else {
        // Unknown data format
        utils::Logger::instance().debug("Unrecognized data: " + data);
        update_statistics(false);
    }
}

void UARTModule::Impl::update_status(Status new_status, const std::string& port) {
    Status old_status = status_.exchange(new_status);
    
    if (old_status != new_status) {
        bool connected = (new_status == Status::CONNECTED);
        call_status_callback(connected, port);
    }
}

void UARTModule::Impl::call_status_callback(bool connected, const std::string& port) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (status_callback_) {
        try {
            status_callback_(connected, port);
        } catch (const std::exception& e) {
            utils::Logger::instance().error("Status callback error: " + std::string(e.what()));
        }
    }
}

void UARTModule::Impl::call_signal_callback(const SignalData& data) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (signal_callback_) {
        try {
            signal_callback_(data);
        } catch (const std::exception& e) {
            utils::Logger::instance().error("Signal callback error: " + std::string(e.what()));
        }
    }
}

void UARTModule::Impl::update_statistics(bool packet_received, size_t bytes) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    
    if (packet_received) {
        stats_.total_packets_received++;
        stats_.total_bytes_received += bytes;
        stats_.last_packet_timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()
        ).count();
    } else {
        stats_.total_packets_dropped++;
    }
    
    // Update rate calculation every second
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_stats_update_);
    if (time_diff.count() >= 1000) {
        double time_seconds = time_diff.count() / 1000.0;
        stats_.packets_per_second = stats_.total_packets_received / time_seconds;
        last_stats_update_ = now;
    }
}

// UARTModule public interface implementation

UARTModule::UARTModule(const Config& config) 
    : pimpl_(std::make_unique<Impl>(config)) {
}

UARTModule::~UARTModule() = default;

bool UARTModule::start() {
    return pimpl_->start();
}

void UARTModule::stop() {
    pimpl_->stop();
}

bool UARTModule::is_connected() const {
    return pimpl_->is_connected();
}

UARTModule::Status UARTModule::get_status() const {
    return pimpl_->get_status();
}

std::string UARTModule::get_connected_port() const {
    return pimpl_->get_connected_port();
}

void UARTModule::set_signal_callback(SignalDataCallback callback) {
    pimpl_->set_signal_callback(callback);
}

void UARTModule::set_status_callback(ConnectionStatusCallback callback) {
    pimpl_->set_status_callback(callback);
}

bool UARTModule::send_command(const std::string& command) {
    return pimpl_->send_command(command);
}

UARTModule::Statistics UARTModule::get_statistics() const {
    return pimpl_->get_statistics();
}

void UARTModule::reset_statistics() {
    pimpl_->reset_statistics();
}

} // namespace core
} // namespace izana