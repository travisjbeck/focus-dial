"use client";

import { useState, FormEvent } from "react";
import { useRouter } from "next/navigation";
import axios from "axios";
import Link from "next/link";

export default function CreateProjectPage() {
  const router = useRouter();
  const [name, setName] = useState("");
  const [color, setColor] = useState("#3B82F6"); // Default blue color
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  // Predefined color options
  const colorOptions = [
    { value: "#3B82F6", label: "Blue" },
    { value: "#10B981", label: "Green" },
    { value: "#F59E0B", label: "Yellow" },
    { value: "#EF4444", label: "Red" },
    { value: "#8B5CF6", label: "Purple" },
    { value: "#EC4899", label: "Pink" },
    { value: "#6B7280", label: "Gray" },
    { value: "#000000", label: "Black" },
  ];

  const handleSubmit = async (e: FormEvent) => {
    e.preventDefault();

    if (!name.trim()) {
      setError("Project name is required");
      return;
    }

    try {
      setLoading(true);
      setError(null);

      await axios.post("/api/projects", { name, color });

      // Navigate back to projects page on success
      router.push("/projects");
      router.refresh();
    } catch (err) {
      console.error("Error creating project:", err);
      setError("Failed to create project. Please try again.");
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="container mx-auto px-4">
      <div className="flex items-center justify-between mb-8">
        <h1 className="text-3xl font-bold text-gray-100">Create New Project</h1>
        <Link 
          href="/projects" 
          className="px-4 py-2 text-sm font-medium text-white bg-black rounded-md border border-gray-800 hover:bg-gray-900 focus:outline-none focus:ring-2 focus:ring-gray-500 flex items-center"
        >
          <svg
            className="w-5 h-5 mr-1"
            fill="none"
            stroke="currentColor"
            viewBox="0 0 24 24"
            xmlns="http://www.w3.org/2000/svg"
          >
            <path
              strokeLinecap="round"
              strokeLinejoin="round"
              strokeWidth="2"
              d="M10 19l-7-7m0 0l7-7m-7 7h18"
            ></path>
          </svg>
          Back to Projects
        </Link>
      </div>

      <div className="bg-black rounded-lg shadow-lg border border-gray-800 p-6 max-w-2xl mx-auto">
        {error && (
          <div className="bg-black border border-red-800 text-red-400 px-4 py-3 rounded-md mb-6">
            <div className="flex items-center">
              <svg
                className="w-5 h-5 mr-2"
                fill="none"
                stroke="currentColor"
                viewBox="0 0 24 24"
                xmlns="http://www.w3.org/2000/svg"
              >
                <path
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  strokeWidth="2"
                  d="M12 8v4m0 4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z"
                />
              </svg>
              <span>{error}</span>
            </div>
          </div>
        )}

        <form onSubmit={handleSubmit}>
          <div className="mb-6">
            <label htmlFor="name" className="block text-gray-300 mb-2">
              Project Name
            </label>
            <input
              type="text"
              id="name"
              className="w-full px-3 py-2 bg-black border border-gray-700 rounded-md text-white focus:outline-none focus:ring-2 focus:ring-gray-500"
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="Enter project name"
              required
            />
          </div>

          <div className="mb-6">
            <label className="block text-gray-300 mb-2">Project Color</label>
            <div className="flex items-center">
              <div
                className="w-10 h-10 rounded-md mr-4 shadow-inner border border-gray-700"
                style={{ backgroundColor: color }}
              />
              <select
                className="w-full px-3 py-2 bg-black border border-gray-700 rounded-md text-white focus:outline-none focus:ring-2 focus:ring-gray-500"
                value={color}
                onChange={(e) => setColor(e.target.value)}
              >
                {colorOptions.map((option) => (
                  <option key={option.value} value={option.value}>
                    {option.label}
                  </option>
                ))}
              </select>
            </div>
          </div>

          <div className="flex justify-end space-x-3">
            <Link 
              href="/projects" 
              className="px-4 py-2 border border-gray-800 rounded-md text-white hover:bg-gray-900"
            >
              Cancel
            </Link>
            <button
              type="submit"
              className="px-4 py-2 bg-white text-black rounded-md hover:bg-gray-200 flex items-center"
              disabled={loading}
            >
              {loading ? (
                <>
                  <div className="inline-block h-4 w-4 animate-spin rounded-full border-2 border-solid border-white border-r-transparent mr-2"></div>
                  Creating...
                </>
              ) : (
                <>
                  <svg
                    className="w-5 h-5 mr-1"
                    fill="none"
                    stroke="currentColor"
                    viewBox="0 0 24 24"
                    xmlns="http://www.w3.org/2000/svg"
                  >
                    <path
                      strokeLinecap="round"
                      strokeLinejoin="round"
                      strokeWidth="2"
                      d="M12 6v6m0 0v6m0-6h6m-6 0H6"
                    ></path>
                  </svg>
                  Create Project
                </>
              )}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
