// =============================================================================
// Interface web embarquee - HTML/CSS/JS sans dependance externe
// =============================================================================
#pragma once

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Duco VMC</title>
<style>
  :root {
    color-scheme: dark;
    --bg: #111518;
    --panel: #1d2327;
    --panel-2: #151a1e;
    --line: #303941;
    --text: #eef4f6;
    --muted: #9faab2;
    --accent: #6fc7df;
    --accent-2: #89d27a;
    --bad: #ee7272;
    --warn: #e6ba62;
  }
  * { box-sizing: border-box; }
  body {
    margin: 0;
    font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    background: var(--bg);
    color: var(--text);
  }
  main {
    width: min(1040px, 100%);
    margin: 0 auto;
    padding: 18px;
  }
  header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    margin-bottom: 16px;
  }
  h1 {
    margin: 0;
    font-size: 1.45rem;
    line-height: 1.1;
    letter-spacing: 0;
  }
  .header-meta {
    display: flex;
    align-items: center;
    justify-content: flex-end;
    flex-wrap: wrap;
    gap: 12px;
    color: var(--muted);
    font-size: 0.9rem;
  }
  .bus-pill {
    display: inline-flex;
    align-items: center;
    gap: 7px;
    min-height: 30px;
    padding: 0 10px;
    border: 1px solid var(--line);
    border-radius: 999px;
    background: var(--panel-2);
    color: var(--text);
    font-weight: 700;
  }
  .dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--bad);
    box-shadow: 0 0 0 3px rgba(238, 114, 114, 0.12);
  }
  .dot.ok {
    background: var(--accent-2);
    box-shadow: 0 0 0 3px rgba(137, 210, 122, 0.12);
  }
  .header-button {
    min-height: 30px;
    padding: 0 11px;
    color: var(--text);
    background: var(--panel-2);
    border: 1px solid var(--line);
    border-radius: 999px;
    font-size: 0.9rem;
    font-weight: 760;
  }
  .layout {
    display: grid;
    grid-template-columns: minmax(0, 1.28fr) minmax(320px, 0.72fr);
    gap: 14px;
    align-items: start;
  }
  .stack {
    display: grid;
    gap: 14px;
  }
  section {
    background: var(--panel);
    border: 1px solid var(--line);
    border-radius: 8px;
    padding: 16px;
  }
  h2 {
    margin: 0 0 13px;
    font-size: 0.96rem;
    color: var(--muted);
    font-weight: 700;
  }
  .metric-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 10px;
  }
  .metric {
    min-height: 64px;
    padding: 10px 12px;
    background: var(--panel-2);
    border: 1px solid rgba(255,255,255,0.06);
    border-radius: 7px;
  }
  .metric span {
    display: block;
    color: var(--muted);
    font-size: 0.82rem;
  }
  .metric strong {
    display: block;
    margin-top: 4px;
    font-size: 1.05rem;
    font-variant-numeric: tabular-nums;
    overflow-wrap: anywhere;
  }
  .metric.wide {
    grid-column: 1 / -1;
  }
  .controls {
    display: grid;
    grid-template-columns: minmax(170px, 1fr) auto;
    gap: 10px;
    align-items: end;
  }
  label {
    display: block;
    margin-bottom: 6px;
    color: var(--muted);
    font-size: 0.84rem;
  }
  input[type=number] {
    width: 100%;
    height: 42px;
    padding: 0 10px;
    color: var(--text);
    background: var(--panel-2);
    border: 1px solid var(--line);
    border-radius: 6px;
    font: inherit;
  }
  .button-grid {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 10px;
    margin-top: 12px;
  }
  .mode-grid {
    grid-template-columns: repeat(4, minmax(0, 1fr));
  }
  button {
    min-height: 42px;
    padding: 0 12px;
    border: 0;
    border-radius: 6px;
    background: var(--accent);
    color: #071014;
    font: inherit;
    font-weight: 760;
    cursor: pointer;
  }
  button.secondary {
    color: var(--text);
    background: #2b3339;
    border: 1px solid var(--line);
  }
  button:disabled {
    opacity: 0.55;
    cursor: wait;
  }
  .status {
    min-height: 22px;
    margin-top: 12px;
    color: var(--muted);
    font-size: 0.92rem;
  }
  .ok-text { color: var(--accent-2); }
  .bad-text { color: var(--bad); }
  .warn-text { color: var(--warn); }
  .state-pill {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    width: fit-content;
    min-height: 28px;
    margin-top: 6px;
    padding: 4px 10px;
    border-radius: 999px;
    border: 1px solid transparent;
    font-size: 0.95rem;
    line-height: 1.15;
  }
  .state-auto {
    color: #d8f3ff;
    background: rgba(111, 199, 223, 0.16);
    border-color: rgba(111, 199, 223, 0.42);
  }
  .state-power {
    color: #ecffd8;
    background: rgba(137, 210, 122, 0.16);
    border-color: rgba(137, 210, 122, 0.42);
  }
  .state-boost {
    color: #fff0d6;
    background: rgba(240, 160, 90, 0.17);
    border-color: rgba(240, 160, 90, 0.46);
  }
  .state-away {
    color: #dee7ed;
    background: rgba(159, 170, 178, 0.14);
    border-color: rgba(159, 170, 178, 0.32);
  }
  .state-open {
    color: #d9ffce;
    background: rgba(137, 210, 122, 0.18);
    border-color: rgba(137, 210, 122, 0.5);
  }
  .state-closed {
    color: #ffdcdc;
    background: rgba(238, 114, 114, 0.16);
    border-color: rgba(238, 114, 114, 0.42);
  }
  .temp-panel {
    padding: 0;
    overflow: hidden;
  }
  .temp-title {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    padding: 16px 16px 0;
  }
  .temp-title h2 { margin: 0; }
  .delta-chip {
    color: var(--text);
    background: var(--panel-2);
    border: 1px solid var(--line);
    border-radius: 999px;
    padding: 6px 10px;
    font-size: 0.9rem;
    font-weight: 750;
    font-variant-numeric: tabular-nums;
  }
  .schema-wrap {
    padding: 8px 10px 12px;
  }
  svg {
    width: 100%;
    height: auto;
    display: block;
  }
  .svg-temp {
    fill: #f7fbfc;
    font-size: 25px;
    font-weight: 780;
    font-variant-numeric: tabular-nums;
  }
  .temp-ring {
    fill: rgba(111, 199, 223, 0.12);
    stroke: var(--accent);
    stroke-width: 3;
  }
  .flow-arrow {
    stroke: #dce5e9;
    stroke-width: 7;
    fill: none;
    stroke-linecap: square;
    stroke-linejoin: miter;
  }
  footer {
    display: flex;
    justify-content: space-between;
    gap: 14px;
    flex-wrap: wrap;
    margin-top: 14px;
    padding: 0 2px;
    color: var(--muted);
    font-size: 0.78rem;
  }
  footer span {
    font-variant-numeric: tabular-nums;
  }
  .settings-overlay {
    position: fixed;
    inset: 0;
    z-index: 20;
    display: flex;
    justify-content: flex-end;
    background: rgba(0, 0, 0, 0.48);
  }
  .settings-overlay[hidden] {
    display: none;
  }
  .settings-panel {
    width: min(460px, 100%);
    height: 100%;
    overflow-y: auto;
    padding: 18px;
    background: var(--bg);
    border-left: 1px solid var(--line);
    box-shadow: -18px 0 40px rgba(0, 0, 0, 0.32);
  }
  .settings-head {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    margin-bottom: 14px;
  }
  .settings-head h2 {
    margin: 0;
    color: var(--text);
    font-size: 1.08rem;
  }
  .settings-panel section {
    margin-bottom: 14px;
  }
  .limit-list {
    display: grid;
    gap: 9px;
    color: var(--muted);
    font-size: 0.9rem;
  }
  .limit-list strong {
    color: var(--text);
    font-variant-numeric: tabular-nums;
  }
  @media (max-width: 780px) {
    main { padding: 14px; }
    header {
      align-items: flex-start;
      flex-direction: column;
      gap: 8px;
    }
    .header-meta { justify-content: flex-start; }
    .layout,
    .metric-grid,
    .controls,
    .button-grid,
    .mode-grid {
      grid-template-columns: 1fr;
    }
    section { padding: 14px; }
    .temp-title { padding: 14px 14px 0; }
    .schema-wrap { padding: 6px 4px 8px; }
    .settings-panel {
      width: 100%;
      border-left: 0;
    }
  }
