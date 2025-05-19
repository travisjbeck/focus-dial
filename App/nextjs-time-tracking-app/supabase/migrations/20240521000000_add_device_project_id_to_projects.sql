-- Add device_project_id column
ALTER TABLE public.projects
ADD COLUMN device_project_id TEXT;

-- Add unique constraint for user_id and device_project_id
ALTER TABLE public.projects
ADD CONSTRAINT projects_user_id_device_project_id_key UNIQUE (user_id, device_project_id); 