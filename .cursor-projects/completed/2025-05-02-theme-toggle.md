# Project: Theme Toggle Implementation
- **Created**: 2025-05-02
- **Status**: Planned
- **Last Updated**: 2025-05-02

## Context & Requirements
The application currently only supports a dark theme. We need to add a theme toggle (light/dark/system) to allow users to choose their preferred theme. The toggle should be placed in the user section of the sidebar (bottom-left) and respect the operating system's preference by default. User preference should be persisted.

**Requirements:**
- Implement light and dark themes.
- Add a theme toggle component (using ShadCN UI).
- Place toggle near the user avatar/logout button in the sidebar.
- Group toggle and logout button within a suitable ShadCN component (e.g., DropdownMenu).
- Default theme should respect the user's OS preference.
- User's selected theme should be saved (e.g., in localStorage).
- Ensure ShadCN components adapt correctly to both themes.

## Development Plan
### Phase 1: Setup & Dependencies
- [ ] Install `next-themes` library.
- [ ] Add ShadCN `DropdownMenu` component.
- [ ] Wrap the application layout (`src/app/layout.tsx`) with `ThemeProvider` from `next-themes`.
- [ ] Verify Tailwind CSS dark mode configuration (`darkMode: "class"`).

### Phase 2: UI Implementation
- [ ] Modify `src/app/layout.tsx`.
- [ ] Replace the current user info display (avatar, email, logout button) with a `DropdownMenuTrigger` (likely wrapping the avatar/email).
- [ ] Create `DropdownMenuContent` containing:
    - [ ] User email display (non-interactive).
    - [ ] Theme selection options (Light, Dark, System) possibly as `DropdownMenuRadioGroup` or individual `DropdownMenuItem`s.
    - [ ] Logout button as a `DropdownMenuItem`.
- [ ] Add icons (Sun, Moon, Laptop) to theme options for clarity.

### Phase 3: Theme Switching Logic
- [ ] Create a client component (`'use client';`) to encapsulate the theme toggle logic within the `DropdownMenu`.
- [ ] Use the `useTheme` hook from `next-themes` within the client component.
- [ ] Connect theme selection menu items to the `setTheme` function from `useTheme`.
- [ ] Ensure the component correctly displays the currently active theme state.

### Phase 4: Styling & Refinement
- [ ] Adjust styling of the `DropdownMenu` and its items to fit aesthetically within the sidebar.
- [ ] Test theme switching thoroughly, ensuring all UI elements (including ShadCN components) adapt correctly.
- [ ] Verify system preference detection and persistence in localStorage.

## Notes & References
- `next-themes` documentation: https://github.com/pacocoursey/next-themes
- ShadCN UI `DropdownMenu`: https://ui.shadcn.com/docs/components/dropdown-menu
- Tailwind CSS Dark Mode: https://tailwindcss.com/docs/dark-mode 