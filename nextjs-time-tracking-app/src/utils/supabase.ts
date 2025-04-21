import { createBrowserClient, createServerClient, CookieOptions } from '@supabase/ssr'
import { type ReadonlyRequestCookies } from 'next/dist/server/web/spec-extension/adapters/request-cookies'
import { type Database } from '@/types/supabase'

// Create a Supabase client for use in the browser
// Uses NEXT_PUBLIC_ variables
export const createBrowserSupabaseClient = () => {
  return createBrowserClient<Database>(
    process.env.NEXT_PUBLIC_SUPABASE_URL!,
    process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY!
  )
}

// Create a Supabase client for use in server components
// Needs cookies() from 'next/headers' passed in
// Uses non-public server-side variables
export const createServerComponentSupabaseClient = (cookieStore: ReadonlyRequestCookies) => {
  return createServerClient<Database>(
    process.env.SUPABASE_URL!, // Use non-public var
    process.env.SUPABASE_ANON_KEY!, // Use non-public var
    {
      cookies: {
        getAll() {
          return cookieStore.getAll()
        },
      },
    }
  )
}

// Create a Supabase client for use in server actions or route handlers
// Needs cookies() from 'next/headers' passed in
// Uses non-public server-side variables
export const createActionSupabaseClient = (cookieStore: ReadonlyRequestCookies) => {
  return createServerClient<Database>( // Use createServerClient for actions/routes as well
    process.env.SUPABASE_URL!, // Use non-public var
    process.env.SUPABASE_ANON_KEY!, // Use non-public var
    {
      cookies: {
        getAll() {
          return cookieStore.getAll()
        },
        setAll(cookiesToSet: { name: string; value: string; options: CookieOptions }[]) {
          try {
            // Use the set method which is available on the mutable cookie store in Server Actions/Route Handlers
            cookiesToSet.forEach(({ name, value, options }) => {
              // The cookieStore from next/headers might need options mapping
              // Let's try passing directly first, may need adjustment
              try {
                cookieStore.set({ name, value, ...options });
              } catch {
                // Fallback if direct spread fails (e.g., due to method mismatch)
                cookieStore.set(name, value, options);
              }
            })
          } catch (error) {
            // Handle potential errors during the iteration or setting cookies
            console.error('Error setting cookies in Supabase client:', error);
          }
        },
      },
    }
  )
} 