// src/app/(auth)/login/page.tsx

import AuthForm from "@/components/AuthForm"; // Import the component
import Link from "next/link"; // Import Link

export default function LoginPage() {
  return (
    <div className="flex min-h-screen flex-col items-center justify-center p-24">
      <h1 className="text-2xl font-semibold mb-6">Login</h1>
      <p className="mb-4 text-gray-400">
        Welcome back! Please log in to continue.
      </p>

      <div className="w-full max-w-sm p-6 bg-white border border-gray-200 rounded-lg shadow dark:bg-gray-800 dark:border-gray-700">
        {/* Replace placeholder with the actual form */}
        <AuthForm mode="login" />

        {/* Add link to signup page */}
        <div className="mt-6 text-center text-sm text-gray-400">
          Don&apos;t have an account? {/* Use &apos; for apostrophe */}
          {/* Update link color */}
          <Link
            href="/signup"
            className="text-gray-400 hover:text-white hover:underline"
          >
            Sign up
          </Link>
        </div>
      </div>
    </div>
  );
}
