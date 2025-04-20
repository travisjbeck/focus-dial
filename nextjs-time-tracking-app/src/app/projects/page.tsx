"use client";

import Link from "next/link";
import ProjectList from "@/components/ProjectList";

export default function ProjectsPage() {
  return (
    <div className="container mx-auto px-4">
      <div className="flex items-center justify-between mb-6">
        <h1 className="text-xl font-bold text-white">Projects</h1>
        <Link
          href="/projects/create"
          className="px-4 py-2 text-sm font-medium text-black bg-white rounded-md hover:bg-gray-200 focus:outline-none focus:ring-2 focus:ring-gray-500"
        >
          Add Project
        </Link>
      </div>

      <ProjectList />
    </div>
  );
}
