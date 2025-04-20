# Project: Timeline Component Refactor
- **Created**: 2025-04-20
- **Status**: Completed
- **Last Updated**: 2025-04-20

## Context & Requirements
The current timeline component on the main dashboard (`page.tsx`) has several issues:
1.  **Incorrect Marker Spacing**: Persistent difficulty in achieving mathematically correct proportional spacing between time markers when the timeline start time is dynamic (e.g., starting at 8 AM instead of midnight). The visual distance between markers does not accurately reflect the time duration they represent relative to the total visible duration.
2.  **Limited Date Range**: The timeline currently only displays "Today's Activity".
3.  **Future Requirement**: The timeline needs to be refactored to support various selectable date ranges (e.g., Today, Yesterday, This Week, Last 7 Days, This Month, Last 30 Days).
4.  **Robustness**: The underlying logic for calculating the timeline range, entry positions, and marker positions needs to be more robust and reliable for arbitrary start/end times and durations.

The goal is to create a reusable, robust, and accurately scaled timeline component that can display time entries for different date periods.

## Development Plan
### Phase 1: Core Logic & Data Handling Refactor
- [x] Define supported date range presets (e.g., `today`, `yesterday`, `this_week`, `last_7_days`, `this_month`, `last_30_days`).
- [x] Create a reusable utility function `calculateDateRange(preset: string): { start: Date, end: Date }` that returns the start and end Date objects for a given preset.
- [x] Refactor data fetching/filtering (`useTimeEntries` or dashboard logic) to accept a `startDate` and `endDate` to retrieve only the relevant entries for the selected period.
- [x] Create a reusable utility function or hook `calculateTimelineMetrics(entries: TimeEntry[], range: { start: Date, end: Date }): { timelineRange: { start: Date, end: Date }, earliestEntryTime?: number }`. This function will determine the *actual* display start time (e.g., 8 AM, earliest entry, or range start) and the end time for the timeline visualization based on the selected range and the entries within it. It should handle default start times (like 8 AM for 'today').
- [x] Refactor `getTimelinePosition` to reliably calculate `left` and `width` percentages based *only* on the `timelineRange` start/end times passed to it. Ensure it handles edge cases like zero duration.
- [x] Refactor `hourMarkers` generation logic:
    - [x] It should accept the calculated `timelineRange` as input.
    - [x] It must generate markers (label + position %) based *solely* on the provided range's start, end, and duration.
    - [x] Ensure mathematically correct proportional spacing for all markers relative to the timeline duration.
    - [x] Handle dynamic start times correctly (e.g., explicitly mark the start time).
    - [x] Consider appropriate marker intervals based on the total duration (e.g., hourly for <= 24h, daily for > 24h).

### Phase 2: UI Integration & Component Update
- [x] Introduce state management in `page.tsx` (or a parent component/context) to hold the currently selected date range preset (defaulting to `today`).
- [x] Add UI elements (e.g., a simple dropdown or segmented control) to the dashboard page (`page.tsx`) allowing the user to select a date range preset.
- [x] Connect the UI selector to the state variable and trigger data refetching/recalculation when the preset changes.
- [x] Update the dashboard component (`page.tsx`) to:
    - [x] Calculate the date range using the new utility based on the selected preset.
    - [x] Fetch/filter entries based on the calculated date range.
    - [x] Calculate the timeline visualization metrics (actual start/end) using the new utility/hook.
    - [x] Pass the calculated `timelineRange` to the `hourMarkers` generation logic.
    - [x] Pass the calculated `timelineRange` and entry times to `getTimelinePosition`.
- [x] Verify that the existing CSS Grid/Overlay structure correctly renders the timeline with the refactored logic and accurately spaced markers.
- [x] Test responsiveness of the timeline with the new logic.

### Phase 3: Testing & Refinement
- [x] Test timeline rendering and spacing with all defined date range presets.
- [x] Test edge cases:
    - [x] Preset ranges with no time entries.
    - [ ] Ranges where the earliest entry is before a default start time (e.g., entry at 7 AM when default is 8 AM). *(Covered by 'today' logic)*
    - [x] Short duration ranges (e.g., a single day).
    - [x] Longer duration ranges (e.g., 7 days, 30 days) - check marker interval logic.
- [x] Fix any remaining visual alignment or spacing bugs.
- [x] Refine marker intervals and labeling for longer date ranges for better readability.

## Notes & References
- Current timeline implementation is in `nextjs-time-tracking-app/src/app/page.tsx`.
- CSS Variables for theming are in `nextjs-time-tracking-app/src/app/globals.css`.
- Need to ensure calculations involving Date objects handle timezones correctly if necessary, although relying on client-side calculations might mitigate this if the date range selection and display happen consistently in the user's local time. Supabase stores timestamps with timezone, so fetched data is reliable. 