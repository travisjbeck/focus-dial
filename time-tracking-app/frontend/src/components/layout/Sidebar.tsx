import { Link, useLocation } from "react-router-dom";
import { LineChart, Layers, Clock, FileText, Settings } from "lucide-react";

const navigationItems = [
  { name: "Dashboard", href: "/", icon: LineChart },
  { name: "Projects", href: "/projects", icon: Layers },
  { name: "Time Entries", href: "/time-entries", icon: Clock },
  { name: "Invoices", href: "/invoices", icon: FileText },
];

export function Sidebar() {
  const location = useLocation();

  return (
    <aside className="border-r border-border bg-card">
      <div className="h-16 border-b border-border flex items-center px-6">
        <Link
          to="/"
          className="text-xl font-bold tracking-tighter text-primary flex items-center gap-2"
        >
          <Clock className="h-6 w-6" />
          <span>Focus Dial</span>
        </Link>
      </div>
      <nav className="p-4 space-y-2">
        {navigationItems.map((item) => {
          const isActive =
            location.pathname === item.href ||
            (item.href !== "/" && location.pathname.startsWith(item.href));

          return (
            <Link
              key={item.name}
              to={item.href}
              className={`
                flex items-center gap-3 px-3 py-2 text-sm rounded-md 
                transition-colors hover:bg-accent hover:text-accent-foreground
                ${
                  isActive
                    ? "bg-accent text-accent-foreground"
                    : "text-foreground/60"
                }
              `}
            >
              <item.icon className="h-4 w-4" />
              {item.name}
            </Link>
          );
        })}
      </nav>
      <div className="absolute bottom-4 left-4 right-4">
        <Link
          to="/settings"
          className="flex items-center gap-3 px-3 py-2 text-sm rounded-md 
            transition-colors hover:bg-accent hover:text-accent-foreground
            text-foreground/60"
        >
          <Settings className="h-4 w-4" />
          Settings
        </Link>
      </div>
    </aside>
  );
}
