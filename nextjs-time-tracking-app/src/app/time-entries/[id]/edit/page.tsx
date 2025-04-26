"use client";

import { useState, useEffect, useTransition } from "react";
import { useRouter, useParams } from "next/navigation";
import Link from "next/link";
import { createBrowserSupabaseClient } from "@/utils/supabase"; // Browser client for fetching
import { useProjects } from "@/lib/hooks/useProjects"; // Hook to get projects
import { updateTimeEntry } from "../../actions"; // Corrected import path (two levels up)
import type { Database } from "@/types/supabase";
import type { ActionResponse } from "../../actions"; // Corrected import path (two levels up)
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { DateTimePicker } from "@/components/ui/datetime-picker"; // Import DateTimePicker
import { Label } from "@/components/ui/label"; // Import Label for better form structure

// Assuming TimeEntry type is defined similarly in your types
// If not, define it based on your Supabase schema
type TimeEntry = Database["public"]["Tables"]["time_entries"]["Row"];
type Project = Database["public"]["Tables"]["projects"]["Row"];

export default function EditTimeEntryPage() {
  const router = useRouter();
  const params = useParams();
  const entryId = Number(params.id);

  const [timeEntry, setTimeEntry] = useState<TimeEntry | null>(null);
  const [selectedProjectId, setSelectedProjectId] = useState<string>("none");
  const [description, setDescription] = useState<string>("");
  const [startTime, setStartTime] = useState<Date | undefined>(undefined); // State for start time
  const [endTime, setEndTime] = useState<Date | undefined>(undefined); // State for end time
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [formErrors, setFormErrors] = useState<Record<string, string>>({});
  const [isPending, startTransition] = useTransition();

  // Use the hook to get projects for the dropdown
  const { data: projects, isLoading: isLoadingProjects } = useProjects();

  // Supabase client for initial data fetch
  const supabase = createBrowserSupabaseClient();

  useEffect(() => {
    if (isNaN(entryId)) {
      setError("Invalid Time Entry ID.");
      setIsLoading(false);
      return;
    }

    const fetchTimeEntry = async () => {
      setIsLoading(true);
      setError(null);
      setFormErrors({}); // Clear previous errors

      const { data: sessionData, error: sessionError } = await supabase.auth.getSession();
      if (sessionError || !sessionData.session) {
        setError("Authentication failed. Please log in.");
        setIsLoading(false);
        // Optionally redirect to login
        // router.push('/login');
        return;
      }

      const {
        data,
        error: fetchError,
      } = await supabase
        .from("time_entries")
        // Select all fields needed by the TimeEntry type and the form
        .select("id, project_id, description, start_time, end_time, created_at, duration, user_id")
        .eq("id", entryId)
        .eq("user_id", sessionData.session.user.id)
        .single();

      if (fetchError || !data) {
        console.error("Error fetching time entry:", fetchError);
        setError("Failed to load time entry data. It might not exist or you don't have permission.");
        setTimeEntry(null);
      } else {
        setTimeEntry(data);
        setSelectedProjectId(data.project_id?.toString() ?? "none");
        setDescription(data.description ?? "");
        // Initialize date states from fetched data
        setStartTime(data.start_time ? new Date(data.start_time) : undefined);
        setEndTime(data.end_time ? new Date(data.end_time) : undefined);
      }
      setIsLoading(false);
    };

    fetchTimeEntry();
  }, [entryId, supabase, router]);

  const handleSubmit = async (event: React.FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    const currentFormErrors: Record<string, string> = {};
    setError(null);

    // Basic validation
    if (!startTime) {
        currentFormErrors.startTime = "Start time is required.";
    }
    if (startTime && endTime && endTime <= startTime) {
        currentFormErrors.endTime = "End time must be after start time.";
    }

    setFormErrors(currentFormErrors);

    if (Object.keys(currentFormErrors).length > 0) {
        return; // Don't submit if there are errors
    }

    if (!timeEntry) return;

    startTransition(async () => {
      const formData = new FormData(event.currentTarget);

      // Add date values manually IF they exist (don't add null/undefined)
      // FormData expects string values, convert Date to ISO string
      if (startTime) {
        formData.set("startTime", startTime.toISOString());
      }
      if (endTime) {
        formData.set("endTime", endTime.toISOString());
      } else {
         // Explicitly handle case where end time might be cleared
         // If your action expects 'null' or simply absence, adjust accordingly
         // Here, we'll just ensure it's not sent if undefined/cleared in UI
         formData.delete("endTime");
         // If you need to explicitly pass null: formData.set("endTime", "null");
      }


      const result: ActionResponse = await updateTimeEntry(entryId, formData);

      if (result.success) {
        // Successfully updated, maybe show a success message
        // and redirect back to the details page or entries list
        // Use toast or similar for feedback
        router.push(`/time-entries/${entryId}`); // Redirect to view page
      } else {
        setError(result.error?.message || "An unexpected error occurred.");
        if (result.error?.fieldErrors) {
          setFormErrors(result.error.fieldErrors);
        }
      }
    });
  };

  const handleSelectChange = (value: string) => {
    setSelectedProjectId(value);
  };

  if (isLoading || isLoadingProjects) {
    return <div className="container mx-auto px-4 text-center py-10 text-gray-400">Loading...</div>;
  }

  if (error && !timeEntry && !isLoading) { // Show error only if loading finished and no entry
    return <div className="container mx-auto px-4 text-center py-10 text-red-500">Error: {error}</div>;
  }

  if (!timeEntry && !isLoading) { // Show not found only if loading finished and no entry
    // Should be covered by error state, but as a fallback
    return <div className="container mx-auto px-4 text-center py-10 text-gray-400">Time entry not found.</div>;
  }

  // Render form only if timeEntry is loaded
  return (
    <div className="container mx-auto px-4">
       <div className="mb-6 flex justify-between items-center">
        <h1 className="text-xl font-bold text-foreground">Edit Time Entry</h1>
        <Link
            href={`/time-entries/${entryId}`}
            className="px-3 py-1 text-xs font-medium text-secondary-foreground bg-secondary hover:bg-secondary/80 rounded-md border border-border"
        >
            Cancel
        </Link>
       </div>

      {error && !isPending && (
        <div className="bg-destructive border border-destructive text-destructive-foreground px-4 py-2 rounded-md mb-4 text-sm">
          {error}
        </div>
      )}

      {/* Only render form if we have the time entry data */}
      {timeEntry && (
          <form onSubmit={handleSubmit} className="bg-card p-6 rounded-lg shadow border border-border">
            <div className="mb-4 grid grid-cols-1 md:grid-cols-2 gap-6">
                <div>
                    <Label htmlFor="startTime" className="block text-sm font-medium text-muted-foreground mb-1">Start Time</Label>
                    <DateTimePicker date={startTime} setDate={setStartTime} disabled={isPending} />
                    {/* Hidden input to submit ISO string */}
                    <input type="hidden" name="startTimeValue" value={startTime ? startTime.toISOString() : ''} />
                    {formErrors.startTime && (
                        <p id="startTime-error" className="mt-1 text-xs text-red-500">{formErrors.startTime}</p>
                    )}
                </div>
                <div>
                    <Label htmlFor="endTime" className="block text-sm font-medium text-muted-foreground mb-1">End Time</Label>
                    <DateTimePicker date={endTime} setDate={setEndTime} disabled={isPending} />
                    {/* Hidden input to submit ISO string */}
                    <input type="hidden" name="endTimeValue" value={endTime ? endTime.toISOString() : ''} />
                    {/* Allow clearing end time maybe? Currently, undefined = not sent */}
                    {formErrors.endTime && (
                        <p id="endTime-error" className="mt-1 text-xs text-red-500">{formErrors.endTime}</p>
                    )}
                     {timeEntry.end_time === null && !endTime && (
                        <p className="mt-1 text-xs text-green-500">Entry is currently running. Setting an end time will stop it.</p>
                    )}
                </div>
            </div>

            <div className="mb-4">
              <Label htmlFor="projectId" className="block text-sm font-medium text-muted-foreground mb-1">Project</Label>
              {/* Keep hidden input for compatibility if needed, or rely solely on Select's value */}
              <input type="hidden" name="projectId" value={selectedProjectId} /> 
              <Select
                value={selectedProjectId}
                onValueChange={handleSelectChange}
                disabled={isPending}
                name="projectIdSelect" // Give select a name if hidden input is removed
              >
                <SelectTrigger className="w-full bg-background border-input text-foreground">
                  <SelectValue placeholder="Select a project" />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="none">-- No Project --</SelectItem>
                  {projects?.map((project: Project) => (
                    <SelectItem key={project.id} value={project.id.toString()}>
                      {project.name}
                    </SelectItem>
                  ))}
                </SelectContent>
              </Select>
              {formErrors.projectId && (
                  <p id="projectId-error" className="mt-1 text-xs text-red-500">{formErrors.projectId}</p>
              )}
            </div>

            <div className="mb-6">
              <Label htmlFor="description" className="block text-sm font-medium text-muted-foreground mb-1">Description</Label>
              <textarea
                id="description"
                name="description"
                rows={3}
                value={description}
                onChange={(e) => setDescription(e.target.value)}
                className="bg-background border border-input text-foreground text-sm rounded-md focus:ring-ring focus:border-input block w-full p-2.5 placeholder-muted-foreground"
                placeholder="(Optional) Add notes about this time entry..."
                aria-describedby={formErrors.description ? "description-error" : undefined}
                disabled={isPending}
              />
              {formErrors.description && (
                  <p id="description-error" className="mt-1 text-xs text-red-500">{formErrors.description}</p>
              )}
            </div>

            <div className="flex justify-end">
              <button
                type="submit"
                className="px-4 py-2 bg-primary text-primary-foreground text-sm font-medium rounded-md hover:bg-primary/90 disabled:opacity-50 disabled:cursor-not-allowed"
                disabled={isPending}
              >
                {isPending ? "Saving..." : "Save Changes"}
              </button>
            </div>
          </form>
      )}
    </div>
  );
} 