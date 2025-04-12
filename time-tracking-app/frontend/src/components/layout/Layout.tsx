import { Outlet } from "react-router-dom";
import { Sidebar } from "./Sidebar";

export function Layout() {
  return (
    <div className="grid grid-cols-1 md:grid-cols-[240px_1fr] min-h-screen">
      <Sidebar />
      <main className="flex flex-col flex-1 p-6 md:p-8 overflow-auto">
        <div className="container mx-auto">
          <Outlet />
        </div>
      </main>
    </div>
  );
}
