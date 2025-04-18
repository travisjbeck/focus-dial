'use server';

import { revalidatePath } from 'next/cache';
import { cookies } from 'next/headers';
import { redirect } from 'next/navigation';
import { createActionSupabaseClient } from '@/utils/supabase';
import { type Database } from '@/types/supabase';

type Project = Database["public"]["Tables"]["projects"]["Row"];

export async function login(formData: FormData) {
  const cookieStore = cookies();
  const supabase = createActionSupabaseClient(cookieStore);

  const email = formData.get('email') as string;
  const password = formData.get('password') as string;

  if (!email || !password) {
    return redirect('/login?error=Email+and+password+are+required');
  }

  const { error } = await supabase.auth.signInWithPassword({
    email,
    password,
  });

  if (error) {
    console.error("Login error:", error.message);
    return redirect('/login?error=Could+not+authenticate+user');
  }

  revalidatePath('/', 'layout');
  redirect('/');
}

export async function signup(formData: FormData) {
  const cookieStore = cookies();
  const supabase = createActionSupabaseClient(cookieStore);

  const email = formData.get('email') as string;
  const password = formData.get('password') as string;

  if (!email || !password) {
    return redirect('/login?error=Email+and+password+are+required+for+signup');
  }
  if (password.length < 6) {
    return redirect('/login?error=Password+must+be+at+least+6+characters');
  }

  const { error } = await supabase.auth.signUp({
    email,
    password,
    options: {
      // emailRedirectTo: 'http://localhost:3000/auth/callback', // Optional: for email confirmation link
    },
  });

  if (error) {
    console.error("Signup error:", error.message);
    if (error.message.includes("User already registered")) {
      return redirect('/login?error=User+already+exists.+Please+log+in.');
    }
    return redirect('/login?error=Could+not+authenticate+user+during+signup');
  }

  revalidatePath('/', 'layout');
  redirect('/?message=Check+email+to+complete+signup');
}

export async function logout() {
  const cookieStore = cookies();
  const supabase = createActionSupabaseClient(cookieStore);

  const { error } = await supabase.auth.signOut();

  if (error) {
    console.error('Logout error:', error.message);
    // Optionally redirect with an error message
    // return redirect('/?error=Could+not+log+out');
  }

  // Redirect to login page after successful logout or if error occurs (for simplicity)
  redirect('/login');
}

// Action to get a single project by ID for the current user
export async function getProjectById(projectId: number): Promise<Project | null> {
  const cookieStore = cookies();
  const supabase = createActionSupabaseClient(cookieStore);
  const { data: { user } } = await supabase.auth.getUser();

  if (!user) {
    console.error("getProjectById: No user found.");
    return null;
  }

  const { data: project, error } = await supabase
    .from('projects')
    .select('*')
    .eq('id', projectId)
    .eq('user_id', user.id)
    .single();

  if (error) {
    console.error('Error fetching project by ID:', error);
    return null;
  }

  return project;
}

// Action to update a project
interface UpdateProjectData {
  name: string;
  color: string;
}

export async function updateProject(
  projectId: number,
  formData: UpdateProjectData
): Promise<{ success: boolean; error?: string }> {
  const cookieStore = cookies();
  const supabase = createActionSupabaseClient(cookieStore);
  const { data: { user } } = await supabase.auth.getUser();

  if (!user) {
    console.error("updateProject: No user found.");
    return { success: false, error: "Unauthorized" };
  }

  // Validate input (basic)
  if (!formData.name?.trim() || !formData.color) {
    return { success: false, error: "Missing required fields: name, color" };
  }

  const { error } = await supabase
    .from('projects')
    .update({
      name: formData.name,
      color: formData.color
    })
    .eq('id', projectId)
    .eq('user_id', user.id); // IMPORTANT: Ensure user owns the project

  if (error) {
    console.error('Error updating project:', error);
    return { success: false, error: error.message };
  }

  // Optional: Revalidate relevant paths after update
  revalidatePath('/projects');
  revalidatePath(`/projects/${projectId}`);
  revalidatePath(`/projects/${projectId}/edit`);
  revalidatePath('/'); // Revalidate dashboard if it shows project info

  return { success: true };
}

// Action to delete a project
export async function deleteProject(
  projectId: number
): Promise<{ success: boolean; error?: string }> {
  const cookieStore = cookies();
  const supabase = createActionSupabaseClient(cookieStore);
  const { data: { user } } = await supabase.auth.getUser();

  if (!user) {
    console.error("deleteProject: No user found.");
    return { success: false, error: "Unauthorized" };
  }

  // Delete project, ensuring ownership
  const { error } = await supabase
    .from('projects')
    .delete()
    .eq('id', projectId)
    .eq('user_id', user.id);

  if (error) {
    console.error('Error deleting project:', error);
    // RLS might also block this, resulting in an error or 0 rows affected
    return { success: false, error: error.message };
  }

  // Revalidate relevant paths after successful deletion
  revalidatePath('/projects');
  revalidatePath('/'); // Revalidate dashboard
  // No need to revalidate the specific project page as it will be gone

  // Redirecting from a server action requires throwing the redirect error
  // Or handle redirection on the client after receiving the success response
  // For now, just return success. Client component can handle redirect.
  return { success: true };
}
