// Bibliothèque permettant la communication I2C, notamment avec l'écran LCD
#include <Wire.h>

// Bibliothèque permettant de lire les données du capteur DHT11
// (température et humidité)
#include <DHT.h>

// Bibliothèque permettant de contrôler un écran LCD 16x2 équipé d'un module I2C
#include <LiquidCrystal_I2C.h>

// Bibliothèque permettant de configurer et de gérer le Wi-Fi de l'ESP32
#include <WiFi.h>

// Bibliothèque permettant de créer un serveur web HTTP sur l'ESP32
#include <WebServer.h>

// Configuration du point d'accès Wi-Fi
const char* ssid     = "Station-ESP32";
const char* password = "ESP32test26";

// Broches
#define DHT_PIN  18
#define DHT_TYPE DHT11
#define LDR_PIN  34

// ===== Enregistrement des mesures =====

const unsigned long SAMPLE_INTERVAL_MS = 2000 ;
const int           MAX_RECORDS        = 1000 ;

struct Measurement {
  unsigned long timeMs      ;
  float         temperature ;
  float         humidity    ;
  int           ldr         ;
};

// Tableau circulaire de MAX_RECORDS mesures
Measurement records[MAX_RECORDS];
int recordCount = 0;        // nombre de mesures enregistrées
int nextRecordIndex = 0;    // prochain emplacement d'écriture

// Instant de la dernière acquisition (ms depuis le démarrage)
unsigned long lastSampleTime = 0;

// Dernières valeurs mesurées (NAN = pas encore de mesure valide)
float lastTemperature = NAN;
float lastHumidity    = NAN;
int   lastLdr         = 0;   // valeur brute ADC 0..4095 (pas des lux)

// Ecran LCD 16 colonnes x 2 lignes
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT_TYPE);

// Serveur web HTTP sur le port 80
WebServer server(80);

// ---------- Page web ----------
const char page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Station environnementale</title>

<style>
* { box-sizing: border-box; }

body {
  font-family: Arial, sans-serif;
  background: #eef3f8;
  color: #203040;
  margin: 0;
  padding: 18px;
}

h1 { text-align: center; font-size: 25px; }
.subtitle { text-align: center; color: #607080; }

.cards {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(145px, 1fr));
  gap: 12px;
  max-width: 900px;
  margin: 20px auto;
}

.card, .chart {
  background: white;
  border-radius: 14px;
  padding: 16px;
  box-shadow: 0 3px 12px #0001;
}

.card { text-align: center; }
.card h2 { font-size: 16px; }
.value { font-size: 28px; font-weight: bold; }

.chart {
  max-width: 900px;
  margin: 14px auto;
}

.chart h2 { font-size: 17px; }

canvas {
  display: block;
  width: 100%;
  height: 210px;
}

.status {
  text-align: center;
  font-size: 14px;
  color: #26734d;
}

.note {
  color: #607080;
  font-size: 12px;
}

.actions { text-align: center; }

.export-button {
  display: inline-block;
  margin: 12px 0 20px;
  padding: 12px 18px;
  background: #1677d2;
  color: white;
  text-decoration: none;
  border-radius: 8px;
  font-weight: bold;
}

.export-button:hover {
  background: #105da8;
}
</style>
</head>

<body>
<h1>Station environnementale</h1>
<p class="subtitle">Tableau de bord en temps réel</p>
<p class="status" id="status">Connexion aux capteurs...</p>

<div class="actions">
  <a class="export-button" href="/export">
    Télécharger les données CSV
  </a>
</div>

<div class="cards">
  <div class="card">
    <h2>Température</h2>
    <div class="value"><span id="temp">--</span> °C</div>
  </div>

  <div class="card">
    <h2>Humidité</h2>
    <div class="value"><span id="hum">--</span> %</div>
  </div>

  <div class="card">
    <h2>Luminosité (LDR)</h2>
    <div class="value"><span id="ldr">--</span></div>
    <p class="note">Valeur ADC sur 4095</p>
  </div>
</div>

<div class="chart">
  <h2>Évolution de la température</h2>
  <canvas id="tempChart" width="600" height="240"></canvas>
</div>

<div class="chart">
  <h2>Évolution de l'humidité</h2>
  <canvas id="humChart" width="600" height="240"></canvas>
</div>

<div class="chart">
  <h2>Évolution de la luminosité</h2>
  <canvas id="ldrChart" width="600" height="240"></canvas>
</div>

<p class="subtitle note">
  Historique des 20 dernières mesures - intervalle de 2 secondes
</p>

<script>
const MAX_POINTS = 20;

// Numéro absolu de la dernière mesure reçue
let compteur = 0;

const historique = {
  temp: [],
  hum: [],
  ldr: []
};

