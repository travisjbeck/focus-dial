"use client";

import React from "react";
import { useProjects } from "@/lib/hooks/useProjects";
import Link from "next/link";
import type { Database } from "@/types/supabase";

type Project = Database["public"]["Tables"]["projects"]["Row"];

export default function ProjectList() {
  const { data: projects, isLoading, error, refetch } = useProjects();

  if (isLoading) {
    return <div className="text-center text-muted-foreground">Loading projects...</div>;
  }

  if (error) {
    return (
      <div className="text-center text-destructive-foreground bg-destructive p-4 rounded-md">
        Error loading projects: {error.message}
        <button
          onClick={() => refetch()}
          className="ml-2 px-2 py-1 text-xs bg-destructive-foreground text-destructive rounded-md hover:opacity-90"
        >
          Retry
        </button>
      </div>
    );
  }

  if (!projects || projects.length === 0) {
    return (
      <div className="bg-card text-card-foreground p-6 rounded-lg shadow border border-border text-center">
        <p className="text-muted-foreground">No projects found.</p>
        <Link
          href="/projects/create"
          className="mt-2 inline-block text-primary hover:underline"
        >
          Create your first project
        </Link>
      </div>
    );
  }

  // Helper to get Tailwind background color class from hex
  const getBgColor = (hexColor: string | null): string => {
    // Basic implementation, might need refinement for dark/light mode, contrast
    // Using inline style as Tailwind cannot generate classes dynamically from arbitrary hex values easily
    // Consider a predefined color mapping if possible
    return hexColor ? hexColor : "#808080"; // Default to gray if no color
  };

  return (
    <div className="space-y-4">
      {projects.map((project: Project) => {
        return (
          <div
            key={project.id}
            className="bg-card rounded-lg shadow p-4 border border-border flex justify-between items-center"
          >
            <div className="flex items-center">
              <span
                className="w-4 h-4 rounded-full mr-3 flex-shrink-0"
                style={{ backgroundColor: getBgColor(project.color) }}
              ></span>
              <span className="text-card-foreground font-medium">{project.name}</span>
            </div>
            {/* Add Edit/Archive buttons later */}
            <Link
              href={`/projects/${project.id}`}
              className="text-sm text-muted-foreground hover:text-foreground hover:underline"
            >
              View Details
            </Link>
          </div>
        );
      })}
    </div>
  );
}
