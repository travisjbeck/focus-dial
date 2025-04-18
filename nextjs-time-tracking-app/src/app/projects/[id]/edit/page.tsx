"use client";

import { useState, useEffect, useTransition } from "react";
import { useRouter } from "next/navigation";
import Link from "next/link";
// Removed axios import

// Import server actions
import { getProjectById, updateProject } from "@/app/(auth)/actions";
// Assuming Project type is defined/exported in actions.ts or types/supabase.ts
// We might need to import it if getProjectById doesn't return a clearly typed object
// import type { Database } from "@/types/supabase"; // Removed unused import
// type Project = Database["public"]["tables"]["projects"]["Row"]; // Removed unused import

interface EditProjectPageProps {
  params: {
    id: string; // ID from the route
  };
}

export default function EditProjectPage({ params }: EditProjectPageProps) {
  const router = useRouter();
  const [isPending, startTransition] = useTransition(); // For form submission state
  const projectId = parseInt(params.id, 10); // Convert param to number for action

  const [formData, setFormData] = useState({
    name: "",
    color: "#ffffff",
  });
  const [isLoading, setIsLoading] = useState(true); // Still needed for initial load
  const [error, setError] = useState<string | null>(null);
  // const [isSubmitting, setIsSubmitting] = useState(false); // Replace with useTransition

  // Fetch initial project data using the server action
  useEffect(() => {
    const fetchProject = async () => {
      setIsLoading(true);
      setError(null);
      const project = await getProjectById(projectId);
      if (project) {
        setFormData({ name: project.name, color: project.color || "#ffffff" });
      } else {
        setError("Failed to load project data or project not found.");
        // Optional: redirect if project not found for user?
        // router.push('/projects');
      }
      setIsLoading(false);
    };

    if (!isNaN(projectId)) {
      fetchProject();
    } else {
      setError("Invalid project ID.");
      setIsLoading(false);
    }
    // Intentionally excluding projectId from dependency array if we only want to fetch once
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []); // Empty array means fetch only on mount

  const handleChange = (
    e: React.ChangeEvent<HTMLInputElement | HTMLSelectElement>
  ) => {
    const { name, value } = e.target;
    setFormData((prev) => ({
      ...prev,
      [name]: value,
    }));
  };

  // Handle form submission using the server action
  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);

    if (!formData.name.trim()) {
      setError("Project name is required");
      return;
    }
    if (isNaN(projectId)) {
      setError("Invalid project ID.");
      return;
    }

    startTransition(async () => {
      const result = await updateProject(projectId, formData);

      if (result.success) {
        router.push("/projects"); // Navigate back on success
        // router.refresh(); // Revalidation should handle refresh
      } else {
        setError(result.error || "Failed to update project.");
      }
    });
  };

  // Predefined color options - kept from original component
  const colorOptions = [
    { value: "#3B82F6", label: "Blue" }, // Default blue color
    { value: "#10B981", label: "Green" },
    { value: "#F59E0B", label: "Yellow" },
    { value: "#EF4444", label: "Red" },
    { value: "#8B5CF6", label: "Purple" },
    { value: "#EC4899", label: "Pink" },
    { value: "#6B7280", label: "Gray" },
    { value: "#FFFFFF", label: "White" },
    { value: "#000000", label: "Black" },
  ];

  // Loading state for initial fetch
  if (isLoading) {
    return (
      <div className="container mx-auto px-4">
        <div className="flex justify-center items-center py-8">
          <div className="animate-spin rounded-full h-8 w-8 border-t-2 border-b-2 border-white"></div>
        </div>
      </div>
    );
  }

  return (
    <div className="container mx-auto px-4">
      <div className="max-w-md mx-auto">
        <div className="flex items-center justify-between mb-6">
          <h1 className="text-xl font-bold text-white">Edit Project</h1>
          {/* Link back to specific project or all projects? */}
          <Link
            href={`/projects/${params.id}`}
            className="text-sm text-gray-400 hover:text-white"
          >
            Back to Project
          </Link>
        </div>

        {/* Display fetch error or submission error */}
        {error && (
          <div className="bg-red-900/30 border border-red-800 text-red-300 px-4 py-3 rounded mb-4">
            <span>{error}</span>
          </div>
        )}

        <div className="bg-gray-800 p-6 rounded-lg shadow border border-gray-700">
          <form onSubmit={handleSubmit}>
            <div className="mb-4">
              <label
                htmlFor="name"
                className="block text-sm font-medium text-gray-300 mb-1"
              >
                Project Name
              </label>
              <input
                type="text"
                id="name"
                name="name"
                value={formData.name}
                onChange={handleChange}
                className="bg-gray-700 border border-gray-600 text-white text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full p-2.5 placeholder-gray-400"
                placeholder="Enter project name"
                disabled={isPending} // Use isPending from useTransition
                required // Added required attribute
              />
            </div>

            <div className="mb-6">
              <label
                htmlFor="color"
                className="block text-sm font-medium text-gray-300 mb-1"
              >
                Project Color
              </label>
              <div className="flex items-center space-x-3">
                {/* Color Picker Input */}
                <input
                  type="color"
                  id="color-picker"
                  name="color" // Ensure name matches state
                  value={formData.color}
                  onChange={handleChange}
                  className="p-0 w-10 h-10 block bg-gray-700 border border-gray-600 cursor-pointer rounded-lg disabled:opacity-50 disabled:pointer-events-none"
                  title="Select color"
                  disabled={isPending}
                />
                {/* Color Select Dropdown */}
                <select
                  id="color-select"
                  name="color" // Ensure name matches state
                  value={formData.color}
                  onChange={handleChange}
                  className="bg-gray-700 border border-gray-600 text-white text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full p-2.5"
                  disabled={isPending}
                >
                  {colorOptions.map((option) => (
                    <option key={option.value} value={option.value}>
                      {option.label} ({option.value})
                    </option>
                  ))}
                </select>
              </div>
            </div>

            <div className="flex justify-end gap-2">
              {/* Link back to project detail page */}
              <Link
                href={`/projects/${params.id}`}
                className="px-4 py-2 text-sm font-medium text-gray-300 bg-gray-600 rounded-lg hover:bg-gray-500"
              >
                Cancel
              </Link>
              <button
                type="submit"
                className="px-4 py-2 text-sm font-medium text-white bg-blue-600 rounded-lg hover:bg-blue-700 focus:ring-4 focus:outline-none focus:ring-blue-800 disabled:opacity-50"
                disabled={isPending} // Use isPending
              >
                {isPending ? "Saving..." : "Save Changes"}
              </button>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
}
