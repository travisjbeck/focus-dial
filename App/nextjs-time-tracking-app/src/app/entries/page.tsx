"use client";

import { useState, useMemo } from "react";
import { useTimeEntries } from "@/lib/hooks/useTimeEntries";
import { useProjects } from "@/lib/hooks/useProjects";
import type { Database } from "@/types/supabase";
import { formatDuration, formatDate } from "@/lib/utils/dateUtils";
import Link from "next/link";

// Use types derived from hooks/database
// type TimeEntry = Database["public"]["Tables"]["sessions"]["Row"]; // Type is inferred by useTimeEntries hook
type Project = Database["public"]["Tables"]["projects"]["Row"];

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
        <h1 className="text-xl font-bold text-foreground">Time Entries</h1>
      </div>

      {/* Summary Cards */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 mb-6">
        <div className="bg-card p-4 rounded-lg shadow border border-border">
          <h3 className="text-xs uppercase text-muted-foreground mb-1">Total Hours</h3>
          <div className="text-2xl font-bold text-card-foreground">
            {isLoading ? "..." : formatDuration(summaryStats.totalDuration)}
          </div>
        </div>
        <div className="bg-card p-4 rounded-lg shadow border border-border">
          <h3 className="text-xs uppercase text-muted-foreground mb-1">
            Total Entries
          </h3>
          <div className="text-2xl font-bold text-card-foreground">
            {isLoading ? "..." : summaryStats.totalEntries}
          </div>
        </div>
        <div className="bg-card p-4 rounded-lg shadow border border-border">
          <h3 className="text-xs uppercase text-muted-foreground mb-1">
            Active Timers
          </h3>
          <div className="text-2xl font-bold text-card-foreground">
            {isLoading ? "..." : summaryStats.activeTimers}
          </div>
        </div>
        <div className="bg-card p-4 rounded-lg shadow border border-border">
          <h3 className="text-xs uppercase text-muted-foreground mb-1">Projects</h3>
          <div className="text-2xl font-bold text-card-foreground">
            {isLoadingProjects ? "..." : projects?.length ?? 0}
          </div>
        </div>
      </div>

      {/* Filter tabs */}
      <div className="flex border-b border-border mb-6">
        <button
          className={`py-2 px-4 text-sm font-medium ${
            activeFilter === "all"
              ? "text-foreground border-b-2 border-primary"
              : "text-muted-foreground hover:text-foreground hover:border-muted-foreground/50 border-b-2 border-transparent"
          }`}
          onClick={() => setActiveFilter("all")}
        >
          All Entries
        </button>
        <button
          className={`py-2 px-4 text-sm font-medium ${
            activeFilter === "active"
              ? "text-foreground border-b-2 border-primary"
              : "text-muted-foreground hover:text-foreground hover:border-muted-foreground/50 border-b-2 border-transparent"
          }`}
          onClick={() => setActiveFilter("active")}
        >
          Active
        </button>
        <button
          className={`py-2 px-4 text-sm font-medium ${
            activeFilter === "completed"
              ? "text-foreground border-b-2 border-primary"
              : "text-muted-foreground hover:text-foreground hover:border-muted-foreground/50 border-b-2 border-transparent"
          }`}
          onClick={() => setActiveFilter("completed")}
        >
          Completed
        </button>
      </div>

      {/* Main Content Area */}
      {isLoading ? (
        <div className="text-center py-8 text-muted-foreground">Loading data...</div>
      ) : error ? (
        <div className="bg-destructive border border-destructive text-destructive-foreground px-4 py-3 rounded-md text-center">
          <span>Error loading data: {error.message}</span>
          <button
            onClick={handleRetry}
            className="ml-2 px-2 py-1 text-xs bg-destructive-foreground text-destructive rounded-md hover:opacity-90"
          >
            Retry
          </button>
        </div>
      ) : filteredEntries.length === 0 ? (
        <div className="bg-card p-6 rounded-lg shadow border border-border text-center">
          <p className="text-muted-foreground">
            {activeFilter === "all"
              ? "No time entries found. Start a timer on your device!"
              : activeFilter === "active"
              ? "No active timers at the moment."
              : "No completed time entries found for this filter."}
          </p>
        </div>
      ) : (
        <div className="overflow-x-auto relative shadow-md sm:rounded-lg border border-border">
          <table className="w-full text-sm text-left text-foreground">
            <thead className="text-xs text-muted-foreground uppercase bg-muted/50 border-b border-border">
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
                    className="border-b border-border hover:bg-muted/50"
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
                        <span className="font-medium text-foreground">
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
                );
              })}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}
