ALTER TABLE public.time_entries
ADD COLUMN duration INTEGER;

COMMENT ON COLUMN public.time_entries.duration IS 'Duration of the time entry in seconds, calculated on stop.';
