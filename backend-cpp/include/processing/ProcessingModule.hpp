/**
 * @file ProcessingModule.hpp
 * @brief Base class and interface for signal processing modules
 * 
 * Defines the extensible processing module architecture that allows
 * adding new signal analysis capabilities like FFT, filters, and
 * data storage without modifying the core system.
 */

#pragma once

#include "../core/DataProcessor.hpp"
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <functional>

namespace izana {
namespace processing {

/**
 * @brief Module parameter value variant type
 */
struct ModuleParameter {
    enum Type { INT, DOUBLE, STRING, BOOL, VECTOR_DOUBLE };
    
    Type type;
    union {
        int int_value;
        double double_value;
        bool bool_value;
    };
    std::string string_value;
    std::vector<double> vector_value;
    
    // Constructors for different types
    ModuleParameter(int val) : type(INT), int_value(val) {}
    ModuleParameter(double val) : type(DOUBLE), double_value(val) {}
    ModuleParameter(bool val) : type(BOOL), bool_value(val) {}
    ModuleParameter(const std::string& val) : type(STRING), string_value(val) {}
    ModuleParameter(const std::vector<double>& val) : type(VECTOR_DOUBLE), vector_value(val) {}
};

/**
 * @brief Parameter map type
 */
using ParameterMap = std::map<std::string, ModuleParameter>;

/**
 * @brief Processing result structure
 */
struct ProcessingResult {
    bool success = true;
    std::string error_message;
    core::DataPointCollection processed_data;
    ParameterMap output_parameters;  // Additional output data
    
    ProcessingResult(bool ok = true) : success(ok) {}
};

/**
 * @brief Base class for all processing modules
 * 
 * Defines the interface that all processing modules must implement.
 * Supports configurable parameters, input/output data handling,
 * and extensible metadata.
 */
class ProcessingModule {
public:
    /**
     * @brief Module information structure
     */
    struct ModuleInfo {
        std::string name;
        std::string version;
        std::string description;
        std::string author;
        std::vector<std::string> input_types;
        std::vector<std::string> output_types;
    };

    /**
     * @brief Virtual destructor
     */
    virtual ~ProcessingModule() = default;

    /**
     * @brief Get module information
     * @return Module information structure
     */
    virtual ModuleInfo get_module_info() const = 0;

    /**
     * @brief Initialize the module with parameters
     * @param parameters Configuration parameters
     * @return true if initialization successful
     */
    virtual bool initialize(const ParameterMap& parameters) = 0;

    /**
     * @brief Process input data
     * @param input Input data points
     * @param channel Channel identifier
     * @param parameters Processing parameters
     * @return Processing result
     */
    virtual ProcessingResult process(
        const core::DataPointCollection& input,
        uint8_t channel,
        const ParameterMap& parameters = ParameterMap{}
    ) = 0;

    /**
     * @brief Reset module state
     */
    virtual void reset() = 0;

    /**
     * @brief Check if module is initialized and ready
     * @return true if ready
     */
    virtual bool is_ready() const = 0;

    /**
     * @brief Get current module parameters
     * @return Current parameter map
     */
    virtual ParameterMap get_parameters() const = 0;

    /**
     * @brief Update module parameters
     * @param parameters New parameters to update
     * @return true if update successful
     */
    virtual bool update_parameters(const ParameterMap& parameters) = 0;

    /**
     * @brief Get module statistics or status
     * @return Status information as parameters
     */
    virtual ParameterMap get_status() const = 0;

    /**
     * @brief Validate input data compatibility
     * @param input Input data to validate
     * @return true if compatible
     */
    virtual bool validate_input(const core::DataPointCollection& input) const;

protected:
    /**
     * @brief Helper function to get parameter value
     * @tparam T Parameter type
     * @param params Parameter map
     * @param key Parameter name
     * @param default_value Default value if not found
     * @return Parameter value
     */
    template<typename T>
    T get_parameter(const ParameterMap& params, const std::string& key, const T& default_value) const;
};

/**
 * @brief Processing pipeline for chaining multiple modules
 * 
 * Allows chaining multiple processing modules in sequence,
 * with configurable data flow and parameter management.
 */
class ProcessingPipeline {
public:
    /**
     * @brief Pipeline configuration
     */
    struct Config {
        bool parallel_processing = false;     ///< Enable parallel processing where possible
        size_t max_queue_size = 1000;        ///< Maximum pipeline queue size
        bool error_propagation = true;       ///< Stop pipeline on module errors
        double timeout_ms = 1000.0;          ///< Processing timeout per stage
    };

    /**
     * @brief Constructor
     * @param config Pipeline configuration
     */
    explicit ProcessingPipeline(const Config& config = Config{});

