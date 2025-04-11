# Focus Dial - Project Tracking Feature Development Plan

## Project Goal

Enhance the Focus Dial firmware to allow users to associate timers with specific projects, track time spent, visually represent the active project with a color on the LED ring, and report the project name via webhook upon timer completion. Provide an accessible web interface for managing projects.

## Key Features

- **Project Management:** Define, edit, and delete projects (name, color) via a persistent web interface accessible on the local network.
- **Persistent Storage:** Store project list and last selected project in NVS.
- **Project Selection:** Select a project (or "No Project") on the device _after_ setting the timer duration but _before_ starting the timer.
- **Visual Feedback:** Display project color on LED ring during selection and timer. Display limited, scrollable project list on OLED during selection.
- **Webhook Integration:** Append selected project name to webhook URL on timer completion.
- **Last Used Default:** Default project selection to the last used project ("No Project" if none used before or first run).

## Development Tasks

_(Prioritized based on discussion)_

### Phase 1: Persistent Web Server & Project API Backend

- [ ] **Integrate Dependencies:** Add `ESPAsyncWebServer` and `AsyncTCP` libraries to `platformio.ini`. Include necessary SPIFFS/LittleFS library for web assets if chosen.
- [ ] **Define Data Structure:** Finalize the format for storing projects (e.g., `std::vector<Project>` struct) and how it maps to JSON for API/NVS (e.g., `[{"name":"P1", "color":"#aabbcc"}, ...]`). Define a practical maximum project limit (e.g., 20).
- [ ] **NVS Storage Logic:** Implement functions (likely in `NetworkController` or a new `ProjectManager` class) to:
  - [ ] Save the project list (as JSON string) to NVS.
  - [ ] Load the project list (as JSON string) from NVS.
  - [ ] Parse JSON string into the in-memory data structure.
  - [ ] Serialize the in-memory structure back to a JSON string.
  - [ ] Save/Load the index of the last used project to/from NVS.
- [ ] **Basic Web Server Setup:**
  - [ ] Initialize and configure `ESPAsyncWebServer`.
  - [ ] Start the server only when Wi-Fi is connected.
  - [ ] Stop the server when Wi-Fi disconnects.
  - [ ] Implement mDNS responder to allow access via `http://focus-dial.local` (or similar).
- [ ] **Project API Endpoints (using `ESPAsyncWebServer`):** Create handlers for:
  - [ ] `GET /api/projects`: Reads projects from NVS, serializes to JSON, returns the list.
  - [ ] `POST /api/projects`: Parses project JSON from request body, adds it to the list, saves updated list to NVS, returns success/failure/new list.
  - [ ] `PUT /api/projects/{index}`: Parses project JSON from request body, updates the project at the specified index, saves updated list to NVS, returns success/failure.
  - [ ] `DELETE /api/projects/{index}`: Removes the project at the specified index, saves updated list to NVS, returns success/failure.
  - [ ] Consider adding an endpoint like `GET /api/status` to check Wi-Fi status, etc. (optional).

### Phase 2: Project Management Web UI (Frontend)

- [ ] **Choose Asset Storage:** Decide whether to embed HTML/CSS/JS in C++ strings or use SPIFFS/LittleFS filesystem. Set up the chosen method.
- [ ] **Create HTML:** Develop the `index.html` file for project management (served by the web server at `/`).
  - [ ] Structure for displaying the current project list (e.g., a table).
  - [ ] Form elements for adding a new project (text input for name, `<input type="color">` for color).
  - [ ] Buttons/controls for "Add", "Edit" (inline or modal), "Delete", "Save".
- [ ] **Create CSS:** Style the HTML page for good usability and appearance.
- [ ] **Create JavaScript:** Write client-side JavaScript (e.g., `app.js`) to:
  - [ ] On page load, fetch the current project list from `GET /api/projects` and render it.
  - [ ] Handle the "Add Project" form submission: send data to `POST /api/projects`, update the UI on success.
  - [ ] Implement "Edit" functionality: allow modifying name/color, send data to `PUT /api/projects/{index}`, update UI.
  - [ ] Implement "Delete" functionality: send request to `DELETE /api/projects/{index}`, update UI.
  - [ ] Provide user feedback (loading states, success/error messages).
