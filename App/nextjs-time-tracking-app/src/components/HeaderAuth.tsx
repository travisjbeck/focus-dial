import { cookies } from "next/headers";
import { createServerComponentSupabaseClient } from "@/utils/supabase";
import { logout } from "@/app/(auth)/actions"; // Import the logout action
import Link from "next/link"; // Import Link

// This needs to be an async component to fetch user data
export default async function HeaderAuth() {
  const cookieStore = cookies();
  const supabase = createServerComponentSupabaseClient(cookieStore);
  const {
    data: { user },
  } = await supabase.auth.getUser();

  if (user) {
    return (
      <div className="flex items-center space-x-4">
        <span className="text-sm text-gray-400">{user.email}</span>
        {/* Link to Settings/API Keys Page */}
        <Link
          href="/settings/api-keys"
          className="px-3 py-1 text-sm font-medium text-gray-300 bg-gray-700 rounded hover:bg-gray-600 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-offset-gray-800 focus:ring-gray-500"
        >
          Settings
        </Link>
        {/* Form to call the logout server action */}
        <form action={logout}>
          <button
            type="submit"
            className="px-3 py-1 text-sm font-medium text-gray-300 bg-gray-700 rounded hover:bg-gray-600 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-offset-gray-800 focus:ring-gray-500"
          >
            Logout
          </button>
        </form>
      </div>
    );
  }

  // Return null or a login link if no user is logged in (optional)
  return null;
  // Or: return <Link href="/login">Login</Link>;
}
