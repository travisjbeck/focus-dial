import { NextRequest, NextResponse } from 'next/server';
import { createServerClient } from '@supabase/ssr';
import { cookies } from 'next/headers';
import type { CookieOptions } from '@supabase/ssr';
import type { Database } from '@/types/supabase';

// Helper function (can be shared if used in multiple route files)
const createRouteHandlerClient = () => {
  const cookieStore = cookies();
  return createServerClient<Database>(
    process.env.SUPABASE_URL!,
    process.env.SUPABASE_ANON_KEY!,
    {
      cookies: {
        get(name: string) { return cookieStore.get(name)?.value; },
        set(name: string, value: string, options: CookieOptions) { cookieStore.set({ name, value, ...options }); },
        remove(name: string, options: CookieOptions) { cookieStore.set({ name, value: '', ...options }); },
      },
    }
  );
};

// GET /api/projects/[id] - Get a specific project for the user
export async function GET(
  request: NextRequest,
  { params }: { params: { id: string } }
) {
  const supabase = createRouteHandlerClient();
  const projectId = parseInt(params.id, 10);

  if (isNaN(projectId)) {
    return NextResponse.json({ error: 'Invalid project ID' }, { status: 400 });
  }

  // Get User
  const { data: { user }, error: userError } = await supabase.auth.getUser();
  if (userError || !user) {
    return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
  }

  try {
    const { data: project, error: dbError } = await supabase
      .from('projects')
      .select('*')
      .eq('id', projectId)
      .eq('user_id', user.id)
      .single();

    if (dbError) {
      // Handle potential errors like project not found for user (PGRST116)
      console.error('Error fetching project by ID:', dbError);
      if (dbError.code === 'PGRST116') { // PostgREST code for "exact number of rows not returned"
        return NextResponse.json({ error: 'Project not found or access denied' }, { status: 404 });
      }
      throw dbError; // Re-throw other errors
    }

    if (!project) { // Should be redundant due to PGRST116 check, but safe practice
      return NextResponse.json({ error: 'Project not found' }, { status: 404 });
    }

    return NextResponse.json(project);

  } catch (error: unknown) {
    console.error('Error getting project:', error);
    const message = error instanceof Error ? error.message : 'Unknown server error';
    return NextResponse.json({ error: 'Internal server error', details: message }, { status: 500 });
  }
}

// PUT /api/projects/[id] - Update a specific project for the user
export async function PUT(
  request: NextRequest,
  { params }: { params: { id: string } }
) {
  const supabase = createRouteHandlerClient();
  const projectId = parseInt(params.id, 10);

  if (isNaN(projectId)) {
    return NextResponse.json({ error: 'Invalid project ID' }, { status: 400 });
  }

  // Get User
  const { data: { user }, error: userError } = await supabase.auth.getUser();
  if (userError || !user) {
    return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
  }

  try {
    // Parse body
    const { name, color } = await request.json();

    // Validate
    if (!name?.trim() || !color) {
      return NextResponse.json({ error: 'Missing required fields: name, color' }, { status: 400 });
    }

    // Update project in Supabase, ensuring ownership
    const { data: updatedProject, error: updateError } = await supabase
      .from('projects')
      .update({ name: name.trim(), color: color })
      .eq('id', projectId)
      .eq('user_id', user.id)
      .select()
      .single();

    if (updateError) {
      // Handle potential errors like project not found for user (PGRST116)
      console.error('Error updating project:', updateError);
      if (updateError.code === 'PGRST116') {
        return NextResponse.json({ error: 'Project not found or access denied' }, { status: 404 });
      }
      throw updateError;
    }

    return NextResponse.json(updatedProject);

  } catch (error: unknown) {
    console.error('Error updating project:', error);
    const message = error instanceof Error ? error.message : 'Unknown server error';
    return NextResponse.json({ error: 'Internal server error', details: message }, { status: 500 });
  }
}

// DELETE /api/projects/[id] - Delete a specific project for the user
export async function DELETE(
  request: NextRequest, // request might not be needed but keep for signature
  { params }: { params: { id: string } }
) {
  const supabase = createRouteHandlerClient();
  const projectId = parseInt(params.id, 10);

  if (isNaN(projectId)) {
    return NextResponse.json({ error: 'Invalid project ID' }, { status: 400 });
  }

  // Get User
  const { data: { user }, error: userError } = await supabase.auth.getUser();
  if (userError || !user) {
    return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
  }

  try {
    // Delete project, ensuring ownership
    const { error: deleteError } = await supabase
      .from('projects')
      .delete()
      .eq('id', projectId)
      .eq('user_id', user.id);

    if (deleteError) {
      // Handle potential errors like project not found (no error thrown by default on delete)
      // RLS policy might prevent deletion, causing a different error.
      console.error('Error deleting project:', deleteError);
      // Check if the error indicates the row didn't exist or user didn't have permission
      // Supabase delete doesn't error if row doesn't match, might need a preceding select if 404 is strictly needed.
      // For now, assume any error is a server error or permission issue.
      return NextResponse.json({ error: 'Failed to delete project', details: deleteError.message }, { status: 500 });
    }

    // Revalidate paths after successful deletion
    cookies().set('revalidateProjects', 'true'); // Simple flag for potential client-side revalidation trigger
    // Or use revalidatePath if running in a compatible environment (might not work reliably in all route handlers)
    // revalidatePath('/projects');
    // revalidatePath('/'); 

    return NextResponse.json({ success: true, id: projectId });

  } catch (error: unknown) {
    console.error('Error deleting project:', error);
    const message = error instanceof Error ? error.message : 'Unknown server error';
    return NextResponse.json({ error: 'Internal server error', details: message }, { status: 500 });
  }
} 