# Proyecto Izana - Usage Examples

This document provides practical examples of how to use, extend, and integrate with the Proyecto Izana digital oscilloscope system.

## Quick Start Examples

### 1. Basic Setup and Operation

#### Starting the System

```bash
# Terminal 1: Start C++ backend
cd backend-cpp
./build.sh --clean Release
./build/izana-backend

# Terminal 2: Start React frontend
cd frontend
npm start

# Terminal 3: Run test simulator (optional)
node test-simulator.js
```

#### Basic Usage in Browser

1. Open http://localhost:3000
2. Connect ESP32 via USB
3. Select appropriate COM port in backend logs
4. Adjust time scale and voltage scale as needed
5. Enable trigger if desired
6. Observe real-time waveforms

### 2. ESP32 Signal Generation Example

```cpp
/**
 * ESP32 code for generating test signals
 * Upload this to ESP32 for testing the oscilloscope
 */

#include <driver/i2s.h>
#include <driver/adc.h>
#include <math.h>

#define SAMPLE_RATE 1000000  // 1 MSPS
#define ADC_CHANNEL ADC1_CHANNEL_0  // GPIO36
#define BUFFER_SIZE 1024

// Signal generation parameters
float frequency = 1000.0;  // 1 kHz sine wave
float amplitude = 1.0;     // Amplitude factor
float offset = 1.65;       // DC offset (half of 3.3V)

// I2S configuration for ADC
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
};

void setup() {
    Serial.begin(115200);
    
    // Initialize I2S with ADC
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_adc_mode(ADC_UNIT_1, ADC_CHANNEL);
    adc1_config_width(ADC_WIDTH_BIT_9);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_11);
    
    Serial.println("ESP32 Oscilloscope Signal Source Ready");
    Serial.println("Commands:");
    Serial.println("  cmd:freq:<hz>    - Set frequency");
    Serial.println("  cmd:amp:<0-1>    - Set amplitude");
    Serial.println("  cmd:offset:<v>   - Set DC offset");
}

void loop() {
    // Handle serial commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        handleCommand(command);
    }
    
    // Generate and read test signal
    generateTestSignal();
    
    // Read ADC values
    uint16_t adcBuffer[BUFFER_SIZE];
    size_t bytesRead;
    
    i2s_read(I2S_NUM_0, adcBuffer, sizeof(adcBuffer), &bytesRead, portMAX_DELAY);
    
    // Send data to backend
    for (int i = 0; i < bytesRead / sizeof(uint16_t); i++) {
        uint16_t adcValue = adcBuffer[i] & 0x1FF; // 9-bit ADC
        Serial.print("signal:");
        Serial.println(adcValue);
        
        // Small delay to maintain sample rate
        delayMicroseconds(1);
    }
}

void generateTestSignal() {
    static unsigned long lastUpdate = 0;
    static float phase = 0;
    
    if (millis() - lastUpdate > 1) { // Update every 1ms
        // Generate sine wave on DAC (if available) or PWM
        float sineValue = sin(2 * PI * frequency * phase / SAMPLE_RATE);
        float signalValue = offset + amplitude * sineValue;
        
        // Output to DAC or PWM pin connected to ADC input
        // This creates a loopback for testing
        int dacValue = (int)(signalValue * 255 / 3.3);
        dacWrite(25, dacValue); // ESP32 DAC channel 1
        
        phase += 1.0;
        if (phase >= SAMPLE_RATE / frequency) {
            phase = 0;
        }
        
        lastUpdate = millis();
    }
}

void handleCommand(String cmd) {
    if (cmd.startsWith("cmd:freq:")) {
        frequency = cmd.substring(9).toFloat();
        Serial.println("Frequency set to: " + String(frequency) + " Hz");
    } else if (cmd.startsWith("cmd:amp:")) {
        amplitude = cmd.substring(8).toFloat();
        Serial.println("Amplitude set to: " + String(amplitude));
    } else if (cmd.startsWith("cmd:offset:")) {
        offset = cmd.substring(11).toFloat();
        Serial.println("Offset set to: " + String(offset) + " V");
    }
}
```

### 3. Frontend Integration Example

