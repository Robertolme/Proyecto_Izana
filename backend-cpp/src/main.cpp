/**
 * @file main.cpp
 * @brief Main application entry point for Proyecto Izana backend
 * 
 * Integrates all modules and provides the main application logic for
 * the modular digital oscilloscope backend system.
 */

#include "core/UARTModule.hpp"
#include "core/DataProcessor.hpp"
#include "core/WebSocketServer.hpp"
#include "processing/ProcessingModule.hpp"
#include "processing/FFTModule.hpp"
#include "processing/FilterModule.hpp"
#include "utils/Logger.hpp"
#include "utils/Config.hpp"

#include <iostream>
#include <memory>
#include <signal.h>
#include <thread>
#include <chrono>
#include <atomic>

using namespace izana;

/**
 * @brief Global application state
 */
class Application {
public:
    Application() : running_(false) {}
    
    bool initialize();
    bool run();
    void stop();
    
private:
    void setup_signal_handlers();
    void on_signal_data(const core::SignalData& data);
    void on_processed_data(const core::DataPointCollection& data, uint8_t channel);
    void on_websocket_message(const core::WebSocketMessage& message);
    void on_connection_status(bool connected, const std::string& port);
    
    void handle_parameter_update(const std::string& json_payload);
    void handle_command(const std::string& json_payload, const std::string& client_id);
    void send_status_update();
    
    // Core modules
    std::unique_ptr<core::UARTModule> uart_module_;
    std::unique_ptr<core::MultiChannelProcessor> data_processor_;
    std::unique_ptr<core::WebSocketServer> websocket_server_;
    std::unique_ptr<processing::ProcessingPipeline> processing_pipeline_;
    
    // Configuration
    utils::Config config_;
    
    // State
    std::atomic<bool> running_;
    std::atomic<bool> uart_connected_;
    std::thread status_thread_;
    
    // Statistics
    uint64_t total_samples_processed_ = 0;
    std::chrono::steady_clock::time_point start_time_;
};

/**
 * @brief Global application instance
 */
std::unique_ptr<Application> g_app;

/**
 * @brief Signal handler for graceful shutdown
 */
void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    if (g_app) {
        g_app->stop();
    }
}

bool Application::initialize() {
    utils::Logger::instance().set_level(utils::Logger::Level::INFO);
    utils::Logger::instance().info("Initializing Proyecto Izana backend...");
    
    // Load configuration
    if (!config_.load("config.json")) {
        utils::Logger::instance().warn("Could not load config.json, using defaults");
    }
    
    // Initialize UART module
    core::UARTModule::Config uart_config;
    uart_config.baud_rate = config_.get<uint32_t>("uart.baud_rate", 115200);
    uart_config.auto_reconnect = config_.get<bool>("uart.auto_reconnect", true);
    uart_config.ports_to_try = config_.get<std::vector<std::string>>("uart.ports", {
        "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyACM0", "/dev/ttyACM1",
        "COM3", "COM4", "COM5", "COM6"
    });
    
    uart_module_ = std::make_unique<core::UARTModule>(uart_config);
    uart_module_->set_signal_callback(
        [this](const core::SignalData& data) { on_signal_data(data); }
    );
    uart_module_->set_status_callback(
        [this](bool connected, const std::string& port) { 
            on_connection_status(connected, port); 
        }
    );
    
    // Initialize data processor
    core::MultiChannelProcessor::Config processor_config;
    processor_config.num_channels = config_.get<uint8_t>("processing.num_channels", 1);
    processor_config.synchronized_triggers = config_.get<bool>("processing.synchronized_triggers", false);
    
    data_processor_ = std::make_unique<core::MultiChannelProcessor>(processor_config);
    data_processor_->set_data_callback(
        [this](const core::DataPointCollection& data, uint8_t channel) {
            on_processed_data(data, channel);
        }
    );
    
    // Initialize WebSocket server
    core::WebSocketServer::Config ws_config;
    ws_config.port = config_.get<uint16_t>("websocket.port", 8080);
    ws_config.max_clients = config_.get<size_t>("websocket.max_clients", 10);
    ws_config.cors_origin = config_.get<std::string>("websocket.cors_origin", "*");
    
    websocket_server_ = std::make_unique<core::WebSocketServer>(ws_config);
    websocket_server_->set_message_callback(
        [this](const core::WebSocketMessage& message) {
            on_websocket_message(message);
        }
    );
    
    // Initialize processing pipeline
    processing::ProcessingPipeline::Config pipeline_config;
    pipeline_config.parallel_processing = config_.get<bool>("processing.parallel", false);
    
    processing_pipeline_ = std::make_unique<processing::ProcessingPipeline>(pipeline_config);
    
    // Add processing modules based on configuration
    auto enabled_modules = config_.get<std::vector<std::string>>("processing.modules", {"fft", "filter"});
    for (const auto& module_name : enabled_modules) {
        auto module = processing::ModuleFactory::instance().create_module(module_name);
        if (module) {
            processing::ParameterMap module_params;
            // Load module-specific parameters from config
            auto module_config = config_.get_object("processing." + module_name);
            // Convert config to parameter map (simplified for this example)
            
            module->initialize(module_params);
            processing_pipeline_->add_module(std::move(module), module_name);
            utils::Logger::instance().info("Added processing module: " + module_name);
        } else {
            utils::Logger::instance().warn("Failed to create processing module: " + module_name);
        }
    }
    
    setup_signal_handlers();
    start_time_ = std::chrono::steady_clock::now();
    
    return true;
}

