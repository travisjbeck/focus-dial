import type { TimeRangeOption } from '@/app/page'; // Import the type from page.tsx

interface DateRange {
  start: Date;
  end: Date;
}

// Define the structure for a timeline marker
export interface TimelineMarker {
  time: string; // Formatted label (e.g., "10:00 AM", "Mon 15")
  position: string; // CSS percentage (e.g., "50%")
}

/**
 * Calculates the start and end Date objects for a given TimeRangeOption.
 * Assumes Sunday as the start of the week.
 */
export function getDateRangeForOption(option: TimeRangeOption): DateRange {
  const now = new Date();
  let start: Date;
  let end: Date;

  // Helper to get the start of a given date (00:00:00)
  const getStartOfDay = (date: Date): Date => {
    const start = new Date(date);
    start.setHours(0, 0, 0, 0);
    return start;
  };

  // Helper to get the end of a given date (23:59:59.999)
  const getEndOfDay = (date: Date): Date => {
    const end = new Date(date);
    end.setHours(23, 59, 59, 999);
    return end;
  };

  switch (option) {
    case "Today":
      start = getStartOfDay(now);
      end = getEndOfDay(now);
      break;
    case "Yesterday":
      const yesterday = new Date(now);
      yesterday.setDate(now.getDate() - 1);
      start = getStartOfDay(yesterday);
      end = getEndOfDay(yesterday);
      break;
    case "Week to Date":
      start = getStartOfDay(now);
      start.setDate(now.getDate() - now.getDay()); // Go back to Sunday (getDay() returns 0 for Sun, 1 for Mon, ...)
      end = getEndOfDay(now);
      break;
    case "Month to Date":
      start = getStartOfDay(now);
      start.setDate(1); // Set to the first day of the month
      end = getEndOfDay(now);
      break;
    case "Year to Date":
      start = getStartOfDay(now);
      start.setMonth(0, 1); // Set to January 1st
      end = getEndOfDay(now);
      break;
    case "Last 7 Days":
      end = getEndOfDay(now);
      start = getStartOfDay(now); 
      start.setDate(now.getDate() - 6); // Today counts as one of the 7 days
      break;
    case "Last 30 Days":
      end = getEndOfDay(now);
      start = getStartOfDay(now);
      start.setDate(now.getDate() - 29); // Today counts as one of the 30 days
      break;
    default:
      // Should not happen with type safety, but default to Today
      console.warn(`Unknown time range option: ${option}. Defaulting to Today.`);
      start = getStartOfDay(now);
      end = getEndOfDay(now);
      break;
  }

  return { start, end };
} 

// --- Timeline Marker Generation --- //

// Helper to format time labels consistently
function formatMarkerTimeLabel(date: Date): string {
  return date.toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
    hour12: true
  });
}

/**
 * Generates time markers for the timeline based on the date range and selected option.
 */
