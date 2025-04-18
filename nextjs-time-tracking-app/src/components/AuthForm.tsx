"use client";

import React, { useTransition } from "react";
// Import the Server Actions
import { login, signup } from "@/app/(auth)/actions";

interface AuthFormProps {
  mode: "login" | "signup";
  // We will add onSubmit or action props later when connecting to Server Actions
}

export default function AuthForm({ mode }: AuthFormProps) {
  // Add useTransition for pending state feedback (optional but good UX)
  const [isPending, startTransition] = useTransition();
  // State to potentially display server-returned errors (if actions were modified to return errors)
  // const [errorMessage, setErrorMessage] = useState<string | null>(null);

  const handleFormAction = (formData: FormData) => {
    // setErrorMessage(null); // Clear previous errors
    startTransition(async () => {
      // Call the appropriate server action
      const action = mode === "login" ? login : signup;
      await action(formData);

      // If the actions were modified to return an error object instead of redirecting:
      // if (result?.error) {
      //   setErrorMessage(result.error);
      // }
    });
  };

  return (
    // Use the action prop with our wrapper function
    <form action={handleFormAction} className="space-y-6">
      {/* Optional: Display server error messages */}
      {/* {errorMessage && ( 
        <div className="p-4 mb-4 text-sm text-red-800 rounded-lg bg-red-50 dark:bg-gray-800 dark:text-red-400" role="alert">
          <span className="font-medium">Error:</span> {errorMessage}
        </div>
      )} */}
      <div>
        <label
          htmlFor="email"
          className="block mb-2 text-sm font-medium text-gray-900 dark:text-white"
        >
          Email
        </label>
        <input
          type="email"
          name="email" // Name attribute is crucial for FormData
          id="email"
          // Remove value and onChange if using uncontrolled form with FormData
          // value={email}
          // onChange={(e) => setEmail(e.target.value)}
          className="bg-gray-50 border border-gray-300 text-gray-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full p-2.5 dark:bg-gray-700 dark:border-gray-600 dark:placeholder-gray-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500"
          placeholder="name@company.com"
          required
          disabled={isPending} // Disable input when pending
        />
      </div>
      <div>
        <label
          htmlFor="password"
          className="block mb-2 text-sm font-medium text-gray-900 dark:text-white"
        >
          Password
        </label>
        <input
          type="password"
          name="password" // Name attribute is crucial for FormData
          id="password"
          // Remove value and onChange if using uncontrolled form with FormData
          // value={password}
          // onChange={(e) => setPassword(e.target.value)}
          placeholder="••••••••"
          className="bg-gray-50 border border-gray-300 text-gray-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full p-2.5 dark:bg-gray-700 dark:border-gray-600 dark:placeholder-gray-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500"
          required
          minLength={6}
          disabled={isPending} // Disable input when pending
        />
      </div>

      <button
        type="submit"
        className="w-full text-white bg-gray-600 hover:bg-gray-700 focus:ring-4 focus:outline-none focus:ring-gray-500 font-medium rounded-lg text-sm px-5 py-2.5 text-center dark:bg-gray-700 dark:hover:bg-gray-600 dark:focus:ring-gray-800 disabled:opacity-50"
        disabled={isPending} // Disable button when pending
      >
        {isPending ? "Processing..." : mode === "login" ? "Log In" : "Sign Up"}
      </button>

      {/* Optional: Link to switch modes could be added here */}
    </form>
  );
}