```javascript
/**
 * Example of integrating Proyecto Izana oscilloscope into another React app
 */

import React, { useState, useEffect, useRef } from 'react';
import io from 'socket.io-client';
import Oscilloscope from './components/Oscilloscope';
import ParameterPanel from './components/ParameterPanel';

const OscilloscopeApp = () => {
  const [connected, setConnected] = useState(false);
  const [systemStatus, setSystemStatus] = useState({});
  const [currentParams, setCurrentParams] = useState({});
  const socketRef = useRef();

  useEffect(() => {
    // Connect to backend WebSocket
    socketRef.current = io('ws://localhost:8080', {
      transports: ['websocket']
    });

    socketRef.current.on('connect', () => {
      console.log('Connected to Izana backend');
      setConnected(true);
    });

    socketRef.current.on('disconnect', () => {
      console.log('Disconnected from Izana backend');
      setConnected(false);
    });

    socketRef.current.on('status_update', (status) => {
      setSystemStatus(status.status);
    });

    return () => {
      if (socketRef.current) {
        socketRef.current.disconnect();
      }
    };
  }, []);

  const handleParameterUpdate = (paramUpdate) => {
    if (socketRef.current && connected) {
      const message = {
        type: 'parameter_update',
        timestamp: Date.now(),
        ...paramUpdate
      };
      
      socketRef.current.emit('parameter_update', message);
      setCurrentParams(prev => ({ ...prev, ...paramUpdate.parameters }));
    }
  };

  const handleCommand = (command, params = {}) => {
    if (socketRef.current && connected) {
      const message = {
        type: 'command',
        timestamp: Date.now(),
        command: command,
        parameters: params
      };
      
      socketRef.current.emit('command', message);
    }
  };

  return (
    <div className="oscilloscope-app">
      <div className="main-content">
        <Oscilloscope 
          socket={socketRef.current}
          connected={connected}
          systemStatus={systemStatus}
        />
      </div>
      <div className="control-panel">
        <ParameterPanel
          connected={connected}
          onParameterUpdate={handleParameterUpdate}
          onCommand={handleCommand}
          currentParams={currentParams}
          availableModules={systemStatus.processing_modules || []}
        />
      </div>
    </div>
  );
};

export default OscilloscopeApp;
```

### 4. Custom Processing Module Example

```cpp
/**
 * Example of creating a custom processing module for the C++ backend
 */

#include "processing/ProcessingModule.hpp"
#include <numeric>
#include <algorithm>
#include <cmath>

class MovingAverageModule : public ProcessingModule {
private:
    int window_size_;
    bool initialized_;
    std::vector<double> history_buffer_;

public:
    MovingAverageModule() : window_size_(10), initialized_(false) {}

    ModuleInfo get_module_info() const override {
        return {
            .name = "moving_average",
            .version = "1.0.0",
            .description = "Moving average filter for signal smoothing",
            .author = "Proyecto Izana Team",
            .input_types = {"time_domain"},
            .output_types = {"time_domain"}
        };
    }

    bool initialize(const ParameterMap& parameters) override {
        try {
            window_size_ = get_parameter(parameters, "window_size", 10);
            
            if (window_size_ <= 0 || window_size_ > 1000) {
                return false;
            }
            
            history_buffer_.clear();
            history_buffer_.reserve(window_size_);
            initialized_ = true;
            
            return true;
        } catch (const std::exception& e) {
            initialized_ = false;
            return false;
        }
    }

    ProcessingResult process(
        const core::DataPointCollection& input,
        uint8_t channel,
        const ParameterMap& parameters) override {
        
        ProcessingResult result;
        
        if (!initialized_) {
            result.success = false;
            result.error_message = "Module not initialized";
            return result;
        }

        result.processed_data.reserve(input.size());

        for (const auto& point : input) {
            // Add new value to history
            history_buffer_.push_back(point.y);
            
            // Maintain window size
            if (history_buffer_.size() > static_cast<size_t>(window_size_)) {
                history_buffer_.erase(history_buffer_.begin());
            }
            
            // Calculate moving average
            double average = std::accumulate(history_buffer_.begin(), 
                                           history_buffer_.end(), 0.0) / history_buffer_.size();
            
            // Create output point
            core::DataPoint output_point;
            output_point.x = point.x;
            output_point.y = average;
            output_point.timestamp_us = point.timestamp_us;
            output_point.channel = channel;
            
            result.processed_data.push_back(output_point);
        }

        // Add output parameters
        result.output_parameters["samples_processed"] = ModuleParameter(static_cast<int>(input.size()));
        result.output_parameters["window_size_used"] = ModuleParameter(static_cast<int>(history_buffer_.size()));

        result.success = true;
        return result;
    }

    void reset() override {
        history_buffer_.clear();
    }

    bool is_ready() const override {
        return initialized_;
    }

    ParameterMap get_parameters() const override {
        ParameterMap params;
        params["window_size"] = ModuleParameter(window_size_);
        params["initialized"] = ModuleParameter(initialized_);
        return params;
    }

    bool update_parameters(const ParameterMap& parameters) override {
        try {
            int new_window_size = get_parameter(parameters, "window_size", window_size_);
            
            if (new_window_size != window_size_ && new_window_size > 0 && new_window_size <= 1000) {
                window_size_ = new_window_size;
                
                // Adjust history buffer size
                if (history_buffer_.size() > static_cast<size_t>(window_size_)) {
                    history_buffer_.erase(history_buffer_.begin(), 
                                         history_buffer_.begin() + (history_buffer_.size() - window_size_));
                }
                
                history_buffer_.reserve(window_size_);
            }
            
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }

    ParameterMap get_status() const override {
        ParameterMap status;
        status["initialized"] = ModuleParameter(initialized_);
        status["current_window_size"] = ModuleParameter(window_size_);
        status["buffer_size"] = ModuleParameter(static_cast<int>(history_buffer_.size()));
        return status;
    }
};

// Register the module
REGISTER_PROCESSING_MODULE(MovingAverageModule, "moving_average");
```