</style>
</head>
<body>
<main>
  <header>
    <h1>Duco VMC</h1>
    <div class="header-meta">
      <span id="bus_pill" class="bus-pill"><span id="bus_dot" class="dot"></span>Bus: <span id="online">...</span></span>
      <span>Mise à jour <span id="last">jamais</span></span>
      <button class="header-button" data-no-disable="1" onclick="openSettings()">Paramètres</button>
    </div>
  </header>

  <div class="layout">
    <section class="temp-panel">
      <div class="temp-title">
        <h2>Températures</h2>
        <span class="delta-chip">Delta Int/ext <span id="delta">-</span></span>
      </div>
      <div class="schema-wrap">
        <svg viewBox="0 0 680 420" role="img" aria-label="Schéma des flux d'air de la VMC">
          <defs>
            <marker id="arrow-white" markerWidth="28" markerHeight="28" refX="25" refY="14" orient="auto" markerUnits="userSpaceOnUse">
              <path d="M0,0 L28,14 L0,28 Z" fill="#dce5e9"></path>
            </marker>
            <linearGradient id="core" x1="0" x2="1" y1="0" y2="1">
              <stop offset="0" stop-color="#252d32"></stop>
              <stop offset="1" stop-color="#151a1e"></stop>
            </linearGradient>
          </defs>

          <rect x="0" y="0" width="680" height="420" rx="10" fill="#151a1e"></rect>
          <rect x="108" y="82" width="438" height="246" rx="8" fill="url(#core)" stroke="#dce5e9" stroke-width="6"></rect>

          <path d="M80 126 L138 218" class="flow-arrow" marker-end="url(#arrow-white)"></path>
          <path d="M510 152 L602 96" class="flow-arrow" marker-end="url(#arrow-white)"></path>

          <ellipse cx="82" cy="78" rx="58" ry="30" class="temp-ring"></ellipse>
          <text id="oda" x="82" y="87" text-anchor="middle" class="svg-temp">-</text>

          <text id="sup" x="168" y="252" class="svg-temp">-</text>

          <ellipse cx="382" cy="176" rx="64" ry="32" class="temp-ring"></ellipse>
          <text id="eta" x="382" y="185" text-anchor="middle" class="svg-temp">-</text>

          <text id="eha" x="576" y="58" text-anchor="middle" class="svg-temp">-</text>
        </svg>
      </div>
    </section>

    <div class="stack">
      <section>
        <h2>Ventilation</h2>
        <div class="metric-grid">
          <div class="metric"><span>Mode</span><strong id="mode" class="state-pill state-auto">...</strong></div>
          <div class="metric"><span>Consigne</span><strong id="comfort">...</strong></div>
          <div class="metric"><span>Débit cible</span><strong id="flow">...</strong></div>
          <div class="metric"><span>Actualisation</span><strong id="poll_state">auto</strong></div>
        </div>

        <div class="controls" style="margin-top: 14px;">
          <div>
            <label for="set_comfort">Réglage consigne</label>
            <input type="number" id="set_comfort" min="10" max="30" step="0.5" value="19.0">
          </div>
          <button onclick="applyComfort()">Appliquer consigne</button>
        </div>

        <div class="button-grid mode-grid">
          <button class="secondary" onclick="applyModeValue(0, 'Auto')">Auto</button>
          <button onclick="applyModeValue(8, 'Puissance 1')">Puissance 1</button>
          <button onclick="applyModeValue(9, 'Puissance 2')">Puissance 2</button>
          <button onclick="applyModeValue(10, 'Puissance 3')">Puissance 3</button>
        </div>
        <div class="button-grid">
          <button class="secondary" onclick="applyModeValue(4, 'Boost P1 15 min')">P1 15 min</button>
          <button class="secondary" onclick="applyModeValue(5, 'Boost P2 15 min')">P2 15 min</button>
          <button class="secondary" onclick="applyModeValue(6, 'Boost P3 15 min')">P3 15 min</button>
        </div>
        <div class="button-grid">
          <button class="secondary" onclick="applyModeValue(132, 'Boost P1 30 min')">P1 30 min</button>
          <button class="secondary" onclick="applyModeValue(133, 'Boost P2 30 min')">P2 30 min</button>
          <button class="secondary" onclick="applyModeValue(134, 'Boost P3 30 min')">P3 30 min</button>
        </div>
        <div class="button-grid">
          <button class="secondary" onclick="applyModeValue(196, 'Boost P1 45 min')">P1 45 min</button>
          <button class="secondary" onclick="applyModeValue(197, 'Boost P2 45 min')">P2 45 min</button>
          <button class="secondary" onclick="applyModeValue(198, 'Boost P3 45 min')">P3 45 min</button>
        </div>
      </section>

      <section>
        <h2>Bypass</h2>
        <div class="metric-grid">
          <div class="metric"><span>État</span><strong id="bypass" class="state-pill">...</strong></div>
          <div class="metric"><span>Ouverture</span><strong id="bypass_position">...</strong></div>
          <div class="metric"><span>Mode</span><strong id="bypass_mode" class="state-pill state-auto">...</strong></div>
        </div>
        <div class="button-grid">
          <button class="secondary" onclick="applyBypassMode(0)">Auto</button>
          <button onclick="applyBypassMode(2)">Ouvrir</button>
          <button onclick="applyBypassMode(1)">Fermer</button>
        </div>
      </section>

      <div id="status" class="status"></div>
    </div>
  </div>

  <div id="settings_overlay" class="settings-overlay" hidden onclick="closeSettingsOnBackdrop(event)">
    <aside class="settings-panel" role="dialog" aria-modal="true" aria-labelledby="settings_title">
      <div class="settings-head">
        <h2 id="settings_title">Paramètres</h2>
        <button class="secondary header-button" data-no-disable="1" onclick="closeSettings()">Fermer</button>
      </div>

      <section>
        <h2>Régulation auto</h2>
        <div class="metric-grid">
          <div class="metric"><span>Activée</span><strong id="auto_enabled" class="state-pill">...</strong></div>
          <div class="metric"><span>État</span><strong id="auto_action" class="state-pill">...</strong></div>
          <div class="metric"><span>Prévision demain</span><strong id="forecast">...</strong></div>
          <div class="metric"><span>Pause manuelle</span><strong id="manual_hold">...</strong></div>
        </div>
        <div class="metric" style="margin-top: 10px;">
          <span>Raison</span>
          <strong id="auto_reason">...</strong>
        </div>
        <div class="button-grid">
          <button id="auto_toggle_btn" onclick="toggleAutoRegulation()">Activer</button>
          <button class="secondary" onclick="clearAutoHold()">Reprendre auto</button>
          <button class="secondary" onclick="refreshStateNow()">Actualiser</button>
        </div>
      </section>

      <section>
        <h2>Système</h2>
        <div class="metric-grid">
          <div class="metric wide"><span>Numéro série</span><strong id="serial">...</strong></div>
          <div class="metric"><span>Filtre restant</span><strong id="filter_remaining">...</strong></div>
          <div class="metric"><span>Temps mode</span><strong id="mode_time_remaining">...</strong></div>
        </div>
      </section>

      <section>
        <h2>Limites d'écriture</h2>
        <div class="limit-list">
          <div>Écritures Duco: <strong><span id="settings_writes_today">-</span>/<span id="settings_write_limit">-</span></strong></div>
          <div>Restant aujourd'hui: <strong id="settings_writes_remaining">-</strong></div>
          <div>Intervalle minimum: <strong id="settings_write_interval">-</strong></div>
          <div>Horloge: <strong id="settings_write_clock">-</strong></div>
        </div>
      </section>
    </aside>
  </div>

  <footer>
    <span>Écritures: <span id="writes_today">-</span>/<span id="write_limit">-</span>, restant <span id="writes_remaining">-</span></span>
    <span>Intervalle min: <span id="write_interval">2 s</span></span>
    <span>Horloge: <span id="write_clock">-</span></span>
  </footer>
