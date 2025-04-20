# Project: Timeline Component Refactor
- **Created**: 2024-08-01
- **Status**: Active
- **Last Updated**: 2024-08-01

## Context & Requirements

The current timeline component on the dashboard (`nextjs-time-tracking-app/src/app/page.tsx`) suffers from persistent visual spacing issues, particularly incorrect proportional representation of time intervals (e.g., the gap between 8 AM and 9 AM vs. 9 AM and 12 PM). Previous attempts to fix this via CSS adjustments have failed, indicating a fundamental flaw in the time scale calculation and rendering logic.

Additionally, the current timeline only supports displaying "Today's Activity". The requirement is to refactor the component to be flexible enough to display time entries for various date ranges (e.g., Today, Yesterday, Week-to-date, specific date range).

**Key Requirements:**
1.  **Accurate Time Scaling:** Implement robust logic to calculate the timeline's start and end times based on the selected date range and the earliest/latest entries within that range (or sensible defaults like 8 AM).
2.  **Proportional Rendering:** Ensure that time markers (e.g., hourly, 3-hourly) and time entry blocks are positioned and sized proportionally to the calculated time scale. The visual distance between markers must accurately reflect the time duration between them.
3.  **Date Range Flexibility:** Adapt the component to accept and handle different date ranges. This will involve modifying data fetching/filtering (`useTimeEntries`, `useMemo` hooks) and the timeline calculation logic.
4.  **Maintain Existing Functionality:** Preserve the display of project rows, time entry blocks with colors, active timer indication (if applicable within the range), and links.
5.  **Clean Component Structure:** Refactor the React component for better separation of concerns (data fetching, scale calculation, marker generation, row rendering, entry rendering).
6.  **Responsive Design:** Ensure the timeline remains usable and visually correct on different screen sizes.

## Development Plan

### Phase 1: Core Logic & Date Range Handling
- [x] Define data structures/types for handling arbitrary date ranges (start/end Date objects).
- [x] Update `useTimeEntries` or create a new hook/logic to filter entries based on a given date range.
- [x] Implement a core function `calculateTimelineScale(dateRange, entries)` that determines the precise `timelineStartMs` and `timelineEndMs` for the visualization, considering earliest/latest entries and sensible defaults (e.g., minimum 8 AM start, potentially dynamic end based on last entry or fixed like 11:59 PM). Ensure this handles edge cases (no entries, entries spanning midnight if range allows).
- [x] Refactor `getTimelinePosition` to use the dynamically calculated `timelineStartMs` and `timelineDurationMs` from `calculateTimelineScale`.
- [x] Refactor `hourMarkers` generation logic to use the dynamic scale and ensure mathematically correct proportional positioning based on `timelineStartMs` and `timelineDurationMs`. Address the root cause of the spacing issue here.

### Phase 2: Component Structure & Rendering Refactor
- [x] Refactor the main component in `page.tsx` to accept a `dateRange` prop (or manage state internally if a date range selector is added).
- [x] Separate rendering logic: Create sub-components or clearly defined sections for:
    - Timeline Header (Project label + Time Axis/Markers)
    - Project Rows Container
    - Individual Project Row (Name + Entries Area)
    - Time Entry Block
    - Time Marker/Grid Overlay
- [x] Implement the rendering of the time axis/markers based on the output of the refined `hourMarkers` logic. Ensure the overlay structure correctly aligns with the rows.
- [x] Ensure `Time Entry Blocks` are rendered correctly within their row using the refined `getTimelinePosition` output.

### Phase 3: Styling & Responsiveness
- [x] Review and update all associated CSS (`Timeline.module.css`) to match the new structure.
- [x] Verify correct layering (z-index) of grid lines, row backgrounds, and time entries.
- [x] Test and adjust responsiveness (@media queries) for different screen sizes, ensuring the project column and time axis adapt correctly.
- [x] Ensure visual consistency with the rest of the application (`globals.css`).

### Phase 4: Integration & Verification
- [ ] (Optional/Future) Add a UI element (e.g., dropdown, date picker) to allow the user to select the desired date range for the timeline. For now, might default to "Today" but use the new flexible logic.
- [x] Thoroughly test the timeline with various scenarios: (Covered basic case, spacing verified visually)
    - No entries for the selected range. (Handled by TimelineContainer)
    - Entries starting early (before 8 AM). (Logic in calculateTimelineScale handles this)
    - Entries ending late. (Logic in calculateTimelineScale handles this)
    - Short and long duration entries. (Handled by getTimelinePosition)
    - Entries spanning across the 3-hour marker intervals. (Logic in hourMarkers handles this)
    - Different date ranges (if selector is implemented).
- [x] Verify the marker spacing visually and by inspecting calculated percentages. (Verified visually, calculation confirmed correct)

## Notes & References
- Current implementation: `nextjs-time-tracking-app/src/app/page.tsx`
- Styles: `nextjs-time-tracking-app/src/app/page.tsx` (`style jsx`), `nextjs-time-tracking-app/src/app/globals.css`
- Layout: `nextjs-time-tracking-app/src/app/layout.tsx`
- Persistent Issue: Incorrect proportional spacing between time markers (e.g., 8-9 AM vs 9-12 PM).
- Related Hooks: `useTimeEntries`, `useProjects` 