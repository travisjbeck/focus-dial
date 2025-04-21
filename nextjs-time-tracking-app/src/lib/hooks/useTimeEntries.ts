"use client";

import { useQuery, useQueryClient } from '@tanstack/react-query';
import { createBrowserSupabaseClient } from '@/utils/supabase';
import { useEffect } from 'react';
import type { RealtimeChannel } from '@supabase/supabase-js';
import type { Database } from '@/types/supabase';

// Use 'time_entries' table - **FIXED TABLE NAME**
export type TimeEntry = Database["public"]["Tables"]["time_entries"]["Row"];

// Fetch function for time entries
const fetchTimeEntries = async (): Promise<TimeEntry[]> => {
  const supabase = createBrowserSupabaseClient();
  const { data: { user } } = await supabase.auth.getUser();

  if (!user) {
    console.warn("useTimeEntries: User not logged in, returning empty array.");
    return [];
  }

  const { data, error } = await supabase
    .from('time_entries') // **FIXED TABLE NAME**
    .select('*')
    .eq('user_id', user.id)
    .order('start_time', { ascending: false }); // Order by start time

  if (error) {
    console.error("Error fetching time entries:", error);
    throw new Error(error.message);
  }

  return data || [];
};

// Custom hook for fetching and subscribing to time entry changes
export function useTimeEntries() {
  const queryClient = useQueryClient();
  const supabase = createBrowserSupabaseClient();

  // Fetch initial data
  const query = useQuery<TimeEntry[], Error>({
    queryKey: ['timeEntries'], // Unique query key
    queryFn: fetchTimeEntries,
  });

  // Effect for real-time subscription
  useEffect(() => {
    if (!supabase) return;

    let channel: RealtimeChannel | undefined;

    const setupChannel = () => {
      channel = supabase
        .channel('table-db-changes-time-entries') // **FIXED CHANNEL NAME**
        .on<TimeEntry>(
          'postgres_changes',
          {
            event: '*',
            schema: 'public',
            table: 'time_entries' // **FIXED TABLE NAME**
            // Optional: filter by user_id if needed and possible
          },
          (payload) => {
            console.log('[Realtime] Change received on time_entries table!', payload);
            queryClient.invalidateQueries({ queryKey: ['timeEntries'] });
          }
        )
        .subscribe((status, err) => {
          if (status === 'SUBSCRIBED') {
            console.log('[Realtime] Subscribed to time_entries changes!');
          }
          if (status === 'CHANNEL_ERROR') {
            console.error('[Realtime] Subscription Error (TimeEntries):', err);
          }
          if (status === 'TIMED_OUT') {
            console.warn('[Realtime] Subscription timed out (TimeEntries).');
          }
        });
    };

    setupChannel();

    // Cleanup
    return () => {
      if (channel) {
        supabase.removeChannel(channel)
          .then(() => console.log("[Realtime] Unsubscribed from time_entries changes."))
          .catch(err => console.error("[Realtime] Error unsubscribing (TimeEntries):", err));
      }
    };
  }, [supabase, queryClient]);

  return query;
} 