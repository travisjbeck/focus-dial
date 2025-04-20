"use client";

import React from "react";
import { useProjects } from "@/lib/hooks/useProjects";
import Link from "next/link";
import type { Database } from "@/types/supabase";

type Project = Database["public"]["Tables"]["projects"]["Row"];

export default function ProjectList() {
  const { data: projects, isLoading, error, refetch } = useProjects();

  if (isLoading) {
    return <div className="text-center text-gray-400">Loading projects...</div>;
  }

  if (error) {
    return (
      <div className="text-center text-red-500">
        Error loading projects: {error.message}
        <button
          onClick={() => refetch()}
          className="ml-2 px-2 py-1 text-xs bg-black hover:bg-gray-900 text-white rounded-md border border-gray-800"
        >
          Retry
        </button>
      </div>
    );
  }

  if (!projects || projects.length === 0) {
    return (
      <div className="text-center text-gray-500">
        <p>No projects found.</p>
        <Link
          href="/projects/create"
          className="mt-2 inline-block text-blue-400 hover:underline"
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
        // Log the project ID and its type before creating the link
        console.log(
          `[ProjectList] Rendering link for project: ID=${
            project.id
          }, Type=${typeof project.id}`
        );
        return (
          <div
            key={project.id}
            className="bg-black rounded-lg shadow p-4 border border-gray-800 flex justify-between items-center"
          >
            <div className="flex items-center">
              <span
                className="w-4 h-4 rounded-full mr-3 flex-shrink-0"
                style={{ backgroundColor: getBgColor(project.color) }}
              ></span>
              <span className="text-white font-medium">{project.name}</span>
            </div>
            {/* Add Edit/Archive buttons later */}
            <Link
              href={`/projects/${project.id}`}
              className="text-sm text-gray-400 hover:text-white hover:underline"
            >
              View Details
            </Link>
          </div>
        );
      })}
    </div>
  );
}
