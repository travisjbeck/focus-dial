import { NextRequest, NextResponse } from 'next/server';
import { createServerClient } from '@supabase/ssr';
import { cookies } from 'next/headers'; // Import cookies
import type { CookieOptions } from '@supabase/ssr';
import type { Database } from '@/types/supabase';

// Helper function to create Supabase client for Route Handlers
const createRouteHandlerClient = () => {
  const cookieStore = cookies();
  return createServerClient<Database>(
    process.env.SUPABASE_URL!,
    process.env.SUPABASE_ANON_KEY!,
    {
      cookies: {
        get(name: string) {
          return cookieStore.get(name)?.value;
        },
        set(name: string, value: string, options: CookieOptions) {
          cookieStore.set({ name, value, ...options });
        },
        remove(name: string, options: CookieOptions) {
          cookieStore.set({ name, value: '', ...options });
        },
      },
    }
  );
};

// GET /api/projects - *** NEEDS REFACTORING TO USE SUPABASE & RLS ***
export async function GET() {
  // Placeholder - Needs to fetch from Supabase for the logged-in user
  const supabase = createRouteHandlerClient();
  const { data: { user } } = await supabase.auth.getUser();

  if (!user) {
    return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
  }

  try {
    // Fetch projects for the current user 
    // Supabase RLS should handle filtering if policies are set correctly
    const { data: projects, error } = await supabase
      .from('projects')
      .select('*')
      .order('created_at', { ascending: false });

    if (error) throw error;

    return NextResponse.json(projects || []);
  } catch (error: unknown) {
    console.error('Error fetching projects:', error);
    const message = error instanceof Error ? error.message : 'Unknown error';
    return NextResponse.json(
      { error: 'Internal server error', details: message },
      { status: 500 }
    );
  }
}

// POST /api/projects - Create a new project for the logged-in user
export async function POST(request: NextRequest) {
  const supabase = createRouteHandlerClient();

  // 1. Get User
  const { data: { user }, error: userError } = await supabase.auth.getUser();

  if (userError || !user) {
    console.error('Error getting user or no user found:', userError);
    return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
  }

  try {
    // 2. Parse Request Body
    const { name, color } = await request.json();

    // Validate required fields
    if (!name || !color) {
      return NextResponse.json(
        { error: 'Missing required fields: name, color' },
        { status: 400 }
      );
    }

    // 3. Insert into Supabase, including user_id
    const { data: createdProject, error: insertError } = await supabase
      .from('projects')
      .insert([{
        name: name,
        color: color,
        user_id: user.id // Associate with the logged-in user
      }])
      .select() // Select the newly created row
      .single(); // Expect only one row back

    if (insertError) {
      console.error('Supabase Insert Error:', insertError);
      // Provide more specific error if possible (e.g., duplicate name constraint?)
      return NextResponse.json(
        { error: 'Failed to create project in database', details: insertError.message },
        { status: 500 }
      );
    }

    // 4. Return Success Response
    return NextResponse.json(createdProject, { status: 201 });

  } catch (error: unknown) {
    // Catch potential JSON parsing errors or other unexpected errors
    console.error('Error creating project:', error);
    const message = error instanceof Error ? error.message : 'Unknown error';
    return NextResponse.json(
      { error: 'Internal server error', details: message },
      { status: 500 }
    );
  }
} 