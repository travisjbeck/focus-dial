import { Suspense } from "react";
// import { cookies } from 'next/headers'; // Needed later for fetching keys
// import { createServerComponentSupabaseClient } from '@/utils/supabase'; // Needed later
import ApiKeyManager from "@/components/ApiKeyManager"; // Client component
import { fetchApiKeys } from "@/app/settings/actions"; // Import the fetch action



export default async function ApiKeysPage() {
  // Fetch existing API key metadata for the user
  const { data: initialApiKeys, error: initialError } = await fetchApiKeys();

  return (
    <div className="container mx-auto px-4 py-8">
      <h1 className="text-2xl font-bold text-primary mb-6">API Keys</h1>
      <p className="text-tertiary mb-6">
        Manage API keys for accessing the Focus Dial webhook endpoint. Treat
        your API keys like passwords and keep them secure.
      </p>

      <Suspense fallback={<div className="text-tertiary">Loading keys...</div>}>
        {/* Pass fetched data/error state to the client component */}
        <ApiKeyManager
          initialApiKeys={initialApiKeys || []}
          initialError={initialError || null}
        />
      </Suspense>
    </div>
  );
}
