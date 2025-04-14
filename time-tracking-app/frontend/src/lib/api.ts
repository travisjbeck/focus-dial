// API endpoints - we use relative URLs because of the proxy in vite.config.ts
const API_ENDPOINTS = {
  PROJECTS: '/api/projects',
  TIME_ENTRIES: '/api/time_entries',
  INVOICES: '/api/invoices',
};

// Types
export interface Project {
  id: number;
  name: string;
  color: string;
  created_at: string;
  updated_at: string;
}

export interface TimeEntry {
  id: number;
  project_id: number;
  start_time: string;
  end_time: string | null;
  duration_seconds: number | null;
  description: string | null;
  invoiced: boolean;
  created_at: string;
  updated_at: string;
  Project?: Project;
}

export interface Invoice {
  id: number;
  project_id: number;
  invoice_date: string;
  total_amount: number;
  status: 'draft' | 'sent' | 'paid';
  created_at: string;
  updated_at: string;
  Project?: Project;
  InvoiceItems?: InvoiceItem[];
}

export interface InvoiceItem {
  id: number;
  invoice_id: number;
  time_entry_id: number;
  created_at: string;
  updated_at: string;
  TimeEntry?: TimeEntry;
}

// API functions
export async function fetchProjects(): Promise<Project[]> {
  const response = await fetch(API_ENDPOINTS.PROJECTS);
  if (!response.ok) {
    throw new Error('Failed to fetch projects');
  }
  return response.json();
}

export async function fetchProject(id: number): Promise<Project> {
  const response = await fetch(`${API_ENDPOINTS.PROJECTS}/${id}`);
  if (!response.ok) {
    throw new Error(`Failed to fetch project with id ${id}`);
  }
  return response.json();
}

export async function createProject(project: Omit<Project, 'id' | 'created_at' | 'updated_at'>): Promise<Project> {
  const response = await fetch(API_ENDPOINTS.PROJECTS, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(project),
  });
  if (!response.ok) {
    throw new Error('Failed to create project');
  }
  return response.json();
}

export async function updateProject(
  id: number,
  project: Partial<Omit<Project, 'id' | 'created_at' | 'updated_at'>>
): Promise<Project> {
  const response = await fetch(`${API_ENDPOINTS.PROJECTS}/${id}`, {
    method: 'PUT',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(project),
  });
  if (!response.ok) {
    throw new Error(`Failed to update project with id ${id}`);
  }
  return response.json();
}

export async function deleteProject(id: number): Promise<void> {
  const response = await fetch(`${API_ENDPOINTS.PROJECTS}/${id}`, {
    method: 'DELETE',
  });
  if (!response.ok) {
    throw new Error(`Failed to delete project with id ${id}`);
  }
}

export async function fetchTimeEntries(params?: {
  project_id?: number;
  start_date?: string;
  end_date?: string;
  invoiced?: boolean;
}): Promise<TimeEntry[]> {
  let url = API_ENDPOINTS.TIME_ENTRIES;
  if (params) {
    const queryParams = new URLSearchParams();
    if (params.project_id) queryParams.append('project_id', params.project_id.toString());
    if (params.start_date) queryParams.append('start_date', params.start_date);
    if (params.end_date) queryParams.append('end_date', params.end_date);
    if (params.invoiced !== undefined) queryParams.append('invoiced', params.invoiced.toString());

    const queryString = queryParams.toString();
    if (queryString) {
      url += `?${queryString}`;
    }
  }

  const response = await fetch(url);
  if (!response.ok) {
    throw new Error('Failed to fetch time entries');
  }
  return response.json();
}

export async function fetchTimeEntry(id: number): Promise<TimeEntry> {
  const response = await fetch(`${API_ENDPOINTS.TIME_ENTRIES}/${id}`);
  if (!response.ok) {
    throw new Error(`Failed to fetch time entry with id ${id}`);
  }
  return response.json();
}

export async function updateTimeEntry(
  id: number,
  timeEntry: Partial<Omit<TimeEntry, 'id' | 'created_at' | 'updated_at'>>
): Promise<TimeEntry> {
  const response = await fetch(`${API_ENDPOINTS.TIME_ENTRIES}/${id}`, {
    method: 'PUT',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(timeEntry),
  });
  if (!response.ok) {
    throw new Error(`Failed to update time entry with id ${id}`);
  }
  return response.json();
}

export async function deleteTimeEntry(id: number): Promise<void> {
  const response = await fetch(`${API_ENDPOINTS.TIME_ENTRIES}/${id}`, {
    method: 'DELETE',
  });
  if (!response.ok) {
    throw new Error(`Failed to delete time entry with id ${id}`);
  }
}

export async function fetchInvoices(): Promise<Invoice[]> {
  const response = await fetch(API_ENDPOINTS.INVOICES);
  if (!response.ok) {
    throw new Error('Failed to fetch invoices');
  }
  return response.json();
}

export async function fetchInvoice(id: number): Promise<Invoice> {
  const response = await fetch(`${API_ENDPOINTS.INVOICES}/${id}`);
  if (!response.ok) {
    throw new Error(`Failed to fetch invoice with id ${id}`);
  }
  return response.json();
}

export async function createInvoice(invoice: {
  project_id: number;
  time_entry_ids: number[];
  total_amount?: number;
}): Promise<Invoice> {
  const response = await fetch(API_ENDPOINTS.INVOICES, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(invoice),
  });
  if (!response.ok) {
    throw new Error('Failed to create invoice');
  }
  return response.json();
}

export async function updateInvoiceStatus(
  id: number,
  status: 'draft' | 'sent' | 'paid'
): Promise<Invoice> {
  const response = await fetch(`${API_ENDPOINTS.INVOICES}/${id}`, {
    method: 'PUT',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ status }),
  });
  if (!response.ok) {
    throw new Error(`Failed to update invoice status for id ${id}`);
  }
  return response.json();
}

export async function deleteInvoice(id: number): Promise<void> {
  const response = await fetch(`${API_ENDPOINTS.INVOICES}/${id}`, {
    method: 'DELETE',
  });
  if (!response.ok) {
    throw new Error(`Failed to delete invoice with id ${id}`);
  }
} 