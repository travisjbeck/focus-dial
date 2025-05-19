"use server";

import { revalidatePath } from "next/cache";
import { cookies } from "next/headers";
import { createActionSupabaseClient } from "@/utils/supabase";
import { differenceInSeconds } from 'date-fns'; // Import date-fns function

// Type for the update payload - adjusted for new fields
export type UpdateTimeEntryPayload = {
  projectId: number | null;
  description: string | null;
  start_time?: string; // ISO String
  end_time?: string | null; // ISO String or null
  duration?: number | null; // Seconds or null
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
  const startTimeStr = formData.get("startTime") as string | null;
  const endTimeStr = formData.get("endTime") as string | null; // Might be null if cleared

  const fieldErrors: Record<string, string> = {};
  let payload: Partial<UpdateTimeEntryPayload> = {}; // Use Partial for building

  // --- Project ID Validation ---
  let projectId: number | null = null;
  if (rawProjectId && rawProjectId !== "none") {
    const parsedId = parseInt(rawProjectId as string, 10);
    if (isNaN(parsedId)) {
      fieldErrors.projectId = "Please select a valid project.";
    }
    projectId = parsedId;
  }
  payload.projectId = projectId;

  // --- Description ---
  payload.description = description?.trim() || null;

  // --- Start Time Validation ---
  let startTime: Date | null = null;
  if (!startTimeStr) {
    fieldErrors.startTime = "Start time is required.";
  } else {
    startTime = new Date(startTimeStr);
    if (isNaN(startTime.getTime())) {
      fieldErrors.startTime = "Invalid start time format.";
      startTime = null; // Prevent further use of invalid date
    } else {
      payload.start_time = startTime.toISOString();
    }
  }

  // --- End Time Validation ---
  let endTime: Date | null = null;
  if (endTimeStr) {
    endTime = new Date(endTimeStr);
    if (isNaN(endTime.getTime())) {
      fieldErrors.endTime = "Invalid end time format.";
      endTime = null; // Prevent further use
    } else {
      payload.end_time = endTime.toISOString();
    }
  } else {
    // If endTimeStr is explicitly null or empty, set payload.end_time to null
    payload.end_time = null;
  }

  // --- Cross-Field Validation (Start vs End) ---
  if (startTime && endTime) {
    // --- Temporary Debug Logging ---
    console.log("[Action Debug] Comparing Dates:");
    console.log(`[Action Debug] Start Time: ${startTime.toISOString()}, (${startTime.getTime()})`);
    console.log(`[Action Debug] End Time:   ${endTime.toISOString()}, (${endTime.getTime()})`);
    console.log(`[Action Debug] Comparison (endTime <= startTime): ${endTime <= startTime}`);
    // --- End Temporary Debug Logging ---

    if (endTime <= startTime) {
      fieldErrors.endTime = "End time must be after start time.";
    }
  }

  // --- Duration Calculation ---
  if (startTime && endTime && !fieldErrors.startTime && !fieldErrors.endTime && endTime > startTime) {
    payload.duration = differenceInSeconds(endTime, startTime);
  } else {
      // If end time is null or invalid, duration should be null
      payload.duration = null;
  }

  // --- Return if validation errors ---
  if (Object.keys(fieldErrors).length > 0) {
    return {
      success: false,
      error: {
        message: "Validation failed. Please check the form.",
        fieldErrors,
      },
    };
  }

  // 3. Perform Update (only include fields that are being updated)
  // Use Supabase column names for the update object keys
  const updateData: { [key: string]: any } = {}; // Use a more generic type for buildin
  if (payload.hasOwnProperty('projectId')) updateData.project_id = payload.projectId; 
  if (payload.hasOwnProperty('description')) updateData.description = payload.description;
  if (payload.hasOwnProperty('start_time')) updateData.start_time = payload.start_time;
  if (payload.hasOwnProperty('end_time')) updateData.end_time = payload.end_time;
  if (payload.hasOwnProperty('duration')) updateData.duration = payload.duration;

  // Ensure we are actually updating something
  if (Object.keys(updateData).length === 0) {
       console.log("No changes detected for time entry", entryId);
       return { success: true }; // Or return an error/message?
  }

  const {
    data: updatedEntry,
    error: updateError,
  } = await supabase
    .from("time_entries")
    .update(updateData) // Use the dynamically built update object
    .eq("id", entryId)
    .eq("user_id", user.id) // Ensure user owns the entry
    .select("id") // Select something to confirm update occurred
    .single();

  if (updateError || !updatedEntry) {
    console.error("Update Time Entry: Supabase error", updateError);
    // Try to provide a more specific error message if possible
    let message = updateError?.message || "Failed to update time entry. It might have been deleted or you don't have permission.";
    // Example: Check for specific Supabase error codes if needed
    // if (updateError?.code === '23514') { // Check constraint violation
    //   message = "Update violates database constraints (e.g., end time before start time).";
    // }
    return {
      success: false,
      error: { message },
    };
  }

  // 4. Revalidate Paths
  // Revalidate the specific entry page, the edit page, the main entries list,
  // and potentially the relevant project page if a project was added/changed.
  // Revalidate dashboard if duration changed.
  revalidatePath(`/time-entries/${entryId}`);
  revalidatePath(`/time-entries/${entryId}/edit`);
  revalidatePath("/entries");
  revalidatePath("/"); // Revalidate dashboard page as duration/times might change display
  if (payload.projectId) {
    revalidatePath(`/projects/${payload.projectId}`);
  }
  // TODO: If the project *changed*, we might need to revalidate the *old* project page too.
  // Fetching the old entry adds complexity, maybe handle later.

  // 5. Return Success
  console.log(`Successfully updated time entry ${entryId}`);
  return { success: true };
} 