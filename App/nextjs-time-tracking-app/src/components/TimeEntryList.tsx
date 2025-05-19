"use client";

import React from "react";
import { useTimeEntries } from "@/lib/hooks/useTimeEntries";
import { useProjects } from "@/lib/hooks/useProjects";
import Link from "next/link";
import type { Database } from "@/types/supabase";

// Correctly type based on the time_entries table
type TimeEntry = Database["public"]["Tables"]["time_entries"]["Row"];
// Define Project type as well
type Project = Database["public"]["Tables"]["projects"]["Row"];

export default function TimeEntryList() {
  // Fetch both time entries and projects
  const {
    data: timeEntries,
    isLoading: isLoadingEntries,
    error: errorEntries,
    refetch: refetchEntries,
  } = useTimeEntries();
  const {
    data: projects,
    isLoading: isLoadingProjects,
    error: errorProjects,
    refetch: refetchProjects,
  } = useProjects();

  // Create a map for quick project lookup by ID
  const projectMap = React.useMemo(() => {
    if (!projects) return new Map<number, Project>();
    return projects.reduce((map, project) => {
      map.set(project.id, project);
      return map;
    }, new Map<number, Project>());
  }, [projects]);

  // Combined loading and error handling
  if (isLoadingEntries || isLoadingProjects) {
    return <div className="text-center text-gray-400">Loading data...</div>;
  }

  // Handle errors from either hook
  const combinedError = errorEntries || errorProjects;
  if (combinedError) {
    return (
      <div className="text-center text-red-500">
        Error loading data: {combinedError.message}
        <button
          onClick={() => {
            if (errorEntries) refetchEntries();
            if (errorProjects) refetchProjects();
          }}
          className="ml-2 px-2 py-1 text-xs bg-black hover:bg-gray-900 text-white rounded-md border border-gray-800"
        >
          Retry
        </button>
      </div>
    );
  }

  if (!timeEntries || timeEntries.length === 0) {
    return (
      <div className="text-center text-gray-500">
        <p>No time entries found.</p>
        {/* Link to start a timer? Or just info text? */}
      </div>
    );
  }

  // Helper to format dates/times (can be expanded)
  const formatDateTime = (isoString: string | null): string => {
    if (!isoString) return "N/A";
    try {
      return new Date(isoString).toLocaleString();
    } catch {
      console.error("Error formatting date:", isoString);
      return "Invalid Date";
    }
  };

  // Helper to format duration (can be expanded)
  const formatDuration = (durationSeconds: number | null): string => {
    if (durationSeconds === null || durationSeconds === undefined) return "-";
    const hours = Math.floor(durationSeconds / 3600);
    const minutes = Math.floor((durationSeconds % 3600) / 60);
    const seconds = durationSeconds % 60;
    // Simple HH:MM:SS format, adjust as needed
    return `${hours.toString().padStart(2, "0")}:${minutes
      .toString()
      .padStart(2, "0")}:${seconds.toString().padStart(2, "0")}`;
  };

  return (
    <div className="overflow-x-auto relative shadow-md sm:rounded-lg border border-gray-800">
      <table className="w-full text-sm text-left text-gray-400">
        <thead className="text-xs text-gray-400 uppercase bg-black border-b border-gray-800">
          <tr>
            <th scope="col" className="py-3 px-6">
              Project
            </th>
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
              Description
            </th>
            <th scope="col" className="py-3 px-6">
              <span className="sr-only">Actions</span>
            </th>
          </tr>
        </thead>
        <tbody>
          {timeEntries.map((entry: TimeEntry) => {
            // Look up project name using the map
            const projectName = entry.project_id
              ? projectMap.get(entry.project_id)?.name
              : null;
            const project = entry.project_id
              ? projectMap.get(entry.project_id)
              : null;
            return (
              <tr
                key={entry.id}
                className="border-b border-gray-800 hover:bg-gray-900"
              >
                <td className="py-4 px-6">
                  <div className="flex items-center">
                    {project && (
                      <span
                        className="w-3 h-3 rounded-full mr-2 flex-shrink-0"
                        style={{
                          backgroundColor: project.color || "#808080",
                        }}
                      ></span>
                    )}
                    <span>
                      {projectName ||
                        (entry.project_id ? `ID: ${entry.project_id}` : "N/A")}
                    </span>
                  </div>
                </td>
                <td className="py-4 px-6">
                  {formatDateTime(entry.start_time)}
                </td>
                <td className="py-4 px-6">
                  {entry.end_time ? (
                    formatDateTime(entry.end_time)
                  ) : (
                    <span className="text-green-500 font-medium">
                      <span className="inline-block w-2 h-2 bg-green-500 rounded-full animate-pulse mr-2"></span>
                      Running
                    </span>
                  )}
                </td>
                <td className="py-4 px-6">{formatDuration(entry.duration)}</td>
                <td
                  className="py-4 px-6 max-w-xs truncate"
                  title={entry.description || ""}
                >
                  {entry.description || "-"}
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
            );
          })}
        </tbody>
      </table>
    </div>
  );
}