</main>

<script>
const $ = id => document.getElementById(id);
const MODE_LABEL = {
  0:"Auto", 4:"Boost P1 15 min", 5:"Boost P2 15 min", 6:"Boost P3 15 min", 7:"Absent",
  8:"Puissance 1", 9:"Puissance 2", 10:"Puissance 3",
  132:"Boost P1 30 min", 133:"Boost P2 30 min", 134:"Boost P3 30 min",
  196:"Boost P1 45 min", 197:"Boost P2 45 min", 198:"Boost P3 45 min"
};
const BYPASS_MODE_LABEL = { 0:"Auto", 1:"Fermé forcé", 2:"Ouvert forcé" };
const AUTO_ACTION_LABEL = {
  disabled:"Désactivée",
  waiting:"Attente",
  idle:"Repos",
  cooling_forecast:"Refroidissement",
  cooling_local:"Refroidissement",
  heat_protection:"Protection chaleur",
  winter_protection:"Protection hiver",
  manual_hold:"Pause manuelle"
};

function fmtTemp(v) {
  return (v == null || Number.isNaN(Number(v))) ? "-" : Number(v).toFixed(1) + "°";
}

function fmtPercent(v) {
  return (v == null || Number.isNaN(Number(v))) ? "-" : Number(v) + " %";
}

