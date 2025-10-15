import { useEffect, useState } from 'react'
import reactLogo from './assets/react.svg'
import './App.css'
import ModernSidebar from './Components/MainPage';
import { Link, Route, Routes } from 'react-router-dom';

function App() {
  return (
    <>
      {/* <header className="p-4 border-b">
        <nav className="flex gap-4">
          <Link to="/">Home</Link>
          <Link to="/about">About</Link>
          <Link to="/settings">Settings</Link>
        </nav>
      </header> */}

      <div className="flex">
        <main className="p-4 flex-1">
          <Routes>
            <Route path="/" element={<ModernSidebar />} />
            {/* <Route path="/about" element={<About />} />
            <Route path="/settings" element={<Settings />} />
            <Route path="*" element={<NotFound />} /> */}
          </Routes>
        </main>
      </div>
    </>
  )
}

export default App
