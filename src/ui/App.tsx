import { useEffect, useState } from 'react'
import reactLogo from './assets/react.svg'
import './App.css'

function App() {
  const [count, setCount] = useState(0)
  const [comPorts, setComPorts] = useState<{ path: string; manufacturer?: string }[]>([]);

  useEffect(() => {
    const fetchPorts = async () => {
      const ports = await window.electron.listComPorts();
      setComPorts(ports);
    };
    
    fetchPorts();
  }, []);

  return (
    <>
      <div>
        <a href="https://react.dev" target="_blank">
          <img src={reactLogo} className="logo react" alt="React logo" />
        </a>
      </div>
      <h1>Vite + React</h1>
      <div className="card">
        <button onClick={() => setCount((count) => count + 1)}>
          count is {count}
        </button>
        
        <select>
          {comPorts.map((port, index) => (
            <option key={index} value={port.path}>
              {port.path} {port.manufacturer && `(${port.manufacturer})`}
            </option>
          ))}
        </select>

        <button onClick={async () => {
          const version = await window.electron.getFirmwareVersion();
          console.log('Firmware Version:', version);
        }}>
          Get Firmware Version
        </button>
        <p>
          Edit <code>src/App.tsx</code> and save to test HMR
        </p>
      </div>
      <p className="read-the-docs">
        Click on the Vite and React logos to learn more
      </p>
    </>
  )
}

export default App
