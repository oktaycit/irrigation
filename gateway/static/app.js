const state = {
  programs: [],
  selectedId: 1,
  valveMask: 0,
  dirty: false,
  loadingForm: false,
};

const fields = [
  "irrigation_min",
  "wait_min",
  "repeat_count",
  "days_mask",
  "ph_x100",
  "ec_x100",
  "fert_a_pct",
  "fert_b_pct",
  "fert_c_pct",
  "fert_d_pct",
  "pre_flush_sec",
  "post_flush_sec",
];

function $(id) {
  return document.getElementById(id);
}

function hhmmToTime(value) {
  const padded = String(value).padStart(4, "0");
  return `${padded.slice(0, 2)}:${padded.slice(2)}`;
}

function timeToHhmm(value) {
  return Number(value.replace(":", ""));
}

function formatHhmm(value) {
  return hhmmToTime(value);
}

function validateProgram(program) {
  const problems = [];
  if (!program.days_mask || program.days_mask < 1 || program.days_mask > 127) problems.push("gün");
  if (program.ec_x100 < 0 || program.ec_x100 > 2000) problems.push("EC");
  if (program.ph_x100 < 0 || program.ph_x100 > 1400) problems.push("pH");
  if (program.wait_min > 999) problems.push("bekleme");
  if (program.valve_mask < 1 || program.valve_mask > 255) problems.push("parsel");
  return problems;
}

function setConnection(ok, text) {
  const badge = $("connectionBadge");
  const dot = $("pingBadge");
  badge.className = `status-pill ${ok ? "ok" : "error"}`;
  badge.textContent = ok ? "Bağlandı" : "Koptu";
  dot.className = `status-dot ${ok ? "ok" : "error"}`;
  $("connectionText").textContent = text;
}

async function fetchJson(path, options) {
  const response = await fetch(path, options);
  const data = await response.json();
  if (!response.ok) {
    throw new Error(data.error || response.statusText);
  }
  return data;
}

async function refreshHealth() {
  try {
    const health = await fetchJson("/health");
    $("portLabel").textContent = health.serial_port.split("/").pop();
    $("hostLabel").textContent = window.location.host;
  } catch {
    $("portLabel").textContent = "CDC yok";
  }
}

async function refreshPing() {
  try {
    const data = await fetchJson("/api/ping");
    setConnection(data.ok, data.ok ? "STM32 CDC yanıt veriyor." : "STM32 yanıtı beklenen formatta değil.");
  } catch (error) {
    setConnection(false, error.message);
  }
}

async function refreshPrograms() {
  try {
    const data = await fetchJson("/api/programs");
    state.programs = data.programs;
    renderProgramList();
    if (!state.dirty) {
      selectProgram(state.selectedId, { force: true });
    } else {
      updateEditorBadgesFromForm();
      $("saveStatus").textContent = "Düzenleme sürüyor, otomatik yenileme bekletildi.";
    }
    updateSummary();
  } catch (error) {
    $("healthText").textContent = error.message;
  }
}

function updateSummary() {
  const enabled = state.programs.filter((program) => program.enabled);
  const invalid = state.programs.filter((program) => validateProgram(program).length > 0);
  const active = state.programs.find((program) => program.enabled) || state.programs[0];

  $("activeProgram").textContent = active ? `P${active.id}` : "--";
  $("activeValves").textContent = active ? maskText(active.valve_mask) : "--";
  $("cycleSummary").textContent = enabled.length ? `${enabled.length} aktif` : "Pasif";
  $("healthText").textContent = invalid.length ? `${invalid.length} reçete kontrol istiyor.` : "Tüm reçeteler sınırlar içinde.";
  $("eventText").textContent = `Son okuma ${new Date().toLocaleTimeString("tr-TR", { hour: "2-digit", minute: "2-digit", second: "2-digit" })}`;
}

function renderProgramList() {
  const list = $("programList");
  list.innerHTML = "";
  state.programs.forEach((program) => {
    const problems = validateProgram(program);
    const row = document.createElement("button");
    row.type = "button";
    row.className = `program-row ${program.enabled ? "enabled" : ""} ${program.id === state.selectedId ? "selected" : ""} ${problems.length ? "invalid" : ""}`;
    row.innerHTML = `
      <span class="program-id">${program.id}</span>
      <span>
        <strong>${formatHhmm(program.start_hhmm)} - ${formatHhmm(program.end_hhmm)}</strong>
        <small>${maskText(program.valve_mask)} · ${program.irrigation_min} dk · pH ${(program.ph_x100 / 100).toFixed(2)} · EC ${(program.ec_x100 / 100).toFixed(2)}</small>
      </span>
      <span class="badge ${problems.length ? "bad" : program.enabled ? "on" : ""}">${problems.length ? "KONTROL" : program.enabled ? "AKTİF" : "PASİF"}</span>
    `;
    row.addEventListener("click", () => selectProgram(program.id));
    list.appendChild(row);
  });
}

