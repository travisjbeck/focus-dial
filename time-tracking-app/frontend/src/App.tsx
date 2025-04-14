import { Routes, Route } from "react-router-dom";
import { Layout } from "./components/layout/Layout";
import { Dashboard } from "./pages/Dashboard";
import { Projects } from "./pages/Projects";
import { Toaster } from "./components/ui/Toaster";

// Let's implement the Time Entries page since we now have working time entries
import { TimeEntries } from "./pages/TimeEntries";

// Placeholder pages (will be implemented later)
const Invoices = () => <div className="p-6">Invoices Page (Coming Soon)</div>;
const Settings = () => <div className="p-6">Settings Page (Coming Soon)</div>;

function App() {
  return (
    <>
      <Routes>
        <Route path="/" element={<Layout />}>
          <Route index element={<Dashboard />} />
          <Route path="projects" element={<Projects />} />
          <Route path="time-entries" element={<TimeEntries />} />
          <Route path="invoices" element={<Invoices />} />
          <Route path="settings" element={<Settings />} />
          <Route path="*" element={<div className="p-6">Page Not Found</div>} />
        </Route>
      </Routes>
      <Toaster />
    </>
  );
}

export default App;
