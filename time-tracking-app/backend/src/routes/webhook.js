const express = require('express');
const router = express.Router();
const { Project, TimeEntry } = require('../models');

/**
 * POST /api/webhook
 * Receives webhook data from the Focus Dial
 * Payload format: JSON {"action": "start|stop|done", "device_project_id": "...", "project_name": "...", "project_color": "#RRGGBB"}
 */
router.post('/', async (req, res) => {
  try {
    console.log('Received webhook payload:', req.body);

    // Check if webhook payload is properly formatted
    if (!req.body || !req.body.event) {
      return res.status(400).json({ error: 'Invalid webhook payload' });
    }

    const { event, project_name, project_color, device_project_id } = req.body;

    // Handle different timer events
    switch (event) {
      case 'timer_start':
        if (!project_name) {
          return res.status(400).json({ error: 'Project name is required for timer_start event' });
        }
        await handleTimerStart(device_project_id, project_name, project_color);
        break;

      case 'timer_stop':
        await handleTimerStop();
        break;

      case 'timer_done':
        await handleTimerDone();
        break;

      default:
        return res.status(400).json({ error: 'Unknown event type' });
    }

    res.json({ success: true });
  } catch (error) {
    console.error('Error processing webhook:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * Handle timer start event
 * Creates or finds the project and starts a new time entry
 */
async function handleTimerStart(deviceProjectId, projectName, projectColor) {
  // Require deviceProjectId for all requests
  if (!deviceProjectId) {
    throw new Error('device_project_id is required for timer_start events');
  }

  // Find or create project based on device_project_id
  let project = await Project.findOne({
    where: { device_project_id: deviceProjectId }
  });

  if (project) {
    // Update project name/color if they've changed
    const needsUpdate = (project.name !== projectName) || (project.color !== projectColor);

    if (needsUpdate) {
      project.name = projectName;
      project.color = projectColor;
      await project.save();
      console.log(`Updated project: ${projectName} (ID: ${deviceProjectId})`);
    }
  } else {
    // Create new project with device_project_id
    project = await Project.create({
      name: projectName,
      color: projectColor,
      device_project_id: deviceProjectId
    });
    console.log(`Created new project: ${projectName} (ID: ${deviceProjectId})`);
  }

  // Create a new time entry
  await TimeEntry.create({
    project_id: project.id,
    start_time: new Date(),
    end_time: null,
    duration_seconds: null
  });

  console.log(`Timer started for project: ${projectName}`);
}

/**
 * Handle timer stop event
 * Finds the most recent active time entry and sets its end time
 */
async function handleTimerStop(deviceProjectId) {
  // Find the most recent active time entry (no end_time)
  const activeEntry = await TimeEntry.findOne({
    where: { end_time: null },
    order: [['created_at', 'DESC']]
  });

  if (activeEntry) {
    const endTime = new Date();
    const startTime = activeEntry.start_time;
    const durationSeconds = Math.round((endTime - startTime) / 1000);

    // Update the time entry
    activeEntry.end_time = endTime;
    activeEntry.duration_seconds = durationSeconds;
    await activeEntry.save();

    console.log(`Timer stopped. Duration: ${durationSeconds} seconds`);
  } else {
    console.log('No active timer found to stop');
  }
}

/**
 * Handle timer done event
 * Similar to stop but might have different semantics in the future
 */
async function handleTimerDone(deviceProjectId) {
  // For now, handle the same as stop
  await handleTimerStop(deviceProjectId);
  console.log('Timer marked as done');
}

module.exports = router; 