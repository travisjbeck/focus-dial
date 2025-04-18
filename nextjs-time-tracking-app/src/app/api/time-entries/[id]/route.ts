import { NextRequest, NextResponse } from 'next/server';
import { getDb, updateTimeEntry, deleteTimeEntry } from '@/lib/db';

interface RouteParams {
  params: {
    id: string;
  }
}

// Helper function to get a time entry by ID
function getTimeEntryById(id: number) {
  const db = getDb();
  return db.prepare('SELECT * FROM time_entries WHERE id = ?').get(id);
}

// GET /api/time-entries/[id] - Get a time entry by ID
export async function GET(request: NextRequest, { params }: RouteParams) {
  try {
    const id = parseInt(params.id, 10);

    if (isNaN(id)) {
      return NextResponse.json({ error: 'Invalid time entry ID' }, { status: 400 });
    }

    const timeEntry = getTimeEntryById(id);

    if (!timeEntry) {
      return NextResponse.json({ error: 'Time entry not found' }, { status: 404 });
    }

    return NextResponse.json(timeEntry);
  } catch (error) {
    console.error(`Error fetching time entry ${params.id}:`, error);
    return NextResponse.json({ error: 'Internal server error' }, { status: 500 });
  }
}

// PUT /api/time-entries/[id] - Update a time entry
export async function PUT(request: NextRequest, { params }: RouteParams) {
  try {
    const id = parseInt(params.id, 10);

    if (isNaN(id)) {
      return NextResponse.json({ error: 'Invalid time entry ID' }, { status: 400 });
    }

    const data = await request.json();
    const updated = updateTimeEntry(id, data);

    if (!updated) {
      return NextResponse.json({ error: 'Time entry not found' }, { status: 404 });
    }

    // Get the updated time entry
    const timeEntry = getTimeEntryById(id);

    return NextResponse.json(timeEntry);
  } catch (error) {
    console.error(`Error updating time entry ${params.id}:`, error);
    return NextResponse.json({ error: 'Internal server error' }, { status: 500 });
  }
}

// DELETE /api/time-entries/[id] - Delete a time entry
export async function DELETE(request: NextRequest, { params }: RouteParams) {
  try {
    const id = parseInt(params.id, 10);

    if (isNaN(id)) {
      return NextResponse.json({ error: 'Invalid time entry ID' }, { status: 400 });
    }

    const deleted = deleteTimeEntry(id);

    if (!deleted) {
      return NextResponse.json({ error: 'Time entry not found' }, { status: 404 });
    }

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error(`Error deleting time entry ${params.id}:`, error);
    return NextResponse.json({ error: 'Internal server error' }, { status: 500 });
  }
} 