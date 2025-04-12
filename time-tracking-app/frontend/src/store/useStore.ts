import { create } from 'zustand';
import { Project, TimeEntry, Invoice, fetchProjects, fetchTimeEntries, fetchInvoices } from '../lib/api';

interface StoreState {
  // Projects
  projects: Project[];
  isLoadingProjects: boolean;
  errorProjects: string | null;
  fetchProjects: () => Promise<void>;

  // Time Entries
  timeEntries: TimeEntry[];
  isLoadingTimeEntries: boolean;
  errorTimeEntries: string | null;
  fetchTimeEntries: (params?: {
    project_id?: number;
    start_date?: string;
    end_date?: string;
    invoiced?: boolean;
  }) => Promise<void>;

  // Invoices
  invoices: Invoice[];
  isLoadingInvoices: boolean;
  errorInvoices: string | null;
  fetchInvoices: () => Promise<void>;

  // Selected items
  selectedProjectId: number | null;
  setSelectedProjectId: (id: number | null) => void;
}

export const useStore = create<StoreState>((set) => ({
  // Projects
  projects: [],
  isLoadingProjects: false,
  errorProjects: null,
  fetchProjects: async () => {
    set({ isLoadingProjects: true, errorProjects: null });
    try {
      const projects = await fetchProjects();
      set({ projects, isLoadingProjects: false });
    } catch (error) {
      set({
        errorProjects: error instanceof Error ? error.message : 'Failed to fetch projects',
        isLoadingProjects: false
      });
    }
  },

  // Time Entries
  timeEntries: [],
  isLoadingTimeEntries: false,
  errorTimeEntries: null,
  fetchTimeEntries: async (params) => {
    set({ isLoadingTimeEntries: true, errorTimeEntries: null });
    try {
      const timeEntries = await fetchTimeEntries(params);
      set({ timeEntries, isLoadingTimeEntries: false });
    } catch (error) {
      set({
        errorTimeEntries: error instanceof Error ? error.message : 'Failed to fetch time entries',
        isLoadingTimeEntries: false
      });
    }
  },

  // Invoices
  invoices: [],
  isLoadingInvoices: false,
  errorInvoices: null,
  fetchInvoices: async () => {
    set({ isLoadingInvoices: true, errorInvoices: null });
    try {
      const invoices = await fetchInvoices();
      set({ invoices, isLoadingInvoices: false });
    } catch (error) {
      set({
        errorInvoices: error instanceof Error ? error.message : 'Failed to fetch invoices',
        isLoadingInvoices: false
      });
    }
  },

  // Selected items
  selectedProjectId: null,
  setSelectedProjectId: (id) => set({ selectedProjectId: id }),
})); 