### 5. Configuration File Example

```json
{
  "uart": {
    "baud_rate": 115200,
    "auto_reconnect": true,
    "reconnect_delay_ms": 3000,
    "ports": [
      "/dev/ttyUSB0",
      "/dev/ttyUSB1", 
      "/dev/ttyACM0",
      "/dev/ttyACM1",
      "COM3",
      "COM4",
      "COM5",
      "COM6"
    ]
  },
  "websocket": {
    "port": 8080,
    "max_clients": 10,
    "cors_origin": "*",
    "enable_compression": true
  },
  "processing": {
    "num_channels": 2,
    "synchronized_triggers": false,
    "parallel": false,
    "modules": ["fft", "filter", "moving_average"],
    "fft": {
      "default_window_size": 1024,
      "default_window_type": "hanning"
    },
    "filter": {
      "default_type": "lowpass",
      "default_cutoff": 10000,
      "default_order": 4
    },
    "moving_average": {
      "default_window_size": 10
    }
  },
  "logging": {
    "level": "info",
    "file": "izana.log",
    "console": true
  }
}
```

### 6. Python Integration Example

```python
"""
Python client example for Proyecto Izana backend
Connects via WebSocket to receive oscilloscope data
"""

import asyncio
import websockets
import json
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
from collections import deque

class IzanaClient:
    def __init__(self, uri="ws://localhost:8080"):
        self.uri = uri
        self.websocket = None
        self.data_buffer = deque(maxlen=10000)
        self.connected = False
        
    async def connect(self):
        try:
            self.websocket = await websockets.connect(self.uri)
            self.connected = True
            print(f"Connected to Izana backend at {self.uri}")
            
            # Start listening for messages
            await self.listen_for_messages()
            
        except Exception as e:
            print(f"Connection failed: {e}")
            self.connected = False
    
    async def listen_for_messages(self):
        try:
            async for message in self.websocket:
                data = json.loads(message)
                await self.handle_message(data)
                
        except websockets.exceptions.ConnectionClosed:
            print("Connection to backend closed")
            self.connected = False
        except Exception as e:
            print(f"Error receiving message: {e}")
    
    async def handle_message(self, message):
        msg_type = message.get('type')
        
        if msg_type == 'signal_data':
            # Extract data points
            data_points = message.get('data', [])
            channel = message.get('channel', 0)
            
            for point in data_points:
                self.data_buffer.append({
                    'x': point['x'],
                    'y': point['y'],
                    'channel': channel,
                    'timestamp': point['timestamp_us']
                })
            
            print(f"Received {len(data_points)} points for channel {channel}")
            
        elif msg_type == 'status_update':
            status = message.get('status', {})
            print(f"System status: UART={status.get('uart_connected', False)}")
            
        elif msg_type == 'error':
            print(f"Backend error: {message.get('error_message', 'Unknown error')}")
    
    async def send_parameter_update(self, params):
        if self.connected and self.websocket:
            message = {
                'type': 'parameter_update',
                'timestamp': int(asyncio.get_event_loop().time() * 1000),
                'channel': 0,
                'parameters': params
            }
            
            await self.websocket.send(json.dumps(message))
            print(f"Sent parameter update: {params}")
    
    async def send_command(self, command, params=None):
        if self.connected and self.websocket:
            message = {
                'type': 'command',
                'timestamp': int(asyncio.get_event_loop().time() * 1000),
                'command': command,
                'parameters': params or {}
            }
            
            await self.websocket.send(json.dumps(message))
            print(f"Sent command: {command}")
    
    def get_recent_data(self, num_points=1000):
        """Get recent data points for plotting"""
        if len(self.data_buffer) == 0:
            return [], []
        
        recent_data = list(self.data_buffer)[-num_points:]
        x_data = [point['x'] for point in recent_data]
        y_data = [point['y'] for point in recent_data]
        
        return x_data, y_data

# Example usage with matplotlib for real-time plotting
class IzanaPlotter:
    def __init__(self, client):
        self.client = client
        self.fig, self.ax = plt.subplots(figsize=(12, 6))
        self.line, = self.ax.plot([], [], 'g-', linewidth=1)
        
        self.ax.set_xlim(0, 0.001)  # 1ms window
        self.ax.set_ylim(0, 3.3)    # 3.3V scale
        self.ax.set_xlabel('Time (s)')
        self.ax.set_ylabel('Voltage (V)')
        self.ax.set_title('Proyecto Izana - Real-time Oscilloscope')
        self.ax.grid(True, alpha=0.3)
        self.ax.set_facecolor('black')
        
        # Set green on black theme
        self.ax.spines['bottom'].set_color('green')
        self.ax.spines['top'].set_color('green')
        self.ax.spines['left'].set_color('green')
        self.ax.spines['right'].set_color('green')
        self.ax.xaxis.label.set_color('green')
        self.ax.yaxis.label.set_color('green')
        self.ax.title.set_color('green')
        self.ax.tick_params(colors='green')
        
        self.fig.patch.set_facecolor('black')
        
    def update_plot(self, frame):
        """Animation update function"""
        if not self.client.connected:
            return self.line,
        
        x_data, y_data = self.client.get_recent_data()
        
        if len(x_data) > 0:
            self.line.set_data(x_data, y_data)
            
            # Auto-scale time axis
            if len(x_data) > 1:
                x_range = max(x_data) - min(x_data)
                if x_range > 0:
                    self.ax.set_xlim(min(x_data), max(x_data))
        
        return self.line,
    
    def start_animation(self):
        """Start real-time plotting"""
        ani = animation.FuncAnimation(
            self.fig, self.update_plot, interval=50, blit=True
        )
        plt.show()
        return ani

# Main execution example
async def main():
    # Create client and connect
    client = IzanaClient()
    
    # Start plotter in separate thread
    plotter = IzanaPlotter(client)
    
    # Connect to backend
    await client.connect()

if __name__ == "__main__":
    # Run the asyncio event loop
    asyncio.run(main())
```

