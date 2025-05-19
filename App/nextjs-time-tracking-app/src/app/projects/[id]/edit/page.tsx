"use client";

import { useState, useEffect, useTransition } from "react";
import { useRouter } from "next/navigation";
import Link from "next/link";
// Removed axios import
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";

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

  const handleColorChange = (value: string) => {
    setFormData((prev) => ({
      ...prev,
      color: value,
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
          <div className="bg-black border border-red-800 text-red-400 px-4 py-3 rounded-md mb-4">
            <span>{error}</span>
          </div>
        )}

        <div className="bg-black p-6 rounded-lg shadow border border-gray-800">
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
                className="bg-black border border-gray-800 text-white text-sm rounded-md focus:ring-gray-500 focus:border-gray-700 block w-full p-2.5 placeholder-gray-400"
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
                  className="p-0 w-10 h-10 block bg-black border border-gray-800 cursor-pointer rounded-md disabled:opacity-50 disabled:pointer-events-none"
                  title="Select color"
                  disabled={isPending}
                />

                {/* Replace with ShadCN UI Select */}
                <input type="hidden" name="color" value={formData.color} />
                <Select
                  value={formData.color}
                  onValueChange={handleColorChange}
                  disabled={isPending}
                >
                  <SelectTrigger className="w-full bg-black border-gray-800 text-white">
                    <SelectValue placeholder="Select a color" />
                  </SelectTrigger>
                  <SelectContent>
                    {colorOptions.map((option) => (
                      <SelectItem key={option.value} value={option.value}>
                        <div className="flex items-center">
                          <span 
                            className="inline-block w-3 h-3 rounded-full mr-2" 
                            style={{ backgroundColor: option.value }}
                          ></span>
                          {option.label} ({option.value})
                        </div>
                      </SelectItem>
                    ))}
                  </SelectContent>
                </Select>
              </div>
            </div>

            <div className="flex justify-end gap-2">
              <Link
                href={`/projects/${params.id}`}
                className="px-4 py-2 text-sm font-medium text-white bg-black border border-gray-800 rounded-md hover:bg-gray-900 focus:outline-none focus:ring-2 focus:ring-gray-500 disabled:opacity-50"
              >
                Cancel
              </Link>
              <button
                type="submit"
                className="px-4 py-2 text-sm font-medium text-black bg-white rounded-md hover:bg-gray-200 focus:outline-none focus:ring-2 focus:ring-gray-500 disabled:opacity-50"
                disabled={isPending}
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
