# Project: Migrate Web App to Supabase with Auth & Live Updates

- **Created**: 2024-04-15
- **Status**: Active
- **Last Updated**: 2024-07-27

## Context & Requirements

Migrate the time tracking web application backend from SQLite to Supabase to leverage its built-in real-time subscription capabilities for live dashboard updates **and implement user authentication**. The migration should:

- Replace the existing SQLite database (`better-sqlite3` driver) with Supabase (PostgreSQL).
- **Implement user authentication (email/password signup & login) using Supabase Auth.**
- **Protect dashboard routes, redirecting unauthenticated users to a login/signup page.**
- **Implement an API key system for authenticating webhook requests from the Focus Dial device.**
- Set up Supabase for local development using Docker and the Supabase CLI.
- Refactor the backend API (`src/app/api/`) to use the Supabase JavaScript client (`@supabase/supabase-js`) and respect user authentication / API keys.
- Implement real-time data fetching and updates on the frontend dashboard using Supabase subscriptions, filtered for the logged-in user.
- Ensure data consistency during and after migration (schema definition, potential data migration script).
- Maintain compatibility with the existing Next.js application structure.
- Remove SQLite-specific code and dependencies.

## Project Context & Directory Structure

### Target Application Structure (Post-Migration)

```
nextjs-time-tracking-app/
├── supabase/                      # Supabase local dev config (managed by CLI)
│   └── ...
├── src/
│   ├── app/
│   │   ├── (auth)/                  # Authentication routes group
│   │   │   ├── login/
│   │   │   │   └── page.tsx         # Login page component
│   │   │   └── signup/
│   │   │       └── page.tsx         # Signup page component
│   │   ├── (app)/                   # Authenticated app routes group
│   │   │   ├── layout.tsx           # Layout for authenticated routes (checks auth)
│   │   │   ├── page.tsx             # Dashboard main page
│   │   │   ├── projects/
│   │   │   │   └── page.tsx         # Projects list page
│   │   │   └── settings/            # User settings page
│   │   │       └── page.tsx         # Page to manage settings (incl. API Key)
│   │   └── api/
│   │       ├── auth/                  # Auth related API routes (e.g., callbacks)
│   │       │   └── ...
│   │       ├── projects/
│   │       │   └── route.ts         # Projects API (using Supabase client, RLS enforced)
│   │       ├── webhook/
│   │       │   └── route.ts         # Webhook endpoint (**API Key Auth**, associates with user)
│   │       ├── time-entries/
│   │       │   └── route.ts         # Time entries API (using Supabase client, RLS enforced)
│   │       └── api-key/             # API route for managing API keys
│   │           └── route.ts         # Handler for GET/POST/DELETE API key
│   ├── lib/
│   │   └── supabase/
│   │       ├── client.ts            # Supabase client initialization (browser/server)
│   │       ├── middleware.ts        # Next.js middleware for auth checks/redirects
│   │       ├── types.ts             # Supabase generated types (optional)
│   │       └── ...                  # Reusable Supabase queries/functions
│   ├── components/
│   │   ├── AuthForm.tsx           # Reusable Login/Signup form component
│   │   ├── ApiKeyManager.tsx      # Component for settings page to show/generate API key
│   │   ├── ProjectList.tsx          # Projects list component (using Supabase subscription)
│   │   └── TimeEntryList.tsx        # Time entries list component (using Supabase subscription)
│   └── hooks/                       # Custom hooks for Supabase data/subscriptions
│       ├── useSupabase.ts         # Hook for Supabase client instance
│       ├── useAuth.ts             # Hook for user auth state
│       ├── useLiveProjects.ts     # Hook for live projects data (user-specific)
│       └── useLiveTimeEntries.ts  # Hook for live time entries data (user-specific)
├── package.json
└── .env.local                     # Store Supabase URL and anon key
```

### Key Files/Areas for Refactoring

1.  **Local Environment Setup**:
    - Initialize Supabase local development environment (`supabase init`, `supabase start`).
2.  **Database Schema**:
    - Define table schemas in `supabase/migrations/`. **Add `user_id` columns (linking to `auth.users`) where appropriate (e.g., `projects`, `time_entries`).**
    - **Define `api_keys` table (e.g., `id`, `user_id`, `key_hash`, `created_at`).**
    - Apply migrations (`supabase db push` or manual SQL).
