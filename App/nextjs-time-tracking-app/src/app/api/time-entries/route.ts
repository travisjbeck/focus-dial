import { NextRequest, NextResponse } from 'next/server';
import { getAllTimeEntries, getTimeEntriesByProjectId, createTimeEntry, TimeEntry } from '@/lib/db';

// GET /api/time-entries - Get all time entries or filter by project_id
export async function GET(request: NextRequest) {
  try {
    const { searchParams } = new URL(request.url);
    const projectId = searchParams.get('project_id');

    let timeEntries;

    if (projectId) {
      // Filter by project ID
      const id = parseInt(projectId, 10);
      if (isNaN(id)) {
        return NextResponse.json({ error: 'Invalid project ID' }, { status: 400 });
      }
      timeEntries = getTimeEntriesByProjectId(id);
    } else {
      // Get all time entries
      timeEntries = getAllTimeEntries();
    }

    return NextResponse.json(timeEntries);
  } catch (error) {
    console.error('Error fetching time entries:', error);
    return NextResponse.json(
      { error: 'Internal server error' },
      { status: 500 }
    );
  }
}

// POST /api/time-entries - Create a new time entry
export async function POST(request: NextRequest) {
  try {
    const data = await request.json();

    // Validate required fields
    if (!data.project_id || !data.start_time) {
      return NextResponse.json(
        { error: 'Missing required fields: project_id, start_time' },
        { status: 400 }
      );
    }

    // Create the time entry
    const timeEntry: TimeEntry = {
      project_id: data.project_id,
      start_time: data.start_time,
      end_time: data.end_time,
      duration: data.duration
    };

    const createdTimeEntry = createTimeEntry(timeEntry);

    return NextResponse.json(createdTimeEntry, { status: 201 });
  } catch (error) {
    console.error('Error creating time entry:', error);
    return NextResponse.json(
      { error: 'Internal server error' },
      { status: 500 }
    );
  }
} 