function fmtDays(v) {
  return (v == null || Number.isNaN(Number(v))) ? "-" : Number(v) + " j";
}

function fmtDurationSeconds(v) {
  if (v == null || Number.isNaN(Number(v))) return "-";
  const total = Math.max(0, Number(v));
  const minutes = Math.floor(total / 60);
  const seconds = total % 60;
  if (minutes <= 0) return seconds + " s";
  const hours = Math.floor(minutes / 60);
  const rest = minutes % 60;
  if (hours <= 0) return minutes + " min";
  return rest ? (hours + " h " + rest + " min") : (hours + " h");
}

function modeStateClass(mode) {
  if (mode === 0) return "state-pill state-auto";
  if ((mode >= 4 && mode <= 6) || (mode >= 132 && mode <= 134) || (mode >= 196 && mode <= 198)) return "state-pill state-boost";
  if (mode >= 8 && mode <= 10) return "state-pill state-power";
  if (mode === 7) return "state-pill state-away";
  return "state-pill";
}

function bypassModeStateClass(mode) {
  if (mode === 1) return "state-pill state-closed";
  if (mode === 2) return "state-pill state-open";
  return "state-pill state-auto";
}

function autoStateClass(action, enabled) {
  if (!enabled || action === "disabled") return "state-pill state-away";
  if (action === "manual_hold") return "state-pill state-boost";
  if (action === "cooling_forecast" || action === "cooling_local") return "state-pill state-power";
  if (action === "heat_protection" || action === "winter_protection") return "state-pill state-closed";
  return "state-pill state-auto";
}

