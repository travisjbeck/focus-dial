import { useEffect, useMemo } from "react";
import { useStore } from "../store/useStore";
import { formatDate, formatDuration } from "../lib/utils";
import {
  Card,
  CardContent,
  CardHeader,
  CardTitle,
} from "../components/ui/card";
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "../components/ui/table";
import { Button } from "../components/ui/button";
import { Clock, FileText, BarChart } from "lucide-react";
import { Link } from "react-router-dom";

export function Dashboard() {
  const {
    projects,
    timeEntries,
    invoices,
    fetchProjects,
    fetchTimeEntries,
    fetchInvoices,
    isLoadingProjects,
    isLoadingTimeEntries,
    isLoadingInvoices,
  } = useStore();

  useEffect(() => {
    fetchProjects();
    fetchTimeEntries();
    fetchInvoices();
  }, [fetchProjects, fetchTimeEntries, fetchInvoices]);

  // Calculate total time tracked this week
  const totalTimeThisWeek = useMemo(() => {
    const now = new Date();
    const startOfWeek = new Date(now);
    startOfWeek.setDate(now.getDate() - now.getDay()); // Start of week (Sunday)
    startOfWeek.setHours(0, 0, 0, 0);

    return timeEntries
      .filter((entry) => new Date(entry.start_time) >= startOfWeek)
      .reduce((total, entry) => total + (entry.duration_seconds || 0), 0);
  }, [timeEntries]);

  // Get recent time entries
  const recentTimeEntries = useMemo(() => {
    return [...timeEntries]
      .sort(
        (a, b) =>
          new Date(b.start_time).getTime() - new Date(a.start_time).getTime()
      )
      .slice(0, 5);
  }, [timeEntries]);

  // Calculate project breakdown
  const projectBreakdown = useMemo(() => {
    const breakdown = projects.map((project) => {
      const entries = timeEntries.filter(
        (entry) => entry.project_id === project.id
      );
      const totalTime = entries.reduce(
        (sum, entry) => sum + (entry.duration_seconds || 0),
        0
      );
      return {
        ...project,
        totalTime,
        entryCount: entries.length,
      };
    });

    return breakdown.sort((a, b) => b.totalTime - a.totalTime);
  }, [projects, timeEntries]);

  const isLoading =
    isLoadingProjects || isLoadingTimeEntries || isLoadingInvoices;

  if (isLoading) {
    return <div className="p-8 text-center">Loading dashboard data...</div>;
  }

  return (
    <div className="space-y-6 p-6">
      <h1 className="text-3xl font-bold tracking-tight">Dashboard</h1>

      {/* Stats Cards */}
      <div className="grid gap-4 md:grid-cols-3">
        <Card>
          <CardHeader className="flex flex-row items-center justify-between pb-2">
            <CardTitle className="text-sm font-medium">
              Total Time This Week
            </CardTitle>
            <Clock className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">
              {formatDuration(totalTimeThisWeek)}
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between pb-2">
            <CardTitle className="text-sm font-medium">
              Active Projects
            </CardTitle>
            <BarChart className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{projects.length}</div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between pb-2">
            <CardTitle className="text-sm font-medium">
              Pending Invoices
            </CardTitle>
            <FileText className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">
              {invoices.filter((invoice) => invoice.status !== "paid").length}
            </div>
          </CardContent>
        </Card>
      </div>

      {/* Recent Time Entries */}
      <div>
        <div className="flex items-center justify-between mb-4">
          <h2 className="text-xl font-semibold">Recent Time Entries</h2>
          <Button variant="outline" size="sm" asChild>
            <Link to="/time-entries">View all</Link>
          </Button>
        </div>
        <Card>
          <CardContent className="p-0">
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead>Project</TableHead>
                  <TableHead>Date</TableHead>
                  <TableHead>Duration</TableHead>
                  <TableHead>Description</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {recentTimeEntries.length === 0 ? (
                  <TableRow>
                    <TableCell colSpan={4} className="text-center">
                      No time entries yet
                    </TableCell>
                  </TableRow>
                ) : (
                  recentTimeEntries.map((entry) => (
                    <TableRow key={entry.id}>
                      <TableCell>
                        <div className="flex items-center gap-2">
                          <div
                            className="w-3 h-3 rounded-full"
                            style={{
                              backgroundColor: entry.Project?.color || "#888",
                            }}
                          />
                          {entry.Project?.name || "Unknown Project"}
                        </div>
                      </TableCell>
                      <TableCell>{formatDate(entry.start_time)}</TableCell>
                      <TableCell>
                        {formatDuration(entry.duration_seconds)}
                      </TableCell>
                      <TableCell className="max-w-[200px] truncate">
                        {entry.description || "-"}
                      </TableCell>
                    </TableRow>
                  ))
                )}
              </TableBody>
            </Table>
          </CardContent>
        </Card>
      </div>

      {/* Projects Breakdown */}
      <div>
        <div className="flex items-center justify-between mb-4">
          <h2 className="text-xl font-semibold">Projects Breakdown</h2>
          <Button variant="outline" size="sm" asChild>
            <Link to="/projects">View all</Link>
          </Button>
        </div>
        <div className="grid gap-4 md:grid-cols-2 lg:grid-cols-3">
          {projectBreakdown.slice(0, 6).map((project) => (
            <Card key={project.id}>
              <CardHeader className="pb-2">
                <div className="flex items-center gap-2">
                  <div
                    className="w-3 h-3 rounded-full"
                    style={{ backgroundColor: project.color }}
                  />
                  <CardTitle className="text-lg truncate">
                    {project.name}
                  </CardTitle>
                </div>
              </CardHeader>
              <CardContent className="pb-3">
                <div className="flex justify-between text-sm text-muted-foreground">
                  <div>Total time:</div>
                  <div className="font-medium">
                    {formatDuration(project.totalTime)}
                  </div>
                </div>
                <div className="flex justify-between text-sm text-muted-foreground">
                  <div>Entries:</div>
                  <div className="font-medium">{project.entryCount}</div>
                </div>
              </CardContent>
            </Card>
          ))}
        </div>
      </div>
    </div>
  );
}
