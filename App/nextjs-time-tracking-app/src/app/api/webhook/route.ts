import { NextRequest, NextResponse } from 'next/server';
import { createClient } from '@supabase/supabase-js'; // Import standard client
import type { Database } from '@/types/supabase';
import crypto from 'crypto'; // Import crypto for hashing

// Remove old db imports
// import { ... } from '@/lib/db';

interface WebhookPayload {
  action: 'start_timer' | 'stop_timer';
  device_project_id: string; // Unique ID from the device
  project_name: string;      // Project name from the device
  project_color: string;     // Project color from the device
  description?: string;      // Optional description
}

// Initialize Supabase client for server-side operations (e.g., webhook)
// Use Service Role Key for elevated privileges to bypass RLS after API key auth.
// IMPORTANT: Store SUPABASE_SERVICE_ROLE_KEY securely in environment variables,
// DO NOT expose it publicly.
const supabaseUrl = process.env.SUPABASE_URL!;
// const supabaseAnonKey = process.env.SUPABASE_ANON_KEY!;
const supabaseServiceRoleKey = process.env.SUPABASE_SERVICE_ROLE_KEY!;
// const supabase = createClient<Database>(supabaseUrl, supabaseAnonKey);
const supabase = createClient<Database>(supabaseUrl, supabaseServiceRoleKey, {
  // Required for service role key: bypass RLS and identify as service role
  auth: {
    autoRefreshToken: false,
    persistSession: false,
    detectSessionInUrl: false
  }
});

// --- Helper Function: Get User ID from API Key ---
async function getUserIdFromApiKey(apiKey: string): Promise<string | null> {
  if (!apiKey) return null;

  // Log the exact key received before hashing
  console.log(`[getUserIdFromApiKey] Received key to hash: "${apiKey}" (Length: ${apiKey.length})`);

  try {
    // Hash the provided key using the same method as when stored
    // IMPORTANT: Use a strong, consistent hashing algorithm (e.g., SHA256)
    const hash = crypto.createHash('sha256').update(apiKey).digest('hex');

    // Query the api_keys table
    const { data, error } = await supabase
      .from('api_keys')
      .select('user_id')
      .eq('key_hash', hash)
      .single();

    if (error || !data) {
      // Improved logging for debugging invalid keys
      if (error && error.code !== 'PGRST116') { // PGRST116 = 'Searched for a single row, but 0 rows were found'
        console.error("[getUserIdFromApiKey] API Key lookup database error:", error);
      } else {
        console.log(`[getUserIdFromApiKey] API Key not found for hash: ${hash}`);
      }
      return null;
    }

    // TODO: Potentially update last_used_at for the key here (async)

    return data.user_id; // Return the associated user_id
  } catch (err) {
    console.error("[getUserIdFromApiKey] Error during API key validation:", err);
    return null;
  }
}
// --- End Helper Function ---


