# Project: Migrate Time Tracking App to Next.js

- **Created**: 2025-04-15
- **Status**: Completed
- **Last Updated**: 2025-04-15

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
  - Integrated database access using a simple SQLite solution.
  - Focus on simplicity and maintainability without complex ORMs.

## 4. Phased Plan

_The plan involves creating a new Next.js application from scratch in a separate directory (e.g., `nextjs-time-tracking-app`) and incrementally porting/re-implementing features._

### Phase 1: Project Setup & Basic Structure

- [x] **Task 1.1: Initialize New Next.js Project**
  - Use `create-next-app` to scaffold a new TypeScript project in a dedicated directory (e.g., `nextjs-time-tracking-app`).
  - Specify Next.js 14 and React 18 explicitly during installation.
  - Choose App Router, TypeScript, and Tailwind CSS configurations.
- [x] **Task 1.2: Verify Tailwind CSS Setup**
  - Ensure Tailwind is correctly configured (`tailwind.config.ts`, `postcss.config.js`, `app/globals.css`).
  - Include necessary Tailwind adjustments if needed.
- [x] **Task 1.3: Establish Project Structure**
  - Define standard folders (e.g., `components/`, `lib/` for utilities/db, `app/api/`, `app/(pages)/`).
- [x] **Task 1.4: Consolidate Dependencies**
  - Review `package.json` from the old `frontend` and `backend`.
  - Install necessary dependencies (e.g., `better-sqlite3` or `sqlite` for direct database access, `axios` for testing).
  - Avoid complex ORMs to keep the codebase straightforward.

### Phase 2: Database & Core Backend Logic

- [x] **Task 2.1: Database Integration**
  - Set up direct SQLite connection using `better-sqlite3` or similar lightweight library (`lib/database.ts`).
  - Create helper functions for common database operations.
  - Ensure the database file/directory is handled correctly (e.g., included in `.gitignore` if needed).
- [x] **Task 2.2: Create Database Schema**
  - Define SQL schema for `projects` and `time_entries` tables.
  - Create a simple migration script to initialize the database structure.
  - Implement utility functions for basic CRUD operations without an ORM.
- [x] **Task 2.3: Implement Project API Routes**
  - Create API route `app/api/projects/route.ts` (handles GET for list, POST for create).
  - Create API route `app/api/projects/[id]/route.ts` (handles GET for single, PUT for update, DELETE for delete).
  - Port the corresponding logic from the Express backend, adapting to Next.js API route syntax and using direct SQL queries.
- [x] **Task 2.4: Implement Webhook API Route**
  - Create API route `app/api/webhook/route.ts` (handles POST).
  - Port the webhook handling logic, including project upsert based on `device_project_id` and time entry creation/update.
  - Use simple SQL transactions where needed for data integrity.

### Phase 3: Frontend Implementation

- [x] **Task 3.1: Port Core UI Components**
  - Migrate essential reusable components (e.g., Layout, Button, Dialog, Input, Card) from the old frontend to the new `components/` directory.
  - Adapt imports and styling as needed for React 18 compatibility.
- [x] **Task 3.2: Implement "Projects" Page**
  - Create the page route (e.g., `app/(pages)/projects/page.tsx`).
  - Implement UI to list projects (fetching from `/api/projects`).
  - Implement functionality to add, edit, and delete projects (interacting with `/api/projects` and `/api/projects/[id]`).
  - Utilize ported UI components.
- [x] **Task 3.3: Implement "Time Entries" Page**
  - Create the page route (e.g., `app/(pages)/entries/page.tsx`).
  - Implement UI to display recorded time entries (requires an API endpoint to fetch time entries, likely with project details - add this to Phase 2 if not planned).
- [x] **Task 3.4: Implement "Dashboard" Page**
  - Create the main dashboard page (`app/page.tsx`).
  - Display relevant summaries or overview information (requires defining what data is needed and potentially new API endpoints).
- [x] **Task 3.5: UI/UX Refinement**
  - Ensure consistent styling and user experience across the application.
  - Apply any pending UI improvements or adjustments.

### Phase 4: Testing & Refinement

- [x] **Task 4.1: API Route Testing**
  - Use tools like Postman or write test scripts to verify all API routes function correctly (valid requests, error handling).
- [x] **Task 4.2: Frontend Interaction Testing**
  - Manually test all UI interactions (creating/editing projects, viewing entries, navigation).
- [x] **Task 4.3: End-to-End Webhook Testing**
  - Configure the Focus Dial firmware to point its webhook URL to the running Next.js application's `/api/webhook` route.
  - Perform the end-to-end test scenario (create project, start/stop timer, rename project, verify database).
- [x] **Task 4.4: Code Cleanup & Optimization**
  - Review code for clarity, consistency, and potential performance improvements.
  - Ensure proper error handling and logging.
  - Verify that the application remains simple and straightforward without unnecessary complexity.

## 5. Completion Summary

The migration of the Time Tracking App to Next.js has been successfully completed. The new application:

1. Uses Next.js 14 with React 18 for improved performance and developer experience
2. Implements a simplified SQLite database integration using better-sqlite3, without the complexity of an ORM
3. Provides full CRUD functionality for projects and time entries
4. Includes a webhook endpoint for receiving timer events from the Focus Dial hardware
5. Features a clean, responsive UI for managing projects and viewing time entries
6. Follows a modular architecture that is easy to maintain and extend

The application successfully passed all testing phases, including the end-to-end webhook test which verified that:

- Projects can be created and updated via the webhook
- Time entries are accurately recorded with start and end times
- Duration calculations are correct
- Project updates are properly propagated throughout the system

This implementation successfully meets all the requirements defined in the project plan while maintaining simplicity and avoiding unnecessary complexity.
