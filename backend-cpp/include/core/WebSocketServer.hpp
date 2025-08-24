/**
 * @file WebSocketServer.hpp
 * @brief WebSocket server for real-time frontend communication
 * 
 * Provides WebSocket-based communication with the frontend, streaming
 * processed data and handling parameter updates. Supports multiple
 * concurrent clients and extensible message protocols.
 */

#pragma once

#include "DataProcessor.hpp"
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace izana {
namespace core {

/**
 * @brief WebSocket message types
 */
enum class MessageType {
    SIGNAL_DATA,          ///< Processed signal data
    PARAMETER_UPDATE,     ///< Parameter update from client
    STATUS_UPDATE,        ///< System status update
    COMMAND,              ///< Command from client
    RESPONSE,             ///< Response to client command
    ERROR                 ///< Error message
};

/**
 * @brief WebSocket message structure
 */
struct WebSocketMessage {
    MessageType type;
    std::string payload;  ///< JSON-formatted payload
    uint64_t timestamp_us;
    std::string client_id;
    
    WebSocketMessage(MessageType t, const std::string& p, const std::string& id = "")
        : type(t), payload(p), timestamp_us(0), client_id(id) {}
};

/**
 * @brief Callback type for incoming messages
 */
using MessageCallback = std::function<void(const WebSocketMessage&)>;

/**
 * @brief Callback type for client connection events
 */
using ClientEventCallback = std::function<void(const std::string& client_id, bool connected)>;

/**
 * @brief WebSocket server for frontend communication
 * 
 * Manages WebSocket connections with multiple clients, streams processed
 * signal data, and handles bidirectional parameter communication.
 * Thread-safe and designed for real-time operation.
 */
class WebSocketServer {
public:
    /**
     * @brief Server configuration
     */
    struct Config {
        uint16_t port = 8080;                ///< Server port
        std::string host = "0.0.0.0";       ///< Bind address
        size_t max_clients = 10;             ///< Maximum concurrent clients
        size_t message_queue_size = 1000;    ///< Per-client message queue size
        uint32_t ping_interval_ms = 30000;   ///< Ping interval for keepalive
        uint32_t ping_timeout_ms = 10000;    ///< Ping timeout
        bool enable_compression = true;       ///< Enable message compression
        std::string cors_origin = "*";       ///< CORS allowed origin
    };

    /**
     * @brief Client connection information
     */
    struct ClientInfo {
        std::string id;
        std::string remote_address;
        uint64_t connect_time_us;
        uint64_t last_activity_us;
        size_t messages_sent;
        size_t messages_received;
        bool authenticated;
    };

    /**
     * @brief Server statistics
     */
    struct Statistics {
        uint64_t total_connections = 0;
        uint64_t active_connections = 0;
        uint64_t total_messages_sent = 0;
        uint64_t total_messages_received = 0;
        uint64_t total_bytes_sent = 0;
        uint64_t total_bytes_received = 0;
        double messages_per_second = 0.0;
    };

    /**
     * @brief Constructor
     * @param config Server configuration
     */
    explicit WebSocketServer(const Config& config = Config{});

    /**
     * @brief Destructor
     */
    ~WebSocketServer();

    /**
     * @brief Start WebSocket server
     * @return true if started successfully
     */
    bool start();

    /**
     * @brief Stop WebSocket server
     */
    void stop();

    /**
     * @brief Check if server is running
     * @return true if running
     */
    bool is_running() const;

    /**
     * @brief Get server port
     * @return Server port number
     */
    uint16_t get_port() const;

    /**
     * @brief Broadcast message to all connected clients
     * @param message Message to broadcast
     */
    void broadcast_message(const WebSocketMessage& message);

    /**
     * @brief Send message to specific client
     * @param client_id Client identifier
     * @param message Message to send
     * @return true if sent successfully
     */
    bool send_message(const std::string& client_id, const WebSocketMessage& message);

    /**
     * @brief Broadcast processed data to all clients
     * @param data Processed data points
     * @param channel Channel identifier
     */
    void broadcast_data(const DataPointCollection& data, uint8_t channel);

    /**
     * @brief Send system status update to all clients
     * @param status_json JSON-formatted status
     */
    void broadcast_status(const std::string& status_json);

    /**
     * @brief Send error message to specific client
     * @param client_id Client identifier
     * @param error_message Error description
     */
    void send_error(const std::string& client_id, const std::string& error_message);

    /**
     * @brief Set callback for incoming messages
     * @param callback Function to call when message is received
     */
    void set_message_callback(MessageCallback callback);

    /**
     * @brief Set callback for client connection events
     * @param callback Function to call on client connect/disconnect
     */
    void set_client_event_callback(ClientEventCallback callback);

    /**
     * @brief Get list of connected clients
     * @return Vector of client information
     */
    std::vector<ClientInfo> get_connected_clients() const;

    /**
     * @brief Disconnect specific client
     * @param client_id Client identifier
     * @param reason Disconnection reason
     */
    void disconnect_client(const std::string& client_id, const std::string& reason = "");

    /**
     * @brief Get server statistics
     * @return Current statistics
     */
    Statistics get_statistics() const;

    /**
     * @brief Reset statistics
     */
    void reset_statistics();

    /**
     * @brief Set message rate limiting
     * @param messages_per_second Maximum messages per second per client
     */
    void set_rate_limit(double messages_per_second);

    /**
     * @brief Enable or disable message compression
     * @param enabled Compression enabled
     */
    void set_compression_enabled(bool enabled);

private:
    /**
     * @brief Internal implementation (PIMPL idiom)
     */
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // Prevent copy construction and assignment
    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;
};

/**
 * @brief JSON utilities for WebSocket message formatting
 */
namespace json_utils {

/**
 * @brief Convert DataPointCollection to JSON string
 * @param data Data points to convert
 * @param channel Channel identifier
 * @return JSON string
 */
std::string data_points_to_json(const DataPointCollection& data, uint8_t channel);

/**
 * @brief Convert TransformParams to JSON string
 * @param params Transform parameters
 * @return JSON string
 */
std::string transform_params_to_json(const TransformParams& params);

/**
 * @brief Parse TransformParams from JSON string
 * @param json_str JSON string
 * @return Parsed parameters
 * @throws std::runtime_error if parsing fails
 */
TransformParams json_to_transform_params(const std::string& json_str);

/**
 * @brief Convert TriggerConfig to JSON string
 * @param config Trigger configuration
 * @return JSON string
 */
std::string trigger_config_to_json(const TriggerConfig& config);

/**
 * @brief Parse TriggerConfig from JSON string
 * @param json_str JSON string
 * @return Parsed configuration
 * @throws std::runtime_error if parsing fails
 */
TriggerConfig json_to_trigger_config(const std::string& json_str);

/**
 * @brief Create status update JSON
 * @param uart_connected UART connection status
 * @param channels_active Active channels bitmask
 * @param sampling_rate Current sampling rate
 * @return JSON string
 */
std::string create_status_json(bool uart_connected, uint8_t channels_active, double sampling_rate);

/**
 * @brief Create error response JSON
 * @param error_code Error code
 * @param error_message Error message
 * @return JSON string
 */
std::string create_error_json(int error_code, const std::string& error_message);

} // namespace json_utils

} // namespace core
} // namespace izana