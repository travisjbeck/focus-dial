# Focus Dial - Time Tracking Web Application Development Plan

## Project Summary

This project involves building a full-stack web application to complement the **Focus Dial** hardware, an ESP32-S2 based timer device. The Focus Dial firmware (already developed) allows users to track time against specific projects and sends this data via webhooks.

The web application will provide a dashboard interface (React SPA) for viewing tracked time, managing projects, adding details to time entries, and generating invoices. It will run on a **Raspberry Pi**, acting as the backend server (Node.js/Express) and database host (SQLite), receiving webhooks from the Focus Dial on the local network.

## Project Status (As of YYYY-MM-DD)

- **Frontend:** Initial components built (Dashboard, Projects, Time Entries pages). Styling implemented using Tailwind CSS v3 and shadcn/ui. Dark theme is functional.
- **Backend:** Basic API structure likely exists, webhook endpoint needs modification (see Future Work).
- **Key Issue Resolved:** Migrated from Tailwind v4 attempt back to v3 due to implementation issues. Cleaned up duplicated/unused components (`Toaster`, `TimeEntries`). Adjusted dark theme text contrast.
- **Future Work:** Significant changes required to handle project identification reliably between device and web app. See `project-webhook-project-ids.md` for the detailed plan.

## 1. Project Context & Goal

### Device Summary

- **Name:** Focus Dial
- **Purpose:** Firmware for the "Focus Dial" hardware.
- **Functionality:** A timer device with a rotary dial input, OLED display (128x64), and NeoPixel LED ring indicator. Allows setting timers with configurable intervals, associated with user-defined projects (name, color). Features Wi-Fi connectivity with webhook callback capabilities and a local web server for configuration.
- **Hardware:** Adafruit QT Py ESP32-S2 (240MHz Tensilica processor, 4MB Flash, 2MB PSRAM).

### Existing Firmware Features (Relevant)

- Timer functionality (start, stop, pause, resume, done).
- Project management via web UI (add, edit, delete projects with names and colors stored in NVS).
- Project selection on the device before starting a timer.
- Visual feedback using LED ring color based on selected project.
- Webhook calls triggered on timer actions (`start`, `stop`, `done`) including the selected project name (e.g., `start|ProjectName`).
- Configuration web UI served directly from the device via `ESPAsyncWebServer` and accessible via mDNS (`focus-dial.local`).

### Web Application Goal

Create a web application (React SPA) to act as a dashboard and time-tracking system. This application will receive webhook data from the Focus Dial, store time entries, allow users to manage/annotate these entries, and generate invoices based on project time.

## 2. Chosen Architecture

- **Backend/Database:** Run on a separate, dedicated machine (e.g., **Raspberry Pi**) using Node.js (Express) and a database (e.g., SQLite initially). **This avoids running complex server/database logic directly on the resource-constrained ESP32-S2.**
- **Frontend:** React Single Page Application (SPA), built and served statically by the backend server on the Raspberry Pi.
- **Communication:** The Focus Dial firmware will send webhook POST requests to an API endpoint hosted on the Raspberry Pi backend server.

## 3. Development Tasks (Raspberry Pi Backend / React Frontend)

**(Note: Many tasks below are superseded or will be impacted by the plan in `project-webhook-project-ids.md`)**

### Phase 1: Backend Setup & API (Raspberry Pi)

- [x] **Environment Setup:** (Assumed Complete)
- [x] **Project Initialization:** (Assumed Complete)
- [x] **Database Schema Design:** (Initial setup likely complete, **needs modification for `device_project_id`**)
- [ ] **API Server Implementation (Express.js):**
  - [x] Set up basic Express server structure (Assumed Complete)
  - [ ] Implement API routes/controllers:
    - [ ] `POST /api/webhook`: Receives webhook from Focus Dial.
      - **Payload Format:** **NEEDS UPDATE** to include `device_project_id`.
      - **Action:** **NEEDS UPDATE** to parse `device_project_id` and use it for project lookup/creation/update (upsert logic).
    - [x] `GET /api/projects`: (Assumed Complete)
    - [x] `POST /api/projects`: (Assumed Complete, manual use)
    - [x] `PUT /api/projects/:id`: (Assumed Complete)
    - [x] `DELETE /api/projects/:id`: (Assumed Complete)
    - [x] `GET /api/time_entries`: (Assumed Complete)
    - [x] `PUT /api/time_entries/:id`: (Assumed Complete)
    - [x] `DELETE /api/time_entries/:id`: (Assumed Complete)
    - [ ] `POST /api/invoices`: (Not Started)
    - [ ] `GET /api/invoices`: (Not Started)
    - [ ] `GET /api/invoices/:id`: (Not Started)
- [ ] **Focus Dial Configuration:** (Needs update after firmware changes)

### Phase 2: Frontend React App (Served by Pi)

- [x] **Project Setup:**
  - [x] Use Vite or `create-react-app` to initialize the React project.
  - [x] Install necessary libraries: `react-router-dom`, Zustand, `date-fns`, **Tailwind CSS v3**, **shadcn/ui**, **lucide-react**.
  - [x] Set up Tailwind CSS v3 configuration.
  - [x] Set up `shadcn/ui`.
- [x] **Design & Styling Guidelines:** (Dark theme established, using Tailwind v3)
- [x] **Component Development:**
  - [x] **Layout:** Main navigation/sidebar, content area.
  - [x] **Dashboard/Overview:** Basic structure exists.
  - [x] **Project Management View:** Basic structure exists.
  - [x] **Time Entries View:** Basic structure exists.
  - [x] **Time Entry Edit Form:** Basic structure exists.
  - [ ] **Invoice Creation View:** (Not Started)
  - [ ] **Invoice List/Detail View:** (Not Started)
- [x] **API Integration:** (Initial integration for projects/time entries done using `fetch`)
- [x] **Routing:** (Initial routing setup)
- [x] **State Management:** (Zustand store setup)
- [ ] **Build & Serve:** (Likely functional, needs verification)

### Phase 3: Refinements & Deployment

- [ ] **Styling:** Minor tweaks only.
- [ ] **Invoice Generation:** (Not Started)
- [ ] **Authentication (Optional):** (Not Started)
- [ ] **Error Handling & Logging:** (Basic in place, needs review)
- [ ] **Testing:** (Not Started)
- [ ] **Deployment:** (Not Started)
- [ ] **mDNS Setup:** (Not Started)
- [ ] **Data Backup Strategy:** (Not Defined)

### Initial Setup Commands for Raspberry Pi

```sh
# Update system packages
sudo apt update && sudo apt upgrade -y

# Install Node.js and npm
curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
sudo apt install -y nodejs

# Verify installation
node -v
npm -v

# Install SQLite
sudo apt install -y sqlite3

# Install pm2 for process management
sudo npm install -g pm2

# Create project directory (if not exists)
mkdir -p ~/focus-dial/time-tracking-app
cd ~/focus-dial/time-tracking-app

# Install Avahi for mDNS (allows access via .local hostname)
sudo apt install -y avahi-daemon
```

**(Note: Node setup command updated to LTS version)**
