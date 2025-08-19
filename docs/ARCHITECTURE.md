# Proyecto Izana - System Architecture

## Overview

Proyecto Izana is a modular digital oscilloscope system designed for real-time signal acquisition, processing, and visualization. The architecture follows a clean separation of concerns with a modular backend that can be extended with new processing capabilities.

## System Flow Diagram

```
┌─────────────────┐    ┌─────────────────────┐    ┌─────────────────────┐
│    ESP32 ADC    │    │   C++ Backend       │    │  React Frontend     │
│                 │    │                     │    │                     │
│ ┌─────────────┐ │    │ ┌─────────────────┐ │    │ ┌─────────────────┐ │
│ │ I2S + ADC   │ │───▶│ │  UART Module    │ │    │ │ Oscilloscope UI │ │
│ │ 1 MSPS      │ │    │ └─────────────────┘ │    │ └─────────────────┘ │
│ └─────────────┘ │    │          │          │    │          ▲          │
│                 │    │          ▼          │    │          │          │
│ ┌─────────────┐ │    │ ┌─────────────────┐ │◀──▶│ ┌─────────────────┐ │
│ │GPIO36 Input │ │    │ │ DataProcessor   │ │    │ │ Parameter Panel │ │
│ └─────────────┘ │    │ │ - Transformations│ │    │ └─────────────────┘ │
└─────────────────┘    │ │ - Trigger Align │ │    │          ▲          │
                       │ │ - Multi-channel │ │    │          │          │
┌─────────────────┐    │ └─────────────────┘ │    │ ┌─────────────────┐ │
│ Test Simulator  │───▶│          │          │    │ │ Measurements    │ │
└─────────────────┘    │          ▼          │    │ │ - Frequency     │ │
                       │ ┌─────────────────┐ │    │ │ - Vpp, Vmax     │ │
                       │ │Processing Modules│ │    │ │ - Amplitude     │ │
                       │ │ - FFT Analysis  │ │    │ └─────────────────┘ │
                       │ │ - Digital Filters│ │    └─────────────────────┘
                       │ │ - Data Storage  │ │              ▲
                       │ └─────────────────┘ │              │
                       │          │          │              │
                       │          ▼          │    ┌─────────────────────┐
                       │ ┌─────────────────┐ │    │    WebSocket        │
                       │ │ WebSocket Server│ │◀───┤   Communication     │
                       │ └─────────────────┘ │    └─────────────────────┘
                       └─────────────────────┘
```

## Core Components

### 1. Hardware Layer (ESP32)

**Responsibilities:**
- High-speed ADC sampling using I2S + ADC peripheral
- Real-time data acquisition at up to 1 MSPS
- UART transmission of sampled data

**Implementation:**
- ESP32 with Arduino framework
- 9-bit resolution ADC (0-511 range)
- GPIO36 as analog input
- 115200 baud UART communication

### 2. C++ Backend

The backend is designed with a modular architecture that allows easy extension and scalability.

#### 2.1 Core Modules

##### UARTModule
```cpp
class UARTModule {
    // Manages serial communication with ESP32
    // Auto-detects available ports
    // Handles reconnection and error recovery
    // Provides data stream interface
};
```

##### DataProcessor
```cpp
class DataProcessor {
    // Converts raw ADC values to (x,y) points
    // Applies transformations (scale, amplitude, offset)
    // Performs trigger-level alignment
    // Manages multi-channel data streams
};
```

##### ParameterManager
```cpp
class ParameterManager {
    // Manages system parameters
    // Provides extensible parameter interface
    // Handles parameter validation and updates
    // Maintains backward compatibility
};
```

##### WebSocketServer
```cpp
class WebSocketServer {
    // Manages WebSocket connections
    // Streams processed data to frontend
    // Handles parameter updates from UI
    // Provides real-time status updates
};
```

#### 2.2 Processing Modules

The system supports pluggable processing modules for advanced signal analysis:

##### FFTModule
```cpp
class FFTModule : public ProcessingModule {
    // Fast Fourier Transform analysis
    // Frequency domain representation
    // Spectral analysis capabilities
};
```

##### FilterModule
```cpp
class FilterModule : public ProcessingModule {
    // Digital signal filters
    // Low-pass, high-pass, band-pass filters
    // Configurable filter parameters
};
```

