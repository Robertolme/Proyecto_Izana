const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const cors = require('cors');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const app = express();
const server = http.createServer(app);
const io = socketIo(server, {
  cors: {
    origin: "http://localhost:3000",
    methods: ["GET", "POST"]
  }
});

const port = 3001;

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static('public'));

// UART Configuration
let uart = null;
let parser = null;
let isConnected = false;

// Function to try different common serial ports
const trySerialPorts = async () => {
  const commonPorts = [
    '/dev/ttyUSB0',
    '/dev/ttyUSB1', 
    '/dev/ttyACM0',
    '/dev/ttyACM1',
    'COM3',
    'COM4',
    'COM5',
    'COM6'
  ];

  for (const portPath of commonPorts) {
    try {
      console.log(`Trying to connect to ${portPath}...`);
      
      const testPort = new SerialPort({
        path: portPath,
        baudRate: 115200,
        autoOpen: false
      });

      await new Promise((resolve, reject) => {
        testPort.open((err) => {
          if (err) {
            reject(err);
          } else {
            resolve();
          }
        });
      });

      console.log(`Successfully connected to ${portPath}`);
      return testPort;
    } catch (error) {
      console.log(`Failed to connect to ${portPath}: ${error.message}`);
    }
  }
  
  return null;
};

// Initialize UART connection
const initializeUART = async () => {
  try {
    uart = await trySerialPorts();
    
    if (!uart) {
      console.log('No serial device found. Will retry in 5 seconds...');
      setTimeout(initializeUART, 5000);
      return;
    }

    // Create parser for line-based data
    parser = uart.pipe(new ReadlineParser({ delimiter: '\n' }));

    // Handle incoming data
    parser.on('data', (line) => {
      const trimmedLine = line.trim();
      console.log('Received from ESP32:', trimmedLine);

      // Parse signal data in format "signal:123"
      const match = trimmedLine.match(/^signal:(\d{1,3})$/);
      if (match) {
        const value = parseInt(match[1], 10);
        
        // Emit to all connected clients
        io.emit('uart-data', {
          value: value,
          timestamp: Date.now(),
          voltage: (value / 511) * 3.3 // Convert to voltage
        });
      }
    });

    // Handle UART events
    uart.on('open', () => {
      console.log('UART connection opened successfully');
      isConnected = true;
      io.emit('uart-status', { connected: true });
    });

    uart.on('close', () => {
      console.log('UART connection closed');
      isConnected = false;
      io.emit('uart-status', { connected: false });
      
      // Try to reconnect after 3 seconds
      setTimeout(initializeUART, 3000);
    });

    uart.on('error', (err) => {
      console.error('UART error:', err.message);
      isConnected = false;
      io.emit('uart-status', { connected: false });
      
      // Try to reconnect after 5 seconds
      setTimeout(initializeUART, 5000);
    });

  } catch (error) {
    console.error('Failed to initialize UART:', error.message);
    isConnected = false;
    
    // Retry connection after 5 seconds
    setTimeout(initializeUART, 5000);
  }
};

// WebSocket connection handling
io.on('connection', (socket) => {
  console.log('Client connected:', socket.id);
  
  // Send current connection status
  socket.emit('uart-status', { connected: isConnected });
  
  socket.on('disconnect', () => {
    console.log('Client disconnected:', socket.id);
  });

  // Handle client requests for UART commands (future expansion)
  socket.on('uart-command', (command) => {
    if (uart && isConnected) {
      console.log('Sending command to ESP32:', command);
      uart.write(command + '\n');
    } else {
      socket.emit('error', { message: 'UART not connected' });
    }
  });

  // Handle test data from simulator (for development/testing)
  socket.on('test-data', (data) => {
    console.log('Received test data from simulator:', data.value);
    
    // Emit to all connected clients as if it came from UART
    io.emit('uart-data', {
      value: data.value,
      timestamp: data.timestamp || Date.now(),
      voltage: (data.value / 511) * 3.3 // Convert to voltage
    });
  });
});

// REST API endpoints
app.get('/api/status', (req, res) => {
  res.json({
    uart: {
      connected: isConnected,
      port: uart ? uart.path : null
    },
    server: {
      uptime: process.uptime(),
      timestamp: new Date().toISOString()
    }
  });
});

app.get('/api/health', (req, res) => {
  res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

// Start server
server.listen(port, () => {
  console.log(`Server running on http://localhost:${port}`);
  console.log('Initializing UART connection...');
  
  // Initialize UART connection
  initializeUART();
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('Shutting down server...');
  
  if (uart && uart.isOpen) {
    uart.close((err) => {
      if (err) {
        console.error('Error closing UART:', err.message);
      } else {
        console.log('UART connection closed');
      }
      process.exit(0);
    });
  } else {
    process.exit(0);
  }
});
