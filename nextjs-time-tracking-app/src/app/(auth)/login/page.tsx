// src/app/(auth)/login/page.tsx

import AuthForm from "@/components/AuthForm"; // Import the component
import Link from "next/link"; // Import Link

export default function LoginPage() {
  return (
    <div className="flex min-h-screen flex-col items-center justify-center p-24">
      <h1 className="text-2xl font-semibold text-foreground mb-6">Login</h1>
      <p className="mb-4 text-muted-foreground">
        Welcome back! Please log in to continue.
      </p>

      <div className="w-full max-w-sm p-6 bg-card border border-border rounded-lg shadow">
        {/* Replace placeholder with the actual form */}
        <AuthForm mode="login" />

        {/* Add link to signup page */}
        <div className="mt-6 text-center text-sm text-muted-foreground">
          Don&apos;t have an account? {/* Use &apos; for apostrophe */}
          {/* Update link color */}
          <Link
            href="/signup"
            className="text-primary hover:underline"
          >
            Sign up
          </Link>
        </div>
      </div>
    </div>
  );
}
