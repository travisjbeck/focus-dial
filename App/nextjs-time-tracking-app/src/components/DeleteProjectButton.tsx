"use client";

import { useState, useTransition } from "react";
import { useRouter } from "next/navigation";
import { deleteProject } from "@/app/(auth)/actions"; // Import the server action
import { Button } from "@/components/ui/button"; // Import Button

interface DeleteProjectButtonProps {
  projectId: number;
}

export default function DeleteProjectButton({
  projectId,
}: DeleteProjectButtonProps) {
  const router = useRouter();
  const [isPending, startTransition] = useTransition();
  const [error, setError] = useState<string | null>(null);

  const handleDelete = () => {
    // Confirmation dialog
    if (
      !confirm(
        "Are you sure you want to delete this project? This action cannot be undone."
      )
    ) {
      return;
    }

    setError(null);
    startTransition(async () => {
      const result = await deleteProject(projectId);
      if (result.success) {
        // On successful deletion, navigate to the projects list page
        router.push("/projects");
        router.refresh(); // Force refresh just in case revalidation is delayed
      } else {
        // Display error message to the user
        setError(result.error || "Failed to delete project.");
      }
    });
  };

  return (
    <>
      {/* Display error message if deletion failed */}
      {error && <p className="text-xs text-red-400 mt-2">Error: {error}</p>}
      <Button
        variant="destructive" // Use destructive variant
        size="sm" // Match size with other buttons
        onClick={handleDelete}
        disabled={isPending}
      >
        {isPending ? "Deleting..." : "Delete"}
      </Button>
    </>
  );
}
