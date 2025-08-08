# Proyecto Izana - Digital Oscilloscope

## Overview

Proyecto Izana is a complete digital oscilloscope solution that receives signals from an ESP32 microcontroller via UART and displays them in a real-time web interface that mimics a traditional oscilloscope.

## Features

### ESP32 Hardware Features
- High-speed ADC sampling using I2S + ADC (up to 1 MSPS)
- 9-bit resolution (0-511 ADC values)
- UART output at 115200 baud
- Compatible with GPIO36 (ADC1_CHANNEL_0)

### Backend Features
- Real-time UART communication with ESP32
- WebSocket streaming for real-time data
- Automatic serial port detection
- Support for multiple common serial ports
- Error handling and automatic reconnection
- REST API for status monitoring

### Frontend Features
- Professional oscilloscope-like interface
- Real-time waveform visualization using Chart.js
- Oscilloscope controls:
  - Timebase adjustment (100-5000 samples)
  - Voltage scale selection (1V, 3.3V, 5V)
  - Trigger functionality with adjustable level
  - Pause/Resume acquisition
  - Clear display
- Real-time measurements:
  - Frequency estimation
  - Peak-to-peak voltage (Vpp)
  - Maximum and minimum voltages
  - Signal amplitude
- Dark theme optimized for oscilloscope viewing
- Responsive design for different screen sizes

## Quick Start

### 1. Hardware Setup
1. Connect your ESP32 to your computer via USB
2. Upload the provided Arduino code to the ESP32
3. Connect your analog signal to GPIO36
4. The ESP32 will output data in format: `signal:123`

### 2. Backend Setup
```bash
cd backend
npm install
node server.js
```
The backend will automatically detect your ESP32 on common serial ports.

### 3. Frontend Setup
```bash
cd frontend
npm install
npm start
```
The oscilloscope interface will be available at `http://localhost:3000`

## Development and Testing

### Test Data Simulator
For development and demonstration purposes, we include a test data simulator:

```bash
node test-simulator.js
```

This generates synthetic waveforms (sine, square, triangle waves + noise) to test the oscilloscope functionality without hardware.

## Architecture

### Data Flow
```
ESP32 ADC → UART (115200 baud) → Backend (Node.js) → WebSocket → Frontend (React)
                                      ↓
                              Test Simulator (for development)
```

### Backend Components
- **Serial Port Management**: Automatic detection and connection
- **WebSocket Server**: Real-time data streaming using Socket.IO
- **Data Processing**: Converts ADC values to voltages
- **Error Handling**: Graceful reconnection and error recovery

### Frontend Components
- **Oscilloscope Component**: Main UI with real-time graphing
- **Chart.js Integration**: High-performance canvas-based plotting
- **Control Panel**: Timebase, voltage scale, trigger controls
- **Measurements Panel**: Real-time signal analysis

## File Structure

```
Proyecto_Izana/
├── backend/
│   ├── server.js           # Main backend server
│   ├── package.json        # Backend dependencies
│   └── ...
├── frontend/
│   ├── src/
│   │   ├── components/
│   │   │   ├── Oscilloscope.js    # Main oscilloscope component
│   │   │   └── Oscilloscope.css   # Styling
│   │   └── App.js          # React app entry point
│   ├── package.json        # Frontend dependencies
│   └── ...
├── codigos del arduino/
│   └── adc/
│       └── adc.ino         # ESP32 firmware
├── test-simulator.js       # Development test data generator
└── README.md              # This file
```

## API Endpoints

### Backend REST API
- `GET /api/status` - Server and UART connection status
- `GET /api/health` - Health check endpoint

### WebSocket Events
- `uart-data` - Real-time ADC data from ESP32
- `uart-status` - UART connection status updates
- `test-data` - Test data from simulator (development)

## Configuration

### Backend Configuration
- **Port**: 3001 (configurable in server.js)
- **UART Baud Rate**: 115200
- **Auto-detected Ports**: `/dev/ttyUSB0`, `/dev/ttyACM0`, `COM3-6`

### Frontend Configuration
- **Backend URL**: `http://localhost:3001` (configurable in Oscilloscope.js)
- **Chart Update Rate**: Real-time (no throttling)
- **Buffer Size**: 10,000 samples maximum

## Signal Format

The ESP32 sends data in the format:
```
signal:123
signal:456
signal:078
```

Where the number is a 9-bit ADC value (0-511) representing the analog input voltage.

## Voltage Conversion

ADC values are converted to voltages using:
```javascript
voltage = (adcValue / 511) * voltageScale
```

Where `voltageScale` is the selected maximum voltage (1V, 3.3V, or 5V).

## Troubleshooting

### No Serial Device Found
- Ensure ESP32 is connected via USB
- Check that the ESP32 appears in device manager
- Try different USB ports or cables
- The system will automatically retry connection

### Frontend Not Connecting
- Verify backend is running on port 3001
- Check browser console for WebSocket errors
- Ensure firewall allows connections to localhost:3001

### No Signal Data
- Verify ESP32 is sending data via serial monitor
- Check that GPIO36 has a signal connected
- Ensure signal is within 0-3.3V range
- Try the test simulator for development

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test with both real hardware and test simulator
5. Submit a pull request

## License

This project is open source. Please refer to the license file for details.

## Credits

Developed as part of Proyecto Izana for high-speed signal acquisition and visualization using ESP32 and web technologies.