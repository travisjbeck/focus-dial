import Link from "next/link";
import { notFound } from "next/navigation";
import { cookies } from "next/headers";
import { createServerComponentSupabaseClient } from "@/utils/supabase";
import { formatDuration, formatDate } from "@/lib/utils/dateUtils"; // Assuming utils exist

interface TimeEntryDetailPageProps {
  params: {
    id: string;
  };
}

export default async function TimeEntryDetailPage({
  params,
}: TimeEntryDetailPageProps) {
  const cookieStore = cookies();
  const supabase = createServerComponentSupabaseClient(cookieStore);

  const {
    data: { user },
  } = await supabase.auth.getUser();

  if (!user) {
    // Handle unauthorized access if necessary, though middleware should cover this
    console.error("Time Entry Detail: No user found.");
    // redirect('/login'); // Or handle as appropriate
    return <div className="container mx-auto px-4 text-red-500">Unauthorized access.</div>;
  }

  // Fetch the specific time entry
  const { data: timeEntry, error: timeEntryError } = await supabase
    .from("time_entries")
    .select(`
      *,
      projects (
        id,
        name,
        color
      )
    `)
    .eq("id", Number(params.id))
    .eq("user_id", user.id)
    .single();

  if (timeEntryError || !timeEntry) {
    console.error("Error fetching time entry details or entry not found:", timeEntryError);
    notFound();
  }

  // Type assertion for project data if needed, Supabase types should handle this
  const project = timeEntry.projects as { id: number; name: string; color: string | null } | null;


  return (
    <div className="container mx-auto px-4">
      <div className="mb-6 flex justify-between items-center">
        <h1 className="text-xl font-bold text-foreground">Time Entry Details</h1>
        <div className="flex space-x-2">
           <Link
             href="/entries" // Link back to the main entries list
             className="px-3 py-1 text-xs font-medium text-secondary-foreground bg-secondary hover:bg-secondary/80 rounded-md border border-border"
           >
             Back to Entries
           </Link>
           <Link
             href={`/time-entries/${timeEntry.id}/edit`}
             className="px-3 py-1 text-xs font-medium text-secondary-foreground bg-secondary hover:bg-secondary/80 rounded-md border border-border"
           >
             Edit
           </Link>
        </div>
      </div>

      <div className="bg-card p-6 rounded-lg shadow border border-border">
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div>
            <h3 className="text-xs uppercase text-muted-foreground mb-1">Project</h3>
            {project ? (
               <Link href={`/projects/${project.id}`} className="flex items-center hover:underline">
                 <span
                   className="w-3 h-3 rounded-full mr-2 flex-shrink-0"
                   style={{ backgroundColor: project.color || '#808080' }}
                 ></span>
                 <span className="text-foreground font-medium">{project.name}</span>
               </Link>
            ) : (
              <span className="text-muted-foreground">No Project</span>
            )}
          </div>
           <div>
             <h3 className="text-xs uppercase text-muted-foreground mb-1">Duration</h3>
             <p className="text-foreground font-medium font-mono">
               {formatDuration(timeEntry.duration)}
             </p>
           </div>
           <div>
             <h3 className="text-xs uppercase text-muted-foreground mb-1">Start Time</h3>
             <p className="text-foreground font-medium">{formatDate(timeEntry.start_time)}</p>
           </div>
           <div>
             <h3 className="text-xs uppercase text-muted-foreground mb-1">End Time</h3>
             <p className="text-foreground font-medium">
               {timeEntry.end_time ? formatDate(timeEntry.end_time) : (
                 <span className="text-green-500">Still Running</span>
               )}
             </p>
           </div>
           
           <div className="md:col-span-2">
             <h3 className="text-xs uppercase text-muted-foreground mb-1">Description</h3>
             <p className="text-foreground whitespace-pre-wrap">
               {timeEntry.description || <span className="text-muted-foreground">No description added.</span>}
             </p>
           </div>
           
        </div>
      </div>
    </div>
  );
} 