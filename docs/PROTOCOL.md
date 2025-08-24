# Proyecto Izana - Protocol Documentation

## Communication Protocols

This document describes the communication protocols used between the different components of the Proyecto Izana digital oscilloscope system.

## 1. ESP32 ↔ Backend (UART Protocol)

### Protocol Specification

The ESP32 communicates with the backend over UART using a simple text-based protocol.

**Connection Parameters:**
- Baud Rate: 115200
- Data Bits: 8
- Parity: None
- Stop Bits: 1
- Flow Control: None

### Data Format

**Signal Data (ESP32 → Backend):**
```
signal:<value>\n
```

Where:
- `<value>` is a 9-bit ADC reading (0-511)
- Each message is terminated with a newline character (`\n`)

**Examples:**
```
signal:123
signal:456
signal:078
signal:511
signal:000
```

**Multi-Channel Data (Future Extension):**
```
signal:<channel>:<value>\n
```

Where:
- `<channel>` is the channel identifier (0-7)
- `<value>` is the ADC reading

**Command Format (Backend → ESP32):**
```
cmd:<command>:<parameters>\n
```

**Available Commands:**
- `cmd:rate:1000000` - Set sampling rate to 1 MSPS
- `cmd:channel:0` - Select active channel
- `cmd:trigger:enable` - Enable hardware trigger
- `cmd:trigger:disable` - Disable hardware trigger
- `cmd:reset` - Reset ESP32 sampling

## 2. Backend ↔ Frontend (WebSocket Protocol)

### Protocol Specification

The backend and frontend communicate over WebSocket using JSON-formatted messages.