3.  **Authentication**:
    - Configure Supabase Auth settings (email/password provider).
    - Implement **Row Level Security (RLS)** policies based on `auth.uid()` to restrict data access to the logged-in user.
    - Create Login/Signup pages/components (`src/app/(auth)/`, `src/components/AuthForm.tsx`).
    - Implement authentication logic using `@supabase/supabase-js` (`signInWithPassword`, `signUp`).
    - Set up **Next.js Middleware** (`src/lib/supabase/middleware.ts`) to protect routes and handle redirects based on auth state.
4.  **Backend API (`src/app/api/`)**:
    - Replace all `better-sqlite3` calls with `@supabase/supabase-js` client methods.
    - Ensure API routes respect RLS (data automatically filtered by Supabase based on user session).
    - **Refactor `webhook/route.ts`:**
      - **Extract API key from request header.**
      - **Query `api_keys` table to find matching (hashed) key and get `user_id`.**
      - **If key is valid, insert `time_entry` with the associated `user_id`.**
      - **Return appropriate status codes (e.g., 200 on success, 401/403 on invalid/missing key).**
5.  **Supabase Client (`src/lib/supabase/`)**:
    - Create client initialization logic (using `createBrowserClient`, `createServerComponentClient`, `createMiddlewareClient`, etc. from `@supabase/ssr`).
    - Potentially generate TypeScript types from DB schema.
6.  **Frontend Components (`src/components/`)**:
    - Replace static data fetching with real-time subscriptions using Supabase client, **ensuring subscriptions filter by user ID.**
    - Refactor `ProjectList.tsx`, `TimeEntryList.tsx`.
7.  **Frontend Hooks (`src/hooks/`)**:
    - Create/update custom hooks for auth state and live data subscriptions.
8.  **Dependencies (`package.json`)**:
    - Add `@supabase/supabase-js`, **`@supabase/ssr`**.
    - Remove `better-sqlite3`.
9.  **Configuration (`.env.local`)**:
    - Add Supabase project URL and anon key.
10. **Remove Obsolete Files**:
    - `src/lib/db/` directory and its contents.
    - `data/focus-dial.sqlite` file.

## Development Plan

### Phase 1: Local Supabase Setup & Auth Config

- [ ] Install Supabase CLI (if not already installed).
- [ ] Initialize Supabase within the project directory (`supabase init`).
- [ ] Start the local Supabase Docker containers (`supabase start`).
- [ ] Verify local Supabase Studio is accessible.
- [ ] Obtain local Supabase URL and anon key.
- [ ] Store Supabase credentials securely (e.g., in `.env.local`, add `.env.local` to `.gitignore`).
- [ ] **Configure Auth providers (enable Email/Password) in Supabase Studio or `config.toml`.**

### Phase 2: Database Schema Migration & RLS

- [ ] Define SQL `CREATE TABLE` statements for `projects`, `time_entries`, and `api_keys` tables in a new migration file (`supabase/migrations/0001_initial_schema.sql`).
  - **Add `user_id UUID REFERENCES auth.users(id)` columns.**
  - **Define `api_keys` table (e.g., `id`, `user_id`, `key_hash`, `created_at`).**
  - Define other columns, primary/foreign keys, constraints.
- [ ] Apply the migration (`supabase db push`).
- [ ] Enable Row Level Security (RLS) on tables.
- [ ] **Define RLS policies for `projects`, `time_entries`, and `api_keys` (e.g., `SELECT/INSERT/UPDATE/DELETE` allowed where `user_id = auth.uid()`).**
- [ ] Enable real-time functionality for `projects`, `time_entries`, and `api_keys` tables.

### Phase 3: Authentication Implementation (Frontend & Middleware)

- [ ] Install `@supabase/supabase-js` and `@supabase/ssr`.
- [ ] Set up Supabase client instances for browser, server components, and middleware using `@supabase/ssr` (`src/lib/supabase/client.ts`).
- [ ] Create Login and Signup pages (`src/app/(auth)/...`).
- [ ] Create `AuthForm.tsx` component for handling email/password input and calling Supabase `signInWithPassword`/`signUp` functions.
- [ ] Implement `src/lib/supabase/middleware.ts` to:
  - Check user auth state on relevant routes (`/`, `/projects`, etc.).
  - Redirect unauthenticated users from protected routes to `/login`.
  - Redirect authenticated users from `/login`, `/signup` to `/`.
- [ ] Add logout functionality.

