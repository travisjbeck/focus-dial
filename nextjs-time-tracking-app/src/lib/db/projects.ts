import { getDb } from './database';

export interface Project {
  id?: number;
  name: string;
  color: string;
  device_project_id?: string;
  created_at?: string;
  updated_at?: string;
}

// Get all projects
export function getAllProjects(): Project[] {
  const db = getDb();
  return db.prepare('SELECT * FROM projects ORDER BY name').all() as Project[];
}

// Get project by ID
export function getProjectById(id: number): Project | undefined {
  const db = getDb();
  return db.prepare('SELECT * FROM projects WHERE id = ?').get(id) as Project | undefined;
}

// Get project by device_project_id
export function getProjectByDeviceId(deviceProjectId: string): Project | undefined {
  const db = getDb();
  return db.prepare('SELECT * FROM projects WHERE device_project_id = ?').get(deviceProjectId) as Project | undefined;
}

// Create a new project
export function createProject(project: Project): Project {
  const db = getDb();
  const { name, color, device_project_id } = project;

  const result = db.prepare(
    'INSERT INTO projects (name, color, device_project_id) VALUES (?, ?, ?)'
  ).run(name, color, device_project_id || null);

  return {
    ...project,
    id: result.lastInsertRowid as number
  };
}

// Update a project
export function updateProject(id: number, project: Partial<Project>): boolean {
  const db = getDb();
  const { name, color, device_project_id } = project;

  // Build the SET clause dynamically based on provided fields
  const updates: string[] = [];
  const values: unknown[] = [];

  if (name !== undefined) {
    updates.push('name = ?');
    values.push(name);
  }

  if (color !== undefined) {
    updates.push('color = ?');
    values.push(color);
  }

  if (device_project_id !== undefined) {
    updates.push('device_project_id = ?');
    values.push(device_project_id);
  }

  updates.push('updated_at = CURRENT_TIMESTAMP');

  if (updates.length === 0) {
    return false;
  }

  values.push(id);

  const result = db.prepare(
    `UPDATE projects SET ${updates.join(', ')} WHERE id = ?`
  ).run(...values);

  return result.changes > 0;
}

// Delete a project
export function deleteProject(id: number): boolean {
  const db = getDb();
  const result = db.prepare('DELETE FROM projects WHERE id = ?').run(id);
  return result.changes > 0;
}

// Find or create project by device_project_id
export function findOrCreateProjectByDeviceId(project: Project): Project {
  const db = getDb();

  // Use a transaction to ensure atomicity
  const tx = db.transaction((project: Project) => {
    const existing = getProjectByDeviceId(project.device_project_id!);

    if (existing) {
      // Update existing project if needed
      updateProject(existing.id!, {
        name: project.name,
        color: project.color
      });
      return existing;
    } else {
      // Create new project
      return createProject(project);
    }
  });

  return tx(project);
} 