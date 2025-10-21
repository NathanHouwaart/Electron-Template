import React, { useState, useEffect } from 'react';
import { Loader2, CheckCircle2, XCircle, Circle, Activity } from 'lucide-react';

type TestStatus = 'pending' | 'running' | 'success' | 'failed';

interface SelfTestUpdate {
  test: string;
  status: TestStatus;
}

interface TestInfo {
  key: string;
  label: string;
  description: string;
}

const testInfo: TestInfo[] = [
  { key: 'rom', label: 'ROM Checksum', description: 'Validates internal ROM integrity' },
  { key: 'ram', label: 'RAM Integrity', description: 'Tests random access memory' },
  { key: 'communication', label: 'Communication Line', description: 'Verifies data transmission' },
  { key: 'echo', label: 'Echo Back', description: 'Tests command echo functionality' },
  { key: 'antenna', label: 'Antenna Continuity', description: 'Checks antenna connection' },
];

// icons are rendered inline via lucide-react components to keep sizes consistent

interface SelfTestPanelProps {
  isConnected: boolean;
}

export const SelfTestPanel: React.FC<SelfTestPanelProps> = ({ isConnected }) => {
  const [tests, setTests] = useState<Record<string, TestStatus>>({
    rom: 'pending',
    ram: 'pending',
    communication: 'pending',
    echo: 'pending',
    antenna: 'pending',
  });
  const [isRunning, setIsRunning] = useState(false);
  const [showDetails, setShowDetails] = useState(false);

  // Reset tests when device is disconnected
  useEffect(() => {
    const unsub = window.electron.onDeviceDisconnected(() => {
      setIsRunning(false);
      setShowDetails(false);
      setTests({ rom: 'pending', ram: 'pending', communication: 'pending', echo: 'pending', antenna: 'pending' });
    });
    return () => unsub();
  }, []);

  const runSelfTests = () => {
    setIsRunning(true);
    setShowDetails(true);
    setTests({
      rom: 'pending',
      ram: 'pending',
      communication: 'pending',
      echo: 'pending',
      antenna: 'pending',
    });

    window.electron.runSelfTests(
      (result: SelfTestUpdate) => {
        console.log('Self-test progress:', result);
        setTests((prev) => {
          const updated = {
            ...prev,
            [result.test]: result.status as TestStatus,
          };
          console.log('Updated tests state:', updated);
          return updated;
        });
      },
      (error: Error | null, _complete: boolean) => {
        console.log('Self-test complete. Error:', error);
        setIsRunning(false);
        if (error) {
          console.error('Self-test error:', error);
        }
      }
    );
  };

  const getIcon = (status: TestStatus) => {
    switch (status) {
      case 'running':
        return <Loader2 className="w-4 h-4 text-blue-500 animate-spin" />;
      case 'success':
        return <CheckCircle2 className="w-4 h-4 text-green-500" />;
      case 'failed':
        return <XCircle className="w-4 h-4 text-red-500" />;
      default:
        return <Circle className="w-4 h-4 text-gray-400" />;
    }
  };

  const getStatusColor = (status: TestStatus) => {
    switch (status) {
      case 'running':
        return 'bg-blue-50 border-blue-200';
      case 'success':
        return 'bg-green-50 border-green-200';
      case 'failed':
        return 'bg-red-50 border-red-200';
      default:
        return 'bg-slate-50 border-slate-200';
    }
  };

  const allTestsPassed = Object.values(tests).every(status => status === 'success');
  const anyTestFailed = Object.values(tests).some(status => status === 'failed');
  const testsCompleted = !isRunning && Object.values(tests).every(status => status !== 'pending');

  return (
    <div className="bg-white border border-slate-200 rounded-xl p-6">
      <div className="flex items-center justify-between mb-6">
        <div>
          <h3 className="text-lg font-semibold text-slate-800 flex items-center gap-2">
            <Activity className="w-5 h-5 text-green-500" />
            Self-Test Diagnostics
          </h3>
          <p className="text-sm text-slate-500 mt-1">
            Comprehensive hardware and communication tests
          </p>
        </div>
        <button
          onClick={runSelfTests}
          disabled={!isConnected || isRunning}
          className="px-6 py-2.5 bg-green-500 hover:bg-green-600 disabled:bg-slate-300 disabled:cursor-not-allowed text-white rounded-lg font-medium transition-colors flex items-center gap-2"
        >
          {isRunning ? (
            <>
              <Loader2 className="w-4 h-4 animate-spin" />
              Running Tests...
            </>
          ) : (
            <>
              <Activity className="w-4 h-4" />
              Run All Tests
            </>
          )}
        </button>
      </div>

      {/* Test Results */}
      {showDetails ? (
  <div className="grid grid-cols-1 sm:grid-cols-2 gap-3 mb-6">
          {testInfo.map((test) => {
            const status = tests[test.key];
            return (
              <div
                key={test.key}
                className={`flex items-center justify-between px-3 py-2 border rounded-md transition-all ${getStatusColor(status)}`}
              >
                <div className="flex items-center gap-3 min-w-0">
                  {getIcon(status)}
                  <div className="flex-1 min-w-0">
                    <div className="font-medium text-sm text-slate-800 truncate">{test.label}</div>
                    {showDetails && (
                      <div className="text-xs text-slate-600 mt-0.5 truncate">{test.description}</div>
                    )}
                  </div>
                </div>
                <div className="flex items-center gap-2">
                  <span className="flex-shrink-0 text-sm font-medium text-slate-700 capitalize min-w-[72px] truncate text-right">
                    {status}
                  </span>
                </div>
              </div>
            );
          })}
        </div>
      ) : (
        <div className="p-4 mb-6 border rounded-md bg-slate-50 text-center text-sm text-slate-600">
          Press "Run All Tests" to display the individual self-test tiles and begin diagnostics.
        </div>
      )}

      {/* Summary */}
      {testsCompleted && (
        <div
          className={`p-4 rounded-lg border flex items-center gap-3 ${
            allTestsPassed
              ? 'bg-green-50 border-green-200'
              : anyTestFailed
              ? 'bg-red-50 border-red-200'
              : 'bg-yellow-50 border-yellow-200'
          }`}
        >
          {allTestsPassed ? (
            <>
              <CheckCircle2 className="w-5 h-5 text-green-600 flex-shrink-0" />
              <div>
                <div className="font-semibold text-sm text-slate-800">All Tests Passed</div>
                <div className="text-xs text-slate-600">Device is functioning correctly</div>
              </div>
            </>
          ) : (
            <>
              <XCircle className="w-5 h-5 text-red-600 flex-shrink-0" />
              <div>
                <div className="font-semibold text-sm text-slate-800">Some Tests Failed</div>
                <div className="text-xs text-slate-600">
                  Please check the device connection and try again
                </div>
              </div>
            </>
          )}
        </div>
      )}
    </div>
  );
};
