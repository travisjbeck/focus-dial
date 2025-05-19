"use client";

import Link from "next/link";
import ProjectList from "@/components/ProjectList";

export default function ProjectsPage() {
  return (
    <div className="container mx-auto px-4">
      <div className="flex items-center justify-between mb-6">
        <h1 className="text-xl font-bold text-foreground">Projects</h1>
        <Link
          href="/projects/create"
          className="px-4 py-2 text-sm font-medium bg-primary text-primary-foreground rounded-md hover:bg-primary/90 focus:outline-none focus:ring-2 focus:ring-ring"
        >
          Add Project
        </Link>
      </div>

      <ProjectList />
    </div>
  );
}
