"use client";

import { useState, useEffect, FormEvent } from "react";
import { useRouter } from "next/navigation";
import { createBrowserSupabaseClient } from "@/utils/supabase";
import type { User } from "@supabase/supabase-js";

export default function ProfilePage() {
  const [isLoading, setIsLoading] = useState(false);
  const [successMessage, setSuccessMessage] = useState("");
  const [errorMessage, setErrorMessage] = useState("");
  const [password, setPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const [passwordError, setPasswordError] = useState("");
  const router = useRouter();
  const supabase = createBrowserSupabaseClient();

  // Get current user details
  const [user, setUser] = useState<User | null>(null);

  // Fetch user on component mount
  useEffect(() => {
    const getUser = async () => {
      const { data } = await supabase.auth.getUser();
      if (data.user) {
        setUser(data.user);
      }
    };
    getUser();
  }, [supabase.auth]);

  const validateForm = (): boolean => {
    setPasswordError("");

    if (password && password.length < 6) {
      setPasswordError("Password must be at least 6 characters");
      return false;
    }

    if (password && password !== confirmPassword) {
      setPasswordError("Passwords do not match");
      return false;
    }

    return true;
  };

  const handleSubmit = async (e: FormEvent) => {
    e.preventDefault();

    if (!validateForm()) {
      return;
    }

    setIsLoading(true);
    setErrorMessage("");
    setSuccessMessage("");

    try {
      // Update password if provided
      if (password) {
        const { error } = await supabase.auth.updateUser({
          password: password,
        });

        if (error) throw error;
        setSuccessMessage("Password updated successfully");
        setPassword("");
        setConfirmPassword("");
      }

      router.refresh();
    } catch (error: unknown) {
      const errorMessage =
        error instanceof Error ? error.message : "Something went wrong";
      setErrorMessage(errorMessage);
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="container mx-auto px-4 max-w-3xl">
      <h1 className="text-2xl font-bold mb-6">Profile Settings</h1>

      {successMessage && (
        <div className="mb-4 p-4 bg-green-800/30 border border-green-800 text-green-300 rounded">
          {successMessage}
        </div>
      )}

      {errorMessage && (
        <div className="mb-4 p-4 bg-red-800/30 border border-red-800 text-red-300 rounded">
          {errorMessage}
        </div>
      )}

      <div className="bg-gray-800 p-6 rounded-lg shadow border border-gray-700">
        <h2 className="text-xl font-semibold mb-4">Account Information</h2>

        <div className="mb-6">
          <div className="flex items-center mb-4">
            <div className="w-12 h-12 rounded-full bg-gray-600 flex items-center justify-center mr-4">
              <span className="text-xl font-medium">
                {user?.email?.charAt(0).toUpperCase() || "U"}
              </span>
            </div>
            <div>
              <p className="text-sm text-gray-400">Email Address</p>
              <p className="font-medium">{user?.email}</p>
            </div>
          </div>
        </div>

        <form onSubmit={handleSubmit}>
          <h3 className="text-lg font-medium mb-4 border-t border-gray-700 pt-4">
            Change Password
          </h3>

          <div className="space-y-4">
            <div>
              <label
                htmlFor="password"
                className="block mb-1 text-sm font-medium text-gray-300"
              >
                New Password
              </label>
              <input
                id="password"
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                className="w-full p-2 bg-gray-700 border border-gray-600 rounded-md text-white focus:ring-blue-500 focus:border-blue-500"
              />
              {passwordError && (
                <p className="mt-1 text-sm text-red-400">{passwordError}</p>
              )}
            </div>

            <div>
              <label
                htmlFor="confirmPassword"
                className="block mb-1 text-sm font-medium text-gray-300"
              >
                Confirm New Password
              </label>
              <input
                id="confirmPassword"
                type="password"
                value={confirmPassword}
                onChange={(e) => setConfirmPassword(e.target.value)}
                className="w-full p-2 bg-gray-700 border border-gray-600 rounded-md text-white focus:ring-blue-500 focus:border-blue-500"
              />
            </div>
          </div>

          <div className="mt-6">
            <button
              type="submit"
              disabled={isLoading}
              className="px-4 py-2 bg-blue-600 text-white rounded-md hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 focus:ring-offset-gray-800 disabled:opacity-50 disabled:cursor-not-allowed"
            >
              {isLoading ? "Saving..." : "Save Changes"}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
