import { useEffect, useState } from "react";
import { useStore } from "../store/useStore";
import { TimeEntry, updateTimeEntry, deleteTimeEntry } from "../lib/api";
import { Button } from "../components/ui/button";
import { Card, CardContent } from "../components/ui/card";
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "../components/ui/table";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
} from "../components/ui/dialog";
import { Input } from "../components/ui/input";
import { Label } from "../components/ui/label";
import { Calendar, Clock, Trash2, Edit } from "lucide-react";
import { useToast } from "../hooks/use-toast";
import { formatDate, formatDuration } from "../lib/utils";

export function TimeEntries() {
  const {
    projects,
    timeEntries,
    fetchProjects,
    fetchTimeEntries,
    isLoadingTimeEntries,
  } = useStore();
  const { toast } = useToast();
  const [isEditDialogOpen, setIsEditDialogOpen] = useState(false);
  const [isDeleteDialogOpen, setIsDeleteDialogOpen] = useState(false);
  const [currentEntry, setCurrentEntry] = useState<TimeEntry | null>(null);
  const [description, setDescription] = useState("");
  const [selectedProjectId, setSelectedProjectId] = useState<number | null>(
    null
  );

  useEffect(() => {
    fetchProjects();
    fetchTimeEntries();
  }, [fetchProjects, fetchTimeEntries]);

  const handleEditClick = (entry: TimeEntry) => {
    setCurrentEntry(entry);
    setDescription(entry.description || "");
    setSelectedProjectId(entry.project_id);
    setIsEditDialogOpen(true);
  };

  const handleDeleteClick = (entry: TimeEntry) => {
    setCurrentEntry(entry);
    setIsDeleteDialogOpen(true);
  };

  const handleSaveChanges = async () => {
    if (!currentEntry) return;

    try {
      await updateTimeEntry(currentEntry.id, {
        description,
        project_id: selectedProjectId || currentEntry.project_id,
      });
      toast({ title: "Time entry updated successfully!" });
      await fetchTimeEntries(); // Refresh time entries list
      setIsEditDialogOpen(false);
      setCurrentEntry(null);
    } catch (error) {
      console.error("Error updating time entry:", error);
      toast({
        title: "Error updating time entry",
        description: error instanceof Error ? error.message : "Unknown error",
        variant: "destructive",
      });
    }
  };

  const handleConfirmDelete = async () => {
    if (!currentEntry) return;

    try {
      await deleteTimeEntry(currentEntry.id);
      toast({ title: "Time entry deleted successfully!" });
      await fetchTimeEntries(); // Refresh time entries list
      setIsDeleteDialogOpen(false);
      setCurrentEntry(null);
    } catch (error) {
      console.error("Error deleting time entry:", error);
      toast({
        title: "Error deleting time entry",
        description: error instanceof Error ? error.message : "Unknown error",
        variant: "destructive",
      });
    }
  };

  if (isLoadingTimeEntries) {
    return <div className="p-6 text-center">Loading time entries...</div>;
  }

  return (
    <div className="space-y-6 p-6">
      <div className="flex items-center justify-between">
        <h1 className="text-3xl font-bold tracking-tight">Time Entries</h1>
      </div>

      {/* Time Entry Filters - Will implement this later 
      <div className="flex flex-wrap gap-4 mb-6">
        <Input className="w-48" placeholder="Search..." />
        <Select>
          <SelectTrigger className="w-48">
            <SelectValue placeholder="All Projects" />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="all">All Projects</SelectItem>
            {projects.map(project => (
              <SelectItem key={project.id} value={project.id.toString()}>
                {project.name}
              </SelectItem>
            ))}
          </SelectContent>
        </Select>
      </div>
      */}

      <Card>
        <CardContent className="p-0">
          <Table>
            <TableHeader>
              <TableRow>
                <TableHead className="w-[120px]">Date</TableHead>
                <TableHead className="w-[100px]">Duration</TableHead>
                <TableHead>Project</TableHead>
                <TableHead>Description</TableHead>
                <TableHead className="w-[100px]">Actions</TableHead>
              </TableRow>
            </TableHeader>
            <TableBody>
              {timeEntries.length === 0 ? (
                <TableRow>
                  <TableCell colSpan={5} className="text-center">
                    No time entries found. Time entries are created when you use
                    the Focus Dial device.
                  </TableCell>
                </TableRow>
              ) : (
                timeEntries.map((entry) => (
                  <TableRow key={entry.id}>
                    <TableCell>{formatDate(entry.start_time)}</TableCell>
                    <TableCell>
                      {formatDuration(entry.duration_seconds)}
                    </TableCell>
                    <TableCell>
                      <div className="flex items-center gap-2">
                        <div
                          className="w-3 h-3 rounded-full border border-border"
                          style={{
                            backgroundColor: entry.Project?.color || "#888",
                          }}
                        />
                        {entry.Project?.name || "Unknown Project"}
                      </div>
                    </TableCell>
                    <TableCell className="max-w-[300px] truncate">
                      {entry.description || (
                        <span className="text-muted-foreground italic">
                          No description
                        </span>
                      )}
                    </TableCell>
                    <TableCell>
                      <div className="flex gap-2">
                        <Button
                          variant="ghost"
                          size="icon"
                          onClick={() => handleEditClick(entry)}
                        >
                          <Edit className="h-4 w-4" />
                        </Button>
                        <Button
                          variant="ghost"
                          size="icon"
                          onClick={() => handleDeleteClick(entry)}
                        >
                          <Trash2 className="h-4 w-4 text-destructive" />
                        </Button>
                      </div>
                    </TableCell>
                  </TableRow>
                ))
              )}
            </TableBody>
          </Table>
        </CardContent>
      </Card>

      {/* Edit Time Entry Dialog */}
      <Dialog open={isEditDialogOpen} onOpenChange={setIsEditDialogOpen}>
        <DialogContent>
          <DialogHeader>
            <DialogTitle>Edit Time Entry</DialogTitle>
            <DialogDescription>
              Add a description or change the project for this time entry.
            </DialogDescription>
          </DialogHeader>
          <div className="grid gap-4 py-4">
            <div className="grid grid-cols-4 items-center gap-4">
              <Label htmlFor="project" className="text-right">
                Project
              </Label>
              <div className="col-span-3">
                <select
                  id="project"
                  className="w-full rounded-md border border-input bg-background px-3 py-2 text-sm"
                  value={selectedProjectId || ""}
                  onChange={(e) => setSelectedProjectId(Number(e.target.value))}
                >
                  {projects.map((project) => (
                    <option key={project.id} value={project.id}>
                      {project.name}
                    </option>
                  ))}
                </select>
              </div>
            </div>
            <div className="grid grid-cols-4 items-center gap-4">
              <Label htmlFor="description" className="text-right">
                Description
              </Label>
              <Input
                id="description"
                value={description}
                onChange={(e) => setDescription(e.target.value)}
                placeholder="What did you work on?"
                className="col-span-3"
              />
            </div>
            {currentEntry && (
              <>
                <div className="grid grid-cols-4 items-center gap-4">
                  <Label className="text-right">Duration</Label>
                  <div className="col-span-3 flex items-center gap-2">
                    <Clock className="h-4 w-4 text-muted-foreground" />
                    <span>{formatDuration(currentEntry.duration_seconds)}</span>
                  </div>
                </div>
                <div className="grid grid-cols-4 items-center gap-4">
                  <Label className="text-right">Date</Label>
                  <div className="col-span-3 flex items-center gap-2">
                    <Calendar className="h-4 w-4 text-muted-foreground" />
                    <span>{formatDate(currentEntry.start_time)}</span>
                  </div>
                </div>
              </>
            )}
          </div>
          <DialogFooter>
            <Button
              variant="outline"
              onClick={() => setIsEditDialogOpen(false)}
            >
              Cancel
            </Button>
            <Button onClick={handleSaveChanges}>Save Changes</Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>

      {/* Delete Time Entry Dialog */}
      <Dialog open={isDeleteDialogOpen} onOpenChange={setIsDeleteDialogOpen}>
        <DialogContent>
          <DialogHeader>
            <DialogTitle>Delete Time Entry</DialogTitle>
            <DialogDescription>
              Are you sure you want to delete this time entry? This action
              cannot be undone.
            </DialogDescription>
          </DialogHeader>
          <DialogFooter>
            <Button
              variant="outline"
              onClick={() => setIsDeleteDialogOpen(false)}
            >
              Cancel
            </Button>
            <Button variant="destructive" onClick={handleConfirmDelete}>
              Delete Entry
            </Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>
    </div>
  );
}
