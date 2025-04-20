import { type NextRequest, NextResponse } from 'next/server';
// Import createServerClient from @supabase/ssr (used for middleware too)
import { createServerClient, type CookieOptions } from '@supabase/ssr';

export async function middleware(request: NextRequest) {
  let response = NextResponse.next({
    request: {
      headers: request.headers,
    },
  });


  // Use server-side env vars (without NEXT_PUBLIC_ prefix)
  const supabaseUrl = process.env.SUPABASE_URL;
  const supabaseAnonKey = process.env.SUPABASE_ANON_KEY;

  if (!supabaseUrl || !supabaseAnonKey) {
    console.error('[Middleware] Missing Supabase URL or Anon Key.');
    return response;
  }

  // Create server client for middleware using createServerClient
  const supabase = createServerClient(
    supabaseUrl,
    supabaseAnonKey,
    {
      cookies: {
        get(name: string) {
          return request.cookies.get(name)?.value;
        },
        set(name: string, value: string, options: CookieOptions) {
          // If the cookie is set, update the request header
          // This is necessary for server components to share the session
          request.cookies.set({
            name,
            value,
            ...options,
          });
          response = NextResponse.next({
            request: {
              headers: request.headers,
            },
          });
          response.cookies.set({
            name,
            value,
            ...options,
          });
        },
        remove(name: string, options: CookieOptions) {
          // If the cookie is removed, update the request header
          request.cookies.set({
            name,
            value: '',
            ...options,
          });
          response = NextResponse.next({
            request: {
              headers: request.headers,
            },
          });
          response.cookies.set({
            name,
            value: '',
            ...options,
          });
        },
      },
    }
  );

  // Refresh session if expired - `getUser` will handle this automatically
  const { data: { user }, error: getUserError } = await supabase.auth.getUser();

  // Log user and error status
  if (getUserError) {
    console.error(`[Middleware] getUser Error: ${getUserError.message}`);
  }

  const unauthenticatedPaths = ['/login', '/signup'];

  // If user is not signed in and the current path is not /login or /signup, redirect the user to /login
  if (!user && !unauthenticatedPaths.includes(request.nextUrl.pathname)) {
    console.log(`[Middleware] Redirecting unauthenticated user from ${request.nextUrl.pathname} to /login`); // Log redirect
    return NextResponse.redirect(new URL('/login', request.url));
  }

  // If user is signed in and the current path IS /login or /signup, redirect the user to /
  if (user && unauthenticatedPaths.includes(request.nextUrl.pathname)) {
    console.log(`[Middleware] Redirecting authenticated user from ${request.nextUrl.pathname} to /`); // Log redirect
    return NextResponse.redirect(new URL('/', request.url));
  }


  // Return the response object, potentially modified by the Supabase client
  return response;
}

export const config = {
  matcher: [
    /*
     * Match all request paths except for the ones starting with:
     * - _next/static (static files)
     * - _next/image (image optimization files)
     * - favicon.ico (favicon file)
     * - images/
     * - assets/
     * - api/ (API routes)
     */
    '/((?!_next/static|_next/image|favicon.ico|images|assets|api).*)',
  ],
}; 