"use client";

import { useState, useMemo } from "react";
import { useTimeEntries } from "@/lib/hooks/useTimeEntries";
import { useProjects } from "@/lib/hooks/useProjects";
import type { Database } from "@/types/supabase";

// Use types derived from hooks/database
// type TimeEntry = Database["public"]["Tables"]["sessions"]["Row"]; // Type is inferred by useTimeEntries hook
type Project = Database["public"]["Tables"]["projects"]["Row"];

// Helper to format duration in seconds to a readable format
function formatDuration(seconds?: number | null): string {
  if (seconds === null || seconds === undefined) return "—";
  const hours = Math.floor(seconds / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const secs = seconds % 60;
  return [hours, minutes, secs]
    .map((v) => v.toString().padStart(2, "0"))
    .join(":");
}

// Helper to format date
function formatDate(dateString?: string | null): string {
  if (!dateString) return "—";
  try {
    return new Date(dateString).toLocaleString();
  } catch {
    console.error("Error formatting date:", dateString);
    return "Invalid Date";
  }
}

export default function EntriesPage() {
  const [activeFilter, setActiveFilter] = useState<
    "all" | "active" | "completed"
  >("all");

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

  // Combine loading and error states
  const isLoading = isLoadingEntries || isLoadingProjects;
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

  // Memoize filtered entries based on data and filter state
  const filteredEntries = useMemo(() => {
    if (!timeEntries) return [];
    return timeEntries.filter((entry) => {
      if (activeFilter === "all") return true;
      if (activeFilter === "active") return !entry.end_time;
      if (activeFilter === "completed") return !!entry.end_time;
      return true;
    });
  }, [timeEntries, activeFilter]);

  // Memoize summary calculations
  const summaryStats = useMemo(() => {
    if (!timeEntries)
      return { totalDuration: 0, totalEntries: 0, activeTimers: 0 };
    const totalDuration = timeEntries.reduce(
      (total, entry) => total + (entry.duration || 0),
      0
    );
    const activeTimers = timeEntries.filter((entry) => !entry.end_time).length;
    return { totalDuration, totalEntries: timeEntries.length, activeTimers };
  }, [timeEntries]);

  const handleRetry = () => {
    if (errorEntries) refetchEntries();
    if (errorProjects) refetchProjects();
  };

  return (
    <div className="container mx-auto px-4">
      <div className="flex items-center justify-between mb-6">
        <h1 className="text-xl font-bold text-white">Time Entries</h1>
      </div>

      {/* Summary Cards */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 mb-6">
        <div className="bg-black p-4 rounded-lg shadow border border-gray-800">
          <h3 className="text-xs uppercase text-gray-500 mb-1">Total Hours</h3>
          <div className="text-2xl font-bold text-white">
            {isLoading ? "..." : formatDuration(summaryStats.totalDuration)}
          </div>
        </div>
        <div className="bg-black p-4 rounded-lg shadow border border-gray-800">
          <h3 className="text-xs uppercase text-gray-500 mb-1">
            Total Entries
          </h3>
          <div className="text-2xl font-bold text-white">
            {isLoading ? "..." : summaryStats.totalEntries}
          </div>
        </div>
        <div className="bg-black p-4 rounded-lg shadow border border-gray-800">
          <h3 className="text-xs uppercase text-gray-500 mb-1">
            Active Timers
          </h3>
          <div className="text-2xl font-bold text-white">
            {isLoading ? "..." : summaryStats.activeTimers}
          </div>
        </div>
        <div className="bg-black p-4 rounded-lg shadow border border-gray-800">
          <h3 className="text-xs uppercase text-gray-500 mb-1">Projects</h3>
          <div className="text-2xl font-bold text-white">
            {isLoadingProjects ? "..." : projects?.length ?? 0}
          </div>
        </div>
      </div>

      {/* Filter tabs */}
      <div className="flex border-b border-gray-800 mb-6">
        <button
          className={`py-2 px-4 text-sm font-medium ${
            activeFilter === "all"
              ? "text-white border-b-2 border-white"
              : "text-gray-500 hover:text-gray-300"
          }`}
          onClick={() => setActiveFilter("all")}
        >
          All Entries
        </button>
        <button
          className={`py-2 px-4 text-sm font-medium ${
            activeFilter === "active"
              ? "text-white border-b-2 border-white"
              : "text-gray-500 hover:text-gray-300"
          }`}
          onClick={() => setActiveFilter("active")}
        >
          Active
        </button>
        <button
          className={`py-2 px-4 text-sm font-medium ${
            activeFilter === "completed"
              ? "text-white border-b-2 border-white"
              : "text-gray-500 hover:text-gray-300"
          }`}
          onClick={() => setActiveFilter("completed")}
        >
          Completed
        </button>
      </div>

      {/* Main Content Area */}
      {isLoading ? (
        <div className="text-center py-8 text-gray-400">Loading data...</div>
      ) : error ? (
        <div className="bg-black border border-red-800 text-red-400 px-4 py-3 rounded-md text-center">
          <span>Error loading data: {error.message}</span>
          <button
            onClick={handleRetry}
            className="ml-2 px-2 py-1 text-xs bg-gray-900 hover:bg-gray-800 text-white rounded-md"
          >
            Retry
          </button>
        </div>
      ) : filteredEntries.length === 0 ? (
        <div className="bg-black p-6 rounded-lg shadow border border-gray-800 text-center">
          <p className="text-gray-400">
            {activeFilter === "all"
              ? "No time entries found. Start a timer on your device!"
              : activeFilter === "active"
              ? "No active timers at the moment."
              : "No completed time entries found for this filter."}
          </p>
        </div>
      ) : (
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
                  Actions
                </th>
              </tr>
            </thead>
            <tbody>
              {filteredEntries.map((entry) => {
                const project = entry.project_id
                  ? projectsMap[entry.project_id]
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
                        <span className="font-medium text-white">
                          {project?.name ||
                            (entry.project_id
                              ? "Unknown Project"
                              : "No Project")}
                        </span>
                      </div>
                    </td>
                    <td className="py-4 px-6">{formatDate(entry.start_time)}</td>
                    <td className="py-4 px-6">
                      {entry.end_time ? (
                        formatDate(entry.end_time)
                      ) : (
                        <span className="text-green-500 font-medium">
                          <span className="inline-block w-2 h-2 bg-green-500 rounded-full animate-pulse mr-2"></span>
                          Running
                        </span>
                      )}
                    </td>
                    <td className="py-4 px-6 font-mono">
                      {formatDuration(entry.duration)}
                    </td>
                    <td className="py-4 px-6">
                      <div className="flex items-center space-x-2">
                        <button className="px-2 py-1 text-xs bg-black hover:bg-gray-900 text-white rounded-md border border-gray-800">
                          View
                        </button>
                        <button className="px-2 py-1 text-xs bg-black hover:bg-gray-900 text-white rounded-md border border-gray-800">
                          Edit
                        </button>
                      </div>
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}
