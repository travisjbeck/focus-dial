# Project: Migrate Time Tracking App to Next.js

## 1. Goal

Build the Focus Dial Time Tracking Web Application using the Next.js framework. This involves porting the existing functionality from the separate React frontend (Vite) and Node.js/Express backend into a single, unified Next.js application.

## 2. Rationale

- **Simplified Development:** A single codebase, build process, and development server streamlines the workflow.
- **Simplified Deployment:** Only one application needs to be deployed and run on the Raspberry Pi.
- **Easier Networking:** Eliminates the complexity of connecting the Focus Dial (hardware) to a separate backend API, especially concerning dynamic IP addresses or the need for MDNS configuration. The webhook can directly target the Next.js application's URL (e.g., `http://raspberrypi.local/api/webhook`).
- **Leverage Next.js Features:** Potential for improved performance, routing, data fetching patterns, and rendering strategies offered by Next.js.

## 3. Starting Point & Context

- **Reference Project:** The existing code within the `time-tracking-app` directory serves as the functional specification and implementation guide. This includes:
  - A React frontend built with Vite and styled with Tailwind CSS.
  - A Node.js/Express backend API.
  - SQLite database managed with Sequelize ORM.
- **Completed Core Features (in `time-tracking-app`):**
  - Backend API endpoints for managing Projects (CRUD).
  - Backend API endpoint (`/api/webhook`) to receive timer events (start/stop) from the Focus Dial.
  - Webhook handler logic using `device_project_id` (generated on the device) to reliably associate time entries with projects, including creating new projects or updating existing ones based on this ID.
  - Database models for `Project` and `TimeEntry`.
  - Basic React frontend UI components and pages for viewing/managing projects (partially complete).
  - Tailwind CSS v4 compatibility adjustments were started on the frontend.
- **Target Architecture:** A single Next.js application containing:
  - React components for the user interface.
  - API Routes (`app/api/...`) to handle backend logic (project management, webhooks).
  - Integrated database access (Sequelize/SQLite).

## 4. Phased Plan

_The plan involves creating a new Next.js application from scratch in a separate directory (e.g., `nextjs-time-tracking-app`) and incrementally porting/re-implementing features._

### Phase 1: Project Setup & Basic Structure

- [ ] **Task 1.1: Initialize New Next.js Project**
  - Use `create-next-app` to scaffold a new TypeScript project in a dedicated directory (e.g., `nextjs-time-tracking-app`).
  - Choose desired configurations (App Router recommended, TypeScript, Tailwind CSS).
- [ ] **Task 1.2: Verify Tailwind CSS Setup**
  - Ensure Tailwind is correctly configured (`tailwind.config.ts`, `postcss.config.js`, `app/globals.css`).
  - Include necessary Tailwind v4 adjustments if `create-next-app` doesn't handle them automatically.
- [ ] **Task 1.3: Establish Project Structure**
  - Define standard folders (e.g., `components/`, `lib/` for utilities/db, `app/api/`, `app/(pages)/`).
- [ ] **Task 1.4: Consolidate Dependencies**
  - Review `package.json` from the old `frontend` and `backend`.
  - Install necessary dependencies (e.g., `sequelize`, `sqlite3`, `axios` for testing) into the new Next.js project.

### Phase 2: Database & Core Backend Logic

- [ ] **Task 2.1: Database Integration**
  - Set up Sequelize configuration (`lib/database.ts` or similar).
  - Establish connection to the SQLite database file (define path, e.g., `data/database.sqlite`).
  - Ensure the database file/directory is handled correctly (e.g., included in `.gitignore` if needed).
- [ ] **Task 2.2: Port Database Models**
  - Recreate the `Project` and `TimeEntry` Sequelize models (`lib/models/Project.ts`, `lib/models/TimeEntry.ts`).
- [ ] **Task 2.3: Implement Project API Routes**
  - Create API route `app/api/projects/route.ts` (handles GET for list, POST for create).
  - Create API route `app/api/projects/[id]/route.ts` (handles GET for single, PUT for update, DELETE for delete).
  - Port the corresponding logic from the Express backend, adapting to Next.js API route syntax.
- [ ] **Task 2.4: Implement Webhook API Route**
  - Create API route `app/api/webhook/route.ts` (handles POST).
  - Port the webhook handling logic from `time-tracking-app/backend/src/routes/webhook.js`, including project upsert based on `device_project_id` and time entry creation/update.

### Phase 3: Frontend Implementation

- [ ] **Task 3.1: Port Core UI Components**
  - Migrate essential reusable components (e.g., Layout, Button, Dialog, Input, Card) from the old frontend to the new `components/` directory.
  - Adapt imports and styling as needed.
- [ ] **Task 3.2: Implement "Projects" Page**
  - Create the page route (e.g., `app/(pages)/projects/page.tsx`).
  - Implement UI to list projects (fetching from `/api/projects`).
  - Implement functionality to add, edit, and delete projects (interacting with `/api/projects` and `/api/projects/[id]`).
  - Utilize ported UI components.
- [ ] **Task 3.3: Implement "Time Entries" Page**
  - Create the page route (e.g., `app/(pages)/entries/page.tsx`).
  - Implement UI to display recorded time entries (requires an API endpoint to fetch time entries, likely with project details - add this to Phase 2 if not planned).
- [ ] **Task 3.4: Implement "Dashboard" Page**
  - Create the main dashboard page (`app/page.tsx`).
  - Display relevant summaries or overview information (requires defining what data is needed and potentially new API endpoints).
- [ ] **Task 3.5: UI/UX Refinement**
  - Ensure consistent styling and user experience across the application.
  - Apply any pending UI improvements or adjustments.

### Phase 4: Testing & Refinement

- [ ] **Task 4.1: API Route Testing**
  - Use tools like Postman or write test scripts (similar to `test-webhook.js`) to verify all API routes function correctly (valid requests, error handling).
- [ ] **Task 4.2: Frontend Interaction Testing**
  - Manually test all UI interactions (creating/editing projects, viewing entries, navigation).
- [ ] **Task 4.3: End-to-End Webhook Testing**
  - Configure the Focus Dial firmware to point its webhook URL to the running Next.js application's `/api/webhook` route.
  - Perform the end-to-end test scenario previously outlined (create project, start/stop timer, rename project, verify database).
- [ ] **Task 4.4: Code Cleanup & Optimization**
  - Review code for clarity, consistency, and potential performance improvements.
  - Ensure proper error handling and logging.
