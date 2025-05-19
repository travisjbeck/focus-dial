-- Delete all time entries first (due to foreign key constraints)
DELETE FROM time_entries;

-- Delete all projects
DELETE FROM projects;

-- Reset the auto-increment counters
DELETE FROM sqlite_sequence WHERE name='time_entries';
DELETE FROM sqlite_sequence WHERE name='projects'; 