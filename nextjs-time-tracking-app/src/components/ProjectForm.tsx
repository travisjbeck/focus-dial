"use client";

import { useState, FormEvent } from "react";
import { useRouter } from "next/navigation";
import axios from "axios";
import { Project } from "@/lib/db";

interface ProjectFormProps {
  project?: Project;
  isEditing?: boolean;
}

export default function ProjectForm({
  project,
  isEditing = false,
}: ProjectFormProps) {
  const router = useRouter();
  const [name, setName] = useState(project?.name || "");
  const [color, setColor] = useState(project?.color || "#3B82F6"); // Default blue color
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const handleSubmit = async (e: FormEvent) => {
    e.preventDefault();

    if (!name || !color) {
      setError("Name and color are required");
      return;
    }

    setIsSubmitting(true);
    setError(null);

    try {
      if (isEditing && project?.id) {
        // Update existing project
        await axios.put(`/api/projects/${project.id}`, {
          name,
          color,
        });
      } else {
        // Create new project
        await axios.post("/api/projects", {
          name,
          color,
        });
      }

      // Redirect to projects page
      router.push("/projects");
      router.refresh();
    } catch (err) {
      console.error("Error saving project:", err);
      setError("Failed to save project. Please try again.");
    } finally {
      setIsSubmitting(false);
    }
  };

  return (
    <form onSubmit={handleSubmit} className="bg-white rounded-lg shadow p-6">
      <h2 className="text-xl font-semibold mb-4">
        {isEditing ? "Edit Project" : "Create New Project"}
      </h2>

      {error && (
        <div className="bg-red-100 border border-red-400 text-red-700 px-4 py-3 rounded mb-4">
          {error}
        </div>
      )}

      <div className="mb-4">
        <label htmlFor="name" className="block text-gray-700 mb-2">
          Project Name
        </label>
        <input
          type="text"
          id="name"
          value={name}
          onChange={(e) => setName(e.target.value)}
          className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-blue-500"
          placeholder="Enter project name"
          required
        />
      </div>

      <div className="mb-6">
        <label htmlFor="color" className="block text-gray-700 mb-2">
          Project Color
        </label>
        <div className="flex items-center">
          <input
            type="color"
            id="color"
            value={color}
            onChange={(e) => setColor(e.target.value)}
            className="h-10 w-10 border border-gray-300 rounded mr-2"
          />
          <input
            type="text"
            value={color}
            onChange={(e) => setColor(e.target.value)}
            className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-blue-500"
            placeholder="#RRGGBB"
            pattern="^#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$"
            required
          />
        </div>
      </div>

      <div className="flex justify-end space-x-2">
        <button
          type="button"
          onClick={() => router.back()}
          className="px-4 py-2 border border-gray-300 rounded text-gray-700 hover:bg-gray-100"
        >
          Cancel
        </button>
        <button
          type="submit"
          disabled={isSubmitting}
          className={`px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 ${
            isSubmitting ? "opacity-70 cursor-not-allowed" : ""
          }`}
        >
          {isSubmitting
            ? "Saving..."
            : isEditing
            ? "Update Project"
            : "Create Project"}
        </button>
      </div>
    </form>
  );
}