function setStatus(text, level) {
  const el = $("status");
  el.textContent = text || "";
  el.className = "status " + (level || "");
}

function setButtonsDisabled(disabled) {
  document.querySelectorAll("button").forEach(btn => {
    if (btn.dataset.noDisable === "1") return;
    btn.disabled = disabled;
  });
}

function openSettings() {
  $("settings_overlay").hidden = false;
}

function closeSettings() {
  $("settings_overlay").hidden = true;
}

function closeSettingsOnBackdrop(event) {
  if (event.target && event.target.id === "settings_overlay") closeSettings();
}

function setBusState(online) {
  $("online").textContent = online ? "en ligne" : "hors ligne";
  $("bus_dot").className = "dot " + (online ? "ok" : "");
  $("bus_pill").className = "bus-pill " + (online ? "ok-text" : "bad-text");
}

function formatHold(ms) {
  if (!ms || ms <= 0) return "non";
  const minutes = Math.ceil(ms / 60000);
  if (minutes < 60) return minutes + " min";
  const hours = Math.floor(minutes / 60);
  const rest = minutes % 60;
  return rest ? (hours + " h " + rest + " min") : (hours + " h");
}

async function refresh() {
  try {
    const r = await fetch("/api/state", { cache: "no-store" });
    const s = await r.json();

    setBusState(!!s.online);
    $("mode").textContent = MODE_LABEL[s.mode] || ("Mode " + s.mode);
    $("mode").className = modeStateClass(Number(s.mode));
    $("comfort").textContent = fmtTemp(s.comfort);
    $("flow").textContent = fmtPercent(s.flow);
    $("serial").textContent = s.serial || "-";
    $("filter_remaining").textContent = fmtDays(s.filter_remaining_days);
    $("mode_time_remaining").textContent = fmtDurationSeconds(s.mode_time_remaining_sec);

    $("bypass").textContent = s.bypass ? "Ouvert" : "Fermé";
    $("bypass").className = "state-pill " + (s.bypass ? "state-open" : "state-closed");
    $("bypass_position").textContent = fmtPercent(s.bypass_position);
    $("bypass_mode").textContent = BYPASS_MODE_LABEL[s.bypass_mode] || ("Mode " + s.bypass_mode);
    $("bypass_mode").className = bypassModeStateClass(Number(s.bypass_mode));

    $("oda").textContent = fmtTemp(s.oda);
    $("sup").textContent = fmtTemp(s.sup);
    $("eta").textContent = fmtTemp(s.eta);
    $("eha").textContent = fmtTemp(s.eha);
    const delta = (s.eta != null && s.oda != null) ? s.eta - s.oda : null;
    $("delta").textContent = fmtTemp(delta);

    $("writes_today").textContent = s.writes_today;
    $("write_limit").textContent = s.write_limit_per_day;
    $("writes_remaining").textContent = s.writes_remaining;
    $("write_interval").textContent = (s.write_min_interval_ms / 1000).toFixed(0) + " s";
    $("write_clock").textContent = s.write_clock_synced ? "synchronisée" : "non synchronisée";
    $("settings_writes_today").textContent = s.writes_today;
    $("settings_write_limit").textContent = s.write_limit_per_day;
    $("settings_writes_remaining").textContent = s.writes_remaining;
    $("settings_write_interval").textContent = (s.write_min_interval_ms / 1000).toFixed(0) + " s";
    $("settings_write_clock").textContent = s.write_clock_synced ? "synchronisée" : "non synchronisée";

    $("auto_enabled").textContent = s.auto_regulation_enabled ? "oui" : "non";
    $("auto_enabled").className = "state-pill " + (s.auto_regulation_enabled ? "state-power" : "state-away");
    $("auto_action").textContent = AUTO_ACTION_LABEL[s.auto_regulation_action] || s.auto_regulation_action || "-";
    $("auto_action").className = autoStateClass(s.auto_regulation_action, s.auto_regulation_enabled);
    $("auto_reason").textContent = s.auto_regulation_reason || "-";
    $("forecast").textContent = s.forecast_ok
      ? (fmtTemp(s.forecast_tomorrow_min) + " / " + fmtTemp(s.forecast_tomorrow_max))
      : (s.forecast_error ? ("indispo: " + s.forecast_error) : "indisponible");
    $("manual_hold").textContent = s.manual_override_active
      ? formatHold(s.manual_override_remaining_ms)
      : "non";
    $("manual_hold").className = "state-pill " + (s.manual_override_active ? "state-boost" : "state-auto");
    $("auto_toggle_btn").textContent = s.auto_regulation_enabled ? "Désactiver" : "Activer";
    $("auto_toggle_btn").dataset.enabled = s.auto_regulation_enabled ? "1" : "0";

    if (s.comfort != null) $("set_comfort").value = Number(s.comfort).toFixed(1);
    $("last").textContent = new Date().toLocaleTimeString();
    $("poll_state").textContent = "OK";
  } catch (e) {
    setBusState(false);
    $("poll_state").textContent = "Erreur";
    setStatus("Erreur API", "bad-text");
  }
}

