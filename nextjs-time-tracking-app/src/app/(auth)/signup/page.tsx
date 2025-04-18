"use client";

import Link from "next/link";
import AuthForm from "@/components/AuthForm";

export default function SignupPage() {
  return (
    <div className="flex min-h-screen flex-col items-center justify-center p-24">
      <h1 className="text-2xl font-semibold text-white text-center mb-6">
        Create an Account
      </h1>
      <p className="mb-4 text-gray-400">Join us! Create your account below.</p>

      <div className="w-full max-w-sm p-6 bg-white border border-gray-200 rounded-lg shadow dark:bg-gray-800 dark:border-gray-700">
        <AuthForm mode="signup" />

        <div className="mt-6 text-center text-sm text-gray-400">
          Already have an account?{" "}
          <Link
            href="/login"
            className="text-gray-400 hover:text-white hover:underline"
          >
            Log in
          </Link>
        </div>
      </div>
    </div>
  );
}
