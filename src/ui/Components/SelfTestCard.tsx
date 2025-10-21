import React, { useCallback, useEffect, useMemo, useState } from 'react';
import {
  Activity,
  CheckCircle2,
  Circle,
  Loader2,
  XCircle,
  Zap,
} from 'lucide-react';

type TestStatus = 'idle' | 'pending' | 'ok' | 'error';

type TestItem = {
  label: string;
  status: TestStatus;
  message?: string;
};

const TEST_LABELS = [
  'ROM self-test',
  'RAM self-test',
  'Communication line',
  'Echo back',
  'Antenna continuity',
];

const STATUS_ICON: Record<TestStatus, React.ReactNode> = {
  idle: <Circle className="w-4 h-4 text-slate-300" />,
  pending: <Loader2 className="w-4 h-4 text-indigo-500 animate-spin" />,
  ok: <CheckCircle2 className="w-4 h-4 text-green-500" />,
  error: <XCircle className="w-4 h-4 text-red-500" />,
};

const deriveInitialState = (): TestItem[] =>
  TEST_LABELS.map((label) => ({ label, status: 'idle' }));

const statusFromCode = (code: number): TestStatus =>
  code === 0 ? 'ok' : 'error';

export interface SelfTestCardProps {
  disabled?: boolean;
}

const SelfTestCard: React.FC<SelfTestCardProps> = ({ disabled }) => {
  const [tests, setTests] = useState<TestItem[]>(() => deriveInitialState());
  const [running, setRunning] = useState(false);

  useEffect(() => {
    const unsubscribe = window.electron.onSelfTestProgress((update) => {
      setTests((prev) =>
        prev.map((test, idx) =>
          idx === update.index
            ? {
                ...test,
                status: statusFromCode(update.status),
                message: update.message,
              }
            : test,
        ),
      );
    });

    return () => {
      unsubscribe();
    };
  }, []);

  const runSelfTests = useCallback(async () => {
    if (disabled || running) {
      return;
    }

    setRunning(true);
    setTests(TEST_LABELS.map((label) => ({ label, status: 'pending' })));

    try {
      const summary = await window.electron.runSelfTests();

      setTests((prev) =>
        prev.map((test, idx) => {
          const result = summary.find((item) => item.index === idx);
          if (!result) {
            return test;
          }
          return {
            ...test,
            status: statusFromCode(result.status),
            message: result.message,
          };
        }),
      );
    } catch (error) {
      console.error('runSelfTests failed', error);
      setTests((prev) =>
        prev.map((test) =>
          test.status === 'pending'
            ? { ...test, status: 'error', message: 'Self-test aborted' }
            : test,
        ),
      );
    } finally {
      setRunning(false);
    }
  }, [disabled, running]);

  const buttonLabel = useMemo(() => {
    if (running) return 'Running self-tests...';
    if (disabled) return 'Connect device to run self-test';
    return 'Run Self Test';
  }, [running, disabled]);

  return (
    <div className="bg-white border border-slate-200 rounded-xl p-6 lg:col-span-2">
      <h3 className="text-lg font-semibold text-slate-800 mb-4 flex items-center gap-2">
        <Activity className="w-5 h-5 text-green-500" />
        Self Test
      </h3>
      <div className="space-y-4">
        <button
          onClick={runSelfTests}
          disabled={disabled || running}
          className="px-6 py-2.5 bg-green-500 hover:bg-green-600 disabled:bg-slate-300 text-white rounded-lg font-medium transition-colors flex items-center gap-2"
        >
          <Zap className="w-4 h-4" />
          {buttonLabel}
        </button>

        <div className="space-y-3">
          {tests.map((test, idx) => (
            <div
              key={idx}
              className="flex items-start gap-3 rounded-lg border border-slate-200 p-3"
            >
              <div className="mt-0.5">{STATUS_ICON[test.status]}</div>
              <div className="flex-1">
                <div className="text-sm font-semibold text-slate-800">
                  {test.label}
                </div>
                <div className="text-xs text-slate-500">
                  {test.message ??
                    (test.status === 'pending'
                      ? 'Running...'
                      : test.status === 'idle'
                      ? 'Awaiting test run'
                      : test.status === 'ok'
                      ? 'Completed successfully'
                      : 'Failed')}
                </div>
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};

export default SelfTestCard;