##### StorageModule
```cpp
class StorageModule : public ProcessingModule {
    // Data logging and storage
    // Multiple file formats support
    // Streaming and batch storage modes
};
```

### 3. Frontend (React)

**Responsibilities:**
- Real-time waveform visualization
- Parameter control interface
- Multi-signal display support
- User interaction and control

**Key Components:**
- Oscilloscope visualization using Chart.js
- Parameter control panels
- Real-time measurements display
- Multi-channel/multi-signal support

## Communication Protocols

### 3.1 ESP32 ↔ Backend (UART)

**Data Format:**
```
signal:<value>\n
```

Where `<value>` is a 9-bit ADC reading (0-511).

**Example:**
```
signal:123
signal:456
signal:078
```

### 3.2 Backend ↔ Frontend (WebSocket)

#### Data Events

**Signal Data:**
```json
{
  "type": "signal_data",
  "channel": 0,
  "data": [
    {
      "x": 0,
      "y": 1.65,
      "timestamp": 1634567890123
    }
  ]
}
```

**Parameter Update:**
```json
{
  "type": "parameter_update",
  "parameters": {
    "timeScale": 1000,
    "amplitudeScale": 3.3,
    "verticalOffset": 0.0,
    "triggerLevel": 1.65,
    "triggerEnabled": true
  }
}
```

**System Status:**
```json
{
  "type": "status_update",
  "status": {
    "uart_connected": true,
    "sampling_rate": 1000000,
    "active_channels": [0],
    "processing_modules": ["fft", "filter"]
  }
}
```

### 3.3 Extensible Parameter System

The parameter system is designed to be extensible without breaking compatibility:

```json
{
  "version": "1.0",
  "parameters": {
    "core": {
      "timeScale": 1000,
      "amplitudeScale": 3.3,
      "verticalOffset": 0.0,
      "triggerLevel": 1.65
    },
    "modules": {
      "fft": {
        "windowSize": 1024,
        "windowType": "hanning"
      },
      "filter": {
        "type": "lowpass",
        "cutoffFreq": 10000
      }
    }
  }
}
```

## Scalability Features

### 1. Multi-Channel Support

The architecture supports multiple input channels:

```cpp
class MultiChannelProcessor {
    std::vector<DataProcessor> channels;
    // Synchronous processing across channels
    // Channel-specific parameter sets
    // Coordinated trigger alignment
};
```

### 2. Processing Pipeline

Extensible processing pipeline allows adding new analysis modules:

```cpp
class ProcessingPipeline {
    std::vector<std::unique_ptr<ProcessingModule>> modules;
    // Chainable processing modules
    // Configurable pipeline order
    // Module-specific parameters
};
```

### 3. Modular Frontend

Frontend is prepared for multiple visualization modes:

- Traditional oscilloscope view
- Frequency domain (FFT) display
- Multi-channel synchronized view
- XY mode for phase analysis
- Persistence mode for signal envelope analysis

## Performance Considerations

### Real-Time Processing
- Lock-free data structures for high-throughput
- Circular buffers for continuous data streaming
- Separate threads for I/O, processing, and communication

### Memory Management
- Fixed-size buffers to avoid allocation overhead
- Memory pools for efficient object management
- Configurable buffer sizes for different use cases

### Scalability Metrics
- Target: 1 MSPS per channel
- Latency: < 100ms end-to-end
- Memory usage: < 100MB for single channel
- CPU usage: < 50% on modern hardware

## Development Guidelines

### Adding New Processing Modules

1. Inherit from `ProcessingModule` base class
2. Implement required virtual methods
3. Register module in `ProcessingPipeline`
4. Add module-specific parameters to `ParameterManager`
5. Update frontend UI for new module controls

### Extending Parameter System

1. Add new parameter group to parameter schema
2. Implement parameter validation
3. Update WebSocket protocol handlers
4. Add frontend UI controls
5. Maintain backward compatibility

### Multi-Channel Extension

1. Extend `DataProcessor` for channel management
2. Update UART protocol for channel identification
3. Modify frontend for multi-channel display
4. Implement channel synchronization logic

This architecture provides a solid foundation for scaling the system to new functionalities while maintaining clean separation of concerns and extensibility.