export async function POST(req: NextRequest) {
  // 1. Authenticate using API Key
  const authHeader = req.headers.get('Authorization');
  const apiKey = authHeader?.startsWith('Bearer ') ? authHeader.substring(7) : null;

  if (!apiKey) {
    return NextResponse.json({ error: 'Missing API Key' }, { status: 401 });
  }

  const userId = await getUserIdFromApiKey(apiKey);
  if (!userId) {
    return NextResponse.json({ error: 'Invalid API Key' }, { status: 401 });
  }

  // User is authenticated via API Key, proceed...
  console.log(`[Webhook] Authenticated user: ${userId}`);

  try {
    const body = await req.json();
    console.log('[Webhook] Received payload:', body);

    // Extract data based on the new WebhookPayload interface
    const { action, device_project_id, project_name, project_color, description } = body as WebhookPayload;

    // Validate required fields from the device
    if (!action || !device_project_id || !project_name || !project_color) {
      return NextResponse.json(
        { error: 'Missing required fields: action, device_project_id, project_name, project_color' },
        { status: 400 }
      );
    }

    // --- Find or Create Project Logic ---
    let dbProjectId: number;

    // 2. Find Project by device_project_id and user_id
    const { data: existingProject, error: findProjectError } = await supabase
      .from('projects')
      .select('id') // Only select the database ID
      .eq('user_id', userId)
      .eq('device_project_id', device_project_id)
      .maybeSingle();

    if (findProjectError) {
      console.error('[Webhook] Error finding project:', findProjectError);
      return NextResponse.json({ error: 'Database error checking project' }, { status: 500 });
    }

    if (existingProject) {
      // 3a. Project Found - Use existing database ID
      dbProjectId = existingProject.id;
      console.log(`[Webhook] Found existing project. DB ID: ${dbProjectId} for Device ID: ${device_project_id}`);
    } else {
      // 3b. Project Not Found - Create it
      console.log(`[Webhook] Project with Device ID: ${device_project_id} not found for user ${userId}. Creating...`);
      const { data: newProject, error: createProjectError } = await supabase
        .from('projects')
        .insert({
          user_id: userId,
          device_project_id: device_project_id,
          name: project_name,
          color: project_color,
        })
        .select('id') // Select the new database ID
        .single(); // Expect a single row back

      if (createProjectError || !newProject) {
        console.error('[Webhook] Error creating project:', createProjectError);
        // Handle potential unique constraint violation if race condition occurs
        if (createProjectError?.code === '23505') { // Unique violation
          return NextResponse.json({ error: 'Project creation conflict, please retry.' }, { status: 409 });
        }
        return NextResponse.json({ error: 'Failed to create project' }, { status: 500 });
      }

      dbProjectId = newProject.id;
      console.log(`[Webhook] Created new project. DB ID: ${dbProjectId} for Device ID: ${device_project_id}`);
    }
    // --- End Find or Create Project Logic ---

    // Use server's current time
    const now = new Date();
    const nowISO = now.toISOString();

    // 4. Perform Action (Start/Stop Timer) using the dbProjectId
    if (action === 'start_timer') {
      // Create a new time entry associated with the user and the *found/created* project
      const { data: newEntry, error: insertError } = await supabase
        .from('time_entries')
        .insert({
          project_id: dbProjectId, // Use the database project ID
          user_id: userId,
          start_time: nowISO,
          description: description || null // Use description from payload if provided
        })
        .select('id') // Only select ID back
        .single();

      if (insertError || !newEntry) {
        console.error("[Webhook] Error creating time entry:", insertError);
        return NextResponse.json({ error: 'Failed to start timer' }, { status: 500 });
      }

      console.log(`[Webhook] Timer started for user ${userId}, DB project ${dbProjectId}, entry ${newEntry.id}`);
      return NextResponse.json({
        success: true,
        message: 'Timer started',
        entry_id: newEntry.id
      });

    } else if (action === 'stop_timer') {
      // Find the most recent active entry for this user/project (using dbProjectId)
      const { data: activeEntry, error: findError } = await supabase
        .from('time_entries')
        .select('*') // Select all needed fields
        .eq('user_id', userId)
        .eq('project_id', dbProjectId) // Use the database project ID
        .is('end_time', null) // Look for entries where end_time is null
        .order('start_time', { ascending: false }) // Get the latest one
        .limit(1)
        .maybeSingle(); // It's possible no active timer exists

      if (findError) {
        console.error("[Webhook] Error finding active time entry:", findError);
        return NextResponse.json({ error: 'Error finding active timer' }, { status: 500 });
      }

      if (!activeEntry) {
        // It's possible a start signal was missed, or stop sent twice.
        console.warn(`[Webhook] No active timer found to stop for user ${userId}, DB project ${dbProjectId}.`);
        return NextResponse.json({ error: 'No active timer found for this project' }, { status: 404 });
      }

      // Calculate duration
      const startTime = new Date(activeEntry.start_time); // start_time should not be null here
      const durationMs = now.getTime() - startTime.getTime();
      const durationSec = Math.floor(durationMs / 1000);

      // Update the entry with end time and duration
      const { error: updateError } = await supabase
        .from('time_entries')
        .update({
          end_time: nowISO,
          duration: durationSec,
          // Optionally update description if provided in stop payload?
          description: description || activeEntry.description // Keep existing if not provided
        })
        .eq('id', activeEntry.id);
      // No project_id/user_id check needed here as we selected based on them already

      if (updateError) {
        console.error("[Webhook] Error updating time entry:", updateError);
        return NextResponse.json({ error: 'Failed to stop timer' }, { status: 500 });
      }

      console.log(`[Webhook] Timer stopped for user ${userId}, DB project ${dbProjectId}, entry ${activeEntry.id}`);
      return NextResponse.json({
        success: true,
        message: 'Timer stopped',
        entry_id: activeEntry.id,
        duration: durationSec
      });
    }

    // Should not be reached if action is validated earlier, but keep as fallback
    return NextResponse.json({ error: 'Invalid action specified' }, { status: 400 });

  } catch (error: unknown) {
    // Catch potential JSON parsing errors or other unexpected issues
    console.error('[Webhook] Error processing webhook:', error);
    const message = error instanceof Error ? error.message : 'Unknown server error';
    // Avoid leaking sensitive details in production
    const responseBody = process.env.NODE_ENV === 'production'
      ? { error: 'Internal server error' }
      : { error: 'Internal server error', details: message };
    return NextResponse.json(responseBody, { status: 500 });
  }
}

// Keep GET for verification
export async function GET() {
  return NextResponse.json({
    status: 'ready',
    message: 'Focus Dial webhook endpoint is ready to receive data (Supabase backend)',
    timestamp: new Date().toISOString()
  });
} 