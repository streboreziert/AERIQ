from fastapi.responses import HTMLResponse, Response

from main import app


HTML_PAGE = """
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AERIQ Dashboard</title>
  <style>
    :root {
      --bg: #f4f7fb;
      --panel: #ffffff;
      --line: #d9e2ef;
      --text: #132238;
      --muted: #5e7188;
      --accent: #1f8f73;
      --shadow: 0 10px 24px rgba(19, 34, 56, 0.08);
    }

    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      font-family: Arial, sans-serif;
      background: var(--bg);
      color: var(--text);
    }

    .page {
      max-width: 1180px;
      margin: 0 auto;
      padding: 28px 16px 40px;
    }

    h1 {
      margin: 0 0 24px;
      font-size: 34px;
    }

    h2 {
      margin: 0 0 16px;
      font-size: 24px;
    }

    .section {
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 18px;
      padding: 20px;
      box-shadow: var(--shadow);
      margin-bottom: 18px;
    }

    .cards {
      display: grid;
      grid-template-columns: repeat(6, minmax(0, 1fr));
      gap: 14px;
      margin-bottom: 14px;
    }

    .card {
      border: 1px solid var(--line);
      border-radius: 16px;
      padding: 16px;
      background: #fbfdff;
    }

    .label {
      font-size: 13px;
      color: var(--muted);
      margin-bottom: 8px;
      text-transform: uppercase;
      letter-spacing: 0.04em;
    }

    .value {
      font-size: 32px;
      font-weight: 700;
    }

    .unit {
      font-size: 14px;
      color: var(--muted);
      margin-left: 6px;
    }

    .latest-time {
      margin-top: 8px;
      color: var(--muted);
      font-size: 14px;
    }

    .subnote {
      color: var(--muted);
      font-size: 14px;
      margin-bottom: 12px;
    }

    table {
      width: 100%;
      border-collapse: collapse;
      font-size: 14px;
    }

    th, td {
      padding: 10px 8px;
      border-bottom: 1px solid var(--line);
      text-align: left;
      white-space: nowrap;
    }

    th {
      color: var(--muted);
      font-size: 12px;
      text-transform: uppercase;
      letter-spacing: 0.04em;
    }

    .table-wrap {
      overflow-x: auto;
    }

    .empty {
      color: var(--muted);
      font-size: 14px;
    }

    @media (max-width: 1100px) {
      .cards {
        grid-template-columns: repeat(3, minmax(0, 1fr));
      }
    }

    @media (max-width: 700px) {
      .cards {
        grid-template-columns: repeat(2, minmax(0, 1fr));
      }
    }

    @media (max-width: 560px) {
      .cards {
        grid-template-columns: 1fr;
      }

      h1 {
        font-size: 28px;
      }
    }
  </style>
</head>
<body>
  <div class="page">
    <h1>🍃 AERIQ Air Quality</h1>

    <section class="section">
      <h2>Current Readings</h2>
      <div class="cards">
        <div class="card">
          <div class="label">CO₂</div>
          <div><span class="value" id="co2Value">--</span><span class="unit">ppm</span></div>
        </div>
        <div class="card">
          <div class="label">Temperature</div>
          <div><span class="value" id="temperatureValue">--</span><span class="unit">°C</span></div>
        </div>
        <div class="card">
          <div class="label">Humidity</div>
          <div><span class="value" id="humidityValue">--</span><span class="unit">%</span></div>
        </div>
        <div class="card">
          <div class="label">Pressure</div>
          <div><span class="value" id="pressureValue">--</span><span class="unit">hPa</span></div>
        </div>
        <div class="card">
          <div class="label">BME Temperature</div>
          <div><span class="value" id="bmeTemperatureValue">--</span><span class="unit">°C</span></div>
        </div>
        <div class="card">
          <div class="label">BME Humidity</div>
          <div><span class="value" id="bmeHumidityValue">--</span><span class="unit">%</span></div>
        </div>
      </div>
      <div class="latest-time">Measured at: <strong id="latestTime">Loading…</strong></div>
    </section>

    <section class="section">
      <h2>Last Hour</h2>
      <div class="subnote" id="hourSummary">Loading…</div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Temp</th>
              <th>Humidity</th>
              <th>CO₂</th>
              <th>Pressure</th>
              <th>BME Temp</th>
              <th>BME Humidity</th>
            </tr>
          </thead>
          <tbody id="hourTable"></tbody>
        </table>
      </div>
    </section>

    <section class="section">
      <h2>Last 24 Hours</h2>
      <div class="subnote" id="daySummary">Loading…</div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Temp</th>
              <th>Humidity</th>
              <th>CO₂</th>
              <th>Pressure</th>
              <th>BME Temp</th>
              <th>BME Humidity</th>
            </tr>
          </thead>
          <tbody id="dayTable"></tbody>
        </table>
      </div>
    </section>
  </div>

  <script>
    const co2Value = document.getElementById("co2Value");
    const temperatureValue = document.getElementById("temperatureValue");
    const humidityValue = document.getElementById("humidityValue");
    const pressureValue = document.getElementById("pressureValue");
    const bmeTemperatureValue = document.getElementById("bmeTemperatureValue");
    const bmeHumidityValue = document.getElementById("bmeHumidityValue");
    const latestTime = document.getElementById("latestTime");
    const hourSummary = document.getElementById("hourSummary");
    const daySummary = document.getElementById("daySummary");
    const hourTable = document.getElementById("hourTable");
    const dayTable = document.getElementById("dayTable");

    function formatMaybeNumber(value, digits = 1) {
      const number = Number(value);
      return Number.isFinite(number) ? number.toFixed(digits) : "--";
    }

    function formatTime(value) {
      if (!value) return "--";
      return String(value).replace("T", " ");
    }

    function average(rows, key, digits = 1) {
      const values = rows.map(row => Number(row[key])).filter(Number.isFinite);
      if (values.length === 0) return "--";
      return (values.reduce((sum, value) => sum + value, 0) / values.length).toFixed(digits);
    }

    function renderRows(rows, tbody, limit = 10) {
      tbody.innerHTML = "";
      const recentRows = rows.slice(-limit).reverse();

      if (recentRows.length === 0) {
        tbody.innerHTML = `<tr><td colspan="6" class="empty">No data yet.</td></tr>`;
        return;
      }

      for (const row of recentRows) {
        const tr = document.createElement("tr");
        tr.innerHTML = `
          <td>${formatMaybeNumber(row.temperature)}</td>
          <td>${formatMaybeNumber(row.humidity)}</td>
          <td>${formatMaybeNumber(row.co2, 0)}</td>
          <td>${formatMaybeNumber(row.pressure)}</td>
          <td>${formatMaybeNumber(row.bme_temperature)}</td>
          <td>${formatMaybeNumber(row.bme_humidity)}</td>
        `;
        tbody.appendChild(tr);
      }
    }

    async function fetchJson(url) {
      const response = await fetch(url);
      if (!response.ok) {
        throw new Error(`${url} returned HTTP ${response.status}`);
      }
      return response.json();
    }

    async function loadDashboard() {
      try {
        const [latest, hourRows, dayRows] = await Promise.all([
          fetchJson("/readings"),
          fetchJson("/api/history?period=hour"),
          fetchJson("/api/history?period=day"),
        ]);

        co2Value.textContent = formatMaybeNumber(latest.co2, 0);
        temperatureValue.textContent = formatMaybeNumber(latest.temperature);
        humidityValue.textContent = formatMaybeNumber(latest.humidity);
        pressureValue.textContent = formatMaybeNumber(latest.pressure);
        bmeTemperatureValue.textContent = formatMaybeNumber(latest.bme_temperature);
        bmeHumidityValue.textContent = formatMaybeNumber(latest.bme_humidity);
        latestTime.textContent = formatTime(latest.received_at);

        hourSummary.textContent =
          `${hourRows.length} rows · avg CO₂ ${average(hourRows, "co2", 0)} ppm · avg BME temp ${average(hourRows, "bme_temperature")} °C`;
        daySummary.textContent =
          `${dayRows.length} rows · avg CO₂ ${average(dayRows, "co2", 0)} ppm · avg BME temp ${average(dayRows, "bme_temperature")} °C`;

        renderRows(hourRows, hourTable, 10);
        renderRows(dayRows, dayTable, 12);
      } catch (error) {
        latestTime.textContent = "Failed";
        hourSummary.textContent = error.message;
        daySummary.textContent = error.message;
        hourTable.innerHTML = `<tr><td colspan="6" class="empty">${error.message}</td></tr>`;
        dayTable.innerHTML = `<tr><td colspan="6" class="empty">${error.message}</td></tr>`;
      }
    }

    loadDashboard();
    setInterval(loadDashboard, 10000);
  </script>
</body>
</html>
"""


@app.get("/web", response_class=HTMLResponse)
def web_page() -> HTMLResponse:
    return HTMLResponse(HTML_PAGE)


@app.get("/favicon.ico", include_in_schema=False)
def favicon() -> Response:
    return Response(status_code=204)