### 7. Advanced Multi-Channel Configuration

```javascript
// Advanced frontend configuration for multi-channel operation
const advancedConfig = {
  channels: [
    {
      id: 0,
      name: "Input Signal", 
      color: "#00ff00",
      enabled: true,
      transformParams: {
        timeScale: 1000,
        amplitudeScale: 3.3,
        verticalOffset: 0.0,
        triggerLevel: 1.65,
        triggerEnabled: true,
        triggerEdge: "rising"
      }
    },
    {
      id: 1,
      name: "Reference",
      color: "#ff8800", 
      enabled: true,
      transformParams: {
        timeScale: 1000,
        amplitudeScale: 5.0,
        verticalOffset: 0.0,
        triggerLevel: 2.5,
        triggerEnabled: false
      }
    }
  ],
  processing: {
    fft: {
      enabled: true,
      windowSize: 2048,
      windowType: "blackman",
      channels: [0] // Only apply FFT to channel 0
    },
    filter: {
      enabled: true,
      type: "bandpass",
      lowCutoff: 100,
      highCutoff: 50000,
      order: 6,
      channels: [0, 1] // Apply to both channels
    },
    crossCorrelation: {
      enabled: true,
      referenceChannel: 1,
      targetChannels: [0],
      maxDelay: 0.001 // 1ms max delay
    }
  },
  display: {
    mode: "overlay", // "overlay", "split", "xy"
    persistence: false,
    intensityGrading: true,
    gridType: "standard" // "standard", "polar", "log"
  }
};

// Apply advanced configuration
function applyAdvancedConfig(socket, config) {
  config.channels.forEach(channel => {
    socket.emit('parameter_update', {
      type: 'channel_config',
      channel: channel.id,
      parameters: {
        enabled: channel.enabled,
        name: channel.name,
        color: channel.color,
        ...channel.transformParams
      }
    });
  });

  Object.keys(config.processing).forEach(moduleName => {
    const moduleConfig = config.processing[moduleName];
    if (moduleConfig.enabled) {
      socket.emit('parameter_update', {
        type: 'module',
        module: moduleName,
        parameters: moduleConfig
      });
    }
  });
}
```

These examples demonstrate the flexibility and extensibility of the Proyecto Izana system, showing how users can:

1. **Set up and operate** the basic oscilloscope functionality
2. **Generate test signals** using ESP32 
3. **Integrate the frontend** into other applications
4. **Create custom processing modules** for specialized analysis
5. **Configure the system** for different use cases
6. **Interface with Python** for scientific computing
7. **Configure multi-channel** advanced setups

The modular architecture allows easy extension and customization for specific application needs while maintaining compatibility with the core system.