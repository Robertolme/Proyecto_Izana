const io = require('socket.io-client');

// Connect to the backend server
const socket = io('http://localhost:3001');

let isConnected = false;
let simulationRunning = false;

socket.on('connect', () => {
  console.log('Test simulator connected to backend');
  isConnected = true;
  startSimulation();
});

socket.on('disconnect', () => {
  console.log('Test simulator disconnected from backend');
  isConnected = false;
  simulationRunning = false;
});

function generateSineWave(time, frequency = 1, amplitude = 255, offset = 256) {
  const value = Math.round(offset + amplitude * Math.sin(2 * Math.PI * frequency * time));
  return Math.max(0, Math.min(511, value)); // Clamp to 9-bit ADC range
}

function generateSquareWave(time, frequency = 0.5, amplitude = 200, offset = 256) {
  const cycle = time * frequency;
  const value = (cycle % 1) < 0.5 ? offset - amplitude : offset + amplitude;
  return Math.max(0, Math.min(511, value));
}

function generateTriangleWave(time, frequency = 0.3, amplitude = 150, offset = 256) {
  const cycle = time * frequency;
  const normalized = cycle % 1;
  let value;
  if (normalized < 0.5) {
    value = offset + amplitude * (2 * normalized - 1);
  } else {
    value = offset + amplitude * (3 - 2 * normalized);
  }
  return Math.max(0, Math.min(511, value));
}

function generateNoise(amplitude = 20, offset = 256) {
  const value = offset + amplitude * (Math.random() - 0.5) * 2;
  return Math.max(0, Math.min(511, value));
}

function startSimulation() {
  if (simulationRunning) return;
  
  simulationRunning = true;
  console.log('Starting oscilloscope test data simulation...');
  console.log('Generating mixed signal: sine wave + square wave + noise');
  
  const startTime = Date.now() / 1000;
  let sampleCount = 0;
  
  const interval = setInterval(() => {
    if (!isConnected || !simulationRunning) {
      clearInterval(interval);
      return;
    }
    
    const currentTime = Date.now() / 1000;
    const elapsedTime = currentTime - startTime;
    
    // Generate a complex signal: sine wave + square wave + triangle + noise
    //const sine = generateSineWave(elapsedTime, 2, 100, 0);        // 2 Hz sine
    const square = generateSquareWave(elapsedTime, 0.5, 80, 0);   // 0.5 Hz square
    //const triangle = generateTriangleWave(elapsedTime, 1.5, 60, 0); // 1.5 Hz triangle
    //const noise = generateNoise(15, 0);                           // Small noise
    
    // Combine signals and normalize
    let combinedValue = square;//sine; //+ square + triangle + noise;
    combinedValue = Math.max(0, Math.min(511, combinedValue));
    
    // Emit as if it came from UART
    socket.emit('test-data', {
      value: Math.round(combinedValue),
      timestamp: Date.now(),
      sampleCount: sampleCount++
    });
    
    // Log every 100 samples
    if (sampleCount % 100 === 0) {
      console.log(`Sample ${sampleCount}: ADC value = ${Math.round(combinedValue)}`);
    }
    
  }, 10); // 100 samples per second (much slower than real 1MSPS for demo)
}

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\nShutting down test simulator...');
  simulationRunning = false;
  socket.disconnect();
  process.exit(0);
});

console.log('ESP32 Oscilloscope Test Data Simulator');
console.log('Connecting to backend at http://localhost:3001...');