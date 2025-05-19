import { initializeDatabase, closeDatabase } from './database';
import { Project } from './projects';
import { TimeEntry } from './timeEntries';

// Initialize database on module import
initializeDatabase();

// Handle graceful shutdown
process.on('exit', () => {
  closeDatabase();
});

// Re-export types and functions
export type { Project, TimeEntry };
export * from './database';
export * from './projects';
export * from './timeEntries'; 