bool Application::run() {
    utils::Logger::instance().info("Starting Proyecto Izana backend...");
    
    // Start WebSocket server
    if (!websocket_server_->start()) {
        utils::Logger::instance().error("Failed to start WebSocket server");
        return false;
    }
    
    utils::Logger::instance().info("WebSocket server started on port " + 
                                  std::to_string(websocket_server_->get_port()));
    
    // Start data processor
    if (!data_processor_->start()) {
        utils::Logger::instance().error("Failed to start data processor");
        return false;
    }
    
    // Start UART module
    if (!uart_module_->start()) {
        utils::Logger::instance().warn("Failed to start UART module (will retry automatically)");
        // Continue running as UART module will retry connection
    }
    
    running_ = true;
    
    // Start status update thread
    status_thread_ = std::thread([this]() {
        while (running_) {
            send_status_update();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });
    
    utils::Logger::instance().info("Proyecto Izana backend started successfully");
    
    // Main loop
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    if (status_thread_.joinable()) {
        status_thread_.join();
    }
    
    return true;
}

void Application::stop() {
    utils::Logger::instance().info("Stopping Proyecto Izana backend...");
    
    running_ = false;
    
    if (uart_module_) {
        uart_module_->stop();
    }
    
    if (data_processor_) {
        data_processor_->stop();
    }
    
    if (websocket_server_) {
        websocket_server_->stop();
    }
    
    utils::Logger::instance().info("Proyecto Izana backend stopped");
}

void Application::setup_signal_handlers() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#ifndef _WIN32
    signal(SIGHUP, signal_handler);
#endif
}

void Application::on_signal_data(const core::SignalData& data) {
    // Forward signal data to data processor
    data_processor_->process_signal_data(data);
    total_samples_processed_++;
}

void Application::on_processed_data(const core::DataPointCollection& data, uint8_t channel) {
    // Apply additional processing pipeline if configured
    if (processing_pipeline_->is_ready()) {
        auto result = processing_pipeline_->process(data, channel);
        if (result.success) {
            // Send processed data via WebSocket
            websocket_server_->broadcast_data(result.processed_data, channel);
        } else {
            utils::Logger::instance().warn("Processing pipeline failed: " + result.error_message);
            // Send original data as fallback
            websocket_server_->broadcast_data(data, channel);
        }
    } else {
        // Send data directly if no processing pipeline
        websocket_server_->broadcast_data(data, channel);
    }
}

void Application::on_websocket_message(const core::WebSocketMessage& message) {
    switch (message.type) {
        case core::MessageType::PARAMETER_UPDATE:
            handle_parameter_update(message.payload);
            break;
            
        case core::MessageType::COMMAND:
            handle_command(message.payload, message.client_id);
            break;
            
        default:
            utils::Logger::instance().warn("Unhandled WebSocket message type");
            break;
    }
}

void Application::on_connection_status(bool connected, const std::string& port) {
    uart_connected_ = connected;
    
    if (connected) {
        utils::Logger::instance().info("UART connected to " + port);
    } else {
        utils::Logger::instance().warn("UART disconnected");
    }
    
    send_status_update();
}

void Application::handle_parameter_update(const std::string& json_payload) {
    try {
        // Parse JSON payload and update parameters
        // This would use a JSON library like nlohmann/json
        utils::Logger::instance().info("Received parameter update: " + json_payload);
        
        // Example: Update transform parameters for all channels
        // In a real implementation, this would parse the JSON and apply updates
        
    } catch (const std::exception& e) {
        utils::Logger::instance().error("Failed to handle parameter update: " + std::string(e.what()));
    }
}

void Application::handle_command(const std::string& json_payload, const std::string& client_id) {
    try {
        // Handle commands like trigger, reset, etc.
        utils::Logger::instance().info("Received command from " + client_id + ": " + json_payload);
        
        // Example commands:
        // - force_trigger
        // - clear_buffer
        // - reset_statistics
        // - get_status
        
    } catch (const std::exception& e) {
        utils::Logger::instance().error("Failed to handle command: " + std::string(e.what()));
        websocket_server_->send_error(client_id, "Command processing failed: " + std::string(e.what()));
    }
}

void Application::send_status_update() {
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    
    double sample_rate = 0.0;
    if (uptime > 0) {
        sample_rate = static_cast<double>(total_samples_processed_) / uptime;
    }
    
    uint8_t active_channels = 1; // Simplified for example
    
    std::string status_json = core::json_utils::create_status_json(
        uart_connected_.load(),
        active_channels,
        sample_rate
    );
    
    websocket_server_->broadcast_status(status_json);
}

/**
 * @brief Main application entry point
 */
int main(int argc, char* argv[]) {
    std::cout << "Proyecto Izana - Modular Digital Oscilloscope Backend" << std::endl;
    std::cout << "Version 1.0.0" << std::endl;
    std::cout << "==============================================" << std::endl;
    
    try {
        g_app = std::make_unique<Application>();
        
        if (!g_app->initialize()) {
            std::cerr << "Failed to initialize application" << std::endl;
            return 1;
        }
        
        if (!g_app->run()) {
            std::cerr << "Application run failed" << std::endl;
            return 1;
        }
        
        g_app.reset();
        
    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Application exited successfully" << std::endl;
    return 0;
}