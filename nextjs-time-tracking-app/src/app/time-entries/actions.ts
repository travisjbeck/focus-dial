"use server";

import { revalidatePath } from "next/cache";
import { cookies } from "next/headers";
import { createActionSupabaseClient } from "@/utils/supabase";

// Type for the update payload
export type UpdateTimeEntryPayload = {
  projectId: number | null; // Allow unassigning from a project
  description: string | null;
};

// Type for the server action response
export type ActionResponse<T = null> = {
  success: boolean;
  data?: T;
  error?: {
    message: string;
    fieldErrors?: Record<string, string>; // Optional field-specific errors
  };
};

export async function updateTimeEntry(
  entryId: number,
  formData: FormData
): Promise<ActionResponse> {
  const cookieStore = cookies();
  const supabase = createActionSupabaseClient(cookieStore);

  // 1. Get User
  const {
    data: { user },
    error: userError,
  } = await supabase.auth.getUser();

  if (userError || !user) {
    console.error("Update Time Entry: Auth error", userError);
    return { success: false, error: { message: "Authentication failed." } };
  }

  // 2. Extract and Validate Data
  const rawProjectId = formData.get("projectId");
  const description = formData.get("description") as string | null;

  let projectId: number | null = null;
  if (rawProjectId && rawProjectId !== "none") {
    const parsedId = parseInt(rawProjectId as string, 10);
    if (isNaN(parsedId)) {
      return {
        success: false,
        error: {
          message: "Invalid project selected.",
          fieldErrors: { projectId: "Please select a valid project." },
        },
      };
    }
    projectId = parsedId;
  }

  const payload: UpdateTimeEntryPayload = {
    // Ensure projectId is null if "none" was selected or if it was empty
    projectId: projectId,
    // Trim description, use null if empty string
    description: description?.trim() || null,
  };

  // 3. Perform Update
  const {
    data: updatedEntry,
    error: updateError,
  } = await supabase
    .from("time_entries")
    .update({
      project_id: payload.projectId,
      description: payload.description,
      // Do NOT update start_time, end_time, or duration here
    })
    .eq("id", entryId)
    .eq("user_id", user.id) // Ensure user owns the entry
    .select("id") // Select something to confirm update occurred
    .single();

  if (updateError || !updatedEntry) {
    console.error("Update Time Entry: Supabase error", updateError);
    return {
      success: false,
      error: {
        message:
          updateError?.message ||
          "Failed to update time entry. It might have been deleted or you don't have permission.",
      },
    };
  }

  // 4. Revalidate Paths
  // Revalidate the specific entry page, the edit page, the main entries list,
  // and potentially the relevant project page if a project was added/changed.
  revalidatePath(`/time-entries/${entryId}`);
  revalidatePath(`/time-entries/${entryId}/edit`);
  revalidatePath("/entries");
  if (payload.projectId) {
    revalidatePath(`/projects/${payload.projectId}`);
  }
  // TODO: If the project *changed*, we might need to revalidate the *old* project page too.
  // This requires fetching the old entry first, which adds complexity.
  // For now, revalidating the main pages covers most cases.

  // 5. Return Success
  console.log(`Successfully updated time entry ${entryId}`);
  return { success: true };
} 