    /**
     * @brief Destructor
     */
    ~ProcessingPipeline();

    /**
     * @brief Add processing module to pipeline
     * @param module Unique pointer to module
     * @param stage_name Optional stage name for identification
     * @return true if added successfully
     */
    bool add_module(std::unique_ptr<ProcessingModule> module, const std::string& stage_name = "");

    /**
     * @brief Remove module from pipeline
     * @param stage_name Stage name or module name
     * @return true if removed successfully
     */
    bool remove_module(const std::string& stage_name);

    /**
     * @brief Get module by stage name
     * @param stage_name Stage name
     * @return Pointer to module or nullptr if not found
     */
    ProcessingModule* get_module(const std::string& stage_name);

    /**
     * @brief Process data through entire pipeline
     * @param input Input data points
     * @param channel Channel identifier
     * @param global_params Global parameters for all modules
     * @return Final processing result
     */
    ProcessingResult process(
        const core::DataPointCollection& input,
        uint8_t channel,
        const ParameterMap& global_params = ParameterMap{}
    );

    /**
     * @brief Set parameters for specific module
     * @param stage_name Stage name
     * @param parameters Parameters to set
     * @return true if successful
     */
    bool set_module_parameters(const std::string& stage_name, const ParameterMap& parameters);

    /**
     * @brief Get parameters for specific module
     * @param stage_name Stage name
     * @return Current parameters
     */
    ParameterMap get_module_parameters(const std::string& stage_name) const;

    /**
     * @brief Reset all modules in pipeline
     */
    void reset_all();

    /**
     * @brief Check if pipeline is ready (all modules initialized)
     * @return true if ready
     */
    bool is_ready() const;

    /**
     * @brief Get list of stage names
     * @return Vector of stage names in processing order
     */
    std::vector<std::string> get_stage_names() const;

    /**
     * @brief Get pipeline statistics
     */
    struct Statistics {
        uint64_t total_processed = 0;
        uint64_t successful_processed = 0;
        uint64_t failed_processed = 0;
        double average_processing_time_ms = 0.0;
        std::map<std::string, uint64_t> stage_processing_times_us;
    };

    /**
     * @brief Get pipeline statistics
     * @return Current statistics
     */
    Statistics get_statistics() const;

    /**
     * @brief Reset pipeline statistics
     */
    void reset_statistics();

private:
    /**
     * @brief Internal implementation (PIMPL idiom)
     */
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // Prevent copy construction and assignment
    ProcessingPipeline(const ProcessingPipeline&) = delete;
    ProcessingPipeline& operator=(const ProcessingPipeline&) = delete;
};

/**
 * @brief Module factory for creating processing modules
 * 
 * Provides registration and creation of processing modules by name,
 * supporting dynamic module loading and configuration.
 */
class ModuleFactory {
public:
    /**
     * @brief Module creation function type
     */
    using CreateFunction = std::function<std::unique_ptr<ProcessingModule>()>;

    /**
     * @brief Get singleton instance
     * @return Factory instance
     */
    static ModuleFactory& instance();

    /**
     * @brief Register module creation function
     * @param name Module name
     * @param creator Creation function
     * @return true if registered successfully
     */
    bool register_module(const std::string& name, CreateFunction creator);

    /**
     * @brief Create module by name
     * @param name Module name
     * @return Unique pointer to created module or nullptr if not found
     */
    std::unique_ptr<ProcessingModule> create_module(const std::string& name);

    /**
     * @brief Check if module is registered
     * @param name Module name
     * @return true if registered
     */
    bool is_registered(const std::string& name) const;

    /**
     * @brief Get list of registered module names
     * @return Vector of module names
     */
    std::vector<std::string> get_registered_modules() const;

    /**
     * @brief Get module information without creating instance
     * @param name Module name
     * @return Module information or empty info if not found
     */
    ProcessingModule::ModuleInfo get_module_info(const std::string& name) const;

private:
    ModuleFactory() = default;
    ~ModuleFactory() = default;
    
    // Prevent copy construction and assignment
    ModuleFactory(const ModuleFactory&) = delete;
    ModuleFactory& operator=(const ModuleFactory&) = delete;

    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * @brief Macro for easy module registration
 */
#define REGISTER_PROCESSING_MODULE(ClassName, ModuleName) \
    namespace { \
        bool _registered_##ClassName = izana::processing::ModuleFactory::instance().register_module( \
            ModuleName, \
            []() -> std::unique_ptr<izana::processing::ProcessingModule> { \
                return std::make_unique<ClassName>(); \
            } \
        ); \
    }

} // namespace processing
} // namespace izana