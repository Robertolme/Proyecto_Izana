/**
 * @file DataProcessor.hpp
 * @brief Core data processing module for signal transformations
 * 
 * This module converts raw ADC values to processed (x,y) points, applies
 * transformations like scaling and offset, and performs trigger alignment.
 * Designed for real-time processing with minimal latency.
 */

#pragma once

#include "UARTModule.hpp"
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>

namespace izana {
namespace core {

/**
 * @brief Processed data point structure
 */
struct DataPoint {
    double x;          ///< Time coordinate (seconds)
    double y;          ///< Voltage coordinate (volts)
    uint64_t timestamp_us; ///< Original timestamp
    uint8_t channel;   ///< Channel identifier
    
    DataPoint(double x_val = 0.0, double y_val = 0.0, uint64_t ts = 0, uint8_t ch = 0)
        : x(x_val), y(y_val), timestamp_us(ts), channel(ch) {}
};

/**
 * @brief Collection of processed data points
 */
using DataPointCollection = std::vector<DataPoint>;

/**
 * @brief Callback type for processed data
 */
using ProcessedDataCallback = std::function<void(const DataPointCollection&, uint8_t channel)>;

/**
 * @brief Trigger configuration structure
 */
struct TriggerConfig {
    bool enabled = false;           ///< Enable trigger functionality
    double level = 1.65;           ///< Trigger level in volts
    enum class Edge { RISING, FALLING, BOTH } edge = Edge::RISING; ///< Trigger edge
    double hysteresis = 0.1;       ///< Hysteresis in volts
    uint32_t pre_trigger_samples = 100;  ///< Samples before trigger point
    uint32_t post_trigger_samples = 900; ///< Samples after trigger point
    double timeout_ms = 1000.0;    ///< Trigger timeout in milliseconds
};

/**
 * @brief Transformation parameters for signal processing
 */
struct TransformParams {
    double time_scale = 1.0;       ///< Time scaling factor (samples/second to seconds)
    double amplitude_scale = 3.3;  ///< Maximum voltage scale (volts)
    double vertical_offset = 0.0;  ///< Vertical offset in volts
    double horizontal_offset = 0.0;///< Horizontal offset in seconds
    bool auto_scale = false;       ///< Enable automatic scaling
    double sample_rate = 1000000.0;///< Expected sample rate (Hz)
};

/**
 * @brief Data processor for real-time signal processing
 * 
 * Converts raw ADC data from UARTModule into processed (x,y) points with
 * applied transformations. Supports trigger-based alignment and multi-channel
 * processing. Thread-safe and optimized for real-time operation.
 */
class DataProcessor {
public:
    /**
     * @brief Processing configuration
     */
    struct Config {
        size_t buffer_size = 10000;        ///< Maximum buffer size per channel
        size_t collection_size = 1000;     ///< Points per collection
        double collection_interval_ms = 50.0; ///< Collection output interval
        uint8_t max_channels = 4;          ///< Maximum supported channels
        bool circular_buffer = true;       ///< Use circular buffer mode
    };

    /**
     * @brief Processing statistics
     */
    struct Statistics {
        uint64_t total_points_processed = 0;
        uint64_t total_collections_sent = 0;
        uint64_t trigger_events = 0;
        uint64_t missed_triggers = 0;
        double processing_rate_hz = 0.0;
        uint64_t last_processing_time_us = 0;
    };

    /**
     * @brief Constructor
     * @param config Processing configuration
     */
    explicit DataProcessor(const Config& config = Config{});

    /**
     * @brief Destructor
     */
    ~DataProcessor();

    /**
     * @brief Start data processing
     * @return true if started successfully
     */
    bool start();

    /**
     * @brief Stop data processing
     */
    void stop();

    /**
     * @brief Check if processor is running
     * @return true if running
     */
    bool is_running() const;

    /**
     * @brief Process incoming signal data
     * @param data Signal data from UART module
     */
    void process_signal_data(const SignalData& data);

    /**
     * @brief Set transformation parameters for a channel
     * @param channel Channel identifier
     * @param params Transformation parameters
     */
    void set_transform_params(uint8_t channel, const TransformParams& params);

