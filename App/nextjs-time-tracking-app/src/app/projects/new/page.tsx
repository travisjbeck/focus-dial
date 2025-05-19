"use client";

import { useState } from "react";
import { useRouter } from "next/navigation";
import axios from "axios";
import Link from "next/link";

export default function NewProjectPage() {
  const router = useRouter();
  const [formData, setFormData] = useState({
    name: "",
    color: "#ffffff",
  });
  const [error, setError] = useState<string | null>(null);
  const [isSubmitting, setIsSubmitting] = useState(false);

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const { name, value } = e.target;
    setFormData((prev) => ({
      ...prev,
      [name]: value,
    }));
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);

    if (!formData.name.trim()) {
      setError("Project name is required");
      return;
    }

    try {
      setIsSubmitting(true);
      await axios.post("/api/projects", formData);
      router.push("/projects");
      router.refresh();
    } catch (err) {
      let errorMessage = "Failed to create project. Please try again.";
      if (axios.isAxiosError(err)) {
        errorMessage = err.response?.data?.error || err.message || errorMessage;
        console.error(
          "Error creating project (Axios):",
          err.response?.data || err.message
        );
      } else if (err instanceof Error) {
        errorMessage = err.message;
        console.error("Error creating project (Generic):", err);
      } else {
        console.error("Unknown error creating project:", err);
      }
      setError(errorMessage);
    } finally {
      setIsSubmitting(false);
    }
  };

  return (
    <div className="container mx-auto px-4">
      <div className="max-w-md mx-auto">
        <div className="flex items-center justify-between mb-6">
          <h1 className="text-xl font-bold text-white">New Project</h1>
          <Link href="/projects" className="btn btn-secondary text-sm">
            Cancel
          </Link>
        </div>

        <div className="card">
          {error && (
            <div className="bg-black border border-red-800 text-red-400 px-4 py-3 rounded mb-4">
              <span>{error}</span>
            </div>
          )}

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
                className="input w-full"
                placeholder="Enter project name"
                disabled={isSubmitting}
              />
            </div>

            <div className="mb-6">
              <label
                htmlFor="color"
                className="block text-sm font-medium text-gray-300 mb-1"
              >
                Project Color
              </label>
              <div className="flex items-center">
                <input
                  type="color"
                  id="color"
                  name="color"
                  value={formData.color}
                  onChange={handleChange}
                  className="w-12 h-12 rounded cursor-pointer mr-3 border-0"
                  disabled={isSubmitting}
                />
                <input
                  type="text"
                  name="color"
                  value={formData.color}
                  onChange={handleChange}
                  className="input"
                  placeholder="#FFFFFF"
                  maxLength={7}
                  disabled={isSubmitting}
                />
              </div>
            </div>

            <div className="flex justify-end">
              <button
                type="submit"
                className="btn btn-primary"
                disabled={isSubmitting}
              >
                {isSubmitting ? "Creating..." : "Create Project"}
              </button>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
}
