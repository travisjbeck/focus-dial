# Focus Dial - Time Tracking Frontend

This is the frontend for the Focus Dial time tracking application. It's a React Single Page Application (SPA) that communicates with the backend API to display time tracking data, manage projects, and generate invoices.

## Technology Stack

- React 18
- TypeScript
- Vite
- React Router v6
- Tailwind CSS v4
- shadcn/ui components
- lucide-react for icons
- Zustand for state management
- date-fns for date formatting

## Prerequisites

- Node.js 16.x or higher
- npm or yarn

## Installation

1. Clone the repository
2. Navigate to the frontend directory:
   ```
   cd time-tracking-app/frontend
   ```
3. Install dependencies:
   ```
   npm install
   # or
   yarn install
   ```

## Development

To start the development server:

```
npm run dev
# or
yarn dev
```

This will start the Vite development server, typically on http://localhost:5173

During development, API requests will be automatically proxied to the backend running on http://localhost:3000.

## Building for Production

To build the application for production:

```
npm run build
# or
yarn build
```

This will create optimized production files in the `build` directory. These files should be served by the backend Express server.

## Project Structure

- `src/components` - React components
  - `src/components/ui` - UI components from shadcn/ui
  - `src/components/layout` - Layout components (sidebar, header, etc.)
- `src/pages` - Page components corresponding to routes
- `src/lib` - Utility functions
- `src/hooks` - Custom React hooks
- `src/store` - Zustand store for state management
- `src/types` - TypeScript type definitions

## Deployment

The frontend is designed to be built and served directly by the backend Express server running on the Raspberry Pi. After building, the output files in the `build` directory should be placed in the correct location for the Express server to serve them. 