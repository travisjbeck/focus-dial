// Remove "use client"; Convert to Server Component

import Link from "next/link";
import { notFound } from "next/navigation"; // Import notFound
import { cookies } from "next/headers"; // Import cookies
import { createServerComponentSupabaseClient } from "@/utils/supabase"; // Import Supabase client
import DeleteProjectButton from "@/components/DeleteProjectButton"; // Import the new button component
import { formatDuration, formatDate } from "@/lib/utils/dateUtils"; // Import from utils
import { Button } from "@/components/ui/button"; // Import Button

// Remove client-side imports if no longer needed
// import { useState, useEffect } from "react";
// import { useRouter } from "next/navigation";
// import axios from "axios";

// Use types directly
// type Project = Database["public"]["Tables"]["projects"]["Row"]; // Remove unused
// type TimeEntry = Database["public"]["Tables"]["time_entries"]["Row"]; // Remove unused

interface ProjectDetailPageProps {
  params: {
    id: string; // Keep string type, Supabase handles conversion
  };
}

// Make the component async
export default async function ProjectDetailPage({
  params,
}: ProjectDetailPageProps) {
  // Remove client-side state and hooks
  // const router = useRouter();
  // const [project, setProject] = useState<Project | null>(null);
  // const [timeEntries, setTimeEntries] = useState<TimeEntry[]>([]);
  // const [isLoading, setIsLoading] = useState(true);
  // const [error, setError] = useState<string | null>(null);
  // const [isDeleting, setIsDeleting] = useState(false);
  // const [deleteError, setDeleteError] = useState<string | null>(null);

  // Fetch data on the server
  const cookieStore = cookies();
  const supabase = createServerComponentSupabaseClient(cookieStore);

  // Get user first
  const {
    data: { user },
  } = await supabase.auth.getUser();
  if (!user) {
    // This case should ideally be handled by middleware, but good to double-check
    console.error(
      "Project Detail: No user found, redirecting via middleware expected."
    );
    // You could potentially redirect here, but middleware is cleaner
    return (
      <div className="container mx-auto px-4 text-red-500">
        Unauthorized access.
      </div>
    ); // Or redirect
  }

  // Fetch project details
  const { data: project, error: projectError } = await supabase
    .from("projects")
    .select("*")
    .eq("id", Number(params.id)) // Convert params.id to number
    .eq("user_id", user.id) // Filter by user ID
    .single(); // Expect a single result

  // Fetch associated time entries
  const { data: timeEntries, error: entriesError } = await supabase
    .from("time_entries")
    .select("*")
    .eq("project_id", Number(params.id)) // Convert params.id to number for filtering
    .eq("user_id", user.id) // Filter by user ID
    .order("start_time", { ascending: false });

  // Handle errors or not found
  if (projectError || !project) {
    console.error(
      "Error fetching project details or project not found:",
      projectError
    );
    notFound(); // Use Next.js notFound function
  }
  if (entriesError) {
    // Log error but potentially still render project details
    console.error("Error fetching time entries for project:", entriesError);
    // Optionally set timeEntries to empty array or show an error message specific to entries
    // timeEntries = []; // For simplicity, let's allow render with empty entries on error
  }

  // Calculate project stats directly from fetched data
  const safeTimeEntries = timeEntries || []; // Ensure it's an array
  const totalEntries = safeTimeEntries.length;
  const totalDuration = safeTimeEntries.reduce(
    (total, entry) => total + (entry.duration || 0),
    0
  );
  const activeEntries = safeTimeEntries.filter((entry) => !entry.end_time);

  // Convert project ID to number for the button component
  // Since project exists, we can use project.id which is already a number
  const projectIdNumber = typeof project.id === 'string' ? parseInt(project.id, 10) : project.id;
  
  // Remove old loading/error checks based on useState
  // if (isLoading) { ... }
  // if (error || !project) { ... } // Handled above with notFound()

  return (
    <div className="container mx-auto px-4">
      <div className="mb-8">
        <div className="flex items-center justify-between mb-4">
          <div className="flex items-center">
            <div
              className="w-6 h-6 rounded-full mr-3"
              style={{ backgroundColor: project.color || "#808080" }}
            />
            <h1 className="text-xl font-bold text-foreground">{project.name}</h1>
          </div>
          <div className="flex space-x-2">
            <Link
              href="/projects"
              passHref // Required for Button inside Link
              legacyBehavior // Required for Button inside Link
            >
              <Button asChild variant="outline" size="sm">
                <a>Back</a>
              </Button>
            </Link>
            <Link
              href={`/projects/${project.id}/edit`}
              passHref // Required for Button inside Link
              legacyBehavior // Required for Button inside Link
            >
              <Button asChild variant="outline" size="sm">
                <a>Edit</a>
              </Button>
            </Link>
            <DeleteProjectButton projectId={projectIdNumber} />
          </div>
        </div>

        <div className="text-xs text-muted-foreground mb-4">
          Created: {formatDate(project.created_at)}
        </div>

        <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 mb-6">
          <div className="bg-card p-4 rounded-lg shadow border border-border">
            <h3 className="text-xs uppercase text-muted-foreground mb-1">Total Time</h3>
            <div className="text-2xl font-bold text-card-foreground">
              {formatDuration(totalDuration)}
            </div>
          </div>
          <div className="bg-card p-4 rounded-lg shadow border border-border">
            <h3 className="text-xs uppercase text-muted-foreground mb-1">
              Time Entries
            </h3>
            <div className="text-2xl font-bold text-card-foreground">{totalEntries}</div>
          </div>
          <div className="bg-card p-4 rounded-lg shadow border border-border">
            <h3 className="text-xs uppercase text-muted-foreground mb-1">
              Active Sessions
            </h3>
            <div className="text-2xl font-bold text-card-foreground">
              {activeEntries.length}
            </div>
          </div>
        </div>

        <div className="bg-card p-6 rounded-lg shadow border border-border">
          <h2 className="text-lg font-medium mb-4 text-card-foreground">Time Entries</h2>

          {safeTimeEntries.length === 0 ? (
            <div className="text-muted-foreground text-center py-4">
              No time entries found for this project.
            </div>
          ) : (
            <div className="overflow-x-auto">
              <table className="w-full text-sm text-left text-foreground">
                <thead className="text-xs text-muted-foreground uppercase bg-muted/50 border-b border-border">
                  <tr>
                    <th scope="col" className="py-3 px-6">Start Time</th>
                    <th scope="col" className="py-3 px-6">End Time</th>
                    <th scope="col" className="py-3 px-6">Duration</th>
                    <th scope="col" className="py-3 px-6"><span className="sr-only">Actions</span></th>
                  </tr>
                </thead>
                <tbody>
                  {safeTimeEntries.map((entry) => (
                    <tr
                      key={entry.id}
                      className="border-b border-border hover:bg-muted/50"
                    >
                      <td className="py-4 px-6">
                        {formatDate(entry.start_time)}
                      </td>
                      <td className="py-4 px-6">
                        {entry.end_time ? (
                          formatDate(entry.end_time)
                        ) : (
                          <span className="inline-flex items-center px-2 py-0.5 rounded-full text-xs font-medium bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-300 border border-green-300 dark:border-green-700">
                            <span className="w-2 h-2 mr-1 bg-green-500 rounded-full animate-pulse"></span>
                            Running
                          </span>
                        )}
                      </td>
                      <td className="py-4 px-6 font-mono">
                        {formatDuration(entry.duration)}
                      </td>
                      <td className="py-4 px-6 text-right">
                        <div className="flex items-center space-x-2 justify-end">
                            <Link
                              href={`/time-entries/${entry.id}`}
                              className="px-2 py-1 text-xs bg-secondary text-secondary-foreground hover:bg-secondary/80 rounded-md border border-border"
                              aria-label={`View details for time entry started at ${formatDate(entry.start_time)}`}
                            >
                              View
                            </Link>
                            <Link
                              href={`/time-entries/${entry.id}/edit`}
                              className="px-2 py-1 text-xs bg-secondary text-secondary-foreground hover:bg-secondary/80 rounded-md border border-border"
                              aria-label={`Edit time entry started at ${formatDate(entry.start_time)}`}
                            >
                              Edit
                            </Link>
                        </div>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
