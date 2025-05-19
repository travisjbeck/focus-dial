'use server';

import { revalidatePath } from 'next/cache';
import { cookies } from 'next/headers';
import { createActionSupabaseClient } from '@/utils/supabase';
import { type Database } from '@/types/supabase';
import crypto from 'crypto';

// Define the type for API Key metadata (excluding the hash)
export type ApiKeyMetadata = Omit<Database["public"]["Tables"]["api_keys"]["Row"], 'key_hash'>;

// --- Fetch API Keys --- //
export async function fetchApiKeys(): Promise<{
  success: boolean;
  data?: ApiKeyMetadata[];
  error?: string;
}> {
  const cookieStore = cookies();
  const supabase = createActionSupabaseClient(cookieStore);
  const { data: { user } } = await supabase.auth.getUser();

  if (!user) {
    console.error("fetchApiKeys: No user found.");
    return { success: false, error: "Unauthorized" };
  }

  try {
    // Select all fields except the key_hash for security
    const { data, error } = await supabase
      .from('api_keys')
      .select('id, user_id, key_name, created_at, last_used_at')
      .eq('user_id', user.id)
      .order('created_at', { ascending: false });

    if (error) throw error;

    return { success: true, data: data || [] };
  } catch (error: unknown) {
    const message = error instanceof Error ? error.message : "Unknown error fetching API keys";
    console.error("Error fetching API keys:", message);
    return { success: false, error: message };
  }
}

// --- Generate API Key --- //
interface GenerateApiKeyArgs {
  name: string;
}
export async function generateApiKey(args: GenerateApiKeyArgs): Promise<{
  success: boolean;
  apiKey?: string; // The raw key (only returned on success)
  error?: string;
}> {
  const cookieStore = cookies();
  const supabase = createActionSupabaseClient(cookieStore);
  const { data: { user } } = await supabase.auth.getUser();

  if (!user) {
    console.error("generateApiKey: No user found.");
    return { success: false, error: "Unauthorized" };
  }

  if (!args.name?.trim()) {
    return { success: false, error: "API Key name cannot be empty." };
  }

  try {
    // 1. Generate a secure random API key
    const apiKey = crypto.randomBytes(32).toString('hex'); // 64 characters long

    // 2. Hash the key for storage (using SHA-256)
    const keyHash = crypto.createHash('sha256').update(apiKey).digest('hex');

    // 3. Insert the hash and metadata into the database
    const { error: insertError } = await supabase
      .from('api_keys')
      .insert({
        user_id: user.id,
        key_name: args.name.trim(),
        key_hash: keyHash
      });

    if (insertError) {
      // Handle potential unique constraint violation (e.g., duplicate key_hash or user_id/key_name combo)
      if (insertError.code === '23505') { // Postgres unique violation code
        console.error("Error inserting API key (unique constraint violation):", insertError);
        return { success: false, error: "Failed to generate key. A key with this name might already exist, or a hash collision occurred (rare)." };
      }
      throw insertError; // Re-throw other insert errors
    }

    // Revalidate the settings path so the new key appears in the list
    revalidatePath('/settings/api-keys');

    // Return SUCCESS along with the RAW API key for the user to copy
    return { success: true, apiKey: apiKey };

  } catch (error: unknown) {
    const message = error instanceof Error ? error.message : "Unknown error generating API key";
    console.error("Error generating API key:", message);
    return { success: false, error: message };
  }
}

// --- Revoke API Key --- //
export async function revokeApiKey(apiKeyId: number): Promise<{
  success: boolean;
  error?: string;
}> {
  const cookieStore = cookies();
  const supabase = createActionSupabaseClient(cookieStore);
  const { data: { user } } = await supabase.auth.getUser();

  if (!user) {
    console.error("revokeApiKey: No user found.");
    return { success: false, error: "Unauthorized" };
  }

  try {
    const { error } = await supabase
      .from('api_keys')
      .delete()
      .eq('id', apiKeyId)
      .eq('user_id', user.id); // Ensure user owns the key they are deleting

    if (error) {
      throw error;
    }

    // Revalidate the path to update the list
    revalidatePath('/settings/api-keys');

    return { success: true };

  } catch (error: unknown) {
    const message = error instanceof Error ? error.message : "Unknown error revoking API key";
    console.error("Error revoking API key:", message);
    return { success: false, error: message };
  }
} 