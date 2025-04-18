"use client";

import React from "react";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";

// Create a client
// const queryClient = new QueryClient();
// It's recommended to create the client inside the component state to avoid
// sharing data between users during server-side rendering or requests.

export default function ReactQueryProvider({
  children,
}: {
  children: React.ReactNode;
}) {
  // Use state to ensure a new client is created per request/render on server,
  // and reused on client.
  const [queryClient] = React.useState(
    () =>
      new QueryClient({
        defaultOptions: {
          queries: {
            // Configure default options if needed, e.g., staleTime
            staleTime: 60 * 1000, // 1 minute
          },
        },
      })
  );

  return (
    <QueryClientProvider client={queryClient}>
      {children}
      {/* Optionally add ReactQueryDevtools for development */}
      {/* <ReactQueryDevtools initialIsOpen={false} /> */}
    </QueryClientProvider>
  );
}
