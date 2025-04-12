# Focus Dial - Project Tracking Feature Development Plan

## Project Goal

Enhance the Focus Dial firmware to allow users to associate timers with specific projects, track time spent, visually represent the active project with a color on the LED ring, and report the project name via webhook upon timer completion. Provide an accessible web interface for managing projects.

## Key Features

- [x] **Project Management:** Define, edit, and delete projects (name, color) via a persistent web interface accessible on the local network.
- [x] **Persistent Storage:** Store project list and last selected project in NVS.
- [x] **Project Selection:** Select a project (or "No Project") on the device _after_ setting the timer duration but _before_ starting the timer.
- [x] **Visual Feedback:** Display project color on LED ring during selection and timer. Display limited, scrollable project list on OLED during selection.
- [x] **Webhook Integration:** Append selected project name to webhook URL on timer completion.
- [x] **Last Used Default:** Default project selection to the last used project ("No Project" if none used before or first run).

## Development Tasks

_(Prioritized based on discussion)_

### Phase 1: Persistent Web Server & Project API Backend

- [x] **Integrate Dependencies:** Add `ESPAsyncWebServer` and `AsyncTCP` libraries to `platformio.ini`. Include necessary SPIFFS/LittleFS library for web assets if chosen.
- [x] **Define Data Structure:** Finalize the format for storing projects (e.g., `std::vector<Project>` struct) and how it maps to JSON for API/NVS (e.g., `[{"name":"P1", "color":"#aabbcc"}, ...]`). Define a practical maximum project limit (e.g., 20).
- [x] **NVS Storage Logic:** Implement functions (likely in `NetworkController` or a new `ProjectManager` class) to:
  - [x] Save the project list (as JSON string) to NVS.
  - [x] Load the project list (as JSON string) from NVS.
  - [x] Parse JSON string into the in-memory data structure.
  - [x] Serialize the in-memory structure back to a JSON string.
  - [x] Save/Load the index of the last used project to/from NVS.
- [x] **Basic Web Server Setup:**
  - [x] Initialize and configure `ESPAsyncWebServer`.
  - [x] Start the server only when Wi-Fi is connected.
  - [x] Stop the server when Wi-Fi disconnects.
  - [x] Implement mDNS responder to allow access via `http://focus-dial.local` (or similar).
- [x] **Project API Endpoints (using `ESPAsyncWebServer`):** Create handlers for:
  - [x] `GET /api/projects`: Reads projects from NVS, serializes to JSON, returns the list.
  - [x] `POST /api/projects`: Parses project JSON from request body, adds it to the list, saves updated list to NVS, returns success/failure/new list.
  - [x] `PUT /api/projects/{index}`: Parses project JSON from request body, updates the project at the specified index, saves updated list to NVS, returns success/failure.
  - [x] `DELETE /api/projects/{index}`: Removes the project at the specified index, saves updated list to NVS, returns success/failure.
  - [x] Consider adding an endpoint like `GET /api/status` to check Wi-Fi status, etc. (optional).

### Phase 2: Project Management Web UI (Frontend)

- [x] **Choose Asset Storage:** Decide whether to embed HTML/CSS/JS in C++ strings or use SPIFFS/LittleFS filesystem. Set up the chosen method. *(Selected LittleFS)*
- [x] **Create HTML:** Develop the `index.html` file for project management (served by the web server at `/`).
  - [x] Structure for displaying the current project list (e.g., a table).
  - [x] Form elements for adding a new project (text input for name, `<input type="color">` for color).
  - [x] Buttons/controls for "Add", "Edit" (inline or modal), "Delete", "Save".
- [x] **Create CSS:** Style the HTML page for good usability and appearance.
- [x] **Create JavaScript:** Write client-side JavaScript (e.g., `app.js`) to:
  - [x] On page load, fetch the current project list from `GET /api/projects` and render it.
  - [x] Handle the "Add Project" form submission: send data to `POST /api/projects`, update the UI on success.
  - [x] Implement "Edit" functionality: allow modifying name/color, send data to `PUT /api/projects/{index}`, update UI.
  - [x] Implement "Delete" functionality: send request to `DELETE /api/projects/{index}`, update UI. *(Implemented via POST /api/deleteProject)*
  - [x] Provide user feedback (loading states, success/error messages).
- [x] **Serve UI Files:** Configure `ESPAsyncWebServer` to serve the HTML, CSS, and JS files.

### Phase 3: Firmware - State Machine Integration & Project Selection UI

- [x] **Project Data Access:** Ensure the loaded project list (from NVS logic in Phase 1) is accessible globally or passed to relevant states.
- [x] **Color Conversion:** Implement a utility function (e.g., in `LEDController` or shared `Utils`) to convert Hex color strings (e.g., `#FF0000`) into the format required by the NeoPixel library (e.g., 32-bit integer `0x00FF0000` or R, G, B components).
- [x] **Define `ProjectSelectState`:**
  - [x] Create `ProjectSelectState.h` and `ProjectSelectState.cpp` in `firmware/src/states/`.
  - [x] Implement `enter()`, `update()`, `exit()` methods.
- [x] **Integrate `ProjectSelectState` into `StateMachine`:**
  - [x] Modify `StateMachine` to add the new state instance.
  - [x] **Crucially:** Adjust transitions so that the state entered _after_ the first button press (which sets duration) transitions to `ProjectSelectState` upon the _second_ button press. The `ProjectSelectState` will then transition to `TimerRunningState` upon project selection confirmation. Ensure existing state transitions are preserved where not directly involved.
- [x] **Implement `ProjectSelectState` UI Logic:**
  - [x] `enter()`: Load projects (including prepending "No Project"). Determine the default selection (last used or "No Project"). Display the initial view (e.g., 1 project) on OLED, highlighting the selection. Set LED ring to the highlighted project's color (or a default for "No Project").
  - [x] `update()`:
    - [x] Read rotary encoder: Update the highlighted selection index, handling list wrapping.
    - [x] Update LED ring color based on the newly highlighted project.
    - [x] Read button press: Confirm selection. Store the selected project index (or a special value for "No Project"). Save the selection as the "last used" project index in NVS. Transition to `TimerRunningState`.
    - [x] Add timeout to return to Idle.
    - [x] Add double-click to return to Idle.
  - [x] `exit()`: Clean up if needed.
- [x] **Display Controller Interaction:** Update `DisplayController` to support drawing the single, centered project name format.

### Phase 4: Firmware - Timer Execution & Webhook Update

- [x] **LED Color During Timer:**
  - [x] Modify `TimerRunningState` (or relevant timer state).
  - [x] Before starting the animation, retrieve the color associated with the globally stored selected project index (using the conversion utility). Use a default color if "No Project" was selected.
  - [x] Pass this color to `LEDController` for the timer animation.
  - [x] Fix color persistence on Pause/Resume.
- [x] **Webhook Modification:**
  - [x] Modify the `sendWebhookAction` function (or where webhooks are triggered) to append `|ProjectName` to the action string if a project was selected.
  - [x] Ensure this works correctly for `start`, `stop` (pause/cancel), and `done` actions.

**Project Complete.**