function dessinerGraphique(id, donnees, couleur, unite, minFixe, maxFixe, decimales) {
  const canvas = document.getElementById(id);
  const ctx = canvas.getContext('2d');

  const w = canvas.width;
  const h = canvas.height;

  ctx.clearRect(0, 0, w, h);

  const gauche = 48;
  const droite = 12;
  const haut = 18;
  const bas = 32;

  const largeur = w - gauche - droite;
  const hauteur = h - haut - bas;

  // Position horizontale identique pour les points et les étiquettes
  const posX = i => gauche + largeur * i / (MAX_POINTS - 1);

  const valides = donnees.filter(v => v !== null);

  if (valides.length === 0) {
    ctx.fillStyle = '#607080';
    ctx.font = '16px Arial';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'alphabetic';
    ctx.fillText('En attente de mesures...', 90, 110);
    return;
  }

  let min = minFixe;
  let max = maxFixe;

  if (min === null || max === null) {
    min = Math.min(...valides);
    max = Math.max(...valides);

    if (min === max) {
      min -= 1;
      max += 1;
    } else {
      const marge = (max - min) * 0.15;
      min -= marge;
      max += marge;
    }
  }

  // Grille et graduations verticales
  ctx.font = '12px Arial';
  ctx.textAlign = 'right';
  ctx.textBaseline = 'middle';
  ctx.strokeStyle = '#e3e9ef';
  ctx.fillStyle = '#607080';
  ctx.lineWidth = 1;

  for (let i = 0; i <= 4; i++) {
    const y = haut + hauteur * i / 4;
    const valeur = max - (max - min) * i / 4;

    ctx.beginPath();
    ctx.moveTo(gauche, y);
    ctx.lineTo(w - droite, y);
    ctx.stroke();

    ctx.fillText(valeur.toFixed(decimales) + unite, gauche - 7, y);
  }

  // Axe horizontal : numéros absolus des mesures
  ctx.textAlign = 'center';
  ctx.textBaseline = 'top';

  const n = donnees.length;

  for (let i = 0; i < n; i += 4) {
    const numero = compteur - n + i + 1;
    ctx.fillText(String(numero), posX(i), h - bas + 8);
  }

  ctx.fillText('Mesure', gauche + largeur / 2, h - 13);

  // Courbe
  ctx.beginPath();
  ctx.strokeStyle = couleur;
  ctx.lineWidth = 3;
  ctx.lineJoin = 'round';

  let traceEnCours = false;

  donnees.forEach((v, i) => {
    if (v === null) {
      traceEnCours = false;
      return;
    }

    const x = posX(i);
    const y = haut + hauteur * (max - v) / (max - min);

    if (!traceEnCours) {
      ctx.moveTo(x, y);
      traceEnCours = true;
    } else {
      ctx.lineTo(x, y);
    }
  });

  ctx.stroke();

  // Points de mesure
  donnees.forEach((v, i) => {
    if (v === null) return;

    const x = posX(i);
    const y = haut + hauteur * (max - v) / (max - min);

    ctx.beginPath();
    ctx.arc(x, y, 3, 0, Math.PI * 2);
    ctx.fillStyle = couleur;
    ctx.fill();
  });
}

async function actualiser() {
  try {
    const reponse = await fetch('/data');

    if (!reponse.ok) {
      throw new Error('Erreur HTTP');
    }

    const d = await reponse.json();

    document.getElementById('temp').textContent =
      d.temperature === null ? '--' : d.temperature;

    document.getElementById('hum').textContent =
      d.humidite === null ? '--' : d.humidite;

    document.getElementById('ldr').textContent = d.ldr;

    compteur++;

    historique.temp.push(d.temperature);
    historique.hum.push(d.humidite);
    historique.ldr.push(d.ldr);

    // Garder uniquement les 20 dernières mesures
    for (const cle in historique) {
      if (historique[cle].length > MAX_POINTS) {
        historique[cle].shift();
      }
    }

    dessinerGraphique(
      'tempChart', historique.temp, '#e45757', ' °C', null, null, 1
    );

    dessinerGraphique(
      'humChart', historique.hum, '#2878d0', ' %', 0, 100, 0
    );

    dessinerGraphique(
      'ldrChart', historique.ldr, '#d99a18', '', 0, 4095, 0
    );

    document.getElementById('status').textContent =
      'Mesures actualisées à ' +
      new Date().toLocaleTimeString('fr-FR');

  } catch (erreur) {
    document.getElementById('status').textContent =
      "Erreur de communication avec l'ESP32";
  }
}

actualiser();
setInterval(actualiser, 2000);
</script>
</body>
</html>
)rawliteral";

// _______________________________________________________________

// Page d'accueil "/"
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", page);
}