### Phase 4: API Key Management

- [ ] **Create API route (`/api/api-key`) to handle GET (retrieve key if exists), POST (generate/hash/store new key), DELETE (remove key).** Ensure RLS protects this.
- [ ] **Create Settings page (`/settings`) accessible only to logged-in users.**
- [ ] **Create `ApiKeyManager.tsx` component for the settings page to display the user's key (if exists) and provide buttons to generate/regenerate or delete it.**

### Phase 5: Backend API Refactoring

- [ ] Create Supabase server client if needed (distinct from middleware/browser clients).
- [ ] Refactor API routes (`projects`, `time-entries`, `api-key`) to use the Supabase client. (Note: RLS should handle most user-specific filtering automatically if policies are correct).
- [ ] **Refactor `webhook/route.ts`:**
  - **Extract API key from request header.**
  - **Query `api_keys` table to find matching (hashed) key and get `user_id`.**
  - **If key is valid, insert `time_entry` with the associated `user_id`.**
  - **Return appropriate status codes (e.g., 200 on success, 401/403 on invalid/missing key).**
- [ ] Test API endpoints (ideally simulating authenticated requests).

### Phase 6: Frontend Real-time Implementation (User-Specific)

- [ ] Create/update custom hooks (`useLiveProjects`, `useLiveTimeEntries`) to subscribe to table changes **filtered by the authenticated user.**
  - Use Supabase real-time `on()` method with appropriate filters (e.g., `eq('user_id', user.id)`).
- [ ] Refactor `ProjectList.tsx` and `TimeEntryList.tsx` to use these hooks.
- [ ] Ensure data displayed is only for the logged-in user.
- [ ] Implement loading states and error handling.

### Phase 7: Cleanup & Refinement

- [ ] Remove the old `src/lib/db/` directory and `better-sqlite3` dependency.
- [ ] Remove `data/focus-dial.sqlite` file.
- [ ] Review code for SQLite references.
- [ ] Test end-to-end flow: signup -> login -> generate API key -> configure device -> webhook POST with key -> data inserted for user -> dashboard update (showing user's data).

### Phase 8: Documentation & Deployment Prep

- [ ] Document Supabase setup, Auth, API Keys, Real-time implementation (`docs/supabase-migration.md`).
- [ ] Update main `README.md`.
- [ ] Outline deployment steps (hosted Supabase project setup, env vars for production URL/keys).

## Notes & References

### Supabase Local Development Setup Steps (High-Level)

1.  **Install Supabase CLI:** Follow official instructions ([link](hhttps://supabase.com/docs/guides/cli)). Requires Docker.
2.  **Navigate to Project Root:** `cd path/to/nextjs-time-tracking-app`
3.  **Initialize Supabase:** `supabase init` (Creates `supabase` directory).
4.  **Start Supabase Services:** `supabase start` (Downloads Docker images, starts containers).
5.  **Get Credentials:** Note local API URL, anon key, etc.
6.  **Access Studio:** Open local Studio URL (usually `ttp://localhost:54323`).

### Key Considerations

- **Row Level Security (RLS):** Essential for securing user data and API keys.
- **API Key Security:** Generate strong keys, **store only hashes in the database**, transfer securely via HTTPS. Consider key rotation/revocation.
- **Webhook Payload:** Ensure the device sends necessary data (project name/color, duration) along with the API key.
- **Real-time Quotas:** Relevant for hosted Supabase.
- **Data Migration:** Script needed if preserving existing SQLite data.
- **Environment Variables:** Manage Supabase URL/keys for local/prod.

### Resources

- [Supabase Docs: Local Development](https://supabase.com/docs/guides/cli/local-development)
- [Supabase Docs: Auth Overview](https://supabase.com/docs/guides/auth)
- [Supabase Docs: Auth with Next.js](https://supabase.com/docs/guides/auth/server-side/nextjs)
- [Supabase Docs: Realtime Subscriptions](https://supabase.com/docs/guides/realtime/subscriptions)
- [Supabase Docs: Row Level Security](https://supabase.com/docs/guides/auth/row-level-security)
- [Supabase JS Client Library (`@supabase/supabase-js`)](https://supabase.com/docs/reference/javascript/introduction)
- [Supabase SSR Helpers (`@supabase/ssr`)](https://supabase.com/docs/guides/auth/server-side/nextjs)
- [Next.js Documentation](https://nextjs.org/docs)
