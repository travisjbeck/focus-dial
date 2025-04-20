// Remove "use client"; Convert to Server Component

import Link from "next/link";
import { notFound } from "next/navigation"; // Import notFound
import { cookies } from "next/headers"; // Import cookies
import { createServerComponentSupabaseClient } from "@/utils/supabase"; // Import Supabase client
import DeleteProjectButton from "@/components/DeleteProjectButton"; // Import the new button component

// Remove client-side imports if no longer needed
// import { useState, useEffect } from "react";
// import { useRouter } from "next/navigation";
// import axios from "axios";

// Use types directly
// type Project = Database["public"]["Tables"]["projects"]["Row"]; // Remove unused
// type TimeEntry = Database["public"]["Tables"]["time_entries"]["Row"]; // Remove unused

// Helper functions (formatDuration, formatDate) remain the same
function formatDuration(seconds?: number | null): string {
  // Updated type
  if (seconds === null || seconds === undefined) return "—";
  const hours = Math.floor(seconds / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const secs = seconds % 60;
  return [hours, minutes, secs]
    .map((v) => v.toString().padStart(2, "0"))
    .join(":");
}
function formatDate(dateString?: string | null): string {
  // Updated type
  if (!dateString) return "—";
  try {
    return new Date(dateString).toLocaleString();
  } catch {
    console.error("Error formatting date:", dateString);
    return "Invalid Date";
  }
}

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
          {/* ... Display project name/color using fetched `project` ... */}
          <div className="flex items-center">
            <div
              className="w-6 h-6 rounded-full mr-3"
              style={{ backgroundColor: project.color || "#808080" }}
            />
            <h1 className="text-xl font-bold text-white">{project.name}</h1>
          </div>
          <div className="flex space-x-2">
            <Link
              href="/projects"
              className="px-3 py-1 text-xs font-medium text-white bg-black rounded-md border border-gray-800 hover:bg-gray-900"
            >
              Back
            </Link>
            <Link
              href={`/projects/${project.id}/edit`}
              className="px-3 py-1 text-xs font-medium text-white bg-black rounded-md border border-gray-800 hover:bg-gray-900"
            >
              Edit
            </Link>
            <DeleteProjectButton projectId={projectIdNumber} />
          </div>
        </div>

        {/* TODO: Handle delete error display with new component state */}
        {/* {deleteError && ( ... )} */}

        <div className="text-xs text-gray-500 mb-4">
          Created: {formatDate(project.created_at)}
        </div>

        {/* Project Stats - Use directly calculated stats */}
        <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 mb-6">
          {/* ... Stat cards using totalDuration, totalEntries, activeEntries.length ... */}
          <div className="bg-black p-4 rounded-lg shadow border border-gray-800">
            <h3 className="text-xs uppercase text-gray-500 mb-1">Total Time</h3>
            <div className="text-2xl font-bold text-white">
              {formatDuration(totalDuration)}
            </div>
          </div>
          <div className="bg-black p-4 rounded-lg shadow border border-gray-800">
            <h3 className="text-xs uppercase text-gray-500 mb-1">
              Time Entries
            </h3>
            <div className="text-2xl font-bold text-white">{totalEntries}</div>
          </div>
          <div className="bg-black p-4 rounded-lg shadow border border-gray-800">
            <h3 className="text-xs uppercase text-gray-500 mb-1">
              Active Sessions
            </h3>
            <div className="text-2xl font-bold text-white">
              {activeEntries.length}
            </div>
          </div>
        </div>

        {/* Time Entries - Use fetched `safeTimeEntries` */}
        <div className="bg-black p-6 rounded-lg shadow border border-gray-800">
          <h2 className="text-lg font-medium mb-4 text-white">Time Entries</h2>

          {safeTimeEntries.length === 0 ? (
            <div className="text-gray-400 text-center py-4">
              No time entries found for this project.
            </div>
          ) : (
            <div className="overflow-x-auto">
              <table className="w-full text-sm text-left text-gray-400">
                <thead className="text-xs text-gray-400 uppercase bg-black border-b border-gray-800">
                  <tr>
                    <th scope="col" className="py-3 px-6">
                      Start Time
                    </th>
                    <th scope="col" className="py-3 px-6">
                      End Time
                    </th>
                    <th scope="col" className="py-3 px-6">
                      Duration
                    </th>
                    <th scope="col" className="py-3 px-6">
                      <span className="sr-only">Actions</span>
                    </th>
                  </tr>
                </thead>
                <tbody>
                  {safeTimeEntries.map((entry) => (
                    <tr
                      key={entry.id}
                      className="border-b border-gray-800 hover:bg-gray-900"
                    >
                      <td className="py-4 px-6">
                        {formatDate(entry.start_time)}
                      </td>
                      <td className="py-4 px-6">
                        {entry.end_time ? (
                          formatDate(entry.end_time)
                        ) : (
                          <span className="inline-flex items-center px-2 py-0.5 rounded-full text-xs font-medium bg-black text-green-500 border border-green-800">
                            <span className="w-2 h-2 mr-1 bg-green-500 rounded-full animate-pulse"></span>
                            Running
                          </span>
                        )}
                      </td>
                      <td className="py-4 px-6 font-mono">
                        {formatDuration(entry.duration)}
                      </td>
                      <td className="py-4 px-6 text-right">
                        <Link
                          href={`/entries/${entry.id}`}
                          className="px-2 py-1 text-xs bg-black hover:bg-gray-900 text-white rounded-md border border-gray-800"
                        >
                          View
                        </Link>
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
