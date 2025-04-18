# Project: Backend Webhook - Device-First Project Refactor

- **Created**: 2025-04-17
- **Status**: Completed
- **Last Updated**: 2024-05-21

## Context & Requirements

The intended workflow dictates that projects are created on the Focus Dial device, which generates its own unique ID (`device_project_id`). The backend webhook (`/api/webhook`) must be refactored to support this.

When receiving a webhook request (authenticated via API key):

1. The payload will contain `device_project_id`, `project_name`, and `project_color`.
2. The backend must look up the project in the Supabase `projects` table using the combination of the authenticated `user_id` and the received `device_project_id`.
3. If the project exists, use its database `id` for time entry operations.
4. If the project does _not_ exist, create it in the `projects` table using the received details (`device_project_id`, `name`, `color`) and the `user_id`. Then use the _newly generated_ database `id` for time entry operations.

This requires changes to the database schema and the webhook route handler.

**Requirements:**

1.  Modify `projects` table schema to include a `device_project_id` column (likely TEXT type, unique per user).
2.  Update `/api/webhook` route handler logic:
    - Modify expected payload (`WebhookPayload` interface).
    - Implement the "find or create" logic for projects based on `user_id` and `device_project_id`.
    - Use the correct database `project.id` (found or created) when inserting/updating `time_entries`.
3.  Ensure API key authentication remains functional.
4.  Update relevant database types (`src/types/supabase.ts`).

**Relevant Files:**

- `supabase/migrations/` (New migration file needed)
- `nextjs-time-tracking-app/src/app/api/webhook/route.ts`
- `nextjs-time-tracking-app/src/types/supabase.ts`

## Development Plan

### Phase 1: Database Schema Update

- [ ] Define the data type for `device_project_id` (e.g., `TEXT` or `VARCHAR`). Assume `TEXT` for flexibility unless device generates specific format.
- [ ] Create a new Supabase migration file (e.g., `YYYYMMDDHHMMSS_add_device_project_id_column.sql`).
- [ ] Add SQL `ALTER TABLE public.projects ADD COLUMN device_project_id TEXT;` to the migration.
- [ ] Add a unique constraint on `(user_id, device_project_id)`: `ALTER TABLE public.projects ADD CONSTRAINT projects_user_id_device_project_id_key UNIQUE (user_id, device_project_id);`.
- [ ] Apply the migration to the local database (`supabase migration up`).
- [ ] Regenerate Supabase TypeScript types (`supabase gen types typescript --local > src/types/supabase.ts`).

### Phase 2: Webhook Refactoring

- [ ] Update the `WebhookPayload` interface in `api/webhook/route.ts` to expect `device_project_id` (string/text), `project_name` (string), `project_color` (string), and remove the database `project_id`.
- [ ] Modify the webhook `POST` handler logic:
  - After authenticating the user (`userId`), extract `device_project_id`, `project_name`, `project_color` from the payload.
  - Implement the "find or create" logic:
    - **Find:** Attempt to `SELECT * FROM projects WHERE user_id = userId AND device_project_id = deviceProjectId`. Use `.maybeSingle()`.
    - **If Found:** Store the found project's database `id`.
    - **If Not Found (Create):** `INSERT INTO projects (user_id, device_project_id, name, color) VALUES (userId, deviceProjectId, projectName, projectColor)` and retrieve the new row (especially the database `id`). Handle potential insert errors (e.g., constraint violations).
  - Replace all subsequent uses of the old `project_id` variable with the database `id` obtained from the find-or-create step (e.g., when inserting/updating `time_entries`).
  - Remove the previous direct ownership check based on the database `project_id` from the payload (ownership is now implicit in the find-or-create logic using `userId`).

### Phase 3: Testing

- [ ] **Test Case 1: Existing Project**
  - Manually insert a project row in Supabase Studio with a specific `user_id` and `device_project_id`.
  - Send a `start_timer` webhook request using the correct API key (for the `user_id`), and the matching `device_project_id`, `name`, `color`.
  - Verify a time entry is created linked to the _existing_ project's database `id`.
  - Verify no _new_ project was created.
  - Send a `stop_timer` request and verify the time entry is updated.
- [ ] **Test Case 2: New Project**
  - Ensure no project exists for the target `user_id` and a _new_ `device_project_id`.
  - Send a `start_timer` webhook request using the API key, the new `device_project_id`, and valid `name`/`color`.
  - Verify a _new_ project row is created in the `projects` table with the correct details.
  - Verify a time entry is created linked to the _newly created_ project's database `id`.
  - Send a `stop_timer` request and verify the time entry is updated.
- [ ] **Test Case 3: Invalid Payload/Auth**
  - Test with missing `device_project_id`, `name`, or `color`.
  - Test with an invalid API key.
  - Verify appropriate error responses (e.g., 400, 401).

## Notes & References

- The `device_project_id` uniqueness is scoped per user.
- Error handling during the find-or-create process is important.
- This refactor focuses solely on the webhook; further work may be needed to align other parts of the app (like UI-based project creation) if desired.
