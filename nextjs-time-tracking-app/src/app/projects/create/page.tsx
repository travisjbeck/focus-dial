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
        <Link href="/projects" className="btn btn-secondary flex items-center">
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

      <div className="card max-w-2xl mx-auto">
        {error && (
          <div className="bg-red-900/30 border border-red-800 text-red-300 px-4 py-3 rounded mb-6">
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
            <label htmlFor="name" className="form-label">
              Project Name
            </label>
            <input
              type="text"
              id="name"
              className="form-input"
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="Enter project name"
              required
            />
          </div>

          <div className="mb-6">
            <label className="form-label">Project Color</label>
            <div className="flex items-center">
              <div
                className="w-10 h-10 rounded-md mr-4 shadow-inner border border-gray-600"
                style={{ backgroundColor: color }}
              />
              <select
                className="form-select"
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
            <Link href="/projects" className="btn btn-secondary">
              Cancel
            </Link>
            <button
              type="submit"
              className="btn btn-primary flex items-center"
              disabled={loading}
            >
              {loading ? (
                <>
                  <div className="loading-spinner loading-spinner-sm mr-2"></div>
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
