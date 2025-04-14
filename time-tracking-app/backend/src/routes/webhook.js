const express = require('express');
const router = express.Router();
const { Project, TimeEntry } = require('../models');

/**
 * POST /api/webhook
 * Receives webhook data from the Focus Dial
 * Payload format: action|projectName|#hexColor
 * Examples: 
 *   'start|My Project|#FF00AA'
 *   'stop||#FFFFFF' (for no project)
 *   'done|Client Project|#00FF00'
 */
router.post('/', async (req, res) => {
  try {
    // Get the request body
    const { body } = req;

    // The webhook data can come in different formats depending on how the Focus Dial sends it
    // Here we handle both raw text and JSON payloads
    let webhookData;

    if (Buffer.isBuffer(body)) {
      // Handle raw buffer (typically from text/plain content-type)
      webhookData = body.toString('utf8');
    } else if (typeof body === 'string') {
      // Handle string data
      webhookData = body;
    } else if (body && body.payload) {
      // Handle payload in JSON object
      webhookData = body.payload;
    } else if (typeof body === 'object') {
      // Try to stringify the object 
      webhookData = JSON.stringify(body);
    } else {
      // Fallback
      webhookData = String(body || '');
    }

    console.log('Received webhook data:', webhookData);

    // Parse the webhook data (format: action|projectName|#hexColor)
    const parts = webhookData.split('|');
    if (parts.length < 1) {
      return res.status(400).json({ error: 'Invalid webhook data format' });
    }

    const action = parts[0]?.toLowerCase().trim();
    const projectName = parts[1] || 'Default Project';
    const hexColor = parts[2] || '#FFFFFF';

    console.log(`Parsed webhook: Action=${action}, Project=${projectName}, Color=${hexColor}`);

    // Handle the action
    switch (action) {
      case 'start':
        await handleTimerStart(projectName, hexColor);
        break;
      case 'stop':
        await handleTimerStop();
        break;
      case 'done':
        await handleTimerDone();
        break;
      default:
        return res.status(400).json({ error: 'Unknown action', receivedAction: action });
    }

    return res.status(200).json({ message: 'Webhook processed successfully' });
  } catch (error) {
    console.error('Error processing webhook:', error);
    return res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * Handle timer start event
 * Creates or finds the project and starts a new time entry
 */
async function handleTimerStart(projectName, hexColor) {
  // Find or create project
  const [project, created] = await Project.findOrCreate({
    where: { name: projectName },
    defaults: { color: hexColor }
  });

  // If project exists but color is different, update it
  if (!created && project.color !== hexColor) {
    project.color = hexColor;
    await project.save();
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
async function handleTimerStop() {
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
async function handleTimerDone() {
  // For now, handle the same as stop
  await handleTimerStop();
  console.log('Timer marked as done');
}

module.exports = router; 