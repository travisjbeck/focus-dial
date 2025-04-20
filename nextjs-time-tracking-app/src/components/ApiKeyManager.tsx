"use client";

import { useState, useTransition, useCallback } from "react";
// Import server actions
import {
  generateApiKey,
  revokeApiKey,
  fetchApiKeys, // Also import fetch for potential manual refetch
} from "@/app/settings/actions";
// Import the type from actions for consistency
import type { ApiKeyMetadata } from "@/app/settings/actions";

// Remove the temporary placeholder type
// type ApiKeyMetadata = { id: number; key_name: string; created_at: string; last_used_at?: string | null };

interface ApiKeyManagerProps {
  initialApiKeys: ApiKeyMetadata[];
  initialError: string | null;
}

export default function ApiKeyManager({
  initialApiKeys,
  initialError,
}: ApiKeyManagerProps) {
  const [apiKeys, setApiKeys] = useState<ApiKeyMetadata[]>(initialApiKeys);
  const [error, setError] = useState<string | null>(initialError);
  const [newlyGeneratedKey, setNewlyGeneratedKey] = useState<string | null>(
    null
  );
  const [isGenerating, startGeneratingTransition] = useTransition();
  const [isRevoking, startRevokingTransition] = useTransition();
  const [revokingId, setRevokingId] = useState<number | null>(null);
  const [newKeyName, setNewKeyName] = useState<string>("Default API Key"); // State for new key name input

  // Function to manually refetch keys - useful after generation/revocation
  const refetchKeys = useCallback(async () => {
    const result = await fetchApiKeys();
    if (result.success && result.data) {
      setApiKeys(result.data);
    } else {
      setError(result.error || "Failed to refresh API keys.");
    }
  }, []);

  const handleGenerateKey = async () => {
    setNewlyGeneratedKey(null);
    setError(null);
    startGeneratingTransition(async () => {
      console.log("Generating key with name:", newKeyName);
      const result = await generateApiKey({
        name: newKeyName || "Default API Key",
      });

      if (result.success && result.apiKey) {
        setNewlyGeneratedKey(result.apiKey);
        setNewKeyName("Default API Key"); // Reset input field
        // Refetch the list to include the new key
        await refetchKeys();
      } else {
        setError(result.error || "Failed to generate key.");
      }
    });
  };

  const handleRevokeKey = async (id: number) => {
    setError(null);
    setRevokingId(id);
    startRevokingTransition(async () => {
      console.log(`Revoking key ${id}...`);
      const result = await revokeApiKey(id);

      if (result.success) {
        // Optimistically update the list or refetch
        setApiKeys((prevKeys) => prevKeys.filter((key) => key.id !== id));
        // Optionally: await refetchKeys(); // If optimistic update isn't preferred
      } else {
        setError(result.error || "Failed to revoke key.");
      }
      setRevokingId(null);
    });
  };

  return (
    <div className="bg-black p-6 rounded-lg shadow border border-gray-800">
      <h2 className="text-lg font-medium text-primary mb-4">Your API Keys</h2>

      {/* Error Display */}
      {error && (
        <div className="bg-red-900/30 border border-red-800 text-red-300 px-4 py-3 rounded mb-4">
          <span>{error}</span>
        </div>
      )}

      {/* Display Newly Generated Key */}
      {newlyGeneratedKey && (
        <div className="bg-green-900/30 border border-green-700 text-green-300 p-4 rounded mb-6">
          <h3 className="font-semibold mb-2">New API Key Generated!</h3>
          <p className="text-sm mb-2">
            Please copy this key and store it securely. You will not be able to
            see it again.
          </p>
          <div className="bg-gray-900 p-2 rounded font-mono text-sm break-all">
            {newlyGeneratedKey}
          </div>
          <button
            onClick={() => setNewlyGeneratedKey(null)}
            className="mt-2 text-xs text-muted hover:text-primary"
          >
            Dismiss
          </button>
        </div>
      )}

      {/* Generate New Key Form */}
      <div className="mb-6 flex items-end gap-3">
        <div className="flex-grow">
          <label
            htmlFor="newKeyName"
            className="block text-sm font-medium text-tertiary mb-1"
          >
            New Key Name
          </label>
          <input
            type="text"
            id="newKeyName"
            value={newKeyName}
            onChange={(e) => setNewKeyName(e.target.value)}
            placeholder="Enter a name for the key (optional)"
            className="bg-black border border-gray-800 text-secondary text-sm rounded-lg focus:ring-indigo-500 focus:border-indigo-500 block w-full p-2.5 placeholder-gray-400"
            disabled={isGenerating}
          />
        </div>
        <button
          onClick={handleGenerateKey}
          disabled={isGenerating || !newKeyName.trim()} // Disable if name is empty or generating
          className="inline-flex items-center px-4 py-2 border border-transparent text-sm font-medium rounded-md shadow-sm text-white bg-indigo-600 hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-offset-gray-800 focus:ring-indigo-500 disabled:opacity-50 h-[42px]" // Match input height
        >
          {isGenerating ? "Generating..." : "Generate Key"}
        </button>
      </div>

      {/* List Existing Keys */}
      <div className="space-y-4">
        {apiKeys.length === 0 && !initialError && (
          <p className="text-tertiary">You don&apos;t have any API keys yet.</p>
        )}
        {apiKeys.map((key) => (
          <div
            key={key.id}
            className="flex items-center justify-between bg-gray-900 p-4 rounded"
          >
            <div>
              <p className="font-medium text-secondary">{key.key_name}</p>
              <p className="text-xs text-muted">
                Created: {new Date(key.created_at).toLocaleDateString()} - Last
                used:{" "}
                {key.last_used_at
                  ? new Date(key.last_used_at).toLocaleDateString()
                  : "Never"}
              </p>
            </div>
            <button
              onClick={() => handleRevokeKey(key.id)}
              disabled={isRevoking && revokingId === key.id}
              className="px-3 py-1 text-xs font-medium text-red-400 bg-gray-800 rounded hover:bg-red-900 hover:text-red-300 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-offset-gray-800 focus:ring-red-500 disabled:opacity-50"
            >
              {isRevoking && revokingId === key.id ? "Revoking..." : "Revoke"}
            </button>
          </div>
        ))}
      </div>
    </div>
  );
}
