"use client";

import { useMemo, useState } from "react";
import Link from "next/link";
import { useTimeEntries } from "@/lib/hooks/useTimeEntries";
import { useProjects } from "@/lib/hooks/useProjects";
import type { Database } from "@/types/supabase";
import { getDateRangeForOption, generateTimelineMarkers } from "@/lib/utils/dateUtils";
// Import the TimeEntry type from the hook
import type { TimeEntry } from "@/lib/hooks/useTimeEntries";

// Use types derived from hooks/database
// type TimeEntry = Database["public"]["Tables"]["sessions"]["Row"]; // Inferred from useTimeEntries
type Project = Database["public"]["Tables"]["projects"]["Row"];
// Local TimeEntry definition is no longer needed
/* type TimeEntry = {
  id: number;
  project_id: number | null;
  start_time: string;
  end_time: string | null;
  duration: number | null;
  user_id: string;
  created_at: string;
  description?: string | null;
}; */

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
    return "Invalid Date";
  }
}

// --- Time Range Options (Moved outside component and type exported) ---
const timeRangeOptions = [
  "Today",
  "Yesterday",
  "Week to Date",
  "Month to Date",
  "Last 7 Days",
  "Last 30 Days",
  "Year to Date",
] as const; // Use const assertion for type safety

export type TimeRangeOption = typeof timeRangeOptions[number]; // Export the type

// --- End Helper Functions ---

