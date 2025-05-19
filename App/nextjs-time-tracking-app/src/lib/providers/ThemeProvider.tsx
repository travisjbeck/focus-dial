"use client"

import * as React from "react";
import { ThemeProvider as NextThemesProvider, type ThemeProviderProps } from "next-themes";
// import { type ThemeProviderProps } from "next-themes/dist/types"; // Incorrect path

// Remove custom interface, use imported type directly
/*
interface CustomThemeProviderProps {
  children: React.ReactNode;
  attribute?: string;
  defaultTheme?: string;
  enableSystem?: boolean;
  // Add other props from next-themes if needed
}
*/

export function ThemeProvider({ children, ...props }: ThemeProviderProps) {
  // Pass props directly
  return <NextThemesProvider {...props}>{children}</NextThemesProvider>;
} 