    /**
     * @brief Get transformation parameters for a channel
     * @param channel Channel identifier
     * @return Current transformation parameters
     */
    TransformParams get_transform_params(uint8_t channel) const;

    /**
     * @brief Set trigger configuration for a channel
     * @param channel Channel identifier
     * @param config Trigger configuration
     */
    void set_trigger_config(uint8_t channel, const TriggerConfig& config);

    /**
     * @brief Get trigger configuration for a channel
     * @param channel Channel identifier
     * @return Current trigger configuration
     */
    TriggerConfig get_trigger_config(uint8_t channel) const;

    /**
     * @brief Set callback for processed data
     * @param callback Function to call when processed data is ready
     */
    void set_data_callback(ProcessedDataCallback callback);

    /**
     * @brief Force trigger for a channel
     * @param channel Channel identifier
     */
    void force_trigger(uint8_t channel);

    /**
     * @brief Clear buffer for a channel
     * @param channel Channel identifier
     */
    void clear_buffer(uint8_t channel);

    /**
     * @brief Clear all buffers
     */
    void clear_all_buffers();

    /**
     * @brief Get processing statistics
     * @return Current statistics
     */
    Statistics get_statistics() const;

    /**
     * @brief Reset statistics
     */
    void reset_statistics();

    /**
     * @brief Get current buffer fill level for a channel
     * @param channel Channel identifier
     * @return Buffer fill percentage (0.0 to 1.0)
     */
    double get_buffer_fill_level(uint8_t channel) const;

private:
    /**
     * @brief Internal implementation (PIMPL idiom)
     */
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // Prevent copy construction and assignment
    DataProcessor(const DataProcessor&) = delete;
    DataProcessor& operator=(const DataProcessor&) = delete;
};

/**
 * @brief Multi-channel coordinator for synchronized processing
 * 
 * Manages multiple DataProcessor instances for synchronized multi-channel
 * operation. Handles trigger coordination across channels and synchronized
 * data output.
 */
class MultiChannelProcessor {
public:
    /**
     * @brief Multi-channel configuration
     */
    struct Config {
        uint8_t num_channels = 1;
        bool synchronized_triggers = false;
        uint8_t trigger_master_channel = 0;
        double channel_skew_compensation_ns[8] = {0}; ///< Per-channel timing compensation
    };

    /**
     * @brief Constructor
     * @param config Multi-channel configuration
     */
    explicit MultiChannelProcessor(const Config& config = Config{});

    /**
     * @brief Destructor
     */
    ~MultiChannelProcessor();

    /**
     * @brief Start all channels
     * @return true if all channels started successfully
     */
    bool start();

    /**
     * @brief Stop all channels
     */
    void stop();

    /**
     * @brief Process signal data for a specific channel
     * @param data Signal data
     */
    void process_signal_data(const SignalData& data);

    /**
     * @brief Set transformation parameters for a channel
     * @param channel Channel identifier
     * @param params Transformation parameters
     */
    void set_transform_params(uint8_t channel, const TransformParams& params);

    /**
     * @brief Set trigger configuration for a channel
     * @param channel Channel identifier
     * @param config Trigger configuration
     */
    void set_trigger_config(uint8_t channel, const TriggerConfig& config);

    /**
     * @brief Set callback for processed data (all channels)
     * @param callback Function to call when processed data is ready
     */
    void set_data_callback(ProcessedDataCallback callback);

    /**
     * @brief Force synchronized trigger across all channels
     */
    void force_synchronized_trigger();

    /**
     * @brief Get processor for a specific channel
     * @param channel Channel identifier
     * @return Pointer to processor or nullptr if invalid channel
     */
    DataProcessor* get_processor(uint8_t channel);

private:
    /**
     * @brief Internal implementation (PIMPL idiom)
     */
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // Prevent copy construction and assignment
    MultiChannelProcessor(const MultiChannelProcessor&) = delete;
    MultiChannelProcessor& operator=(const MultiChannelProcessor&) = delete;
};

} // namespace core
} // namespace izana