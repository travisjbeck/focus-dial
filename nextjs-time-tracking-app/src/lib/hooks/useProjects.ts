"use client"; // Hooks that use other client hooks like useEffect must be client components

import { useQuery, useQueryClient } from '@tanstack/react-query';
import { createBrowserSupabaseClient } from '@/utils/supabase';
import { useEffect } from 'react';
import type { RealtimeChannel } from '@supabase/supabase-js';
import type { Database } from '@/types/supabase';

type Project = Database["public"]["Tables"]["projects"]["Row"];

// Fetch function for projects
const fetchProjects = async (): Promise<Project[]> => {
  const supabase = createBrowserSupabaseClient(); // Create client for fetching
  const { data: { user } } = await supabase.auth.getUser();

  if (!user) {
    // Handle case where user is not logged in, though middleware should prevent this
    // Depending on requirements, might return empty array or throw error
    console.warn("useProjects: User not logged in, returning empty array.");
    return [];
  }

  const { data, error } = await supabase
    .from('projects')
    .select('*')
    .eq('user_id', user.id) // Ensure we only fetch user's projects
    .order('created_at', { ascending: false });

  if (error) {
    console.error("Error fetching projects:", error);
    throw new Error(error.message);
  }

  return data || [];
};

// Custom hook for fetching and subscribing to project changes
export function useProjects() {
  const queryClient = useQueryClient();
  // It's often better to create the Supabase client instance *outside* the effect 
  // if it's stable across renders, or use useMemo if needed.
  const supabase = createBrowserSupabaseClient();

  // Fetch initial data using React Query
  const query = useQuery<Project[], Error>({
    queryKey: ['projects'], // Query key for caching
    queryFn: fetchProjects, // Function to fetch data
    // Add staleTime and gcTime as needed, inherited from provider defaultOptions for now
  });

  // Effect to set up the real-time subscription
  useEffect(() => {
    // Ensure Supabase client is available
    if (!supabase) return;

    let channel: RealtimeChannel | undefined;

    const setupChannel = () => {
      channel = supabase
        .channel('table-db-changes-projects') // Unique channel name
        .on<Project>(
          'postgres_changes', // Listen to database changes
          {
            event: '*', // Listen to INSERT, UPDATE, DELETE
            schema: 'public',
            table: 'projects'
            // Optional: filter changes further, e.g., by user_id
            // filter: `user_id=eq.${userId}` // Requires userId to be available here
          },
          (payload) => {
            console.log('[Realtime] Change received on projects table!', payload);
            // Invalidate the projects query cache to trigger a refetch
            queryClient.invalidateQueries({ queryKey: ['projects'] });
          }
        )
        .subscribe((status, err) => {
          if (status === 'SUBSCRIBED') {
            console.log('[Realtime] Subscribed to projects changes!');
          }
          if (status === 'CHANNEL_ERROR') {
            console.error('[Realtime] Subscription Error:', err);
          }
          if (status === 'TIMED_OUT') {
            console.warn('[Realtime] Subscription timed out.');
          }
        });
    };

    setupChannel();

    // Cleanup function: remove the channel subscription when the component unmounts
    return () => {
      if (channel) {
        supabase.removeChannel(channel)
          .then(() => console.log("[Realtime] Unsubscribed from projects changes."))
          .catch(err => console.error("[Realtime] Error unsubscribing:", err));
      }
    };
  }, [supabase, queryClient]); // Dependencies for the effect

  // Return the state and functions from useQuery
  return query;
} 