**Connection:**
- Default Port: 8080
- Protocol: WebSocket (ws://)
- Message Format: JSON

### Message Types

#### 2.1 Signal Data (Backend → Frontend)

**Message Structure:**
```json
{
  "type": "signal_data",
  "timestamp": 1634567890123,
  "channel": 0,
  "data": [
    {
      "x": 0.000000,
      "y": 1.65432,
      "timestamp_us": 1634567890123456
    },
    {
      "x": 0.000001,
      "y": 1.67890,
      "timestamp_us": 1634567890123457
    }
  ]
}
```

**Fields:**
- `type`: Message type identifier
- `timestamp`: Message timestamp (milliseconds since epoch)
- `channel`: Channel identifier (0-7)
- `data`: Array of data points
  - `x`: Time coordinate in seconds
  - `y`: Voltage coordinate in volts
  - `timestamp_us`: Original sample timestamp in microseconds

#### 2.2 Parameter Update (Frontend → Backend)

**Message Structure:**
```json
{
  "type": "parameter_update",
  "timestamp": 1634567890123,
  "channel": 0,
  "parameters": {
    "timeScale": 1000,
    "amplitudeScale": 3.3,
    "verticalOffset": 0.0,
    "triggerLevel": 1.65,
    "triggerEnabled": true,
    "triggerEdge": "rising"
  }
}
```

**Core Parameters:**
- `timeScale`: Number of samples per collection (100-10000)
- `amplitudeScale`: Maximum voltage scale in volts (1.0, 3.3, 5.0)
- `verticalOffset`: Vertical offset in volts (-5.0 to +5.0)
- `triggerLevel`: Trigger level in volts
- `triggerEnabled`: Enable/disable triggering
- `triggerEdge`: Trigger edge ("rising", "falling", "both")

#### 2.3 System Status (Backend → Frontend)

**Message Structure:**
```json
{
  "type": "status_update",
  "timestamp": 1634567890123,
  "status": {
    "uart_connected": true,
    "uart_port": "/dev/ttyUSB0",
    "sampling_rate": 1000000,
    "active_channels": [0, 1],
    "processing_modules": ["fft", "filter"],
    "buffer_fill": {
      "0": 0.75,
      "1": 0.82
    },
    "statistics": {
      "samples_per_second": 999847,
      "packets_per_second": 999,
      "dropped_packets": 12,
      "uptime_seconds": 3600
    }
  }
}
```

#### 2.4 Processing Module Parameters (Extensible)

**FFT Module Parameters:**
```json
{
  "type": "parameter_update",
  "timestamp": 1634567890123,
  "module": "fft",
  "parameters": {
    "windowSize": 1024,
    "windowType": "hanning",
    "overlapRatio": 0.5,
    "frequencyRange": [0, 500000],
    "enabled": true
  }
}
```

**Filter Module Parameters:**
```json
{
  "type": "parameter_update",
  "timestamp": 1634567890123,
  "module": "filter",
  "parameters": {
    "type": "lowpass",
    "cutoffFreq": 10000,
    "order": 4,
    "enabled": true
  }
}
```

#### 2.5 Command Messages (Frontend → Backend)

**Message Structure:**
```json
{
  "type": "command",
  "timestamp": 1634567890123,
  "command": "force_trigger",
  "parameters": {
    "channel": 0
  }
}
```

**Available Commands:**
- `force_trigger`: Manually trigger data collection
- `clear_buffer`: Clear data buffer for specified channel
- `reset_statistics`: Reset system statistics
- `save_data`: Save current data to file
- `load_config`: Load configuration from file

#### 2.6 Error Messages (Backend → Frontend)

**Message Structure:**
```json
{
  "type": "error",
  "timestamp": 1634567890123,
  "error_code": 1001,
  "error_message": "UART connection lost",
  "details": {
    "port": "/dev/ttyUSB0",
    "last_data_timestamp": 1634567889000
  }
}
```

## 3. Parameter System Extensions

### 3.1 Versioned Parameters

The parameter system supports versioning for backward compatibility:

```json
{
  "version": "1.1",
  "parameters": {
    "core": {
      "timeScale": 1000,
      "amplitudeScale": 3.3
    },
    "modules": {
      "fft": {
        "windowSize": 1024
      }
    }
  }
}
```

### 3.2 Module Registration

New modules can register their parameter schemas:

```json
{
  "type": "module_registration",
  "module_name": "custom_filter",
  "version": "1.0",
  "parameter_schema": {
    "enabled": {
      "type": "boolean",
      "default": false,
      "description": "Enable custom filter"
    },
    "coefficient": {
      "type": "array",
      "items": "number",
      "default": [1.0, 0.5, 0.25],
      "description": "Filter coefficients"
    }
  }
}
```

## 4. Usage Examples

### 4.1 JavaScript Frontend Example

```javascript
// Connect to WebSocket server
const ws = new WebSocket('ws://localhost:8080');

// Handle incoming messages
ws.onmessage = function(event) {
  const message = JSON.parse(event.data);
  
  switch(message.type) {
    case 'signal_data':
      updateOscilloscope(message.data, message.channel);
      break;
      
    case 'status_update':
      updateStatusDisplay(message.status);
      break;
      
    case 'error':
      showError(message.error_message);
      break;
  }
};

// Send parameter update
function updateTimeScale(scale) {
  const message = {
    type: 'parameter_update',
    timestamp: Date.now(),
    channel: 0,
    parameters: {
      timeScale: scale
    }
  };
  ws.send(JSON.stringify(message));
}

// Send command
function forceTrigger() {
  const message = {
    type: 'command',
    timestamp: Date.now(),
    command: 'force_trigger',
    parameters: {
      channel: 0
    }
  };
  ws.send(JSON.stringify(message));
}
```

### 4.2 ESP32 Arduino Example

```cpp
void setup() {
  Serial.begin(115200);
  // Initialize ADC and I2S
  initializeADC();
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    handleCommand(command);
  }
  
  uint16_t adcValue = readADC();
  Serial.print("signal:");
  Serial.println(adcValue);
  
  delayMicroseconds(1); // 1 MSPS
}

void handleCommand(String cmd) {
  if (cmd.startsWith("cmd:rate:")) {
    int rate = cmd.substring(9).toInt();
    setSamplingRate(rate);
  }
}
```

### 4.3 C++ Backend Module Example

```cpp
class CustomProcessingModule : public ProcessingModule {
public:
  ModuleInfo get_module_info() const override {
    return {
      .name = "custom_processor",
      .version = "1.0",
      .description = "Custom signal processing module",
      .author = "Proyecto Izana",
      .input_types = {"time_domain"},
      .output_types = {"time_domain", "frequency_domain"}
    };
  }
  
  ProcessingResult process(
    const DataPointCollection& input,
    uint8_t channel,
    const ParameterMap& parameters) override {
    
    ProcessingResult result;
    
    // Custom processing logic here
    for (const auto& point : input) {
      DataPoint processed_point;
      processed_point.x = point.x;
      processed_point.y = point.y * 2.0; // Example: double amplitude
      processed_point.timestamp_us = point.timestamp_us;
      processed_point.channel = channel;
      
      result.processed_data.push_back(processed_point);
    }
    
    return result;
  }
};

// Register the module
REGISTER_PROCESSING_MODULE(CustomProcessingModule, "custom_processor");
```

## 5. Error Handling

### 5.1 UART Error Codes

- `1001`: UART connection lost
- `1002`: Invalid data format received
- `1003`: Buffer overflow
- `1004`: Timeout waiting for data

### 5.2 WebSocket Error Codes

- `2001`: Invalid message format
- `2002`: Unknown message type
- `2003`: Parameter validation failed
- `2004`: Command execution failed

### 5.3 Processing Error Codes

- `3001`: Module initialization failed
- `3002`: Processing pipeline error
- `3003`: Invalid input data
- `3004`: Module parameter error

## 6. Performance Considerations

### 6.1 Data Rate Management

**Maximum Data Rates:**
- UART: ~11.5 KB/s at 115200 baud
- WebSocket: Limited by network and client processing capacity
- Processing: Target <100ms latency end-to-end

**Rate Limiting:**
- Frontend can request specific update rates
- Backend automatically adapts to client capabilities
- Configurable buffer sizes prevent memory issues

### 6.2 Protocol Efficiency

**Optimizations:**
- Binary encoding option for high-data-rate scenarios
- Data compression for WebSocket messages
- Batch data transmission to reduce overhead
- Client-side buffering and interpolation

This protocol documentation provides a complete specification for all communication interfaces in the Proyecto Izana system, enabling easy extension and integration of new components.