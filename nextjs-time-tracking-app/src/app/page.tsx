"use client";

import { useMemo } from "react";
import Link from "next/link";
import { useTimeEntries } from "@/lib/hooks/useTimeEntries";
import { useProjects } from "@/lib/hooks/useProjects";
import type { Database } from "@/types/supabase";

// Use types derived from hooks/database
// type TimeEntry = Database["public"]["Tables"]["sessions"]["Row"]; // Inferred from useTimeEntries
type Project = Database["public"]["Tables"]["projects"]["Row"];

// Helper functions (formatDuration, formatDate) remain the same
function formatDuration(seconds?: number | null): string {
  if (seconds === null || seconds === undefined) return "—";
  const hours = Math.floor(seconds / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const secs = seconds % 60;
  return [hours, minutes, secs]
    .map((v) => v.toString().padStart(2, "0"))
    .join(":");
}
function formatDate(dateString?: string | null): string {
  if (!dateString) return "—";
  try {
    return new Date(dateString).toLocaleString();
  } catch {
    console.error("Error formatting date:", dateString);
    return "Invalid Date";
  }
}
// --- End Helper Functions ---

export default function Dashboard() {
  // Use hooks to get live data
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

  // Keep combined error handling
  const error = errorEntries || errorProjects;

  // Memoize project map for efficient lookups
  const projectsMap = useMemo(() => {
    const map: Record<string, Project> = {};
    if (projects) {
      projects.forEach((project) => {
        map[project.id] = project;
      });
    }
    return map;
  }, [projects]);

  // Memoize recent entries (take first 5)
  const recentEntries = useMemo(() => {
    return timeEntries?.slice(0, 5) ?? [];
  }, [timeEntries]);

  // Memoize summary calculations based on *all* entries
  const summaryStats = useMemo(() => {
    if (!timeEntries)
      return { totalDuration: 0, activeProject: null, isTimerRunning: false };
    const totalDuration = timeEntries.reduce(
      (total, entry) => total + (entry.duration || 0),
      0
    );
    const activeEntry = timeEntries.find((entry) => !entry.end_time);
    const activeProject = activeEntry?.project_id
      ? projectsMap[activeEntry.project_id]
      : null;

    return {
      totalDuration,
      activeProject,
      isTimerRunning: !!activeEntry,
    };
  }, [timeEntries, projectsMap]);

  const handleRetry = () => {
    if (errorEntries) refetchEntries();
    if (errorProjects) refetchProjects();
  };

  return (
    <div className="container mx-auto px-4">
      <h1 className="sr-only">Dashboard</h1>

      {/* Render error first if exists */}
      {error && (
        <div className="bg-gray-900 border border-red-800 text-red-400 px-4 py-3 rounded text-center mb-6">
          <span>Error loading dashboard data: {error.message}</span>
          <button
            onClick={handleRetry}
            className="ml-2 px-2 py-1 text-xs bg-gray-600 hover:bg-gray-500 text-white rounded"
          >
            Retry
          </button>
        </div>
      )}

      {/* Stats Overview */}
      <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 mb-8">
        <div className="bg-gray-800 p-4 rounded-lg shadow border border-gray-700">
          <h3 className="text-xs uppercase text-gray-500 mb-1">Projects</h3>
          <div className="text-2xl font-bold">
            {isLoadingProjects ? "..." : projects?.length ?? 0}
          </div>
        </div>
        <div className="bg-gray-800 p-4 rounded-lg shadow border border-gray-700">
          <h3 className="text-xs uppercase text-gray-500 mb-1">Total Hours</h3>
          <div className="text-2xl font-bold">
            {isLoadingEntries
              ? "..."
              : formatDuration(summaryStats.totalDuration)}
          </div>
        </div>
        <div className="bg-gray-800 p-4 rounded-lg shadow border border-gray-700 relative">
          <h3 className="text-xs uppercase text-gray-500 mb-1">Active Timer</h3>
          <div className="text-2xl font-bold">
            {isLoadingEntries ? (
              "..."
            ) : summaryStats.isTimerRunning ? (
              <>
                <span className="inline-block w-2 h-2 bg-red-500 rounded-full animate-pulse mr-2"></span>
                {summaryStats.activeProject?.name || "Unknown"}
              </>
            ) : (
              "None"
            )}
          </div>
        </div>
      </div>

      {/* Recent Time Entries Section */}
      <div className="bg-gray-800 p-6 rounded-lg shadow border border-gray-700 mb-8">
        <div className="flex justify-between items-center mb-4">
          <h2 className="text-lg font-semibold text-white">
            Recent Time Entries
          </h2>
          <Link
            href="/entries"
            className="text-sm text-gray-400 hover:text-white"
          >
            View All →
          </Link>
        </div>
        {isLoadingEntries ? (
          <div className="text-center py-4 text-gray-400">
            Loading entries...
          </div>
        ) : recentEntries.length === 0 ? (
          <div className="text-gray-500 text-center py-4">
            No recent time entries found.
          </div>
        ) : (
          <div className="overflow-x-auto">
            <table className="w-full text-sm text-left text-gray-400">
              <thead className="text-xs text-gray-400 uppercase bg-gray-700">
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
                </tr>
              </thead>
              <tbody>
                {recentEntries.map((entry) => {
                  const project = entry.project_id
                    ? projectsMap[entry.project_id]
                    : null;
                  return (
                    <tr
                      key={entry.id}
                      className="border-b border-gray-700 hover:bg-gray-600"
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
                          <span className="font-medium text-white">
                            {project?.name ||
                              (entry.project_id
                                ? "Unknown Project"
                                : "No Project")}
                          </span>
                        </div>
                      </td>
                      <td className="py-4 px-6">
                        {formatDate(entry.start_time)}
                      </td>
                      <td className="py-4 px-6">
                        {formatDate(entry.end_time)}
                      </td>
                      <td className="py-4 px-6 font-mono">
                        {formatDuration(entry.duration)}
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        )}
      </div>

      {/* Projects Preview Section */}
      <div className="bg-gray-800 p-6 rounded-lg shadow border border-gray-700">
        <div className="flex justify-between items-center mb-4">
          <h2 className="text-lg font-semibold text-white">Projects</h2>
          <Link
            href="/projects"
            className="text-sm text-gray-400 hover:text-white"
          >
            View All →
          </Link>
        </div>
        {isLoadingProjects ? (
          <div className="text-center py-4 text-gray-400">
            Loading projects...
          </div>
        ) : !projects || projects.length === 0 ? (
          <div className="text-gray-500 text-center py-4">
            No projects found.{" "}
            <Link
              href="/projects/create"
              className="text-blue-400 hover:underline"
            >
              Create one?
            </Link>
          </div>
        ) : (
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
            {/* Show only first 4 projects as preview */}
            {projects.slice(0, 4).map((project) => (
              <Link
                key={project.id}
                href={`/projects/${project.id}`}
                className="block group"
              >
                <div className="border border-gray-700 rounded p-4 h-full hover:bg-gray-900 transition-colors group-hover:border-gray-600">
                  <div className="flex items-center mb-2">
                    <span
                      className="w-4 h-4 rounded-full mr-2 flex-shrink-0"
                      style={{ backgroundColor: project.color || "#808080" }}
                    ></span>
                    <h3 className="font-medium truncate text-white">
                      {project.name}
                    </h3>
                  </div>
                  {/* Add other project info if needed */}
                </div>
              </Link>
            ))}
          </div>
        )}
      </div>
    </div>
  );
}
