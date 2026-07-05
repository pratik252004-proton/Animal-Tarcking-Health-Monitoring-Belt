const mqtt = require('mqtt');
const readline = require('readline');

// ─── Configuration ───────────────────────────────────────────────────

const ADAFRUIT_IO = {
  USERNAME: '',
  AIO_KEY: '',
  BROKER: 'io.adafruit.com',
  PORT: 8883,
};

const FEED_PREFIX = `${ADAFRUIT_IO.USERNAME}/feeds`;
const FEEDS = {
  GPS:      `${FEED_PREFIX}/gpsloc`,
  BATTERY:  `${FEED_PREFIX}/battery`,
  BPM:      `${FEED_PREFIX}/bpm`,
  TEMP:     `${FEED_PREFIX}/temperature`,
  HUMIDITY: `${FEED_PREFIX}/humidity`,
  GEOFENCE: `${FEED_PREFIX}/geofence`,
};

const GEOFENCE_CENTER = { lat: 18.47011077655484, lon: 73.86976837618745 };

// ─── Registered Animals ──────────────────────────────────────────────
// Add more animals here to simulate a full herd.

const ANIMALS = {
  1: { id: 'Cow-001',     rfid: 'Rfid-001', name: 'Lakshmi  (Cow)',     baseLat:  0.0000, baseLon:  0.0000 }, // at center
  2: { id: 'Cow-002',     rfid: 'Rfid-002', name: 'Nandini  (Cow)',     baseLat:  0.0003, baseLon:  0.0003 }, // ~35m NE
  3: { id: 'Buffalo-001', rfid: 'Rfid-003', name: 'Kaali    (Buffalo)', baseLat: -0.0003, baseLon:  0.0003 }, // ~35m SE
};

// ─── Simulation Modes ────────────────────────────────────────────────

const MODES = {
  1: { name: 'NORMAL (Healthy, Safe)',    temp: 37.5, bpm: 75,  batt: 85, lat: 0,     lon: 0 },
  2: { name: 'LOW BATTERY (< 20%)',       temp: 37.5, bpm: 75,  batt: 15, lat: 0,     lon: 0 },
  3: { name: 'HIGH TEMP (> 39.5°C)',      temp: 40.5, bpm: 75,  batt: 80, lat: 0,     lon: 0 },
  4: { name: 'HIGH BPM (> 100)',          temp: 37.5, bpm: 110, batt: 80, lat: 0,     lon: 0 },
  5: { name: 'GEOFENCE BREACH (Outside)', temp: 37.5, bpm: 75,  batt: 80, lat: 0.005, lon: 0.005 },
};

// ─── Helpers ────────────────────────────────────────────────────────

function jitter(value, amount = 0.5) {
  return value + (Math.random() * amount * 2 - amount);
}

function getGPS(baseLat, baseLon, offsetLat, offsetLon) {
  const lat = baseLat + offsetLat + (Math.random() * 0.0002 - 0.0001);
  const lon = baseLon + offsetLon + (Math.random() * 0.0002 - 0.0001);
  return `${lat.toFixed(6)},${lon.toFixed(6)},0`;
}

function publish(feed, animal, val) {
  const payload = JSON.stringify({ id: animal.id, rfid: animal.rfid, val });
  client.publish(feed, payload);
}

// ─── MQTT Setup ──────────────────────────────────────────────────────

const client = mqtt.connect(`mqtts://${ADAFRUIT_IO.BROKER}`, {
  port: ADAFRUIT_IO.PORT,
  username: ADAFRUIT_IO.USERNAME,
  password: ADAFRUIT_IO.AIO_KEY,
});

const rl = readline.createInterface({ input: process.stdin, output: process.stdout });

client.on('connect', () => {
  console.log(`\n✅ Connected to Adafruit IO as ${ADAFRUIT_IO.USERNAME}`);
  selectAnimal();
});

client.on('error', (err) => {
  console.error('❌ MQTT Error:', err.message);
  process.exit(1);
});

// ─── Step 1: Select Animal ───────────────────────────────────────────

function selectAnimal() {
  console.log('\n─── SELECT ANIMAL ───');
  Object.keys(ANIMALS).forEach(key => {
    console.log(`${key}. ${ANIMALS[key].name}  [ID: ${ANIMALS[key].id} / RFID: ${ANIMALS[key].rfid}]`);
  });
  console.log('Q. Quit');

  rl.question('\nEnter animal (1-4): ', (choice) => {
    if (choice.toLowerCase() === 'q') {
      console.log('Exiting...');
      client.end();
      process.exit(0);
    }
    const animal = ANIMALS[choice];
    if (!animal) {
      console.log('❌ Invalid choice. Try again.');
      selectAnimal();
      return;
    }
    console.log(`\n🐄 Selected: ${animal.name}  (ID: ${animal.id})`);
    selectMode(animal);
  });
}

// ─── Step 2: Select Mode ─────────────────────────────────────────────

function selectMode(animal) {
  console.log('\n─── SELECT SIMULATION MODE ───');
  Object.keys(MODES).forEach(key => {
    console.log(`${key}. ${MODES[key].name}`);
  });
  console.log('B. Back (change animal)');
  console.log('Q. Quit');

  rl.question('\nEnter mode (1-5): ', (choice) => {
    if (choice.toLowerCase() === 'q') {
      console.log('Exiting...');
      client.end();
      process.exit(0);
    }
    if (choice.toLowerCase() === 'b') {
      selectAnimal();
      return;
    }
    const mode = MODES[choice];
    if (!mode) {
      console.log('❌ Invalid choice. Try again.');
      selectMode(animal);
      return;
    }
    console.log(`\n🚀 Starting Simulation: ${animal.name} → ${mode.name}`);
    console.log('Press Ctrl+C to stop and go back to menu.\n');
    startSimulation(animal, mode);
  });
}

// ─── Step 3: Run Simulation ──────────────────────────────────────────

function startSimulation(animal, mode) {
  const interval = setInterval(() => {
    const temp     = jitter(mode.temp, 0.2).toFixed(1);
    const bpm      = Math.round(jitter(mode.bpm, 2));
    const battery  = mode.name.includes('LOW BATTERY')
                       ? jitter(15, 1).toFixed(0)
                       : jitter(mode.batt, 0.5).toFixed(0);
    const humidity = Math.round(jitter(60, 5));
    const gpsVal   = getGPS(
                       GEOFENCE_CENTER.lat + animal.baseLat,
                       GEOFENCE_CENTER.lon + animal.baseLon,
                       mode.lat, mode.lon
                     );
    const geoStat  = (mode.lat === 0 && mode.lon === 0) ? 'Inside' : 'Outside';

    publish(FEEDS.TEMP,     animal, temp);
    publish(FEEDS.BPM,      animal, bpm);
    publish(FEEDS.BATTERY,  animal, battery);
    publish(FEEDS.HUMIDITY, animal, humidity);
    publish(FEEDS.GPS,      animal, gpsVal);
    publish(FEEDS.GEOFENCE, animal, geoStat);

    console.log(`📡 [${animal.id}] Temp=${temp}°C  BPM=${bpm}  Batt=${battery}%  Geo=${geoStat}`);
  }, 5000);

  const onSigInt = () => {
    clearInterval(interval);
    rl.removeListener('SIGINT', onSigInt);
    console.log('\n🛑 Simulation stopped.');
    selectAnimal(); // go back to animal selection
  };

  rl.on('SIGINT', onSigInt);
}
