import { getDb } from './database';

export interface TimeEntry {
  id?: number;
  project_id: number;
  start_time: string;
  end_time?: string;
  duration?: number;
  created_at?: string;
  updated_at?: string;
}

// Get all time entries
export function getAllTimeEntries(): TimeEntry[] {
  const db = getDb();
  return db.prepare('SELECT * FROM time_entries ORDER BY start_time DESC').all() as TimeEntry[];
}

// Get time entries by project ID
export function getTimeEntriesByProjectId(projectId: number): TimeEntry[] {
  const db = getDb();
  return db.prepare('SELECT * FROM time_entries WHERE project_id = ? ORDER BY start_time DESC')
    .all(projectId) as TimeEntry[];
}

// Get active time entry (has start_time but no end_time) for a project
export function getActiveTimeEntryByProjectId(projectId: number): TimeEntry | undefined {
  const db = getDb();
  return db.prepare(
    'SELECT * FROM time_entries WHERE project_id = ? AND end_time IS NULL ORDER BY start_time DESC LIMIT 1'
  ).get(projectId) as TimeEntry | undefined;
}

// Create a new time entry
export function createTimeEntry(entry: TimeEntry): TimeEntry {
  const db = getDb();
  const { project_id, start_time, end_time, duration } = entry;

  const result = db.prepare(
    'INSERT INTO time_entries (project_id, start_time, end_time, duration) VALUES (?, ?, ?, ?)'
  ).run(project_id, start_time, end_time || null, duration || null);

  return {
    ...entry,
    id: result.lastInsertRowid as number
  };
}

// Update a time entry
export function updateTimeEntry(id: number, entry: Partial<TimeEntry>): boolean {
  const db = getDb();
  const { project_id, start_time, end_time, duration } = entry;

  // Build the SET clause dynamically based on provided fields
  const updates: string[] = [];
  const values: unknown[] = [];

  if (project_id !== undefined) {
    updates.push('project_id = ?');
    values.push(project_id);
  }

  if (start_time !== undefined) {
    updates.push('start_time = ?');
    values.push(start_time);
  }

  if (end_time !== undefined) {
    updates.push('end_time = ?');
    values.push(end_time);
  }

  if (duration !== undefined) {
    updates.push('duration = ?');
    values.push(duration);
  }

  updates.push('updated_at = CURRENT_TIMESTAMP');

  if (updates.length === 0) {
    return false;
  }

  values.push(id);

  const result = db.prepare(
    `UPDATE time_entries SET ${updates.join(', ')} WHERE id = ?`
  ).run(...values);

  return result.changes > 0;
}

// Delete a time entry
export function deleteTimeEntry(id: number): boolean {
  const db = getDb();
  const result = db.prepare('DELETE FROM time_entries WHERE id = ?').run(id);
  return result.changes > 0;
}

// Start a timer (create a new time entry with start_time)
export function startTimer(projectId: number): TimeEntry {
  const db = getDb();

  // Use a transaction to ensure we properly handle existing timers
  const tx = db.transaction((projectId: number) => {
    // First, check if there's already an active timer for this project
    const activeEntry = getActiveTimeEntryByProjectId(projectId);

    if (activeEntry) {
      // If there's an active timer, return it without creating a new one
      return activeEntry;
    }

    // Create a new time entry with current timestamp
    const now = new Date().toISOString();
    return createTimeEntry({
      project_id: projectId,
      start_time: now
    });
  });

  return tx(projectId);
}

// Stop a timer (update end_time and calculate duration)
export function stopTimer(projectId: number): TimeEntry | undefined {
  const db = getDb();

  // Use a transaction to ensure atomicity
  const tx = db.transaction((projectId: number) => {
    // Find the active time entry for this project
    const activeEntry = getActiveTimeEntryByProjectId(projectId);

    if (!activeEntry) {
      return undefined;
    }

    // Calculate end time and duration
    const now = new Date();
    const endTime = now.toISOString();
    const startTime = new Date(activeEntry.start_time);
    const durationMs = now.getTime() - startTime.getTime();
    const durationSec = Math.floor(durationMs / 1000);

    // Update the time entry
    updateTimeEntry(activeEntry.id!, {
      end_time: endTime,
      duration: durationSec
    });

    // Return the updated entry
    return {
      ...activeEntry,
      end_time: endTime,
      duration: durationSec
    };
  });

  return tx(projectId);
} 