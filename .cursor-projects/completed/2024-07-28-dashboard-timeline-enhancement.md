# Project: Dashboard Timeline Widget Enhancement
- **Created**: 2024-07-28
- **Status**: Completed
- **Last Updated**: 2024-07-28

## Context & Requirements
The current dashboard features a "Today's Activity" timeline widget which only displays time entries for the current day. This project aims to enhance this widget by adding a dropdown selector allowing users to choose from various predefined time ranges (e.g., Today, Yesterday, This Week, This Month, Last 7 Days, Last 30 Days, This Year). The timeline visualization and relevant summary data within the widget must update dynamically based on the selected range.

## Development Plan
### Phase 1: UI Component & State
- [x] Task 1: Add a dropdown component (e.g., `<select>`) to the "Today's Activity" card header in `nextjs-time-tracking-app/src/app/page.tsx`.
- [x] Task 2: Define the time range options (e.g., "Today", "Yesterday", "Week to Date", "Month to Date", "Year to Date", "Last 7 Days", "Last 30 Days").
- [x] Task 3: Implement state management (`useState`) in `page.tsx` to hold the currently selected time range option. Default to "Today".

### Phase 2: Date Range Calculation Logic
- [x] Task 1: Create a utility function `getDateRangeForOption(option)` in a suitable location (e.g., `src/lib/utils/dateUtils.ts`) that takes a selected time range option string and returns an object `{ start: Date, end: Date }`.
- [x] Task 2: Implement the date calculation logic within this function for all defined time range options. Ensure correct handling of date boundaries (start/end of day, week, month, year).
- [x] Task 3: Update the `timelineRange` calculation in `page.tsx` to use this new utility function based on the selected state value.

### Phase 3: Data Filtering & Display Logic
- [x] Task 1: Modify the `todayEntries` memoization (rename to `selectedRangeEntries`) to filter the main `timeEntries` based on the calculated `timelineRange` from Phase 2.
- [x] Task 2: Update the `entriesByProject` memoization to use `selectedRangeEntries`.
- [x] Task 3: Update the timeline rendering logic (JSX) to use `selectedRangeEntries` and associated data.
- [x] Task 4: Ensure `getTimelinePosition` calculates positions correctly relative to the potentially variable `timelineRange`.
- [x] Task 5: Update the `hourMarkers` generation logic to be appropriate for the selected time range (e.g., maybe simplify or change markers for ranges longer than a day). Start simple: Keep hourly for Today/Yesterday, maybe just show start/end dates for longer ranges.
- [x] Task 6: Update the message displayed when no entries are found for the selected range (e.g., "No entries recorded for [Selected Range]").

### Phase 4: Refinement & Summary Stats
- [x] Task 1: Update the "Total Hours" stat displayed in the overview section *if* it makes sense contextually, or add a specific "Total Hours (Selected Range)" display within the activity widget itself. For now, let's add a total duration display *within* the activity widget for the selected period.
- [x] Task 2: Review component logic for clarity, performance, and type safety.
- [x] Task 3: Verify loading states and error handling remain robust.
- [x] Task 4: Ensure consistent styling and responsiveness.

## Notes & References
- Main component file: `nextjs-time-tracking-app/src/app/page.tsx`
- Hooks: `useTimeEntries`, `useProjects`
- Potential Utility File: `src/lib/utils/dateUtils.ts`
- Remember to handle timezone consistency. Dates from Supabase are likely UTC (`timestamp with time zone`). Calculations should ideally work correctly regardless of the user's local timezone. Using `Date` objects and their methods should handle this reasonably well, but be mindful. Start/end of day should be based on local time interpretation for user clarity (e.g., "Today" means *local* today). 