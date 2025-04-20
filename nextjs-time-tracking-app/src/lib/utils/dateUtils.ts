import type { TimeRangeOption } from '@/app/page'; // Import the type from page.tsx

interface DateRange {
  start: Date;
  end: Date;
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