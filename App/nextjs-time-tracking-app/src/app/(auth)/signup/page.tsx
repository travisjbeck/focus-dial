"use client";

import Link from "next/link";
import AuthForm from "@/components/AuthForm";

export default function SignupPage() {
  return (
    <div className="flex min-h-screen flex-col items-center justify-center p-24">
      <h1 className="text-2xl font-semibold text-foreground text-center mb-6">
        Create an Account
      </h1>
      <p className="mb-4 text-muted-foreground">Join us! Create your account below.</p>

      <div className="w-full max-w-sm p-6 bg-card border border-border rounded-lg shadow">
        <AuthForm mode="signup" />

        <div className="mt-6 text-center text-sm text-muted-foreground">
          Already have an account?{" "}
          <Link
            href="/login"
            className="text-primary hover:underline"
          >
            Log in
          </Link>
        </div>
      </div>
    </div>
  );
}
