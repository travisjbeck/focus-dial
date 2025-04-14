import { useEffect, useState } from "react";
import { useStore } from "../store/useStore";
import {
  Project,
  updateProject,
  deleteProject,
  createProject,
} from "../lib/api";
import { Button } from "../components/ui/button";
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
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
  DialogTrigger,
} from "../components/ui/dialog";
import { Input } from "../components/ui/input";
import { Label } from "../components/ui/label";
import { Trash2, Edit } from "lucide-react";
import { useToast } from "../hooks/use-toast";

export function Projects() {
  const { projects, fetchProjects, isLoadingProjects } = useStore();
  const { toast } = useToast();
  const [isEditDialogOpen, setIsEditDialogOpen] = useState(false);
  const [isDeleteDialogOpen, setIsDeleteDialogOpen] = useState(false);
  const [isAddDialogOpen, setIsAddDialogOpen] = useState(false);
  const [currentProject, setCurrentProject] = useState<Project | null>(null);
  const [newProjectName, setNewProjectName] = useState("");
  const [newProjectColor, setNewProjectColor] = useState("#ffffff");

  useEffect(() => {
    fetchProjects();
  }, [fetchProjects]);

  const handleEditClick = (project: Project) => {
    setCurrentProject(project);
    setNewProjectName(project.name);
    setNewProjectColor(project.color);
    setIsEditDialogOpen(true);
  };

  const handleDeleteClick = (project: Project) => {
    setCurrentProject(project);
    setIsDeleteDialogOpen(true);
  };

  const handleAddClick = () => {
    setNewProjectName("");
    setNewProjectColor("#ffffff");
    setIsAddDialogOpen(true);
  };

  const handleSaveChanges = async () => {
    if (!currentProject) return;

    try {
      await updateProject(currentProject.id, {
        name: newProjectName,
        color: newProjectColor,
      });
      toast({ title: "Project updated successfully!" });
      await fetchProjects(); // Refresh projects list
      setIsEditDialogOpen(false);
      setCurrentProject(null);
    } catch (error) {
      console.error("Error updating project:", error);
      toast({
        title: "Error updating project",
        description: error instanceof Error ? error.message : "Unknown error",
        variant: "destructive",
      });
    }
  };

  const handleConfirmDelete = async () => {
    if (!currentProject) return;

    try {
      await deleteProject(currentProject.id);
      toast({ title: "Project deleted successfully!" });
      await fetchProjects(); // Refresh projects list
      setIsDeleteDialogOpen(false);
      setCurrentProject(null);
    } catch (error) {
      console.error("Error deleting project:", error);
      toast({
        title: "Error deleting project",
        description: error instanceof Error ? error.message : "Unknown error",
        variant: "destructive",
      });
    }
  };

  const handleAddProject = async () => {
    try {
      await createProject({
        name: newProjectName,
        color: newProjectColor,
      });
      toast({ title: "Project added successfully!" });
      await fetchProjects(); // Refresh projects list
      setIsAddDialogOpen(false);
    } catch (error) {
      console.error("Error adding project:", error);
      toast({
        title: "Error adding project",
        description: error instanceof Error ? error.message : "Unknown error",
        variant: "destructive",
      });
    }
  };

  if (isLoadingProjects) {
    return <div className="p-6 text-center">Loading projects...</div>;
  }

  return (
    <div className="space-y-6 p-6">
      <div className="flex items-center justify-between">
        <h1 className="text-3xl font-bold tracking-tight">Projects</h1>
        <Button onClick={handleAddClick}>Add Project</Button>
      </div>

      <Card>
        <CardContent className="p-0">
          <Table>
            <TableHeader>
              <TableRow>
                <TableHead className="w-[50px]">Color</TableHead>
                <TableHead>Name</TableHead>
                <TableHead className="w-[100px]">Actions</TableHead>
              </TableRow>
            </TableHeader>
            <TableBody>
              {projects.length === 0 ? (
                <TableRow>
                  <TableCell colSpan={3} className="text-center">
                    No projects found. Projects are created automatically via
                    webhooks or can be added manually.
                  </TableCell>
                </TableRow>
              ) : (
                projects.map((project) => (
                  <TableRow key={project.id}>
                    <TableCell>
                      <div
                        className="w-6 h-6 rounded-full border border-border"
                        style={{ backgroundColor: project.color }}
                      />
                    </TableCell>
                    <TableCell className="font-medium">
                      {project.name}
                    </TableCell>
                    <TableCell>
                      <div className="flex gap-2">
                        <Button
                          variant="ghost"
                          size="icon"
                          onClick={() => handleEditClick(project)}
                        >
                          <Edit className="h-4 w-4" />
                        </Button>
                        <Button
                          variant="ghost"
                          size="icon"
                          onClick={() => handleDeleteClick(project)}
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

      {/* Edit Project Dialog */}
      <Dialog open={isEditDialogOpen} onOpenChange={setIsEditDialogOpen}>
        <DialogContent>
          <DialogHeader>
            <DialogTitle>Edit Project</DialogTitle>
            <DialogDescription>
              Make changes to your project details.
            </DialogDescription>
          </DialogHeader>
          <div className="grid gap-4 py-4">
            <div className="grid grid-cols-4 items-center gap-4">
              <Label htmlFor="name" className="text-right">
                Name
              </Label>
              <Input
                id="name"
                value={newProjectName}
                onChange={(e) => setNewProjectName(e.target.value)}
                className="col-span-3"
              />
            </div>
            <div className="grid grid-cols-4 items-center gap-4">
              <Label htmlFor="color" className="text-right">
                Color
              </Label>
              <Input
                id="color"
                type="color"
                value={newProjectColor}
                onChange={(e) => setNewProjectColor(e.target.value)}
                className="col-span-3 p-1 h-10"
              />
            </div>
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

      {/* Delete Project Dialog */}
      <Dialog open={isDeleteDialogOpen} onOpenChange={setIsDeleteDialogOpen}>
        <DialogContent>
          <DialogHeader>
            <DialogTitle>Delete Project</DialogTitle>
            <DialogDescription>
              Are you sure you want to delete the project "
              {currentProject?.name}"? This action cannot be undone. Projects
              with associated time entries cannot be deleted.
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
              Delete Project
            </Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>

      {/* Add Project Dialog */}
      <Dialog open={isAddDialogOpen} onOpenChange={setIsAddDialogOpen}>
        <DialogContent>
          <DialogHeader>
            <DialogTitle>Add New Project</DialogTitle>
            <DialogDescription>
              Manually add a new project. Projects are also created
              automatically when a new one is received via webhook.
            </DialogDescription>
          </DialogHeader>
          <div className="grid gap-4 py-4">
            <div className="grid grid-cols-4 items-center gap-4">
              <Label htmlFor="new-name" className="text-right">
                Name
              </Label>
              <Input
                id="new-name"
                value={newProjectName}
                onChange={(e) => setNewProjectName(e.target.value)}
                className="col-span-3"
              />
            </div>
            <div className="grid grid-cols-4 items-center gap-4">
              <Label htmlFor="new-color" className="text-right">
                Color
              </Label>
              <Input
                id="new-color"
                type="color"
                value={newProjectColor}
                onChange={(e) => setNewProjectColor(e.target.value)}
                className="col-span-3 p-1 h-10"
              />
            </div>
          </div>
          <DialogFooter>
            <Button variant="outline" onClick={() => setIsAddDialogOpen(false)}>
              Cancel
            </Button>
            <Button onClick={handleAddProject}>Add Project</Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>
    </div>
  );
}
