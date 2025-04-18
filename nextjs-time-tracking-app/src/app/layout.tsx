import type { Metadata } from "next";
import { Inter } from "next/font/google";
import "./globals.css";
import Link from "next/link";
import HeaderAuth from "@/components/HeaderAuth";
import ReactQueryProvider from "@/lib/providers/ReactQueryProvider";

const inter = Inter({ subsets: ["latin"] });

export const metadata: Metadata = {
  title: "Focus Dial - Time Tracker",
  description: "Personal time tracking dashboard",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <ReactQueryProvider>
        <body className={inter.className}>
          <div className="min-h-screen flex flex-col">
            <header className="bg-black text-white border-b border-gray-800">
              <div className="container mx-auto px-4 py-4 flex justify-between items-center">
                <Link href="/" className="text-xl font-bold flex items-center">
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
                <div className="flex items-center space-x-6">
                  <nav>
                    <ul className="flex space-x-6">
                      <li>
                        <Link
                          href="/"
                          className="hover:text-gray-300 text-sm uppercase tracking-wide font-medium"
                        >
                          Dashboard
                        </Link>
                      </li>
                      <li>
                        <Link
                          href="/projects"
                          className="hover:text-gray-300 text-sm uppercase tracking-wide font-medium"
                        >
                          Projects
                        </Link>
                      </li>
                      <li>
                        <Link
                          href="/entries"
                          className="hover:text-gray-300 text-sm uppercase tracking-wide font-medium"
                        >
                          Time Entries
                        </Link>
                      </li>
                    </ul>
                  </nav>
                  <HeaderAuth />
                </div>
              </div>
            </header>
            <main className="flex-grow py-6">{children}</main>
            <footer className="bg-black py-4 text-center text-gray-500 text-xs border-t border-gray-800">
              <div className="container mx-auto px-4">
                Focus Dial Time Tracker &copy; {new Date().getFullYear()}
              </div>
            </footer>
          </div>
        </body>
      </ReactQueryProvider>
    </html>
  );
}
