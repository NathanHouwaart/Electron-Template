import { useEffect, useState } from 'react';
import {
  Wifi, Download, Zap, Radio, Loader2,
  Info, Key, Lock, Shield, Plus, Search, Settings, Activity,
  WifiOff, Circle
} from 'lucide-react';
import { SelfTestPanel } from './SelfTestPanel';

export const ModernSidebar = () => {
  const [currentPage, setCurrentPage] = useState('dashboard');
  const [selectedPort, setSelectedPort] = useState('COM3');
  const [connectionStatus, setConnectionStatus] = useState('disconnected');
  const [firmware, setFirmware] = useState('');

  const [ports, setPorts] = useState<{ path: string; manufacturer?: string }[]>([]);

  useEffect(() => {

    const getAvailablePorts = async () => {
      const ports = await window.electron.listComPorts();
      setPorts(ports);
    };
    getAvailablePorts();
  }, []);

  // Listen for device disconnect events and reset UI state
  useEffect(() => {
    const unsub = window.electron.onDeviceDisconnected(() => {
      setConnectionStatus('disconnected');
      setFirmware('');
      setSelectedPort('');
      // Refresh ports list
      window.electron.listComPorts().then((p) => setPorts(p));
    });
    return () => unsub();
  }, []);

  const handleConnect = async () => {
    if (connectionStatus !== 'disconnected') return;
    if (!selectedPort) return;

    window.electron.connect(selectedPort).then(
      (result) => {
        console.log("Result of connect:", result);
        if (result) {
          setConnectionStatus('connected');
          handleGetFirmware();
        } else {
          setConnectionStatus('Error connecting');
          setTimeout(() => {
            setConnectionStatus('disconnected');
          }, 2000);
        }
      }
    ).catch(
      (error) => {
        console.error("Error during connect:", error);
        setConnectionStatus('Error connecting');
        setTimeout(() => {
          setConnectionStatus('disconnected');
        }, 2000);
      }
    );
    setConnectionStatus('connecting');
  };

  const handleDisconnect = async () => {
    if (connectionStatus !== 'connected') return;

    window.electron.disconnect().then(
      (result) => {
        console.log("Result of disconnect:", result);
        if (!result) {
          setConnectionStatus('Error disconnecting');
          setTimeout(() => {
            setConnectionStatus('connected');
          }, 2000);
        } else {
          setConnectionStatus('disconnected');
          setFirmware('');
        }
      }
    );
    setConnectionStatus('disconnecting');
  };

  const handleGetFirmware = () => {
    // if(connectionStatus !== 'connected') return;
    window.electron.getFirmwareVersion().then(
      (version) => {
        setFirmware(version);
      }
    );

  };

  const handleGetVersion = () => {
    window.electron.getVersion().then(
      (version) => {
        console.log("App Version:", version);
      }
    );
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-50 to-slate-100 flex">
      {/* Sidebar */}
      <div className="w-72 bg-white border-r border-slate-200 flex flex-col">
        {/* Logo/Header */}
        <div className="p-6 border-b border-slate-200">
          <div className="flex items-center gap-3">
            <div className="w-10 h-10 bg-gradient-to-br from-blue-500 to-purple-600 rounded-xl flex items-center justify-center">
              <Shield className="w-6 h-6 text-white" />
            </div>
            <div>
              <h1 className="text-lg font-bold text-slate-800">SecurePass</h1>
              <p className="text-xs text-slate-500">NFC Password Manager</p>
            </div>
          </div>
        </div>

        {/* Connection Status */}
        <div className="p-4 border-b border-slate-200">
          <div className="bg-slate-50 rounded-lg p-3">
            <div className="flex items-center justify-between mb-2">
              <span className="text-xs font-medium text-slate-600">Connection</span>
              <div className={`flex items-center gap-1.5 ${connectionStatus === 'connected' ? 'text-green-600' :
                  connectionStatus === 'connecting' ? 'text-yellow-600' :
                    'text-slate-400'
                }`}>
                {connectionStatus === 'connected' ? (
                  <><Circle className="w-2 h-2 fill-current" /><span className="text-xs font-medium">Connected</span></>
                ) : connectionStatus === 'connecting' ? (
                  <><Loader2 className="w-3 h-3 animate-spin" /><span className="text-xs font-medium">Connecting</span></>
                ) : (
                  <><WifiOff className="w-3 h-3" /><span className="text-xs font-medium">Disconnected</span></>
                )}
              </div>
            </div>
            {connectionStatus === 'connected' && (
              <div className="text-xs text-slate-600 font-mono">{selectedPort}</div>
            )}
          </div>
        </div>

        {/* Navigation */}
        <nav className="flex-1 p-4">
          <div className="space-y-1">
            <button
              onClick={() => setCurrentPage('dashboard')}
              className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-all ${currentPage === 'dashboard'
                  ? 'bg-blue-50 text-blue-600 font-medium'
                  : 'text-slate-600 hover:bg-slate-50'
                }`}
            >
              <Key className="w-5 h-5" />
              <span>Passwords</span>
            </button>
            <button
              onClick={() => setCurrentPage('pn532')}
              className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-all ${currentPage === 'pn532'
                  ? 'bg-blue-50 text-blue-600 font-medium'
                  : 'text-slate-600 hover:bg-slate-50'
                }`}
            >
              <Radio className="w-5 h-5" />
              <span>PN532 Reader</span>
            </button>
            <button
              onClick={() => setCurrentPage('settings')}
              className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-all ${currentPage === 'settings'
                  ? 'bg-blue-50 text-blue-600 font-medium'
                  : 'text-slate-600 hover:bg-slate-50'
                }`}
            >
              <Settings className="w-5 h-5" />
              <span>Settings</span>
            </button>
          </div>
        </nav>

        {/* Footer */}
        <div className="p-4 border-t border-slate-200">
          <div className="text-xs text-slate-500 text-center">
            v1.0.0 • Secure & Encrypted
          </div>
        </div>
      </div>

      {/* Main Content */}
      <div className="flex-1 flex flex-col">
        {/* Top Bar */}
        <div className="bg-white border-b border-slate-200 px-8 py-4">
          <div className="flex items-center justify-between">
            <div>
              <h2 className="text-2xl font-bold text-slate-800">
                {currentPage === 'dashboard' ? 'Password Vault' :
                  currentPage === 'pn532' ? 'PN532 Management' :
                    'Settings'}
              </h2>
              <p className="text-sm text-slate-500">
                {currentPage === 'dashboard' ? 'Manage your secured passwords' :
                  currentPage === 'pn532' ? 'Configure and test your NFC reader' :
                    'Application preferences'}
              </p>
            </div>
            {currentPage === 'dashboard' && (
              <button className="px-4 py-2 bg-blue-500 hover:bg-blue-600 text-white rounded-lg font-medium flex items-center gap-2 transition-colors">
                <Plus className="w-4 h-4" />
                Add Password
              </button>
            )}
          </div>
        </div>

        {/* Content Area */}
        <div className="flex-1 p-8 overflow-y-auto">
          {currentPage === 'dashboard' && (
            <div className="max-w-5xl">
              {/* Search Bar */}
              <div className="mb-6">
                <div className="relative">
                  <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-5 h-5 text-slate-400" />
                  <input
                    type="text"
                    placeholder="Search passwords..."
                    className="w-full pl-10 pr-4 py-3 bg-white border border-slate-200 rounded-xl focus:outline-none focus:ring-2 focus:ring-blue-500"
                  />
                </div>
              </div>

              {/* Password Cards */}
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                {[
                  { name: 'GitHub', username: 'user@email.com', icon: '💻' },
                  { name: 'Gmail', username: 'personal@gmail.com', icon: '📧' },
                  { name: 'AWS Console', username: 'admin', icon: '☁️' },
                  { name: 'Dropbox', username: 'user@email.com', icon: '📦' },
                ].map((item, idx) => (
                  <div key={idx} className="bg-white border border-slate-200 rounded-xl p-5 hover:shadow-md transition-shadow">
                    <div className="flex items-start justify-between mb-3">
                      <div className="flex items-center gap-3">
                        <div className="text-2xl">{item.icon}</div>
                        <div>
                          <h3 className="font-semibold text-slate-800">{item.name}</h3>
                          <p className="text-sm text-slate-500">{item.username}</p>
                        </div>
                      </div>
                      <button className="p-2 hover:bg-slate-50 rounded-lg transition-colors">
                        <Lock className="w-4 h-4 text-slate-400" />
                      </button>
                    </div>
                    <div className="flex gap-2">
                      <button className="flex-1 px-3 py-2 bg-slate-50 hover:bg-slate-100 text-slate-700 rounded-lg text-sm font-medium transition-colors">
                        Copy
                      </button>
                      <button className="flex-1 px-3 py-2 bg-blue-50 hover:bg-blue-100 text-blue-600 rounded-lg text-sm font-medium transition-colors">
                        View
                      </button>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          )}

          {currentPage === 'pn532' && (
            <div className="max-w-4xl">
              <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                {/* Connection Card */}
                <div className="bg-white border border-slate-200 rounded-xl p-6">
                  <h3 className="text-lg font-semibold text-slate-800 mb-4 flex items-center gap-2">
                    <Wifi className="w-5 h-5 text-blue-500" />
                    Connection
                  </h3>
                  <div className="space-y-4">
                    <div>
                      <label className="block text-sm font-medium text-slate-700 mb-2">
                        Serial Port
                      </label>
                      <select
                        value={selectedPort}
                        onChange={(e) => setSelectedPort(e.target.value)}
                        className="w-full px-4 py-2.5 bg-slate-50 border border-slate-200 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500"
                      >
                        <option value="">Select a port</option>
                        {ports.map((port, index) => (
                          <option key={index} value={port.path}>{port.path} {port.manufacturer && `(${port.manufacturer})`}</option>
                        ))}
                      </select>
                    </div>
                    <div className="flex gap-3">
                      <button
                        onClick={handleConnect}
                        disabled={connectionStatus === 'connected'}
                        className="w-full px-4 py-2.5 bg-blue-500 hover:bg-blue-600 disabled:bg-slate-300 text-white rounded-lg font-medium transition-colors"
                      >
                        {connectionStatus === 'connected' ? 'Connected' : 'Connect'}
                      </button>

                      <button
                        onClick={handleDisconnect}
                        disabled={connectionStatus !== 'connected'}
                        className="w-full px-4 py-2.5 bg-red-500 hover:bg-red-600 disabled:bg-slate-300 text-white rounded-lg font-medium transition-colors"
                      >
                        Disconnect
                      </button>
                    </div>
                  </div>
                </div>

                {/* Firmware Card */}
                <div className="bg-white border border-slate-200 rounded-xl p-6">
                  <h3 className="text-lg font-semibold text-slate-800 mb-4 flex items-center gap-2">
                    <Info className="w-5 h-5 text-purple-500" />
                    Firmware
                  </h3>
                  <div className="space-y-4">
                    <button
                      onClick={handleGetFirmware}
                      disabled={connectionStatus !== 'connected'}
                      className="w-full px-4 py-2.5 bg-purple-500 hover:bg-purple-600 disabled:bg-slate-300 text-white rounded-lg font-medium transition-colors flex items-center justify-center gap-2"
                    >
                      <Download className="w-4 h-4" />
                      Get Firmware Info
                    </button>
                    {firmware && (
                      <div className="p-4 bg-slate-50 rounded-lg border border-slate-200">
                        <div className="text-xs text-slate-500 mb-1">Version</div>
                        <div className="font-mono text-sm font-semibold text-slate-800">{firmware}</div>
                      </div>
                    )}
                  </div>
                </div>

                {/* Self Test Card */}
                <div className="lg:col-span-2">
                  <SelfTestPanel isConnected={connectionStatus === 'connected'} />
                </div>

                {/* Get Version Card */}
                <div className="bg-white border border-slate-200 rounded-xl p-6 lg:col-span-2">
                  <h3 className="text-lg font-semibold text-slate-800 mb-4 flex items-center gap-2">
                    <Activity className="w-5 h-5 text-green-500" />
                    Debug Version
                  </h3>
                  <div className="space-y-4">
                    <button
                      onClick={handleGetVersion}
                      disabled={connectionStatus !== 'connected'}
                      className="px-6 py-2.5 bg-green-500 hover:bg-green-600 disabled:bg-slate-300 text-white rounded-lg font-medium transition-colors flex items-center gap-2"
                    >
                      <Zap className="w-4 h-4" />
                      Get Version (Debug)
                    </button>
                  </div>
                </div>
              </div>
            </div>
          )}

          {currentPage === 'settings' && (
            <div className="max-w-2xl">
              <div className="bg-white border border-slate-200 rounded-xl p-6">
                <h3 className="text-lg font-semibold text-slate-800 mb-4">General Settings</h3>
                <div className="space-y-4">
                  <div className="flex items-center justify-between py-3 border-b border-slate-100">
                    <div>
                      <div className="font-medium text-slate-800">Auto-lock</div>
                      <div className="text-sm text-slate-500">Lock after 5 minutes of inactivity</div>
                    </div>
                    <button className="w-12 h-6 bg-blue-500 rounded-full relative">
                      <div className="w-5 h-5 bg-white rounded-full absolute right-0.5 top-0.5"></div>
                    </button>
                  </div>
                  <div className="flex items-center justify-between py-3 border-b border-slate-100">
                    <div>
                      <div className="font-medium text-slate-800">Require NFC card</div>
                      <div className="text-sm text-slate-500">Always require card for authentication</div>
                    </div>
                    <button className="w-12 h-6 bg-blue-500 rounded-full relative">
                      <div className="w-5 h-5 bg-white rounded-full absolute right-0.5 top-0.5"></div>
                    </button>
                  </div>
                </div>
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
};

export default ModernSidebar;
