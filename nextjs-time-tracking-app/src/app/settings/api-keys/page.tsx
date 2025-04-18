import { Suspense } from "react";
// import { cookies } from 'next/headers'; // Needed later for fetching keys
// import { createServerComponentSupabaseClient } from '@/utils/supabase'; // Needed later
import ApiKeyManager from "@/components/ApiKeyManager"; // Client component
import { fetchApiKeys } from "@/app/settings/actions"; // Import the fetch action

// Define the type explicitly based on the action's return type
import type { Database } from "@/types/supabase";
type ApiKeyMetadata = Omit<
  Database["public"]["Tables"]["api_keys"]["Row"],
  "key_hash"
>;

export default async function ApiKeysPage() {
  // TODO: Fetch existing API key metadata for the user later
  // const cookieStore = cookies();
  // const supabase = createServerComponentSupabaseClient(cookieStore);
  // const { data: { user } } = await supabase.auth.getUser();
  // const { data: apiKeys, error } = user ? await supabase.from('api_keys')... : { data: [], error: null };

  // Fetch existing API key metadata for the user
  const { data: initialApiKeys, error: initialError } = await fetchApiKeys();

  return (
    <div className="container mx-auto px-4 py-8">
      <h1 className="text-2xl font-bold text-white mb-6">API Keys</h1>
      <p className="text-gray-400 mb-6">
        Manage API keys for accessing the Focus Dial webhook endpoint. Treat
        your API keys like passwords and keep them secure.
      </p>

      <Suspense fallback={<div className="text-gray-400">Loading keys...</div>}>
        {/* Pass fetched data/error state to the client component */}
        <ApiKeyManager
          initialApiKeys={initialApiKeys || []}
          initialError={initialError}
        />
      </Suspense>
    </div>
  );
}
