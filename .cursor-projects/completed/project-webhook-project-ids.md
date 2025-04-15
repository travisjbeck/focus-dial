# Project: Webhook Project IDs

## 1. Goal

Implement a system where projects created or managed on the Focus Dial device have a persistent, unique identifier. This identifier will be generated and stored on the Focus Dial itself. When a timer is started, this unique ID will be included in the webhook payload sent to the Time Tracking Web Application. The web application backend will use this ID to reliably associate time entries with the correct project, even if the project name or color is changed on the device later. This prevents the creation of duplicate projects in the web application when a project is simply renamed on the Focus Dial.

## 2. Current State & Context

- **Focus Dial:**
  - Firmware manages projects (name, color) stored in NVS.
  - A configuration web UI (served by the ESP32) allows users to add/edit/delete projects via API endpoints (`/api/projects`).
  - When a timer starts, the firmware sends a webhook containing the _currently selected_ project's `name` and `color` to the web application backend URL.
  - Project selection happens on the device before starting a timer.
  - There is currently no persistent unique ID for projects stored on the device.
- **Time Tracking Web Application:**
  - React frontend (`time-tracking-app/frontend`) and Node.js/Express/Sequelize/SQLite backend (`time-tracking-app/backend`).
  - Backend receives webhooks at an endpoint (likely `/webhook`).
  - The webhook handler currently looks up projects based on the `project_name` received in the payload. If a project with that name exists, it uses it. If not, it creates a _new_ project based on the name and color from the payload.
  - This leads to duplicate projects if a user renames a project on the Focus Dial (e.g., "Task A" -> "Client B Task A") because the backend sees the new name as a new project.
- **Relevant Files (Assumed/Confirmed paths):**
  - Firmware: `firmware/src/main.cpp`, `firmware/src/controllers/NetworkController.cpp` (webhook sending), handlers for `/api/projects` (project storage), `platformio.ini`.
  - Backend: `time-tracking-app/backend/src/index.js`, `time-tracking-app/backend/src/routes/webhook.js` (or similar), `time-tracking-app/backend/src/controllers/webhookController.js` (or similar), `time-tracking-app/backend/src/models/Project.js`, `time-tracking-app/backend/src/models/TimeEntry.js`.

> **Note:** This is a personal project with no existing production data. There is no need for legacy fallback support or backwards compatibility. All implementations should require the new `device_project_id` field.

## 3. Plan

### Phase 1: Focus Dial Firmware & Config UI Modifications

**Objective:** Generate, store, and transmit a unique ID for each project from the device.

- [x] **Task 1.1: Design Unique ID Strategy**

  - [x] Determine the best method for generating unique IDs on the ESP32 (Chip ID + Counter: `"{ChipID}-{Counter}"`).

- [x] **Task 1.2: Update Project Storage (NVS)**

  - [x] Modify NVS data structure to include `device_project_id` field.
  - [x] Update `/api/projects` POST handler to generate & store `device_project_id`.
  - [x] Update `/api/projects` PUT handler to preserve/assign `device_project_id`.
  - [x] Update `/api/projects` DELETE handler for new structure. (Added proper ID-based deletion via `/api/deleteProjectById`)

- [x] **Task 1.3: Update Project Retrieval**

  - [x] Modify `/api/projects` GET handler to return `device_project_id`.

- [x] **Task 1.4: Update Webhook Payload**

  - [x] Modify webhook sending code to fetch `device_project_id`.
  - [x] Update JSON payload to include `device_project_id`.

- [x] **Task 3.2: Webhook Payload Testing**
  - [x] Start/Stop timer on device.
  - [x] Monitor webhook payload, verify `device_project_id` presence.

### Phase 2: Web Application Backend Modifications

**Objective:** Update the backend to use device_project_id as the primary identifier for projects.

- [x] **Task 2.1: Update Database Schema**

  - [x] Modify `Project` Sequelize model to add `device_project_id` column (STRING, unique, allowNull).
  - [x] Apply schema change to the database (migration or sync).

- [x] **Task 2.2: Update Webhook Controller**

  - [x] Modify controller to parse `device_project_id` from payload.
  - [x] Change project lookup logic from `project_name` to `device_project_id`.

- [x] **Task 2.3: Implement Project Upsert Logic**
  - [x] Update `handleTimerStart` function to require `device_project_id`.
  - [x] Implement logic to update project name/color if a project with matching `device_project_id` exists.
  - [x] Create a new project with the given `device_project_id` if no matching project is found.

### Phase 3: Testing and Verification

**Objective:** Ensure the entire system works end-to-end.

- [ ] **Task 3.1: End-to-End Testing**
  - [ ] Create multiple projects on the Focus Dial.
  - [ ] Start/stop timers for different projects.
  - [ ] Rename a project on the Focus Dial and verify it doesn't create a duplicate in the web app.
  - [ ] Check database integrity and verify all associations are correct.
