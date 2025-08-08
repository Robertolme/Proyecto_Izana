import React, { useState, useEffect, useRef } from 'react';
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
} from 'chart.js';
import { Line } from 'react-chartjs-2';
import io from 'socket.io-client';
import './Oscilloscope.css';

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
);

const Oscilloscope = () => {
  const [connected, setConnected] = useState(false);
  const [data, setData] = useState([]);
  const [isRunning, setIsRunning] = useState(true);
  const [timeScale, setTimeScale] = useState(1000); // samples to display
  const [voltageScale, setVoltageScale] = useState(3.3); // max voltage
  const [triggerLevel, setTriggerLevel] = useState(256); // trigger level (0-511)
  const [triggerEnabled, setTriggerEnabled] = useState(false);
  const [measurements, setMeasurements] = useState({
    frequency: 0,
    amplitude: 0,
    vpp: 0, // voltage peak to peak
    vmax: 0,
    vmin: 0
  });

  const socketRef = useRef();
  const dataBufferRef = useRef([]);
  const maxBufferSize = 10000; // maximum samples to keep in memory

  const calculateMeasurements = React.useCallback((samples) => {
    if (samples.length < 10) return;

    const voltages = samples.map(s => s.voltage);
    const vmax = Math.max(...voltages);
    const vmin = Math.min(...voltages);
    const vpp = vmax - vmin;
    const amplitude = vpp / 2;

    // Simple frequency estimation using zero crossings
    const average = voltages.reduce((a, b) => a + b, 0) / voltages.length;
    let crossings = 0;
    for (let i = 1; i < voltages.length; i++) {
      if ((voltages[i-1] < average && voltages[i] >= average) || 
          (voltages[i-1] >= average && voltages[i] < average)) {
        crossings++;
      }
    }
    
    // Estimate frequency (very approximate)
    const timeSpan = (samples[samples.length-1].timestamp - samples[0].timestamp) / 1000; // seconds
    const frequency = timeSpan > 0 ? (crossings / 2) / timeSpan : 0;

    setMeasurements({
      frequency: Math.round(frequency * 100) / 100,
      amplitude: Math.round(amplitude * 1000) / 1000,
      vpp: Math.round(vpp * 1000) / 1000,
      vmax: Math.round(vmax * 1000) / 1000,
      vmin: Math.round(vmin * 1000) / 1000
    });
  }, []);

  const addDataPoint = React.useCallback((value) => {
    const timestamp = Date.now();
    const voltage = (value / 511) * voltageScale; // Convert ADC value to voltage
    
    dataBufferRef.current.push({ timestamp, value, voltage });
    
    // Keep buffer size manageable
    if (dataBufferRef.current.length > maxBufferSize) {
      dataBufferRef.current.shift();
    }

    // Update display data
    const displayData = dataBufferRef.current.slice(-timeScale);
    setData(displayData);

    // Calculate measurements
    calculateMeasurements(displayData);
  }, [voltageScale, timeScale, calculateMeasurements]);

  useEffect(() => {
    // Connect to backend WebSocket
    socketRef.current = io('http://localhost:3001');

    socketRef.current.on('connect', () => {
      console.log('Connected to backend');
      setConnected(true);
    });

    socketRef.current.on('disconnect', () => {
      console.log('Disconnected from backend');
      setConnected(false);
    });

    socketRef.current.on('uart-data', (signalData) => {
      if (isRunning && signalData.value !== undefined) {
        addDataPoint(signalData.value);
      }
    });

    return () => {
      if (socketRef.current) {
        socketRef.current.disconnect();
      }
    };
  }, [isRunning, addDataPoint]);

  const chartData = {
    labels: data.map((_, index) => index),
    datasets: [
      {
        label: 'Signal',
        data: data.map(point => point.voltage),
        borderColor: '#00ff00',
        backgroundColor: 'rgba(0, 255, 0, 0.1)',
        borderWidth: 1,
        pointRadius: 0,
        tension: 0.1,
      },
      ...(triggerEnabled ? [{
        label: 'Trigger',
        data: Array(data.length).fill((triggerLevel / 511) * voltageScale),
        borderColor: '#ff0000',
        backgroundColor: 'transparent',
        borderWidth: 1,
        pointRadius: 0,
        borderDash: [5, 5],
      }] : [])
    ],
  };

  const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    animation: false,
    scales: {
      x: {
        title: {
          display: true,
          text: 'Samples',
          color: '#ffffff'
        },
        ticks: {
          color: '#ffffff'
        },
        grid: {
          color: '#444444'
        }
      },
      y: {
        title: {
          display: true,
          text: 'Voltage (V)',
          color: '#ffffff'
        },
        min: 0,
        max: voltageScale,
        ticks: {
          color: '#ffffff'
        },
        grid: {
          color: '#444444'
        }
      },
    },
    plugins: {
      legend: {
        labels: {
          color: '#ffffff'
        }
      },
      title: {
        display: true,
        text: 'Digital Oscilloscope',
        color: '#ffffff'
      }
    },
    elements: {
      line: {
        tension: 0
      }
    }
  };

  const handlePauseResume = () => {
    setIsRunning(!isRunning);
  };

  const handleClear = () => {
    dataBufferRef.current = [];
    setData([]);
  };

  const handleTimeScaleChange = (event) => {
    setTimeScale(parseInt(event.target.value));
  };

  const handleVoltageScaleChange = (event) => {
    setVoltageScale(parseFloat(event.target.value));
  };

  const handleTriggerLevelChange = (event) => {
    setTriggerLevel(parseInt(event.target.value));
  };

  return (
    <div className="oscilloscope">
      <div className="oscilloscope-header">
        <h1>Digital Oscilloscope - Proyecto Izana</h1>
        <div className="status">
          Status: <span className={connected ? 'connected' : 'disconnected'}>
            {connected ? 'Connected' : 'Disconnected'}
          </span>
        </div>
      </div>

      <div className="oscilloscope-content">
        <div className="chart-container">
          <Line data={chartData} options={chartOptions} />
        </div>

        <div className="controls-panel">
          <div className="control-section">
            <h3>Acquisition</h3>
            <button 
              className={`control-btn ${isRunning ? 'running' : 'stopped'}`}
              onClick={handlePauseResume}
            >
              {isRunning ? 'Pause' : 'Resume'}
            </button>
            <button className="control-btn" onClick={handleClear}>
              Clear
            </button>
          </div>

          <div className="control-section">
            <h3>Timebase</h3>
            <label>
              Samples: 
              <select value={timeScale} onChange={handleTimeScaleChange}>
                <option value={100}>100</option>
                <option value={500}>500</option>
                <option value={1000}>1000</option>
                <option value={2000}>2000</option>
                <option value={5000}>5000</option>
              </select>
            </label>
          </div>

          <div className="control-section">
            <h3>Vertical Scale</h3>
            <label>
              Max Voltage: 
              <select value={voltageScale} onChange={handleVoltageScaleChange}>
                <option value={1.0}>1.0V</option>
                <option value={3.3}>3.3V</option>
                <option value={5.0}>5.0V</option>
              </select>
            </label>
          </div>

          <div className="control-section">
            <h3>Trigger</h3>
            <label>
              <input
                type="checkbox"
                checked={triggerEnabled}
                onChange={(e) => setTriggerEnabled(e.target.checked)}
              />
              Enable Trigger
            </label>
            <label>
              Level (ADC): 
              <input
                type="range"
                min="0"
                max="511"
                value={triggerLevel}
                onChange={handleTriggerLevelChange}
                disabled={!triggerEnabled}
              />
              <span>{triggerLevel}</span>
            </label>
          </div>

          <div className="control-section measurements">
            <h3>Measurements</h3>
            <div className="measurement">
              <label>Frequency:</label>
              <span>{measurements.frequency} Hz</span>
            </div>
            <div className="measurement">
              <label>Amplitude:</label>
              <span>{measurements.amplitude} V</span>
            </div>
            <div className="measurement">
              <label>Vpp:</label>
              <span>{measurements.vpp} V</span>
            </div>
            <div className="measurement">
              <label>Vmax:</label>
              <span>{measurements.vmax} V</span>
            </div>
            <div className="measurement">
              <label>Vmin:</label>
              <span>{measurements.vmin} V</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Oscilloscope;