export default function Dashboard() {
  // --- State for selected time range ---
  const [selectedRange, setSelectedRange] = useState<TimeRangeOption>("Today");

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
    const map: Record<number, Project> = {};
    if (projects) {
      projects.forEach((project) => {
        map[project.id] = project;
      });
    }
    return map;
  }, [projects]);

  // Calculate timeline range based on selected option
  const timelineRange = useMemo(() => {
    // 1. Get the default range (e.g., Today 00:00 to 23:59)
    const defaultRange = getDateRangeForOption(selectedRange);

    // 2. Check if we need to adjust the start time
    if (selectedRange === "Today" || selectedRange === "Yesterday") {
      if (timeEntries && timeEntries.length > 0) {
        // 3. Find the earliest entry within the default day range
        const earliestEntryTime = timeEntries
          .filter(entry => {
            const entryStart = new Date(entry.start_time);
            // Check if entry starts within the default day boundaries
            return entryStart >= defaultRange.start && entryStart <= defaultRange.end;
          })
          .reduce((earliest, entry) => {
            const entryStart = new Date(entry.start_time);
            return entryStart < earliest ? entryStart : earliest;
          }, new Date(defaultRange.end)); // Start comparison with the end of the day

        // 4. Calculate 8:00 AM for the selected day
        const eightAm = new Date(defaultRange.start);
        eightAm.setHours(8, 0, 0, 0);

        // 5. Determine the final start time
        // Use the earlier of 8 AM or the earliest entry, but not earlier than the day's start
        const finalStartTime = new Date(Math.min(eightAm.getTime(), earliestEntryTime.getTime()));

        // Ensure the calculated start isn't *before* the actual day start if entries are sparse
        // (This check might be redundant if filter works correctly, but safe)
        if (finalStartTime >= defaultRange.start) {
           defaultRange.start = finalStartTime;
        } else {
          // If the earliest entry was somehow before 8 AM and also before the calculated day start (unlikely),
          // fall back to 8 AM or the default start if 8 AM is somehow invalid.
          defaultRange.start = eightAm > defaultRange.start ? eightAm : defaultRange.start;
        }
      } else {
        // If no entries for the day, just start at 8 AM
        const eightAm = new Date(defaultRange.start);
        eightAm.setHours(8, 0, 0, 0);
        // Ensure 8 AM is actually later than midnight before setting
        if (eightAm > defaultRange.start) {
          defaultRange.start = eightAm;
        }
      }
    }

    // 6. Return the (potentially adjusted) range
    return defaultRange;
  }, [selectedRange, timeEntries]); // Add timeEntries dependency

  // Filter entries based on the *final* calculated timeline range
  const selectedRangeEntries = useMemo(() => {
    if (!timeEntries) return [];

    const { start, end } = timelineRange; // Get the calculated range

    return timeEntries.filter(entry => {
      const entryStartDate = new Date(entry.start_time);
      // Include entries that start within the range
      return entryStartDate >= start && entryStartDate <= end;
      // Note: We might want to include entries that *overlap* the range too.
      // For simplicity, we'll start with entries *starting* within the range.
    });
  }, [timeEntries, timelineRange]); 

  // Group filtered entries by project
  const entriesByProject = useMemo(() => {
    const grouped: Record<number, TimeEntry[]> = {};
    
    if (projects?.length) {
      // Initialize with empty arrays for all projects
      projects.forEach(project => {
        grouped[project.id] = [];
      });
      
      // Group filtered entries by project
      if (selectedRangeEntries.length) { // Use filtered entries
        selectedRangeEntries.forEach(entry => { // Use filtered entries
          if (entry.project_id) {
            if (!grouped[entry.project_id]) {
              grouped[entry.project_id] = [];
            }
            grouped[entry.project_id].push(entry);
          }
        });
      }
      
      // Sort grouped entries (no change needed here, logic is sound)
      const sortedEntries: Record<number, TimeEntry[]> = {};
      Object.entries(grouped)
        .filter(([, entries]) => {
          return entries.length > 0;
        })
        .forEach(([projectId, entries]) => {
          sortedEntries[Number(projectId)] = entries;
        });
      Object.entries(grouped)
        .filter(([, entries]) => {
          return entries.length === 0;
        })
        .forEach(([projectId, entries]) => {
          sortedEntries[Number(projectId)] = entries;
        });
      
      return sortedEntries;
    }
    
    return grouped;
  }, [selectedRangeEntries, projects]); // Depend on filtered entries

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

  // Calculate total duration for the selected range
  const selectedRangeTotalDuration = useMemo(() => {
    const now = new Date().getTime(); // Get current time once
    return selectedRangeEntries.reduce(
      (total, entry) => {
        if (entry.end_time === null) {
          // Timer is still running
          const startTime = new Date(entry.start_time).getTime();
          // Calculate elapsed seconds since start_time, ensure it's not negative
          const elapsedSeconds = Math.max(0, Math.floor((now - startTime) / 1000));
          return total + elapsedSeconds;
        } else {
          // Timer is completed, use stored duration
          return total + (entry.duration || 0);
        }
      },
      0
    );
  }, [selectedRangeEntries]);

  // Add this new useMemo hook before the return statement
  const activeProjectsCountInRange = useMemo(() => {
    const projectIds = new Set<number>();
    selectedRangeEntries.forEach(entry => {
        if (entry.project_id !== null) {
            projectIds.add(entry.project_id);
        }
    });
    return projectIds.size;
  }, [selectedRangeEntries]);

  const handleRetry = () => {
    if (errorEntries) {
      refetchEntries();
    }
    if (errorProjects) {
      refetchProjects();
    }
  };

  // Calculate position and width for timeline entries
  const getTimelinePosition = (startTime: string, endTime: string | null) => {
    const start = new Date(startTime).getTime();
    const end = endTime ? new Date(endTime).getTime() : new Date().getTime();
    
    const timelineStart = timelineRange.start.getTime();
    const timelineEnd = timelineRange.end.getTime();
    const timelineWidth = timelineEnd - timelineStart;

    // Handle edge case where timelineWidth is zero (e.g., range is invalid or start=end)
    if (timelineWidth <= 0) {
      return { left: '0%', width: '0%' };
    }
    
    // Clamp start and end times to the timeline boundaries
    const clampedStart = Math.max(start, timelineStart);
    const clampedEnd = Math.min(end, timelineEnd);

    // Calculate left position (percentage from start of timeline)
    const leftPos = ((clampedStart - timelineStart) / timelineWidth) * 100;
    
    // Calculate width (percentage of timeline width)
    const width = ((clampedEnd - clampedStart) / timelineWidth) * 100;
    
    // Ensure left and width are valid percentages between 0 and 100
    const finalLeft = Math.max(0, Math.min(100, leftPos));
    const finalWidth = Math.max(0, Math.min(100 - finalLeft, width));

    return {
      left: `${finalLeft}%`,
      width: `${finalWidth}%`
    };
  };

  // Generate hour markers for the timeline using the utility function
  const hourMarkers = useMemo(() => {
    // Pass the calculated range and selected option to the utility
    return generateTimelineMarkers(timelineRange, selectedRange);
  }, [timelineRange, selectedRange]); 

  // Filter projects with entries for the timeline display
  const projectsWithEntries = useMemo(() => {
    return Object.entries(entriesByProject)
      .filter(([, entries]) => entries.length > 0) // Keep only projects with entries
      .map(([projectId]) => projectId); // Map to get the project ID
  }, [entriesByProject]); 

  return (
    <div className="container mx-auto px-4">
      <h1 className="sr-only">Dashboard</h1>

      {/* Render error first if exists */}
      {error && (
        <div className="bg-black border border-red-800 text-red-400 px-4 py-3 rounded-md text-center mb-6">
          <span>Error loading dashboard data: {error.message}</span>
          <button
            onClick={handleRetry}
            className="ml-2 px-2 py-1 text-xs bg-gray-900 hover:bg-gray-800 text-white rounded-md"
          >
            Retry
          </button>
        </div>
      )}

      {/* Stats Overview */}
      <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 mb-8">
        <div className="bg-black p-4 rounded-lg shadow border border-gray-800">
          <h3 className="text-xs uppercase text-gray-500 mb-1">Projects</h3>
          <div className="text-2xl font-bold">
            {isLoadingEntries || isLoadingProjects ? "..." : activeProjectsCountInRange}
          </div>
        </div>
        <div className="bg-black p-4 rounded-lg shadow border border-gray-800">
          <h3 className="text-xs uppercase text-gray-500 mb-1">Total Hours</h3>
          <div className="text-2xl font-bold">
            {isLoadingEntries
              ? "..."
              : formatDuration(selectedRangeTotalDuration)}
          </div>
        </div>
        <div className="bg-black p-4 rounded-lg shadow border border-gray-800 relative">
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

      {/* Timeline Time Entries Section */}
      <div className="bg-black p-6 rounded-lg shadow border border-gray-800 mb-8">
        {/* Row 1: View All Entries Link (Top Right) */}
        <div className="flex justify-end mb-2"> { /* Push link to right, add margin bottom */}
          <Link
            href="/entries"
            className="text-xs text-gray-400 hover:text-white"
          >
            View All Entries →
          </Link>
        </div>

        {/* Row 2: Title and Dropdown */}
        <div className="flex justify-between items-center mb-4">
          {/* Left side: Title and total duration */}
          <div className="flex items-baseline space-x-2">
            <h2 className="text-lg font-semibold text-white">
              Activity Timeline
            </h2>
            {/* Display total duration for selected range */}
            <span className="text-sm text-gray-400">
              ({formatDuration(selectedRangeTotalDuration)} total)
            </span>
          </div>
          {/* Right side: Dropdown only */}
          <div className="flex items-center space-x-4">
            {/* Dropdown */}
            <select
              value={selectedRange}
              onChange={(e) => setSelectedRange(e.target.value as TimeRangeOption)}
              className="form-select py-1 text-sm"
            >
              {timeRangeOptions.map(option => (
                <option key={option} value={option}>
                  {option}
                </option>
              ))}
            </select>
          </div>
        </div>
        {isLoadingEntries ? (
          <div className="text-center py-4 text-gray-400">
            Loading entries...
          </div>
        ) : selectedRangeEntries.length === 0 ? ( // Use filtered entries count
          <div className="text-gray-500 text-center py-8">
            No entries recorded for {selectedRange}. {/* Update message */}
          </div>
        ) : (
          <div className="mt-6">
            {/* Timeline Container with responsive grid layout */}
            {projectsWithEntries.length > 0 ? (
              <div className="timeline-container">
                {/* Timeline header */}
                <div className="timeline-header">
                  <div className="project-column">
                    <span className="project-label">Project</span>
                  </div>
                  <div className="time-grid relative">
                    {hourMarkers.map((marker, idx) => (
                      <div 
                        key={idx} 
                        className="time-marker"
                        style={{ left: marker.position }}
                      >
                        <div className="marker-label">{marker.time}</div>
                        <div className="marker-line"></div>
                      </div>
                    ))}
                  </div>
                </div>

                {/* Timeline rows */}
                <div className="timeline-body">
                  {/* Map over only the projects that have entries in the selected range */}
                  {projectsWithEntries.map((projectIdStr, rowIndex) => {
                    const projectId = Number(projectIdStr); // Convert string ID back to number
                    // Get the project details and entries using the projectId
                    const project = projectsMap[projectId];
                    const entries = entriesByProject[projectId];

                    // Basic safety checks (should ideally not happen if data fetching is correct)
                    if (!project || !entries) {
                      console.warn(`Missing project data for ID: ${projectId}`);
                      return null;
                    }

                    // We already know entries.length > 0 because projectsWithEntries filters for it,
                    // so no need for an additional length check here.

                    return (
                      <div 
                        key={projectId} 
                        className={`timeline-row ${rowIndex % 2 === 0 ? 'timeline-row-alt' : ''}`}
                      >
                        {/* Project name */}
                        <div className="project-name">
                          <Link 
                            href={`/projects/${projectId}`}
                            className="font-medium text-white hover:text-gray-300 truncate block"
                            title={project.name}
                          >
                            {project.name}
                          </Link>
                        </div>
                        
                        {/* Timeline entries */}
                        <div className="time-entries">
                          {entries.map((entry: TimeEntry) => {
                            const position = getTimelinePosition(entry.start_time, entry.end_time);
                            const isActive = !entry.end_time;
                            
                            return (
                              <div
                                key={entry.id}
                                className={`time-entry ${isActive ? "active-entry" : ""}`}
                                style={{
                                  left: position.left,
                                  width: position.width,
                                  backgroundColor: project.color || '#808080',
                                  zIndex: 1 /* Above the grid lines */
                                }}
                                title={`${formatDate(entry.start_time)} - ${entry.end_time ? formatDate(entry.end_time) : "Running"} (${formatDuration(entry.duration)})`}
                              />
                            );
                          })}
                        </div>
                      </div>
                    );
                  })}
                </div>
                
                {/* Add CSS for the timeline grid */}
                <style jsx>{`
                  .timeline-container {
                    width: 100%;
                    overflow-x: auto;
                    display: flex;
                    flex-direction: column;
                    position: relative;
                  }
                  
                  .timeline-header, .timeline-row {
                    display: grid;
                    grid-template-columns: minmax(100px, 160px) 1fr;
                    align-items: center;
                  }
                  
                  .timeline-header {
                    margin-bottom: 0.25rem;
                    font-size: 0.75rem;
                    color: rgb(156 163 175);
                  }
                  
                  .timeline-body {
                    display: flex;
                    flex-direction: column;
                    gap: 0.5rem;
                    position: relative;
                  }
                  
                  .project-column {
                    padding: 0;
                    position: relative;
                    height: 30px;
                  }
                  
                  .project-label {
                    position: absolute;
                    top: 0;
                    font-weight: 500;
                    background-color: rgba(0, 0, 0, 0.7);
                    padding: 0 4px;
                    border-radius: 2px;
                    z-index: 2;
                  }
                  
                  .time-grid {
                    position: relative;
                    height: 30px;
                  }
                  
                  .time-marker {
                    position: absolute;
                    transform: translateX(-50%);
                    text-align: center;
                    top: 0;
                    width: 1px;
                    height: 100%;
                  }
                  
                  .marker-line {
                    height: 6px;
                    width: 1px;
                    background-color: rgb(75 85 99 / 40%); /* Match opacity with line extension */
                    margin: 0 auto;
                    position: relative;
                    z-index: 1;
                  }
                  
                  /* Add vertical grid lines that extend through all content */
                  .time-marker::after {
                    content: '';
                    position: absolute;
                    top: 24px;
                    left: 0;
                    width: 1px;
                    background-color: rgb(75 85 99 / 40%);
                    height: 2000px; /* Very tall to cover all potential content */
                    z-index: 0;
                    pointer-events: none;
                  }
                  
                  .marker-label {
                    background-color: rgba(0, 0, 0, 0.7);
                    padding: 0 4px;
                    border-radius: 2px;
                    white-space: nowrap;
                    position: relative;
                    bottom: 3px;
                    z-index: 2;
                  }
                  
                  .timeline-row {
                    padding: 0.5rem 0;
                    border-radius: 4px;
                  }
                  
                  .timeline-row-alt {
                    background-color: rgba(75, 85, 99, 0.15); /* Brightened by ~15% */
                  }
                  
                  .project-name {
                    padding-right: 1rem;
                    overflow: hidden;
                  }
                  
                  .time-entries {
                    position: relative;
                    height: 28px;
                    width: 100%;
                  }
                  
                  .time-entry {
                    position: absolute;
                    height: 100%;
                    top: 0;
                    border-radius: 2px;
                    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.2);
                    z-index: 1; /* Above the grid lines */
                  }
                  
                  .active-entry {
                    animation: pulse 2s cubic-bezier(0.4, 0, 0.6, 1) infinite;
                  }
                  
                  @keyframes pulse {
                    0%, 100% {
                      opacity: 1;
                    }
                    50% {
                      opacity: 0.8;
                    }
                  }
                  
                  @media (max-width: 640px) {
                    .timeline-header, .timeline-row {
                      grid-template-columns: minmax(80px, 120px) 1fr;
                    }
                  }
                `}</style>
              </div>
            ) : (
              <div className="text-gray-500 text-center py-6">
                No project activity tracked for {selectedRange}. {/* Update message */}
              </div>
            )}
          </div>
        )}
      </div>

      {/* Projects Preview Section has been removed */}
    </div>
  );
}
