# Proyecto Izana - Modular Digital Oscilloscope

**A high-performance, modular digital oscilloscope system with real-time signal acquisition, processing, and visualization capabilities.**

---

## 🌟 Overview

Proyecto Izana is a complete digital oscilloscope solution featuring:

- **High-speed ESP32 ADC sampling** (up to 1 MSPS via I2S)
- **Modular C++ backend** with extensible processing pipeline
- **Real-time React frontend** with professional oscilloscope interface
- **WebSocket communication** for live data streaming
- **Multi-channel support** with synchronized triggers
- **Advanced signal processing** (FFT, filters, custom modules)

## 🚀 Key Features

### Hardware Layer (ESP32)
- ⚡ **1 MSPS sampling rate** using I2S + ADC
- 📊 **9-bit resolution** (0-511 ADC values)
- 🔌 **UART communication** at 115200 baud
- 📡 **GPIO36 analog input** (ADC1_CHANNEL_0)

### C++ Backend
- 🏗️ **Modular architecture** with extensible processing modules
- 🔄 **Multi-channel processing** with synchronized triggers
- 📈 **Real-time transformations** (scaling, offset, trigger alignment)
- 🌐 **WebSocket server** for frontend communication
- 🛠️ **Plugin system** for FFT, filters, and custom analysis

### React Frontend
- 📊 **Professional oscilloscope interface** with Chart.js visualization
- ⚙️ **Advanced parameter controls** (timebase, voltage, trigger)
- 🎛️ **Multi-channel display** with independent settings
- 🔧 **Processing module configuration** (FFT, filters, storage)
- 📱 **Responsive design** for different screen sizes

---

## 📁 Project Structure

```
Proyecto_Izana/
├── 🔧 backend-cpp/          # Modular C++ backend
│   ├── include/             # Header files
│   │   ├── core/           # Core modules (UART, DataProcessor, WebSocket)
│   │   ├── processing/     # Processing modules (FFT, filters)
│   │   └── utils/          # Utilities (Logger, Config)
│   ├── src/                # Implementation files
│   ├── CMakeLists.txt      # Build configuration
│   └── build.sh           # Build script
├── 💻 backend/              # Node.js backend (original/legacy)
├── ⚛️ frontend/             # React frontend
│   ├── src/components/     # UI components
│   │   ├── Oscilloscope.js # Main oscilloscope display
│   │   └── ParameterPanel.js # Advanced controls
├── 📟 codigos del arduino/  # ESP32 firmware
├── 📚 docs/                # Documentation
│   ├── ARCHITECTURE.md     # System architecture
│   ├── PROTOCOL.md         # Communication protocols
│   └── EXAMPLES.md         # Usage examples
└── 🧪 test-simulator.js    # Development test simulator
```

---

## 🚀 Quick Start

### 1. Hardware Setup
```bash
# Connect ESP32 to computer via USB
# Upload firmware from 'codigos del arduino/adc/adc.ino'
# Connect analog signal to GPIO36
```

### 2. C++ Backend (Recommended)
```bash
cd backend-cpp

# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential cmake libserial-dev nlohmann-json3-dev

# Build (includes dependency installation)
./build.sh --install-deps --clean Release

# Run
./build/izana-backend
```

### 3. Frontend
```bash
cd frontend
npm install
npm start
# Open http://localhost:3000
```

### 4. Alternative: Node.js Backend (Legacy)
```bash
cd backend
npm install
node server.js
# Server runs on port 3001
```

---

## 📖 Documentation

| Document | Description |
|----------|-------------|
| [**ARCHITECTURE.md**](docs/ARCHITECTURE.md) | Complete system architecture and design patterns |
| [**PROTOCOL.md**](docs/PROTOCOL.md) | Communication protocols and message formats |
| [**EXAMPLES.md**](docs/EXAMPLES.md) | Practical usage examples and integration guides |

---

## 🏗️ Architecture Highlights

### Modular Design
- **Extensible Processing Pipeline**: Add FFT, filters, custom modules without core changes
- **Multi-Channel Support**: Up to 8 synchronized channels with independent parameters
- **Plugin Architecture**: Dynamic loading of processing modules
- **Parameter Versioning**: Backward-compatible parameter system

### Real-Time Performance
- **Lock-Free Data Structures**: High-throughput data processing
- **Circular Buffers**: Continuous streaming without memory allocation
- **Threaded Architecture**: Separate threads for I/O, processing, communication
- **WebSocket Streaming**: Low-latency real-time data transmission

### Scalability Features
- **Processing Modules**: FFT analysis, digital filters, data storage
- **Multi-Channel Coordination**: Synchronized triggers and data alignment  
- **Extensible Protocols**: JSON-based messaging with type safety
- **Configuration Management**: Flexible, hierarchical configuration system

---

## 🔬 Technical Specifications

| Component | Specification |
|-----------|---------------|
| **Sampling Rate** | Up to 1 MSPS (I2S + ADC) |
| **Resolution** | 9-bit (0-511 ADC values) |
| **Channels** | Up to 8 synchronized channels |
| **Communication** | UART (115200) + WebSocket |
| **Latency** | < 100ms end-to-end |
| **Buffer Size** | Configurable (default 10K samples) |
| **Platform Support** | Linux, Windows, macOS |

---

## 🎯 Usage Examples

### Basic Operation
1. **Connect Hardware**: ESP32 via USB, signal to GPIO36
2. **Start Backend**: C++ backend automatically detects ESP32
3. **Open Frontend**: Browser interface at localhost:3000
4. **Configure**: Adjust timebase, voltage scale, trigger settings
5. **Observe**: Real-time waveform visualization

### Advanced Multi-Channel Setup
```javascript
// Configure 2-channel synchronized acquisition
const config = {
  channels: [
    { id: 0, name: "Input", enabled: true, triggerLevel: 1.65 },
    { id: 1, name: "Reference", enabled: true, triggerLevel: 2.5 }
  ],
  synchronizedTriggers: true,
  masterChannel: 0
};
```

### Custom Processing Module (C++)
```cpp
class CustomFilter : public ProcessingModule {
public:
    ProcessingResult process(const DataPointCollection& input, 
                           uint8_t channel, 
                           const ParameterMap& params) override {
        // Your custom signal processing here
        // Return processed data points
    }
};

REGISTER_PROCESSING_MODULE(CustomFilter, "custom_filter");
```

---

## 🐛 Troubleshooting

| Issue | Solution |
|-------|----------|
| **ESP32 not detected** | Check USB cable, driver installation, port permissions |
| **No signal data** | Verify GPIO36 connection, signal voltage (0-3.3V range) |
| **WebSocket connection fails** | Check firewall, ensure backend is running on correct port |
| **Build errors** | Install missing dependencies with `./build.sh --install-deps` |

For detailed troubleshooting, see the [**EXAMPLES.md**](docs/EXAMPLES.md) documentation.

---

## 🤝 Contributing

1. **Fork** the repository
2. **Create** feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** changes (`git commit -m 'Add amazing feature'`)
4. **Push** to branch (`git push origin feature/amazing-feature`)
5. **Open** Pull Request

---

## 📜 License

This project is open source. See LICENSE file for details.

---

## 🙏 Acknowledgments

- **ESP32 Community** for excellent hardware and documentation
- **Chart.js** for powerful visualization capabilities  
- **WebSocket** and **Socket.IO** for real-time communication
- **CMake** and **React** ecosystems for development tools

---

**Proyecto Izana - Building the future of modular signal acquisition and analysis**