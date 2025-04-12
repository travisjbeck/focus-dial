# Focus Dial - Time Tracking Web Application Development Plan

## Project Summary

This project involves building a full-stack web application to complement the **Focus Dial** hardware, an ESP32-S2 based timer device. The Focus Dial firmware (already developed) allows users to track time against specific projects and sends this data via webhooks.

The web application will provide a dashboard interface (React SPA) for viewing tracked time, managing projects, adding details to time entries, and generating invoices. It will run on a **Raspberry Pi**, acting as the backend server (Node.js/Express) and database host (SQLite), receiving webhooks from the Focus Dial on the local network.

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

### Phase 1: Backend Setup & API (Raspberry Pi)

- [ ] **Environment Setup:**
  - [ ] Set up Raspberry Pi with OS (e.g., Raspberry Pi OS).
  - [ ] Install Node.js and npm/yarn.
  - [ ] Choose and install a database (e.g., SQLite).
  - [ ] Set up a process manager (e.g., `pm2`).
- [ ] **Project Initialization:**
  - [ ] Create a new Node.js project directory.
  - [ ] Initialize `package.json`.
  - [ ] Install core dependencies: `express`, `sqlite3`, `cors`, potentially an ORM (`sequelize` or `prisma`).
- [ ] **Database Schema Design:**
  - [ ] Define tables: `projects` (id, name, color, created_at, updated_at), `time_entries` (id, project_id, start_time, end_time, duration_seconds, description, invoiced, created_at, updated_at), `invoices` (id, project_id, invoice_date, total_amount, status, created_at, updated_at), `invoice_items` (id, invoice_id, time_entry_id).
  - [ ] Set up database creation/migration scripts.
- [ ] **API Server Implementation (Express.js):**
  - [ ] Set up basic Express server structure (app setup, middleware like `cors`, `json` body parser).
  - [ ] Implement API routes/controllers:
    - [ ] `POST /api/webhook`: Receives webhook from Focus Dial.
      - **Payload Format:** `action|projectName|#hexColor` (e.g., `start|My Project|#FF00AA`, `stop||#FFFFFF` for no project).
      - **Action:** Parse action, projectName, hexColor.
      - **Check if `project_name` exists in `projects` table; if not, create a new project record automatically using the received `hexColor`.**
      - Create/update `time_entries` in DB based on the action. Needs robust logic to handle start/stop/done events and calculate duration.
    - [ ] `GET /api/projects`: Returns list of projects.
    - [ ] `POST /api/projects`: **(Primarily for future use or manual overrides if needed)** Creates a new project. *(Note: Standard creation is automatic via webhook)*.
    - [ ] `PUT /api/projects/:id`: Updates project details (e.g., color).
    - [ ] `DELETE /api/projects/:id`: Deletes a project.
    - [ ] `GET /api/time_entries`: Returns time entries (allow filtering by project, date range, invoiced status).
    - [ ] `PUT /api/time_entries/:id`: Updates a time entry (description, project_id, invoiced status).
    - [ ] `DELETE /api/time_entries/:id`: Deletes a time entry.
    - [ ] `POST /api/invoices`: Creates an invoice for selected uninvoiced time entries for a project. Marks associated entries as invoiced.
    - [ ] `GET /api/invoices`: Lists invoices.
    - [ ] `GET /api/invoices/:id`: Gets details of a specific invoice, including associated time entries.
- [ ] **Focus Dial Configuration:**
    - [ ] Manually update the Webhook URL on the Focus Dial (via its config UI) to point to the Raspberry Pi's IP address and the `/api/webhook` endpoint (e.g., `http://<PI_IP_ADDRESS>:<PORT>/api/webhook`).

### Phase 2: Frontend React App (Served by Pi)

- [ ] **Project Setup:**
  - [ ] Use Vite or `create-react-app` to initialize the React project.
  - [ ] Install necessary libraries: `react-router-dom`, state management (e.g., Zustand, Context), date formatting (e.g., `date-fns`), **Tailwind CSS v4**, **shadcn/ui**, **lucide-react**. (Use built-in `fetch` instead of Axios).
  - [ ] Set up Tailwind CSS v4 configuration.
  - [ ] Set up `shadcn/ui` (includes initializing components).
- [ ] **Design & Styling Guidelines:**
    - [ ] **Aesthetics:** The web app's design MUST match the modern, dark-themed aesthetic (Vercel/Tailwind inspired) established in the device's configuration UI.
    - [ ] **Reference Files:** Refer to the existing configuration UI files for style guidance: `firmware/data/index.html`, `firmware/data/styles.css`, `firmware/data/app.js`.
    - [ ] **Technology:** Utilize **Tailwind CSS v4** for all styling.
    - [ ] **Components:** Use **shadcn/ui** components as the primary UI building blocks.
    - [ ] **Icons:** Use **lucide-react** for icons.
    - [ ] **UI:** Ensure a clean, intuitive, and responsive user interface.
- [ ] **Component Development:**
  - [ ] **Layout:** Main navigation/sidebar, content area (using shadcn/ui structure where applicable).
  - [ ] **Dashboard/Overview:** Summary of recent activity, total time per project this week/month.
  - [ ] **Project Management View:** Display projects, allow editing colors/details or deleting. *(Note: Initial project creation is handled automatically via webhook from the Focus Dial)*.
  - [ ] **Time Entries View:** Table/list display of time entries, filtering controls, ability to trigger edit/delete.
  - [ ] **Time Entry Edit Form:** Modal or inline form for editing description, changing project.
  - [ ] **Invoice Creation View:** Select project, view/select uninvoiced entries, trigger invoice generation.
  - [ ] **Invoice List/Detail View:** Display generated invoices and their line items.
- [ ] **API Integration:**
  - [ ] Create services/hooks to fetch/mutate data via backend API endpoints using the built-in **`fetch` API**.
  - [ ] Implement loading states and error handling/display.
- [ ] **Routing:** Set up client-side routing using `react-router-dom`.
- [ ] **State Management:** Manage shared application state effectively.
- [ ] **Build & Serve:**
  - [ ] Configure the Express server to serve the static React build (`dist` or `build` folder).
  - [ ] Ensure Express handles non-API routes by serving the React `index.html` to support client-side routing.

### Phase 3: Refinements & Deployment

- [ ] **Styling:** (This task is now covered by the initial setup and guidelines in Phase 2, can be removed or kept for minor tweaks)
- [ ] **Invoice Generation:** Implement a user-friendly display format for invoices (HTML/CSS). (PDF generation is optional complexity).
- [ ] **Authentication (Optional):** Add basic user login if needed.
- [ ] **Error Handling & Logging:** Enhance backend logging and frontend error reporting.
- [ ] **Testing:** Add basic tests for critical API endpoints and UI components.
- [ ] **Deployment:** Configure `pm2` for reliable server operation on the Pi.
- [ ] **mDNS Setup:** Configure mDNS on the Pi (e.g., using `avahi-daemon`) to allow accessing the app via a local hostname like `http://focus-dial.app` or `http://tracker.local`.
- [ ] **Data Backup Strategy:** Define a simple backup strategy for the database file on the Pi. 