# Self-Test Implementation Guide

## Overview
This document describes the implementation of the real-time self-test feature for the PN532 NFC reader module. The feature allows users to run comprehensive hardware diagnostics with live progress updates.

## Features Implemented

### 1. **C++ Async Worker** (`addons/Pn532Wrapper.cc`)
- Created `SelfTestWorker` class that extends `Napi::AsyncProgressWorker`
- Runs 5 different self-tests sequentially:
  - **ROM Checksum Test**: Validates internal ROM integrity
  - **RAM Integrity Test**: Tests random access memory
  - **Communication Line Test**: Verifies data transmission with test pattern `0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA`
  - **Echo Back Test**: Tests command echo functionality with pattern `0xBA, 0xAD, 0xF0, 0x0D, 0x12, 0x34, 0x56, 0x78`
  - **Antenna Continuity Test**: Checks antenna connection with threshold settings

- Each test sends two progress updates:
  - Status: "running" when test starts
  - Status: "success" or "failed" when test completes (based on status == 0)

### 2. **React Component** (`src/ui/Components/SelfTestPanel.tsx`)
- Modern, responsive UI component with:
  - Individual test cards showing status
  - Animated spinner for running tests
  - Success/failure icons with color coding
  - Summary section showing overall results
  - Disabled state when device is not connected

### 3. **IPC Communication**
- **Main Process** (`src/electron/main.ts`):
  - Listens for `run-self-tests` event
  - Creates PN532_Wrapper instance and calls `runSelfTests()`
  - Forwards progress updates via `self-test-progress` event
  - Sends completion notification via `self-test-complete` event

- **Preload Script** (`src/electron/preload.cts`):
  - Exposes `runSelfTests()` function to renderer
  - Manages IPC event listeners
  - Cleans up listeners on completion

### 4. **Type Definitions**
- Added `SelfTestUpdate` type for progress updates
- Extended `ExposedElectronAPI` to include `runSelfTests` method
- Added `RendererEvents` type for IPC events

## File Structure

```
addons/
  ├── Pn532Wrapper.h          # Added RunSelfTests method declaration
  └── Pn532Wrapper.cc         # Implemented SelfTestWorker class

src/
  ├── electron/
  │   ├── main.ts             # Added IPC handler for run-self-tests
  │   ├── preload.cts         # Exposed runSelfTests to renderer
  │   └── bindings.ts         # Added runSelfTests to interface
  ├── ui/Components/
  │   ├── SelfTestPanel.tsx   # New component for self-test UI
  │   └── MainPage.tsx        # Integrated SelfTestPanel
  └── types/
      └── deviceaddon.d.ts    # (existing type definitions)

types.d.ts                    # Added SelfTestUpdate and updated types
```

## Usage

### In the React Component:
```typescript
import { SelfTestPanel } from './SelfTestPanel';

<SelfTestPanel isConnected={connectionStatus === 'connected'} />
```

### The component automatically:
1. Shows "Run All Tests" button (disabled when not connected)
2. Displays all 5 tests in pending state
3. Updates each test's status in real-time as they execute
4. Shows overall summary when complete

## Test Status Flow

```
pending → running → success/failed
```

Each test independently progresses through these states, allowing users to see:
- Which test is currently running (animated spinner)
- Which tests have completed successfully (green checkmark)
- Which tests have failed (red X icon)
- Which tests are still waiting (gray circle)

## Color Coding

- **Pending**: Gray background, gray circle icon
- **Running**: Blue background, animated spinner
- **Success**: Green background, green checkmark
- **Failed**: Red background, red X icon

## Error Handling

- Connection validation: Tests can only run when device is connected
- Progress callback errors are logged to console
- Completion callback handles both success and error states
- IPC event listeners are properly cleaned up after completion

## Build Instructions

1. Rebuild the native addon:
   ```bash
   npm run build:addon:rebuild
   ```

2. Build the TypeScript:
   ```bash
   npm run build
   ```

3. Start the application:
   ```bash
   npm run dev
   ```

## Testing

To test the self-test feature:
1. Connect to a PN532 device via the Connection panel
2. Navigate to "PN532 Reader" in the sidebar
3. Click "Run All Tests" in the Self-Test Diagnostics section
4. Watch the real-time progress as each test executes
5. Review the summary when all tests complete

## Future Enhancements

Potential improvements:
- Add test execution time display
- Implement retry mechanism for failed tests
- Add detailed error messages from C++ layer
- Save test history/logs
- Export test results to file
