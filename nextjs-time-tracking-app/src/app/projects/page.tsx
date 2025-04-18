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
          className="px-4 py-2 text-sm font-medium text-white bg-gray-600 rounded-lg hover:bg-gray-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-offset-gray-900 focus:ring-gray-500"
        >
          Add Project
        </Link>
      </div>

      <ProjectList />
    </div>
  );
}
