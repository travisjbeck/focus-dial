# Project: Firmware Webhook Payload Update

- **Created**: 2024-05-21
- **Status**: Completed
- **Last Updated**: 2024-05-21

## Context & Requirements

Following the backend refactor (Project `2025-04-17-backend-webhook-device-project-refactor.md`), the Focus Dial firmware (`firmware/src/`) needs to be updated to send the correct payload format to the `/api/webhook` endpoint.

The backend now expects a JSON payload containing:

- `action`: "start_timer" or "stop_timer" (string)
- `device_project_id`: The unique identifier generated and stored on the device for the selected project (string)
- `project_name`: The name of the selected project (string)
- `project_color`: The color associated with the selected project (string, likely hex format e.g., "#FFFFFF")
- `description`: Optional timer description (string)

**Requirements:**

1.  Modify the firmware code responsible for sending webhook requests (likely within `NetworkController.cpp` or similar).
2.  Retrieve the `device_project_id`, `name`, and `color` associated with the currently selected project on the device when a timer is started or stopped.
3.  Construct the JSON payload according to the new format.
4.  Send the payload to the configured webhook URL via an HTTP POST request.
5.  Ensure the `Authorization: Bearer <API_KEY>` header is still included correctly, using the API key stored in the device's configuration.
6.  Test the firmware changes by starting/stopping timers for different projects and verifying the correct data is received by the (local) backend webhook endpoint logs.

**Relevant Files:**

- `firmware/src/controllers/NetworkController.cpp` (Likely location of webhook sending logic)
- `firmware/src/controllers/ProjectController.cpp` (Likely location for retrieving current project details)
- `firmware/lib/` (Check relevant libraries, e.g., for HTTP client, JSON serialization)
- `platformio.ini` (Check dependencies if new libraries are needed)

## Development Plan

### Phase 1: Code Investigation & Identification (Completed)

- [x] Locate the exact function(s) in the firmware codebase responsible for:
  - [x] Retrieving the currently selected project's details (`device_project_id`, `name`, `color`). (Via `ProjectManager`)
  - [x] Retrieving the stored API key. (Logic added, retrieval location identified: NVS)
  - [x] Formatting the webhook JSON payload. (`NetworkController::sendWebhookNotification`)
  - [x] Sending the HTTP POST request for the webhook. (`NetworkController::sendWebhookRequest`)
- [x] Identify the libraries used for JSON creation (e.g., ArduinoJson) and HTTP requests (e.g., `HTTPClient`).

### Phase 2: Implement API Key Input & Storage (New)

- [ ] **Modify Web UI (HTML/CSS/JS in `firmware/data/`):**
  - [ ] Add an input field for the API Key in the appropriate settings section.
  - [ ] Add a 'Save API Key' button.
  - [ ] Implement JavaScript (`fetch`) to POST the entered key to `/api/apikey`.
- [ ] **Modify Firmware (`NetworkController.cpp` & `.h`):**
  - [ ] Define and register a new route handler `handleUpdateApiKey` for `POST /api/apikey` in `_setupWebServerRoutes`.
  - [ ] Implement `handleUpdateApiKey` to:
    - Parse the API key from the POST request body (likely URL-encoded form data or JSON).
    - Save the received key to NVS (`preferences.putString("api_key", receivedKey)`).
    - Send an appropriate success/error response.
  - [ ] _(Self-correction: API key loading in `begin()` and header addition in `sendWebhookRequest` are already done, keep them)_.

### Phase 3: Firmware Modification (Existing - Now Phase 3)

- [ ] Update the JSON payload creation logic to match the new required format (`action`, `device_project_id`, `project_name`, `project_color`, optional `description`). _(Self-correction: Payload format in `sendWebhookNotification` already appears correct, but verify)_.
- [ ] Ensure the correct project details (`device_project_id`, `name`, `color`) are fetched and included in the payload for both `
