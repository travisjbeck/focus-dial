import { type CookieOptions, createServerClient, createBrowserClient } from '@supabase/ssr'
import { type NextRequest, NextResponse } from 'next/server'
import { type ReadonlyRequestCookies } from 'next/headers'

// Define a placeholder type for the database schema
// TODO: Replace with actual DB types generated via:
// npx supabase gen types typescript --project-id <your-project-id> --schema public > src/lib/supabase/types_db.ts
// eslint-disable-next-line @typescript-eslint/no-explicit-any
type Database = any;

// --- Client for use in Client Components ---
export function createSupabaseBrowserClient() {
  // Ensure environment variables are defined
  const supabaseUrl = process.env.NEXT_PUBLIC_SUPABASE_URL;
  const supabaseAnonKey = process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY;

  if (!supabaseUrl || !supabaseAnonKey) {
    throw new Error('Missing Supabase URL or Anon Key in environment variables.');
  }

  return createBrowserClient<Database>(supabaseUrl, supabaseAnonKey);
}

// --- Client for use in Server Components, Route Handlers, and Server Actions ---
export function createSupabaseServerClient(
  cookieStore: ReadonlyRequestCookies
) {
  const supabaseUrl = process.env.NEXT_PUBLIC_SUPABASE_URL;
  const supabaseAnonKey = process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY;

  if (!supabaseUrl || !supabaseAnonKey) {
    throw new Error('Missing Supabase URL or Anon Key in environment variables.');
  }

  return createServerClient<Database>(
    supabaseUrl,
    supabaseAnonKey,
    {
      cookies: {
        get(name: string) {
          return cookieStore.get(name)?.value;
        },
        set(name: string, value: string, options: CookieOptions) {
          console.warn(
            `Attempted to set cookie ('${name}') via createSupabaseServerClient helper. Cookie setting should be handled directly using 'cookies().set' in Server Actions or Route Handlers.`
          );
        },
        remove(name: string, options: CookieOptions) {
          console.warn(
            `Attempted to remove cookie ('${name}') via createSupabaseServerClient helper. Cookie removal should be handled directly using 'cookies().delete' in Server Actions or Route Handlers.`
          );
        },
      },
    }
  );
}

// --- Client Wrapper for use in Middleware ---
// This function wraps createServerClient for middleware usage.
export function createSupabaseMiddlewareClient(request: NextRequest) {
  const supabaseUrl = process.env.NEXT_PUBLIC_SUPABASE_URL;
  const supabaseAnonKey = process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY;

  if (!supabaseUrl || !supabaseAnonKey) {
    throw new Error('Missing Supabase URL or Anon Key in environment variables.');
  }

  let response = NextResponse.next({
    request: {
      headers: request.headers,
    },
  });

  const supabase = createServerClient(
    supabaseUrl,
    supabaseAnonKey,
    {
      cookies: {
        getAll() {
          return request.cookies.getAll();
        },
        setAll(cookiesToSet: { name: string; value: string; options: CookieOptions }[]) {
          try {
            cookiesToSet.forEach(({ name, value, options }) => request.cookies.set(name, value));
            response = NextResponse.next({
              request: {
                headers: request.headers,
              },
            });
            cookiesToSet.forEach(({ name, value, options }) => {
              response.cookies.set(name, value, options);
            });
          } catch (error) {
            console.error("Error setting cookies in middleware client (setAll):", error);
          }
        },
      },
    }
  );

  return { supabase, response };
}

// --- Simplified Server Client (Optional) ---
export function createSimpleServerClient(
  cookieStore: ReadonlyRequestCookies
) {
  return createSupabaseServerClient(cookieStore);
} 