function selectProgram(id, options = {}) {
  if (state.dirty && !options.force && id !== state.selectedId) {
    const shouldDiscard = window.confirm("Kaydedilmemiş değişiklikler var. Program değiştirilsin mi?");
    if (!shouldDiscard) return;
  }

  const program = state.programs.find((item) => item.id === id);
  if (!program) return;

  state.loadingForm = true;
  state.selectedId = id;
  state.valveMask = program.valve_mask;
  $("editorTitle").textContent = `Program ${program.id}`;
  $("enabled").checked = Boolean(program.enabled);
  $("start_hhmm").value = hhmmToTime(program.start_hhmm);
  $("end_hhmm").value = hhmmToTime(program.end_hhmm);
  fields.forEach((field) => {
    $(field).value = program[field];
  });
  $("phTarget").textContent = (program.ph_x100 / 100).toFixed(2);
  $("ecTarget").textContent = (program.ec_x100 / 100).toFixed(2);
  $("preFlush").textContent = `${program.pre_flush_sec} sn`;
  renderValves();
  renderProgramList();
  state.loadingForm = false;
  state.dirty = false;
  $("saveStatus").textContent = "";
}

function maskText(mask) {
  const valves = [];
  for (let i = 0; i < 8; i += 1) {
    if (mask & (1 << i)) valves.push(i + 1);
  }
  return valves.length ? `P${valves.join(",")}` : "Parsel yok";
}

function renderValves() {
  const grid = $("valveGrid");
  grid.innerHTML = "";
  for (let i = 0; i < 8; i += 1) {
    const bit = 1 << i;
    const button = document.createElement("button");
    button.type = "button";
    button.className = `valve-button ${state.valveMask & bit ? "active" : ""}`;
    button.textContent = `P${i + 1}`;
    button.addEventListener("click", () => {
      state.valveMask ^= bit;
      markDirty();
      renderValves();
    });
    grid.appendChild(button);
  }
}

function markDirty() {
  if (state.loadingForm) return;
  state.dirty = true;
  updateEditorBadgesFromForm();
  $("saveStatus").textContent = "Kaydedilmemiş değişiklik var.";
}

function updateEditorBadgesFromForm() {
  const ph = Number($("ph_x100").value || 0);
  const ec = Number($("ec_x100").value || 0);
  const preFlush = Number($("pre_flush_sec").value || 0);
  $("phTarget").textContent = (ph / 100).toFixed(2);
  $("ecTarget").textContent = (ec / 100).toFixed(2);
  $("preFlush").textContent = `${preFlush} sn`;
}

async function saveProgram() {
  const payload = {
    enabled: $("enabled").checked ? 1 : 0,
    start_hhmm: timeToHhmm($("start_hhmm").value),
    end_hhmm: timeToHhmm($("end_hhmm").value),
    valve_mask: state.valveMask,
  };
  fields.forEach((field) => {
    payload[field] = Number($(field).value);
  });

  $("saveStatus").textContent = "Kaydediliyor...";
  try {
    const result = await fetchJson(`/api/programs/${state.selectedId}`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    state.dirty = false;
    $("saveStatus").textContent = result.ok ? "Kaydedildi." : "Cevap kontrol istiyor.";
    await refreshPrograms();
  } catch (error) {
    $("saveStatus").textContent = error.message;
  }
}

function tickClock() {
  $("clockLabel").textContent = new Date().toLocaleString("tr-TR", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });
}

function bindUi() {
  $("refreshBtn").addEventListener("click", refreshPrograms);
  $("saveBtn").addEventListener("click", saveProgram);
  $("programForm").addEventListener("input", markDirty);
  $("programForm").addEventListener("change", markDirty);
  document.querySelector(".fert-grid").addEventListener("input", markDirty);
  document.querySelector(".fert-grid").addEventListener("change", markDirty);
  document.querySelectorAll("[data-mode]").forEach((button) => {
    button.addEventListener("click", () => {
      document.querySelectorAll("[data-mode]").forEach((item) => item.classList.remove("active"));
      button.classList.add("active");
      $("modeLabel").textContent = button.dataset.mode === "service" ? "Servis İzleme" : "Program İzleme";
    });
  });
}

async function boot() {
  bindUi();
  renderValves();
  tickClock();
  setInterval(tickClock, 1000);
  await refreshHealth();
  await refreshPing();
  await refreshPrograms();
  setInterval(refreshPing, 5000);
  setInterval(refreshPrograms, 15000);
  $("splash").classList.add("hidden");
}

boot();