/// Dernières mesures au format JSON ("/data")
void handleData() {
  String json = "{\"temperature\":";

  if (isnan(lastTemperature)) {
    json += "null";
  } else {
    json += String(lastTemperature, 1);
  }

  json += ",\"humidite\":";

  if (isnan(lastHumidity)) {
    json += "null";
  } else {
    json += String(lastHumidity, 1);
  }

  json += ",\"ldr\":";
  json += String(lastLdr);
  json += "}";

  server.send(200, "application/json", json);
}

// Ajoute une valeur décimale au CSV, ou NA si elle est invalide
void ajouterValeur(String &ligne, float valeur, int decimales) {
  if (isnan(valeur)) {
    ligne += "NA";
  } else {
    ligne += String(valeur, decimales);
  }
}

// Export CSV envoyé ligne par ligne (évite un énorme String en RAM)
void handleExport() {
  server.sendHeader("Content-Disposition",
                    "attachment; filename=station_esp32.csv");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv; charset=utf-8", "");

  server.sendContent("Temps_ecoule_s;Temperature_C;Humidite_pct;LDR_ADC\r\n");

  // Si le tableau est plein, nextRecordIndex désigne la plus ancienne mesure
  int startIndex = (recordCount == MAX_RECORDS) ? nextRecordIndex : 0;

  for (int i = 0; i < recordCount; i++) {
    int index = (startIndex + i) % MAX_RECORDS;
    Measurement m = records[index];

    String ligne = String(m.timeMs / 1000.0, 3);
    ligne += ";";
    ajouterValeur(ligne, m.temperature, 1);
    ligne += ";";
    ajouterValeur(ligne, m.humidity, 1);
    ligne += ";";
    ligne += String(m.ldr);
    ligne += "\r\n";

    server.sendContent(ligne);
  }

  // Fin du transfert (chunk vide)
  server.sendContent("");
}

void acquireMeasurement() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int   ldrValue = analogRead(LDR_PIN);

  // Mémoriser les dernières valeurs
  lastTemperature = temperature;
  lastHumidity    = humidity;
  lastLdr         = ldrValue;

  // Enregistrer la mesure
  records[nextRecordIndex].timeMs      = millis();
  records[nextRecordIndex].temperature = temperature;
  records[nextRecordIndex].humidity    = humidity;
  records[nextRecordIndex].ldr         = ldrValue;

  nextRecordIndex = (nextRecordIndex + 1) % MAX_RECORDS;

  if (recordCount < MAX_RECORDS) {
    recordCount++;
  }
}

// Affichage sur le moniteur série et sur le LCD (valeurs déjà mesurées)
void afficherMesure() {
  if (isnan(lastTemperature) || isnan(lastHumidity)) {
    Serial.println("Erreur de lecture DHT11");

    lcd.setCursor(0, 0);
    lcd.print("Erreur DHT11    ");
  } else {
    Serial.print("Temperature : ");
    Serial.print(lastTemperature, 1);
    Serial.println(" C");

    Serial.print("Humidite : ");
    Serial.print(lastHumidity, 1);
    Serial.println(" %");

    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(lastTemperature, 1);
    lcd.print((char)223);
    lcd.print("C H:");
    lcd.print(lastHumidity, 0);
    lcd.print("%   ");
  }

  Serial.print("Valeur LDR (ADC) : ");
  Serial.println(lastLdr);
  Serial.println("------------------------");

  lcd.setCursor(0, 1);
  lcd.print("LDR ADC: ");
  lcd.print(lastLdr);
  lcd.print("    ");
}
// _______________________________________________________________

void setup() {
  Serial.begin(9600);

  // Initialisation des capteurs
  dht.begin();
  analogReadResolution(12);  // ADC : valeurs de 0 a 4095

  WiFi.mode(WIFI_AP);

  bool resultat = WiFi.softAP(ssid, password);

  if (resultat) {
    Serial.println();
    Serial.println("Point d'acces demarre !");
    Serial.print("Reseau Wi-Fi : ");
    Serial.println(ssid);
    Serial.print("Adresse du tableau de bord : http://");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Erreur : impossible de creer le Wi-Fi.");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/export", HTTP_GET, handleExport);
  server.begin();

  Serial.println("Serveur web demarre.");


  // Initialisation du LCD
  Wire.begin(21, 22);        // SDA, SCL
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Station env.");
  lcd.setCursor(0, 1);
  lcd.print("Initialisation");
  delay(2000);
  lcd.clear();

  Serial.println("Station environnementale");
  Serial.println("------------------------");


}

void loop() {
  // Le serveur est appelé à chaque passage : aucun delay() bloquant
  server.handleClient();

  unsigned long now = millis();

  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;
    acquireMeasurement();   // une seule lecture des capteurs
    afficherMesure();       // réutilise lastTemperature, lastHumidity, lastLdr
  }
}