async function postCommand(url, okText) {
  setButtonsDisabled(true);
  setStatus("Commande en cours...", "");
  try {
    const r = await fetch(url, { method: "POST" });
    const body = await r.text();
    if (!r.ok) {
      setStatus(body || ("Erreur HTTP " + r.status), r.status === 429 ? "warn-text" : "bad-text");
      return;
    }
    setStatus(okText || "Commande envoyée", "ok-text");
    setTimeout(refreshStateNow, 700);
  } catch (e) {
    setStatus("Erreur réseau", "bad-text");
  } finally {
    setTimeout(() => setButtonsDisabled(false), 2100);
  }
}

function applyModeValue(mode, label) {
  postCommand("/api/set_mode?value=" + mode, label + " appliqué");
}

function applyComfort() {
  postCommand("/api/set_comfort?value=" + encodeURIComponent($("set_comfort").value), "Consigne appliquée");
}

function applyBypassMode(mode) {
  const labels = {0:"Bypass auto", 1:"Bypass fermé", 2:"Bypass ouvert"};
  postCommand("/api/set_bypass_mode?value=" + mode, labels[mode]);
}

function toggleAutoRegulation() {
  const enabled = $("auto_toggle_btn").dataset.enabled === "1";
  postCommand("/api/set_auto_regulation?enabled=" + (enabled ? "0" : "1"),
              enabled ? "Régulation désactivée" : "Régulation activée");
}

function clearAutoHold() {
  postCommand("/api/clear_auto_hold", "Pause manuelle levée");
}

async function refreshStateNow() {
  await fetch("/api/refresh_state", { method: "POST" });
  setTimeout(refresh, 600);
}

refresh();
setInterval(refresh, 5000);
</script>
</body>
</html>
)HTML";
