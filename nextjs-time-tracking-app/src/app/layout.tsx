import type { Metadata } from "next";
import { Inter } from "next/font/google";
import "./globals.css";
import Link from "next/link";
import ReactQueryProvider from "@/lib/providers/ReactQueryProvider";
import { cookies } from "next/headers";
import { createServerComponentSupabaseClient } from "@/utils/supabase";
import { logout } from "@/app/(auth)/actions";

const inter = Inter({ subsets: ["latin"] });

export const metadata: Metadata = {
  title: "Focus Dial - Time Tracker",
  description: "Personal time tracking dashboard",
};

export default async function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  const cookieStore = cookies();
  const supabase = createServerComponentSupabaseClient(cookieStore);
  const {
    data: { user },
  } = await supabase.auth.getUser();

  return (
    <html lang="en">
      <ReactQueryProvider>
        <body className={inter.className}>
          {user ? (
            // Authenticated Layout with Sidebar
            <div className="layout-wrapper">
              {/* Sidebar */}
              <aside className="w-64 bg-black text-white border-r border-gray-800 flex flex-col h-screen fixed">
                {/* Logo section */}
                <div className="p-4 border-b border-gray-800">
                  <Link
                    href="/"
                    className="text-xl font-bold flex items-center"
                  >
                    <svg
                      className="w-6 h-6 mr-2"
                      viewBox="0 0 24 24"
                      fill="none"
                      xmlns="http://www.w3.org/2000/svg"
                    >
                      <circle
                        cx="12"
                        cy="12"
                        r="10"
                        stroke="currentColor"
                        strokeWidth="2"
                      />
                      <path
                        d="M12 6V12L16 14"
                        stroke="currentColor"
                        strokeWidth="2"
                        strokeLinecap="round"
                      />
                    </svg>
                    <span>Focus Dial</span>
                  </Link>
                </div>

                <div className="flex-1 flex flex-col overflow-y-auto">
                  {/* Application section */}
                  <div className="p-4">
                    <p className="text-xs uppercase text-gray-500 mb-3">
                      Application
                    </p>
                    <nav>
                      <ul className="space-y-2">
                        <li>
                          <Link
                            href="/"
                            className="flex items-center p-2 hover:bg-gray-900 rounded-md text-gray-300 hover:text-white transition-colors"
                          >
                            <svg
                              className="w-5 h-5 mr-3"
                              fill="none"
                              viewBox="0 0 24 24"
                              stroke="currentColor"
                            >
                              <path
                                strokeLinecap="round"
                                strokeLinejoin="round"
                                strokeWidth={2}
                                d="M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6"
                              />
                            </svg>
                            <span>Home</span>
                          </Link>
                        </li>
                        <li>
                          <Link
                            href="/projects"
                            className="flex items-center p-2 hover:bg-gray-900 rounded-md text-gray-300 hover:text-white transition-colors"
                          >
                            <svg
                              className="w-5 h-5 mr-3"
                              fill="none"
                              viewBox="0 0 24 24"
                              stroke="currentColor"
                            >
                              <path
                                strokeLinecap="round"
                                strokeLinejoin="round"
                                strokeWidth={2}
                                d="M3 7v10a2 2 0 002 2h14a2 2 0 002-2V9a2 2 0 00-2-2h-6l-2-2H5a2 2 0 00-2 2z"
                              />
                            </svg>
                            <span>Projects</span>
                          </Link>
                        </li>
                        <li>
                          <Link
                            href="/entries"
                            className="flex items-center p-2 hover:bg-gray-900 rounded-md text-gray-300 hover:text-white transition-colors"
                          >
                            <svg
                              className="w-5 h-5 mr-3"
                              fill="none"
                              viewBox="0 0 24 24"
                              stroke="currentColor"
                            >
                              <path
                                strokeLinecap="round"
                                strokeLinejoin="round"
                                strokeWidth={2}
                                d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z"
                              />
                            </svg>
                            <span>Time Entries</span>
                          </Link>
                        </li>
                      </ul>
                    </nav>
                  </div>

                  {/* Settings section */}
                  <div className="p-4 border-t border-gray-800 mt-4">
                    <p className="text-xs uppercase text-gray-500 mb-3">
                      Settings
                    </p>
                    <nav>
                      <ul className="space-y-2">
                        <li>
                          <Link
                            href="/settings/profile"
                            className="flex items-center p-2 hover:bg-gray-900 rounded-md text-gray-300 hover:text-white transition-colors"
                          >
                            <svg
                              className="w-5 h-5 mr-3"
                              fill="none"
                              viewBox="0 0 24 24"
                              stroke="currentColor"
                            >
                              <path
                                strokeLinecap="round"
                                strokeLinejoin="round"
                                strokeWidth={2}
                                d="M16 7a4 4 0 11-8 0 4 4 0 018 0zM12 14a7 7 0 00-7 7h14a7 7 0 00-7-7z"
                              />
                            </svg>
                            <span>Profile</span>
                          </Link>
                        </li>
                        <li>
                          <Link
                            href="/settings/api-keys"
                            className="flex items-center p-2 hover:bg-gray-900 rounded-md text-gray-300 hover:text-white transition-colors"
                          >
                            <svg
                              className="w-5 h-5 mr-3"
                              fill="none"
                              viewBox="0 0 24 24"
                              stroke="currentColor"
                            >
                              <path
                                strokeLinecap="round"
                                strokeLinejoin="round"
                                strokeWidth={2}
                                d="M15 7a2 2 0 012 2m4 0a6 6 0 01-7.743 5.743L11 17H9v2H7v2H4a1 1 0 01-1-1v-2.586a1 1 0 01.293-.707l5.964-5.964A6 6 0 1121 9z"
                              />
                            </svg>
                            <span>API Keys</span>
                          </Link>
                        </li>
                      </ul>
                    </nav>
                  </div>
                </div>

                {/* User avatar section */}
                <div className="p-4 border-t border-gray-800 mt-auto">
                  <div className="flex items-center">
                    <div className="w-8 h-8 rounded-full bg-gray-900 flex items-center justify-center mr-3">
                      <span className="text-sm font-medium">
                        {user.email?.charAt(0).toUpperCase() || "U"}
                      </span>
                    </div>
                    <div className="flex-1 min-w-0">
                      <p className="text-sm font-medium text-white truncate">
                        {user.email}
                      </p>
                      <form action={logout}>
                        <button
                          type="submit"
                          className="text-xs text-gray-400 hover:text-white"
                        >
                          Log out
                        </button>
                      </form>
                    </div>
                  </div>
                </div>
              </aside>

              {/* Main content */}
              <div className="ml-64 main-content w-full">
                <main className="py-6 px-6">{children}</main>
                <footer className="bg-black py-4 text-center text-gray-500 text-xs border-t border-gray-800">
                  <div className="container mx-auto px-4">
                    Focus Dial Time Tracker &copy; {new Date().getFullYear()}
                  </div>
                </footer>
              </div>
            </div>
          ) : (
            // Unauthenticated Layout (Login/Signup pages)
            <div className="min-h-screen flex flex-col">
              <main className="flex-grow">{children}</main>
              <footer className="bg-black py-4 text-center text-gray-500 text-xs border-t border-gray-800">
                <div className="container mx-auto px-4">
                  Focus Dial Time Tracker &copy; {new Date().getFullYear()}
                </div>
              </footer>
            </div>
          )}
        </body>
      </ReactQueryProvider>
    </html>
  );
}
