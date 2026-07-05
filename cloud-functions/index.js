/**
 * Firebase Cloud Function — Twilio SMS/WhatsApp Notification Sender
 *
 * This function receives alert data via HTTP POST and sends
 * SMS and/or WhatsApp messages using Twilio.
 *
 * Deploy: cd cloud-functions && firebase deploy --only functions
 */

const functions = require('firebase-functions');
const admin = require('firebase-admin');
const twilio = require('twilio');
const cors = require('cors')({ origin: true });

admin.initializeApp();

// ─── Twilio Configuration ────────────────────────────────────────────
const TWILIO_SID = '';
const TWILIO_TOKEN = '';
const TWILIO_PHONE = '';

const twilioClient = twilio(TWILIO_SID, TWILIO_TOKEN);

// ─── Alert Message Templates ─────────────────────────────────────────

const ALERT_TEMPLATES = {
  geofence: (data) =>
    `🚨 PASHU-SUTRA ALERT\n\nAnimal [${data.rfidTag || 'N/A'}] is OUTSIDE the geofence!\nLocation: ${data.latitude || '?'}, ${data.longitude || '?'}\nTime: ${new Date().toLocaleString('en-IN', { timeZone: 'Asia/Kolkata' })}\n\n⚠️ Immediate attention required.`,

  temperature_high: (data) =>
    `🌡️ PASHU-SUTRA HEALTH ALERT\n\nAnimal [${data.rfidTag || 'N/A'}] has HIGH TEMPERATURE!\nReading: ${data.value}°C (Threshold: ${data.threshold || 39.5}°C)\nTime: ${new Date().toLocaleString('en-IN', { timeZone: 'Asia/Kolkata' })}\n\n⚕️ Check for illness immediately.`,

  bpm_high: (data) =>
    `❤️ PASHU-SUTRA HEALTH ALERT\n\nAnimal [${data.rfidTag || 'N/A'}] has HIGH HEART RATE!\nReading: ${data.value} BPM (Threshold: ${data.threshold || 100} BPM)\nTime: ${new Date().toLocaleString('en-IN', { timeZone: 'Asia/Kolkata' })}\n\n⚕️ Check for stress/illness.`,

  vaccination_2days: (data) =>
    `📋 PASHU-SUTRA REMINDER\n\nVaccination "${data.vaccineName}" for Animal [${data.rfidTag || 'N/A'}] is due in 2 days (${data.date}).\n\nPlease prepare accordingly.`,

  vaccination_1day: (data) =>
    `⚠️ PASHU-SUTRA REMINDER\n\nVaccination "${data.vaccineName}" for Animal [${data.rfidTag || 'N/A'}] is due TOMORROW (${data.date}).\n\nPlease ensure availability.`,

  vaccination_today: (data) =>
    `🔴 PASHU-SUTRA URGENT\n\nVaccination "${data.vaccineName}" for Animal [${data.rfidTag || 'N/A'}] is due TODAY (${data.date}).\n\n💉 Please administer immediately.`,
};

// ─── Cloud Function: Send Notification ───────────────────────────────

exports.sendNotification = functions.https.onRequest((req, res) => {
  cors(req, res, async () => {
    // Only allow POST
    if (req.method !== 'POST') {
      return res.status(405).json({ error: 'Method not allowed' });
    }

    const { alertType, data, recipientPhone, channel = 'sms' } = req.body;

    if (!alertType || !data || !recipientPhone) {
      return res.status(400).json({
        error: 'Missing required fields: alertType, data, recipientPhone',
      });
    }

    // Build message from template
    const templateFn = ALERT_TEMPLATES[alertType];
    if (!templateFn) {
      return res.status(400).json({ error: `Unknown alert type: ${alertType}` });
    }

    const messageBody = templateFn(data);

    try {
      const messageOptions = {
        body: messageBody,
        to: recipientPhone,
      };

      if (channel === 'whatsapp') {
        // WhatsApp requires 'whatsapp:' prefix
        messageOptions.from = `whatsapp:${TWILIO_PHONE}`;
        messageOptions.to = `whatsapp:${recipientPhone}`;
      } else {
        messageOptions.from = TWILIO_PHONE;
      }

      const result = await twilioClient.messages.create(messageOptions);

      console.log(`[Twilio] Message sent: ${result.sid} via ${channel}`);

      // Log to Firestore for history
      await admin.firestore().collection('notification_logs').add({
        alertType,
        channel,
        recipientPhone,
        messageSid: result.sid,
        messageBody,
        timestamp: admin.firestore.FieldValue.serverTimestamp(),
        status: result.status,
      });

      return res.status(200).json({
        success: true,
        messageSid: result.sid,
        status: result.status,
      });
    } catch (error) {
      console.error('[Twilio] Error:', error.message);
      return res.status(500).json({
        error: 'Failed to send notification',
        details: error.message,
      });
    }
  });
});
