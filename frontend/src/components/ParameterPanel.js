import React, { useState } from 'react';
import './ParameterPanel.css';

/**
 * Enhanced parameter panel for advanced oscilloscope controls
 * Supports the new modular backend architecture with extensible parameters
 */
const ParameterPanel = ({ 
  connected, 
  onParameterUpdate, 
  onCommand,
  currentParams,
  availableModules = [] 
}) => {
  const [showAdvanced, setShowAdvanced] = useState(false);
  const [activeTab, setActiveTab] = useState('basic');

  // Core oscilloscope parameters
  const [coreParams, setCoreParams] = useState({
    timeScale: currentParams?.timeScale || 1000,
    amplitudeScale: currentParams?.amplitudeScale || 3.3,
    verticalOffset: currentParams?.verticalOffset || 0.0,
    horizontalOffset: currentParams?.horizontalOffset || 0.0,
    sampleRate: currentParams?.sampleRate || 1000000,
    autoScale: currentParams?.autoScale || false,
    ...currentParams
  });

  // Trigger configuration
  const [triggerParams, setTriggerParams] = useState({
    enabled: currentParams?.triggerEnabled || false,
    level: currentParams?.triggerLevel || 1.65,
    edge: currentParams?.triggerEdge || 'rising',
    hysteresis: currentParams?.triggerHysteresis || 0.1,
    preTriggerSamples: currentParams?.preTriggerSamples || 100,
    postTriggerSamples: currentParams?.postTriggerSamples || 900,
    timeout: currentParams?.triggerTimeout || 1000.0
  });

  // Multi-channel parameters
  const [channelParams, setChannelParams] = useState({
    activeChannels: currentParams?.activeChannels || [0],
    synchronizedTriggers: currentParams?.synchronizedTriggers || false,
    masterChannel: currentParams?.masterChannel || 0
  });

  // Processing module parameters
  const [moduleParams, setModuleParams] = useState({
    fft: {
      enabled: false,
      windowSize: 1024,
      windowType: 'hanning',
      overlapRatio: 0.5,
      frequencyRange: [0, 500000]
    },
    filter: {
      enabled: false,
      type: 'lowpass',
      cutoffFreq: 10000,
      order: 4
    },
    storage: {
      enabled: false,
      format: 'csv',
      autoSave: false,
      saveInterval: 60
    }
  });

  const handleCoreParamChange = (param, value) => {
    const newParams = { ...coreParams, [param]: value };
    setCoreParams(newParams);
    
    if (connected) {
      onParameterUpdate({
        type: 'core',
        channel: channelParams.masterChannel,
        parameters: newParams
      });
    }
  };

  const handleTriggerParamChange = (param, value) => {
    const newParams = { ...triggerParams, [param]: value };
    setTriggerParams(newParams);
    
    if (connected) {
      onParameterUpdate({
        type: 'trigger',
        channel: channelParams.masterChannel,
        parameters: newParams
      });
    }
  };

  const handleChannelParamChange = (param, value) => {
    const newParams = { ...channelParams, [param]: value };
    setChannelParams(newParams);
    
    if (connected) {
      onParameterUpdate({
        type: 'channel',
        parameters: newParams
      });
    }
  };

  const handleModuleParamChange = (module, param, value) => {
    const newModuleParams = {
      ...moduleParams,
      [module]: {
        ...moduleParams[module],
        [param]: value
      }
    };
    setModuleParams(newModuleParams);
    
    if (connected) {
      onParameterUpdate({
        type: 'module',
        module: module,
        parameters: newModuleParams[module]
      });
    }
  };

  const handleCommand = (command, params = {}) => {
    if (connected) {
      onCommand(command, params);
    }
  };

  return (
    <div className="parameter-panel">
      <div className="panel-header">
        <h3>Oscilloscope Controls</h3>
        <div className="connection-status">
          <span className={`status-indicator ${connected ? 'connected' : 'disconnected'}`}>
            {connected ? 'Connected' : 'Disconnected'}
          </span>
        </div>
      </div>

      <div className="tab-navigation">
        <button 
          className={activeTab === 'basic' ? 'active' : ''}
          onClick={() => setActiveTab('basic')}
        >
          Basic
        </button>
        <button 
          className={activeTab === 'trigger' ? 'active' : ''}
          onClick={() => setActiveTab('trigger')}
        >
          Trigger
        </button>
        <button 
          className={activeTab === 'channels' ? 'active' : ''}
          onClick={() => setActiveTab('channels')}
        >
          Channels
        </button>
        <button 
          className={activeTab === 'processing' ? 'active' : ''}
          onClick={() => setActiveTab('processing')}
        >
          Processing
        </button>
      </div>

      {/* Basic Controls Tab */}
      {activeTab === 'basic' && (
        <div className="tab-content">
          <div className="param-group">
            <label>Time Scale (samples)</label>
            <input
              type="range"
              min="100"
              max="10000"
              step="100"
              value={coreParams.timeScale}
              onChange={(e) => handleCoreParamChange('timeScale', parseInt(e.target.value))}
              disabled={!connected}
            />
            <span>{coreParams.timeScale}</span>
          </div>

          <div className="param-group">
            <label>Voltage Scale (V)</label>
            <select
              value={coreParams.amplitudeScale}
              onChange={(e) => handleCoreParamChange('amplitudeScale', parseFloat(e.target.value))}
              disabled={!connected}
            >
              <option value="1.0">1.0V</option>
              <option value="3.3">3.3V</option>
              <option value="5.0">5.0V</option>
              <option value="12.0">12.0V</option>
            </select>
          </div>

          <div className="param-group">
            <label>Vertical Offset (V)</label>
            <input
              type="range"
              min="-5.0"
              max="5.0"
              step="0.1"
              value={coreParams.verticalOffset}
              onChange={(e) => handleCoreParamChange('verticalOffset', parseFloat(e.target.value))}
              disabled={!connected}
            />
            <span>{coreParams.verticalOffset.toFixed(1)}</span>
          </div>

          <div className="param-group">
            <label>Horizontal Offset (s)</label>
            <input
              type="range"
              min="-0.001"
              max="0.001"
              step="0.00001"
              value={coreParams.horizontalOffset}
              onChange={(e) => handleCoreParamChange('horizontalOffset', parseFloat(e.target.value))}
              disabled={!connected}
            />
            <span>{(coreParams.horizontalOffset * 1000).toFixed(2)} ms</span>
          </div>

          <div className="param-group">
            <label>
              <input
                type="checkbox"
                checked={coreParams.autoScale}
                onChange={(e) => handleCoreParamChange('autoScale', e.target.checked)}
                disabled={!connected}
              />
              Auto Scale
            </label>
          </div>

          <div className="control-buttons">
            <button 
              onClick={() => handleCommand('clear_buffer', { channel: channelParams.masterChannel })}
              disabled={!connected}
            >
              Clear
            </button>
            <button 
              onClick={() => handleCommand('force_trigger', { channel: channelParams.masterChannel })}
              disabled={!connected}
            >
              Force Trigger
            </button>
            <button 
              onClick={() => handleCommand('reset_statistics')}
              disabled={!connected}
            >
              Reset Stats
            </button>
          </div>
        </div>
      )}

      {/* Trigger Controls Tab */}
      {activeTab === 'trigger' && (
        <div className="tab-content">
          <div className="param-group">
            <label>
              <input
                type="checkbox"
                checked={triggerParams.enabled}
                onChange={(e) => handleTriggerParamChange('enabled', e.target.checked)}
                disabled={!connected}
              />
              Enable Trigger
            </label>
          </div>

          <div className="param-group">
            <label>Trigger Level (V)</label>
            <input
              type="range"
              min="0.0"
              max={coreParams.amplitudeScale}
              step="0.01"
              value={triggerParams.level}
              onChange={(e) => handleTriggerParamChange('level', parseFloat(e.target.value))}
              disabled={!connected || !triggerParams.enabled}
            />
            <span>{triggerParams.level.toFixed(2)}</span>
          </div>

          <div className="param-group">
            <label>Trigger Edge</label>
            <select
              value={triggerParams.edge}
              onChange={(e) => handleTriggerParamChange('edge', e.target.value)}
              disabled={!connected || !triggerParams.enabled}
            >
              <option value="rising">Rising</option>
              <option value="falling">Falling</option>
              <option value="both">Both</option>
            </select>
          </div>

          <div className="param-group">
            <label>Hysteresis (V)</label>
            <input
              type="range"
              min="0.01"
              max="1.0"
              step="0.01"
              value={triggerParams.hysteresis}
              onChange={(e) => handleTriggerParamChange('hysteresis', parseFloat(e.target.value))}
              disabled={!connected || !triggerParams.enabled}
            />
            <span>{triggerParams.hysteresis.toFixed(2)}</span>
          </div>

          <div className="param-group">
            <label>Pre-trigger Samples</label>
            <input
              type="range"
              min="10"
              max="500"
              step="10"
              value={triggerParams.preTriggerSamples}
              onChange={(e) => handleTriggerParamChange('preTriggerSamples', parseInt(e.target.value))}
              disabled={!connected || !triggerParams.enabled}
            />
            <span>{triggerParams.preTriggerSamples}</span>
          </div>

          <div className="param-group">
            <label>Post-trigger Samples</label>
            <input
              type="range"
              min="100"
              max="2000"
              step="10"
              value={triggerParams.postTriggerSamples}
              onChange={(e) => handleTriggerParamChange('postTriggerSamples', parseInt(e.target.value))}
              disabled={!connected || !triggerParams.enabled}
            />
            <span>{triggerParams.postTriggerSamples}</span>
          </div>

          <div className="param-group">
            <label>Timeout (ms)</label>
            <input
              type="range"
              min="100"
              max="10000"
              step="100"
              value={triggerParams.timeout}
              onChange={(e) => handleTriggerParamChange('timeout', parseFloat(e.target.value))}
              disabled={!connected || !triggerParams.enabled}
            />
            <span>{triggerParams.timeout}</span>
          </div>
        </div>
      )}

      {/* Multi-channel Controls Tab */}
      {activeTab === 'channels' && (
        <div className="tab-content">
          <div className="param-group">
            <label>Active Channels</label>
            <div className="channel-checkboxes">
              {[0, 1, 2, 3].map(channel => (
                <label key={channel}>
                  <input
                    type="checkbox"
                    checked={channelParams.activeChannels.includes(channel)}
                    onChange={(e) => {
                      const newChannels = e.target.checked
                        ? [...channelParams.activeChannels, channel]
                        : channelParams.activeChannels.filter(c => c !== channel);
                      handleChannelParamChange('activeChannels', newChannels);
                    }}
                    disabled={!connected}
                  />
                  CH{channel}
                </label>
              ))}
            </div>
          </div>

          <div className="param-group">
            <label>
              <input
                type="checkbox"
                checked={channelParams.synchronizedTriggers}
                onChange={(e) => handleChannelParamChange('synchronizedTriggers', e.target.checked)}
                disabled={!connected}
              />
              Synchronized Triggers
            </label>
          </div>

          <div className="param-group">
            <label>Master Channel</label>
            <select
              value={channelParams.masterChannel}
              onChange={(e) => handleChannelParamChange('masterChannel', parseInt(e.target.value))}
              disabled={!connected}
            >
              {channelParams.activeChannels.map(channel => (
                <option key={channel} value={channel}>Channel {channel}</option>
              ))}
            </select>
          </div>
        </div>
      )}

      {/* Processing Modules Tab */}
      {activeTab === 'processing' && (
        <div className="tab-content">
          {/* FFT Module */}
          <div className="module-section">
            <h4>FFT Analysis</h4>
            <div className="param-group">
              <label>
                <input
                  type="checkbox"
                  checked={moduleParams.fft.enabled}
                  onChange={(e) => handleModuleParamChange('fft', 'enabled', e.target.checked)}
                  disabled={!connected}
                />
                Enable FFT
              </label>
            </div>

            {moduleParams.fft.enabled && (
              <>
                <div className="param-group">
                  <label>Window Size</label>
                  <select
                    value={moduleParams.fft.windowSize}
                    onChange={(e) => handleModuleParamChange('fft', 'windowSize', parseInt(e.target.value))}
                    disabled={!connected}
                  >
                    <option value="256">256</option>
                    <option value="512">512</option>
                    <option value="1024">1024</option>
                    <option value="2048">2048</option>
                    <option value="4096">4096</option>
                  </select>
                </div>

                <div className="param-group">
                  <label>Window Type</label>
                  <select
                    value={moduleParams.fft.windowType}
                    onChange={(e) => handleModuleParamChange('fft', 'windowType', e.target.value)}
                    disabled={!connected}
                  >
                    <option value="rectangular">Rectangular</option>
                    <option value="hanning">Hanning</option>
                    <option value="hamming">Hamming</option>
                    <option value="blackman">Blackman</option>
                  </select>
                </div>
              </>
            )}
          </div>

          {/* Filter Module */}
          <div className="module-section">
            <h4>Digital Filter</h4>
            <div className="param-group">
              <label>
                <input
                  type="checkbox"
                  checked={moduleParams.filter.enabled}
                  onChange={(e) => handleModuleParamChange('filter', 'enabled', e.target.checked)}
                  disabled={!connected}
                />
                Enable Filter
              </label>
            </div>

            {moduleParams.filter.enabled && (
              <>
                <div className="param-group">
                  <label>Filter Type</label>
                  <select
                    value={moduleParams.filter.type}
                    onChange={(e) => handleModuleParamChange('filter', 'type', e.target.value)}
                    disabled={!connected}
                  >
                    <option value="lowpass">Low Pass</option>
                    <option value="highpass">High Pass</option>
                    <option value="bandpass">Band Pass</option>
                    <option value="bandstop">Band Stop</option>
                  </select>
                </div>

                <div className="param-group">
                  <label>Cutoff Frequency (Hz)</label>
                  <input
                    type="range"
                    min="100"
                    max="100000"
                    step="100"
                    value={moduleParams.filter.cutoffFreq}
                    onChange={(e) => handleModuleParamChange('filter', 'cutoffFreq', parseInt(e.target.value))}
                    disabled={!connected}
                  />
                  <span>{moduleParams.filter.cutoffFreq}</span>
                </div>
              </>
            )}
          </div>

          {/* Storage Module */}
          <div className="module-section">
            <h4>Data Storage</h4>
            <div className="param-group">
              <label>
                <input
                  type="checkbox"
                  checked={moduleParams.storage.enabled}
                  onChange={(e) => handleModuleParamChange('storage', 'enabled', e.target.checked)}
                  disabled={!connected}
                />
                Enable Storage
              </label>
            </div>

            {moduleParams.storage.enabled && (
              <>
                <div className="param-group">
                  <label>Format</label>
                  <select
                    value={moduleParams.storage.format}
                    onChange={(e) => handleModuleParamChange('storage', 'format', e.target.value)}
                    disabled={!connected}
                  >
                    <option value="csv">CSV</option>
                    <option value="json">JSON</option>
                    <option value="binary">Binary</option>
                  </select>
                </div>

                <div className="param-group">
                  <label>
                    <input
                      type="checkbox"
                      checked={moduleParams.storage.autoSave}
                      onChange={(e) => handleModuleParamChange('storage', 'autoSave', e.target.checked)}
                      disabled={!connected}
                    />
                    Auto Save
                  </label>
                </div>
              </>
            )}
          </div>
        </div>
      )}
    </div>
  );
};

export default ParameterPanel;