- [ ] **Serve UI Files:** Configure `ESPAsyncWebServer` to serve the HTML, CSS, and JS files.

### Phase 3: Firmware - State Machine Integration & Project Selection UI

- [ ] **Project Data Access:** Ensure the loaded project list (from NVS logic in Phase 1) is accessible globally or passed to relevant states.
- [ ] **Color Conversion:** Implement a utility function (e.g., in `LEDController` or shared `Utils`) to convert Hex color strings (e.g., `#FF0000`) into the format required by the NeoPixel library (e.g., 32-bit integer `0x00FF0000` or R, G, B components).
- [ ] **Define `ProjectSelectState`:**
  - [ ] Create `ProjectSelectState.h` and `ProjectSelectState.cpp` in `firmware/src/states/`.
  - [ ] Implement `enter()`, `update()`, `exit()` methods.
- [ ] **Integrate `ProjectSelectState` into `StateMachine`:**
  - [ ] Modify `StateMachine` to add the new state instance.
  - [ ] **Crucially:** Adjust transitions so that the state entered _after_ the first button press (which sets duration) transitions to `ProjectSelectState` upon the _second_ button press. The `ProjectSelectState` will then transition to `TimerRunningState` upon project selection confirmation. Ensure existing state transitions are preserved where not directly involved.
- [ ] **Implement `ProjectSelectState` UI Logic:**
  - `enter()`: Load projects (including prepending "No Project"). Determine the default selection (last used or "No Project"). Display the initial view (e.g., 3 projects) on OLED, highlighting the selection. Set LED ring to the highlighted project's color (or a default for "No Project").
  - `update()`:
    - Read rotary encoder: Update the highlighted selection index, handling list wrapping and scrolling the visible portion on the OLED.
    - Update LED ring color based on the newly highlighted project.
    - Read button press: Confirm selection. Store the selected project index (or a special value for "No Project"). Save the selection as the "last used" project index in NVS. Transition to `TimerRunningState`.
  - `exit()`: Clean up if needed.
- [ ] **Display Controller Interaction:** Update `DisplayController` if necessary to support drawing the limited, scrollable project list format.

### Phase 4: Firmware - Timer Execution & Webhook Update

- [ ] **LED Color During Timer:**
  - [ ] Modify `TimerRunningState` (or relevant timer state).
  - [ ] Before starting the animation, retrieve the color associated with the globally stored selected project index (using the conversion utility). Use a default color if "No Project" was selected.
  - [ ] Pass this color to `LEDController` for the timer animation.
- [ ] **Webhook Modification:**
  - [ ] Modify the `NetworkController::sendWebhook` (or equivalent).
  - [ ] Retrieve the _name_ of the selected project based on the stored index.
  - [ ] If a project (and not "No Project") was selected:
    - URL-encode the project name.
    - Append the encoded name as a query parameter (e.g., `?project=Project%20A`) to the `webhookURL`. Handle URLs that may already have query parameters correctly.
  - [ ] Send the (potentially modified) webhook request.

### Phase 5: Testing & Refinement

- [ ] **API Testing:** Use tools like `curl` or Postman to test all `/api/projects` endpoints thoroughly.
- [ ] **Web UI Testing:** Test CRUD operations via the web interface in different browsers. Test responsiveness and error handling.
- [ ] **Firmware Testing:**
  - [ ] Test the exact state machine flow.
  - [ ] Test project selection UI (scrolling, highlighting, confirmation, OLED display, LED feedback).
  - [ ] Test "No Project" selection.
  - [ ] Test last-used project default.
  - [ ] Verify correct LED colors during timer.
  - [ ] Verify webhook calls include the correct query parameter (or none for "No Project").
  - [ ] Test mDNS resolution.
- [ ] **Edge Case Testing:** Max projects, empty project list, NVS errors, Wi-Fi disconnections during web server use, special characters in project names.
- [ ] **Code Cleanup & Documentation:** Refactor, add comments, update `README.md` if needed.