export function generateTimelineMarkers(
  timelineRange: DateRange,
  selectedRange: TimeRangeOption
): TimelineMarker[] {
  const markers: TimelineMarker[] = [];
  const timelineStart = timelineRange.start;
  const timelineEnd = timelineRange.end;
  const timelineStartMs = timelineStart.getTime();
  const timelineEndMs = timelineEnd.getTime();
  const timelineDurationMs = timelineEndMs - timelineStartMs;

  if (timelineDurationMs <= 0) return []; // Exit if range is invalid or zero duration

  const rangeDurationHours = timelineDurationMs / (1000 * 60 * 60);

  // Determine interval based on range duration for better granularity
  let intervalHours: number;

  const multiDayOptions: TimeRangeOption[] = [
    "Week to Date",
    "Month to Date",
    "Year to Date",
    "Last 7 Days",
    "Last 30 Days",
  ];

  // Prioritize range type for interval decision
  if (multiDayOptions.includes(selectedRange)) {
    if (rangeDurationHours > 24 * 7) intervalHours = 24 * 7; // Weekly
    else intervalHours = 24; // Daily
  } else { // Single-day view (Today/Yesterday)
    if (rangeDurationHours <= 8) intervalHours = 1;
    else if (rangeDurationHours <= 16) intervalHours = 2;
    else intervalHours = 3;
  }

  // Calculate the first potential marker time: Floor the start time to the nearest previous interval boundary.
  const currentMarkerTime = new Date(timelineStart);
  if (intervalHours >= 24) { // Daily or Weekly alignment
    if (intervalHours === 24 * 7) { // Weekly: Align to start of week (Sunday)
      const dayOfWeek = currentMarkerTime.getDay(); // 0=Sun
      currentMarkerTime.setDate(currentMarkerTime.getDate() - dayOfWeek);
    }
    currentMarkerTime.setHours(0, 0, 0, 0); // Align to midnight
  } else { // Hourly alignment
    const startHour = currentMarkerTime.getHours();
    const alignedHour = Math.floor(startHour / intervalHours) * intervalHours;
    currentMarkerTime.setHours(alignedHour, 0, 0, 0);
  }

  let iterations = 0;
  const maxIterations = 100;

  // Generate markers
  while (currentMarkerTime.getTime() <= timelineEndMs && iterations < maxIterations) {
    const markerTimeMs = currentMarkerTime.getTime();

    // Add marker only if it's visually within or equal to the timeline range boundaries
    if (markerTimeMs >= timelineStartMs) {
      const positionPercent = ((markerTimeMs - timelineStartMs) / timelineDurationMs) * 100;
      const clampedPosition = Math.max(0, Math.min(100, positionPercent));

      let label = "";
      if (intervalHours === 24 * 7) {
        label = currentMarkerTime.toLocaleDateString([], { month: 'short', day: 'numeric' });
      } else if (intervalHours === 24) {
        label = currentMarkerTime.toLocaleDateString([], { weekday: 'short', day: 'numeric' });
      } else {
        label = formatMarkerTimeLabel(currentMarkerTime);
      }

      // Add the marker if its position is valid
      if (clampedPosition >= 0 && clampedPosition <= 100) {
        markers.push({
          time: label,
          position: `${clampedPosition}%`
        });
      }
    }

    // Increment marker time
    if (intervalHours === 24 * 7) {
      currentMarkerTime.setDate(currentMarkerTime.getDate() + 7);
    } else {
      currentMarkerTime.setHours(currentMarkerTime.getHours() + intervalHours);
    }
    iterations++;
  }

  if (iterations >= maxIterations) {
    console.error("Max iterations reached in hour marker generation."); // Keep server-side error log
  }

  // Refinement: Add specific start time marker if needed for hourly intervals
  if (intervalHours < 24) {
    const actualStartTimeLabel = formatMarkerTimeLabel(timelineStart);
    const firstMarker = markers[0];
    const needsStartLabel = !firstMarker || firstMarker.position !== '0%' || firstMarker.time !== actualStartTimeLabel;

    if (needsStartLabel) {
      // Check if the first calculated marker is very close to the actual start time
      // This avoids adding a duplicate label like "8:00 AM" at 0% if the first interval marker is also 8:00 AM
      let firstMarkerTimeMs = Infinity;
      if (firstMarker && firstMarker.position.startsWith('0')) { // Check if first marker is at/near 0%
        // Attempt to parse time (this part is inherently fragile)
        try {
          const parts = firstMarker.time.match(/(\d+):(\d+)\s*(AM|PM)/i);
          if (parts) {
            let hours = parseInt(parts[1], 10);
            const minutes = parseInt(parts[2], 10);
            const isPM = parts[3].toUpperCase() === 'PM';
            if (hours === 12) hours = isPM ? 12 : 0; // Handle 12 AM/PM
            else if (isPM) hours += 12;

            const tempDate = new Date(timelineStart);
            tempDate.setHours(hours, minutes, 0, 0);
            firstMarkerTimeMs = tempDate.getTime();
          } 
        } catch { /* ignore parsing errors */ }
      }

      // Add the start label only if it's significantly different from the first marker time (e.g., > 1 minute)
      if (Math.abs(timelineStartMs - firstMarkerTimeMs) > 60 * 1000) {
          markers.unshift({ time: actualStartTimeLabel, position: '0%' });
      }
      // If the first marker *is* 0% but has the wrong label, replace its label
      else if (firstMarker && firstMarker.position === '0%' && firstMarker.time !== actualStartTimeLabel) {
         firstMarker.time = actualStartTimeLabel;
      }
    }
  }

  // Remove duplicate markers by position (keep first occurrence)
  const uniqueMarkersMap = new Map<string, TimelineMarker>();
  for (const marker of markers) {
    if (!uniqueMarkersMap.has(marker.position)) {
      uniqueMarkersMap.set(marker.position, marker);
    }
  }

  return Array.from(uniqueMarkersMap.values());
} 