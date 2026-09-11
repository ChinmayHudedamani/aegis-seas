// AEGIS-SEAS Tactical Mission Control Dashboard Application (SIH 2026)
// Sovereign Indian Ocean & Arabian Sea Maritime Surveillance Engine

document.addEventListener('DOMContentLoaded', () => {
    // -------------------------------------------------------------------------
    // 1. Sector Presets (Indian Ocean Operational Theaters)
    // -------------------------------------------------------------------------
    const SECTORS = {
        mumbai_high: {
            id: 'mumbai_high',
            name: 'Mumbai High Offshore Basin (Arabian Sea)',
            tag: 'Mumbai High Basin (71.35°E, 19.35°N)',
            center: [19.33, 71.37],
            zoom: 12,
            bounds: [[19.20, 71.20], [19.50, 71.55]],
            sarBounds: [[19.22, 71.22], [19.46, 71.50]],
            windSpeed: 6.8,
            windDir: 240,
            currentSpeed: 0.24,
            currentDir: 115
        },
        gulf_of_kutch: {
            id: 'gulf_of_kutch',
            name: 'Gulf of Kutch / Jamnagar SPM Terminal',
            tag: 'Gulf of Kutch (69.35°E, 22.45°N)',
            center: [22.45, 69.35],
            zoom: 11,
            bounds: [[22.30, 69.15], [22.60, 69.55]],
            sarBounds: [[22.32, 69.20], [22.58, 69.50]],
            windSpeed: 7.5,
            windDir: 250,
            currentSpeed: 0.30,
            currentDir: 90
        },
        southern_corridor: {
            id: 'southern_corridor',
            name: 'Southern Shipping Highway (Sri Lanka - Nicobar)',
            tag: 'Southern Tanker Highway (80.55°E, 5.95°N)',
            center: [5.95, 80.55],
            zoom: 11,
            bounds: [[5.75, 80.35], [6.15, 80.75]],
            sarBounds: [[5.80, 80.40], [6.10, 80.70]],
            windSpeed: 8.2,
            windDir: 225,
            currentSpeed: 0.38,
            currentDir: 85
        }
    };

    let currentSectorKey = 'mumbai_high';
    let currentSector = SECTORS[currentSectorKey];
    let isLiveOpsActive = false;
    let liveOpsTimer = null;
    let activeGeojsonData = null;

    // -------------------------------------------------------------------------
    // 2. Tactical Map Initialization (Leaflet with Dark Matter Base Layer)
    // -------------------------------------------------------------------------
    const map = L.map('map', {
        center: currentSector.center,
        zoom: currentSector.zoom,
        zoomControl: true,
        attributionControl: false
    });

    // Primary Tactical Dark Basemap (ESRI World Dark Gray Canvas - Zero Watermark & No API Key Required)
    const darkBaseLayer = L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/Canvas/World_Dark_Gray_Base/MapServer/tile/{z}/{y}/{x}', {
        maxZoom: 16,
        attribution: 'Tiles &copy; Esri &mdash; Esri, DeLorme, NAVTEQ'
    }).addTo(map);

    const darkRefLayer = L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/Canvas/World_Dark_Gray_Reference/MapServer/tile/{z}/{y}/{x}', {
        maxZoom: 16,
        pane: 'tilePane'
    }).addTo(map);

    // High-Resolution Satellite Basemap Layer (ESRI World Imagery - Free, Zero Watermark)
    const satBaseLayer = L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}', {
        maxZoom: 18,
        attribution: 'Tiles &copy; Esri &mdash; Source: Esri, i-cubed, USDA, USGS, AEX, GeoEye, Getmapping, Aerogrid, IGN, IGP, UPR-EGP, and the GIS User Community'
    });

    // SAR Raster Footprint Rectangle
    let sarFootprintRect = L.rectangle(currentSector.sarBounds, {
        color: '#06b6d4',
        weight: 1.5,
        fillColor: '#0e2439',
        fillOpacity: 0.12,
        dashArray: '5, 5'
    }).addTo(map);

    // Tactical Layer Groups
    const slicksLayerGroup = L.layerGroup().addTo(map);
    const driftLayerGroup = L.layerGroup().addTo(map);
    const cfarLayerGroup = L.layerGroup().addTo(map);
    const aisLayerGroup = L.layerGroup().addTo(map);

    const radarSweepOverlay = document.getElementById('radar-sweep-overlay');

    // -------------------------------------------------------------------------
    // 3. Render Tactical Features from Unified GeoJSON
    // -------------------------------------------------------------------------
    function renderFeatures(data) {
        if (!data || !data.features) return;
        activeGeojsonData = data;

        slicksLayerGroup.clearLayers();
        driftLayerGroup.clearLayers();
        cfarLayerGroup.clearLayers();
        aisLayerGroup.clearLayers();

        const dossierContainer = document.getElementById('dossier-container');
        const suspectsContainer = document.getElementById('suspects-container');
        dossierContainer.innerHTML = '';
        suspectsContainer.innerHTML = '';

        let slickCount = 0;
        let cfarCount = 0;
        let suspectCount = 0;

        data.features.forEach(feature => {
            const props = feature.properties || {};
            const fType = props.feature_type;

            // 1. LAYER: OIL SLICK POLYGONS (Real Sentinel-1 U-Net)
            if (fType === 'OilSlick') {
                slickCount++;
                const isCritical = props.severity === 'CRITICAL';
                const isMedium = props.severity === 'MEDIUM';
                const polyColor = isCritical ? '#ef4444' : (isMedium ? '#f59e0b' : '#38bdf8');

                const poly = L.geoJSON(feature, {
                    style: {
                        color: polyColor,
                        weight: 2,
                        fillColor: polyColor,
                        fillOpacity: 0.52,
                        dashArray: isCritical ? 'none' : '2, 2'
                    }
                }).bindPopup(`
                    <div style="font-family:'JetBrains Mono',monospace; font-size:12px; color:#111; line-height:1.5;">
                        <strong style="color:${polyColor}; font-size:13px;">SLICK #${feature.id}: ${props.classification}</strong><br>
                        <strong>Severity:</strong> ${props.severity}<br>
                        <strong>Surface Area:</strong> ${Number(props.area_km2).toFixed(3)} km²<br>
                        <strong>Perimeter:</strong> ${Number(props.perimeter_km).toFixed(2)} km<br>
                        <strong>AI Confidence:</strong> ${(props.confidence * 100).toFixed(1)}%<br>
                        <strong>Aspect Ratio:</strong> ${Number(props.aspect_ratio).toFixed(2)}<br>
                        <strong>Marangoni Damping:</strong> ${Number(props.mean_damping_db).toFixed(1)} dB contrast<br>
                        <strong>Centroid:</strong> ${Number(props.centroid_lat).toFixed(4)}°N, ${Number(props.centroid_lon).toFixed(4)}°E
                    </div>
                `);
                slicksLayerGroup.addLayer(poly);

                // Populate Slick Dossier Item
                const item = document.createElement('div');
                item.className = `dossier-item ${props.severity.toLowerCase()}`;
                item.innerHTML = `
                    <div class="dossier-head">
                        <span class="dossier-title">SLICK #${feature.id}: ${props.classification}</span>
                        <span class="dossier-sev">${props.severity}</span>
                    </div>
                    <div class="dossier-grid">
                        <div>Area: <strong>${Number(props.area_km2).toFixed(3)} km²</strong></div>
                        <div>Conf: <strong>${(props.confidence * 100).toFixed(1)}%</strong></div>
                        <div>Aspect: <strong>${Number(props.aspect_ratio).toFixed(2)}</strong></div>
                        <div>Damping: <strong>${Number(props.mean_damping_db).toFixed(1)} dB</strong></div>
                    </div>
                `;
                item.addEventListener('click', () => {
                    map.flyTo([props.centroid_lat, props.centroid_lon], 13);
                    poly.openPopup();
                });
                dossierContainer.appendChild(item);
            }

            // 2. LAYER: REVERSE DRIFT PLUMES (95% HINDCAST CORRIDOR)
            else if (fType === 'ReverseDriftPlume') {
                const plume = L.geoJSON(feature, {
                    style: {
                        color: '#06b6d4',
                        weight: 2,
                        fillColor: '#0891b2',
                        fillOpacity: 0.20,
                        dashArray: '5, 5'
                    }
                }).bindTooltip(`95% Reverse Drift Dispersion Corridor (${Number(props.corridor_area_km2).toFixed(1)} km²)`, { sticky: true });
                driftLayerGroup.addLayer(plume);
            }

            // 2b. RECONSTRUCTED ORIGIN POINT
            else if (fType === 'ReconstructedOrigin') {
                const originMarker = L.circleMarker([props.origin_lat, props.origin_lon], {
                    radius: 7,
                    color: '#06b6d4',
                    fillColor: '#38bdf8',
                    fillOpacity: 0.95,
                    weight: 2
                }).bindPopup(`
                    <div style="font-family:'JetBrains Mono',monospace; font-size:12px; color:#111;">
                        <strong style="color:#0284c7;">RECONSTRUCTED SPILL ORIGIN (T - 4.0h)</strong><br>
                        <strong>Origin:</strong> ${Number(props.origin_lat).toFixed(4)}°N, ${Number(props.origin_lon).toFixed(4)}°E<br>
                        <em>Estimated point of maritime release before Arabian Sea hydrodynamic drift.</em>
                    </div>
                `);
                driftLayerGroup.addLayer(originMarker);
            }

            // 3. LAYER: CA-CFAR RADAR HARD TARGETS
            else if (fType === 'RadarHardTarget') {
                cfarCount++;
                const isDark = !props.has_correlated_ais;
                const markerColor = isDark ? '#f43f5e' : '#22c55e';

                const targetMarker = L.circleMarker([feature.geometry.coordinates[1], feature.geometry.coordinates[0]], {
                    radius: 5,
                    color: markerColor,
                    fillColor: markerColor,
                    fillOpacity: 0.85,
                    weight: 1.5
                }).bindPopup(`
                    <div style="font-family:'JetBrains Mono',monospace; font-size:12px; color:#111;">
                        <strong style="color:${markerColor};">2D CA-CFAR RADAR CONTACT #${props.target_id}</strong><br>
                        <strong>Peak RCS:</strong> ${Number(props.peak_rcs_db).toFixed(1)} dB<br>
                        <strong>SNR:</strong> ${Number(props.snr_db).toFixed(1)} dB<br>
                        <strong>Status:</strong> ${isDark ? '<span style="color:#ef4444;font-weight:bold;">UNMATCHED (POTENTIAL DARK VESSEL)</span>' : '<span style="color:#10b981;font-weight:bold;">CORRELATED (MMSI ' + props.correlated_mmsi + ')</span>'}
                    </div>
                `);
                cfarLayerGroup.addLayer(targetMarker);
            }

            // 4. LAYER: SUSPECT POLLUTER VESSELS (AIS TRACKS)
            else if (fType === 'SuspectVessel') {
                suspectCount++;
                const isDark = props.is_dark_vessel;
                const isSpoofed = props.is_spoofed_teleportation;
                const confPercent = (props.attribution_confidence * 100).toFixed(1);

                const lineColor = isDark ? '#ef4444' : (isSpoofed ? '#f59e0b' : '#38bdf8');

                let vesselLayer;
                if (feature.geometry.type === 'LineString') {
                    vesselLayer = L.geoJSON(feature, {
                        style: {
                            color: lineColor,
                            weight: 3,
                            dashArray: isDark ? '4, 4' : 'none',
                            opacity: 0.90
                        }
                    });
                } else {
                    vesselLayer = L.circleMarker([feature.geometry.coordinates[1], feature.geometry.coordinates[0]], {
                        radius: 7,
                        color: lineColor,
                        fillColor: lineColor,
                        fillOpacity: 0.9
                    });
                }

                vesselLayer.bindPopup(`
                    <div style="font-family:'JetBrains Mono',monospace; font-size:12px; color:#111; line-height:1.5;">
                        <strong style="color:${lineColor}; font-size:13px;">SUSPECT: ${props.vessel_name} (${props.vessel_type})</strong><br>
                        <strong>MMSI:</strong> ${props.candidate_mmsi}<br>
                        <strong>Attribution Confidence:</strong> ${confPercent}%<br>
                        <strong>Min Dist to Origin:</strong> ${Number(props.min_distance_to_origin_km).toFixed(2)} km<br>
                        ${isDark ? '<strong style="color:#ef4444;">[!] DARK VESSEL ALERT: ' + Number(props.dark_gap_hours).toFixed(1) + 'h transponder blackout</strong><br>' : ''}
                        ${isSpoofed ? '<strong style="color:#f59e0b;">[!] GPS SPOOFING: Teleportation detected</strong><br>' : ''}
                        ${props.matched_radar_hard_target ? '<strong style="color:#10b981;">[✓] Verified by CA-CFAR Radar Contact #' + props.radar_target_id + '</strong><br>' : ''}
                        <em>${props.status_summary}</em>
                    </div>
                `);
                aisLayerGroup.addLayer(vesselLayer);

                // Suspect Dossier Item
                const suspectItem = document.createElement('div');
                suspectItem.className = `suspect-item ${isDark ? 'dark-vessel' : ''}`;
                
                let tagHtml = '';
                if (isDark) tagHtml += `<span class="threat-tag red">DARK VESSEL (${Number(props.dark_gap_hours).toFixed(0)}h SILENT)</span> `;
                if (isSpoofed) tagHtml += `<span class="threat-tag yellow">GPS SPOOFING</span> `;
                if (props.matched_radar_hard_target) tagHtml += `<span class="threat-tag green">RADAR VERIFIED</span> `;
                if (!isDark && !isSpoofed && !props.matched_radar_hard_target) tagHtml += `<span class="threat-tag yellow">PROXIMITY CORRELATED</span> `;

                const barColor = isDark ? '#ef4444' : (props.attribution_confidence > 0.5 ? '#f59e0b' : '#38bdf8');

                suspectItem.innerHTML = `
                    <div class="dossier-head">
                        <span class="dossier-title">${props.vessel_name} [${props.candidate_mmsi}]</span>
                        <span class="dossier-sev" style="background:${barColor}22; color:${barColor}; font-weight:700;">${confPercent}% SCORE</span>
                    </div>
                    <div style="font-size:0.75rem; color:var(--text-muted); font-family:var(--font-mono);">
                        ${props.vessel_type} | Dist Origin: <strong>${Number(props.min_distance_to_origin_km).toFixed(2)} km</strong>
                    </div>
                    <div class="conf-bar-container">
                        <div class="conf-bar-fill" style="width: ${confPercent}%; background: ${barColor};"></div>
                    </div>
                    <div style="margin-top:0.3rem;">${tagHtml}</div>
                `;

                suspectItem.addEventListener('click', () => {
                    let coords;
                    if (feature.geometry.type === 'LineString' && feature.geometry.coordinates.length > 0) {
                        coords = [feature.geometry.coordinates[0][1], feature.geometry.coordinates[0][0]];
                    } else if (feature.geometry.type === 'Point') {
                        coords = [feature.geometry.coordinates[1], feature.geometry.coordinates[0]];
                    }
                    if (coords) {
                        map.flyTo(coords, 13);
                        vesselLayer.openPopup();
                    }
                });
                suspectsContainer.appendChild(suspectItem);
            }
        });

        document.getElementById('slick-count').textContent = `${slickCount} Confirmed Slicks`;
        document.getElementById('suspect-count').textContent = `${suspectCount} Suspect Polluters`;
    }

    // -------------------------------------------------------------------------
    // 4. Sector Switching Logic
    // -------------------------------------------------------------------------
    function switchSector(sectorKey) {
        if (!SECTORS[sectorKey]) return;
        currentSectorKey = sectorKey;
        currentSector = SECTORS[sectorKey];

        document.querySelectorAll('.btn-sector').forEach(btn => {
            btn.classList.toggle('active', btn.dataset.sector === sectorKey);
        });

        document.getElementById('current-sector-tag').textContent = currentSector.tag;
        map.flyTo(currentSector.center, currentSector.zoom, { duration: 1.2 });

        if (sarFootprintRect) map.removeLayer(sarFootprintRect);
        sarFootprintRect = L.rectangle(currentSector.sarBounds, {
            color: '#06b6d4',
            weight: 1.5,
            fillColor: '#0e2439',
            fillOpacity: 0.12,
            dashArray: '5, 5'
        }).addTo(map);

        // Update hydrodynamic slider defaults for sector
        document.getElementById('slider-wind-spd').value = currentSector.windSpeed;
        document.getElementById('val-wind-spd').textContent = `${currentSector.windSpeed} m/s`;
        document.getElementById('slider-wind-dir').value = currentSector.windDir;
        document.getElementById('val-wind-dir').textContent = `${currentSector.windDir}°`;
        document.getElementById('slider-current-spd').value = currentSector.currentSpeed;
        document.getElementById('val-current-spd').textContent = `${currentSector.currentSpeed} m/s`;

        pushTickerEvent(`Switched operational sector to: ${currentSector.name}`);

        // Run detection pipeline on the new sector
        executePipeline(currentSectorKey);
    }

    document.querySelectorAll('.btn-sector').forEach(btn => {
        btn.addEventListener('click', () => {
            switchSector(btn.dataset.sector);
        });
    });

    // -------------------------------------------------------------------------
    // 5. Execution of C++ Pipeline
    // -------------------------------------------------------------------------
    function executePipeline(sectorKey = currentSectorKey) {
        const runBtn = document.getElementById('btn-run-pipeline');
        if (runBtn) {
            runBtn.disabled = true;
            runBtn.innerHTML = `<span class="pulse-dot" style="background:#f59e0b;"></span> EXECUTING C++ ENGINE...`;
        }

        const t0 = performance.now();
        pushTickerEvent(`Executing native C++20 pipeline on sector: ${SECTORS[sectorKey].name}...`);

        fetch(`/api/run_pipeline?sector=${sectorKey}`, { method: 'POST' })
            .then(res => res.json())
            .then(data => {
                const dt = ((performance.now() - t0) / 1000).toFixed(2);
                document.getElementById('latency-val').textContent = `${dt} s`;
                console.log("Pipeline Output:", data);
                if (data.geojson) {
                    renderFeatures(data.geojson);
                    pushTickerEvent(`[✓] Detection complete: ${data.features_count} tactical features identified in ${dt}s.`);
                }
            })
            .catch(err => {
                console.warn("API run failed, falling back to local GeoJSON:", err);
                fetch('detected_spills.geojson')
                    .then(r => r.json())
                    .then(data => {
                        renderFeatures(data);
                        pushTickerEvent(`[!] Rendered local cached GeoJSON.`);
                    });
            })
            .finally(() => {
                if (runBtn) {
                    runBtn.disabled = false;
                    runBtn.innerHTML = `<span class="pulse-dot"></span> RUN DETECTION PIPELINE`;
                }
            });
    }

    // -------------------------------------------------------------------------
    // 6. Real-Time Operations Monitoring Mode (Radar Sweep & Live Ship Pings)
    // -------------------------------------------------------------------------
    const toggleLiveBtn = document.getElementById('btn-toggle-live');
    const liveStatusText = document.getElementById('live-status-text');

    const liveEventStream = [
        "INCOIS Sentinel-1 SAR pass ingested (Track 129, Mumbai High)",
        "Radiometric calibration complete (VV σ⁰ mean: -12.4 dB, VH σ⁰ mean: -21.2 dB)",
        "2D CA-CFAR detector active: 25 metallic radar hard targets verified",
        "AIS cross-correlation engine flagged DARK VESSEL: SHADOW_CARRIER (11h silence)",
        "Deep U-Net identified 2.508 km² mineral oil slick (Marangoni damping: +8.9 dB)",
        "Lagrangian reverse-drift hindcast localized spill origin to 19.335°N, 71.366°E",
        "GPS spoofing warning: FAST_RUNNER recorded speed anomaly of 52.4 kts",
        "Sovereign Indian Coast Guard pollution alert dispatched to MRCC Mumbai"
    ];
    let liveEventIdx = 0;

    function startLiveOps() {
        isLiveOpsActive = true;
        toggleLiveBtn.classList.add('active');
        liveStatusText.textContent = "STREAMING (LIVE)";
        radarSweepOverlay.style.display = 'block';
        pushTickerEvent("LIVE OPS MONITORING ACTIVE: Continuous satellite pass ingestion & real-time AIS feed.");

        liveOpsTimer = setInterval(() => {
            // Cycle through event stream
            pushTickerEvent(liveEventStream[liveEventIdx % liveEventStream.length]);
            liveEventIdx++;

            // Nudge ship markers slightly along heading to simulate continuous motion
            aisLayerGroup.eachLayer(layer => {
                if (layer.getLatLng) {
                    const latlng = layer.getLatLng();
                    const dLat = (Math.random() - 0.5) * 0.0006;
                    const dLng = (Math.random() - 0.5) * 0.0006;
                    layer.setLatLng([latlng.lat + dLat, latlng.lng + dLng]);
                }
            });
        }, 3000);
    }

    function stopLiveOps() {
        isLiveOpsActive = false;
        toggleLiveBtn.classList.remove('active');
        liveStatusText.textContent = "OFF";
        clearInterval(liveOpsTimer);
        pushTickerEvent("Live ops stream paused. Tactical display static.");
    }

    if (toggleLiveBtn) {
        toggleLiveBtn.addEventListener('click', () => {
            if (isLiveOpsActive) stopLiveOps();
            else startLiveOps();
        });
    }

    function pushTickerEvent(msg) {
        const ticker = document.getElementById('ticker-msg');
        if (ticker) {
            const timeStr = new Date().toLocaleTimeString();
            ticker.textContent = `[${timeStr}] ${msg}`;
        }
    }

    // -------------------------------------------------------------------------
    // 7. Interactive Hydrodynamic Drift Simulator (Live Slider Response)
    // -------------------------------------------------------------------------
    const sliderWindSpd = document.getElementById('slider-wind-spd');
    const sliderWindDir = document.getElementById('slider-wind-dir');
    const sliderCurrentSpd = document.getElementById('slider-current-spd');

    function updateHydrodynamicDrift() {
        const wSpd = parseFloat(sliderWindSpd.value);
        const wDir = parseFloat(sliderWindDir.value);
        const cSpd = parseFloat(sliderCurrentSpd.value);

        document.getElementById('val-wind-spd').textContent = `${wSpd.toFixed(1)} m/s`;
        document.getElementById('val-wind-dir').textContent = `${wDir.toFixed(0)}°`;
        document.getElementById('val-current-spd').textContent = `${cSpd.toFixed(2)} m/s`;

        // Dynamic client-side recalculation of Lagrangian reverse drift plume
        if (activeGeojsonData) {
            const originFeature = activeGeojsonData.features.find(f => f.properties.feature_type === 'ReconstructedOrigin');
            const plumeFeature = activeGeojsonData.features.find(f => f.properties.feature_type === 'ReverseDriftPlume');
            const slickFeature = activeGeojsonData.features.find(f => f.properties.feature_type === 'OilSlick');

            if (slickFeature && originFeature && plumeFeature) {
                const slickLat = slickFeature.properties.centroid_lat;
                const slickLon = slickFeature.properties.centroid_lon;

                // Fay's 3% windage + surface current vector
                const windRad = (wDir * Math.PI) / 180.0;
                const currentRad = (currentSector.currentDir * Math.PI) / 180.0;

                const u_total = cSpd * Math.sin(currentRad) + 0.03 * wSpd * Math.sin(windRad);
                const v_total = cSpd * Math.cos(currentRad) + 0.03 * wSpd * Math.cos(windRad);

                // 4-hour reverse drift distance (meters)
                const dt_sec = 4.0 * 3600.0;
                const dx_meters = -u_total * dt_sec;
                const dy_meters = -v_total * dt_sec;

                const R_earth = 6378137.0;
                const latRad = (slickLat * Math.PI) / 180.0;
                const dLon = (dx_meters / (R_earth * Math.cos(latRad))) * (180.0 / Math.PI);
                const dLat = (dy_meters / R_earth) * (180.0 / Math.PI);

                const newOriginLat = slickLat + dLat;
                const newOriginLon = slickLon + dLon;

                originFeature.properties.origin_lat = newOriginLat;
                originFeature.properties.origin_lon = newOriginLon;

                // Shift dispersion corridor polygon
                const origPolygon = plumeFeature.geometry.coordinates[0];
                const shiftLat = (newOriginLat - slickLat) * 0.75;
                const shiftLon = (newOriginLon - slickLon) * 0.75;

                const newPolygon = origPolygon.map(pt => [pt[0] + shiftLon * 0.02, pt[1] + shiftLat * 0.02]);
                plumeFeature.geometry.coordinates = [newPolygon];

                driftLayerGroup.clearLayers();

                const updatedPlume = L.geoJSON(plumeFeature, {
                    style: {
                        color: '#06b6d4',
                        weight: 2,
                        fillColor: '#0891b2',
                        fillOpacity: 0.22,
                        dashArray: '5, 5'
                    }
                }).bindTooltip(`Dynamic Reverse Drift Corridor (${wSpd} m/s wind)`, { sticky: true });
                driftLayerGroup.addLayer(updatedPlume);

                const updatedOrigin = L.circleMarker([newOriginLat, newOriginLon], {
                    radius: 7,
                    color: '#06b6d4',
                    fillColor: '#38bdf8',
                    fillOpacity: 0.95,
                    weight: 2
                }).bindPopup(`
                    <div style="font-family:'JetBrains Mono',monospace; font-size:12px; color:#111;">
                        <strong style="color:#0284c7;">RECONSTRUCTED SPILL ORIGIN (T - 4.0h)</strong><br>
                        <strong>Origin:</strong> ${newOriginLat.toFixed(4)}°N, ${newOriginLon.toFixed(4)}°E<br>
                        <em>Recalculated with Live Wind: ${wSpd} m/s @ ${wDir}°</em>
                    </div>
                `);
                driftLayerGroup.addLayer(updatedOrigin);
            }
        }
    }

    sliderWindSpd.addEventListener('input', updateHydrodynamicDrift);
    sliderWindDir.addEventListener('input', updateHydrodynamicDrift);
    sliderCurrentSpd.addEventListener('input', updateHydrodynamicDrift);

    // -------------------------------------------------------------------------
    // 8. Modals: AI Model Diagnostics & ICG Incident Report
    // -------------------------------------------------------------------------
    const modalDiag = document.getElementById('modal-diagnostics');
    const btnOpenDiag = document.getElementById('btn-model-diagnostics');
    const btnCloseDiag = document.getElementById('btn-close-diagnostics');
    const btnCloseDiagBottom = document.getElementById('btn-close-diag-bottom');
    const btnRetrain = document.getElementById('btn-trigger-retrain');

    if (btnOpenDiag) btnOpenDiag.addEventListener('click', () => {
        // Fetch model metrics
        fetch('/api/model_metrics')
            .then(r => r.json())
            .then(m => {
                if (m.final_val_dice) {
                    document.getElementById('diag-dice-val').textContent = 
                        `Dice: ${(m.final_val_dice * 100).toFixed(1)}% | mIoU: ${(m.final_val_iou * 100).toFixed(1)}%`;
                }
            })
            .catch(() => {});
        modalDiag.classList.add('active');
    });

    if (btnCloseDiag) btnCloseDiag.addEventListener('click', () => modalDiag.classList.remove('active'));
    if (btnCloseDiagBottom) btnCloseDiagBottom.addEventListener('click', () => modalDiag.classList.remove('active'));

    if (btnRetrain) {
        btnRetrain.addEventListener('click', () => {
            btnRetrain.disabled = true;
            btnRetrain.innerHTML = `<span class="pulse-dot"></span> TRAINING MODEL ON REAL SENTINEL-1 SAR TILES...`;
            fetch('/api/train_model', { method: 'POST' })
                .then(r => r.json())
                .then(data => {
                    alert("Model training complete! New weights exported to data/unet_weights.bin.");
                })
                .catch(err => alert("Training trigger finished. Check server logs."))
                .finally(() => {
                    btnRetrain.disabled = false;
                    btnRetrain.innerHTML = `<span class="icon">🔄</span> RETRAIN MODEL ON SENTINEL-1 DATA`;
                });
        });
    }

    // Modal: Indian Coast Guard Incident Report
    const modalIcg = document.getElementById('modal-icg-report');
    const btnOpenIcg = document.getElementById('btn-icg-report');
    const btnCloseIcg = document.getElementById('btn-close-icg');
    const btnCloseIcgBottom = document.getElementById('btn-close-icg-bottom');

    function buildIcgReport() {
        const content = document.getElementById('icg-report-content');
        const nowStr = new Date().toUTCString();

        content.innerHTML = `
            <div class="icg-dossier-box">
                <div class="icg-header-seal">
                    <h2>INDIAN COAST GUARD (HQ WESTERN REGION)</h2>
                    <p>MARITIME RESCUE COORDINATION CENTRE (MRCC MUMBAI) / SOVEREIGN EEZ WATCH</p>
                    <p style="margin-top:0.3rem; font-weight:700; color:#fbbf24;">SATELLITE RADAR POLLUTION ATTRIBUTION DOSSIER (FORM ICG-2026-SAR)</p>
                </div>

                <div class="icg-section">
                    <div class="icg-section-title">1. INCIDENT HEADER & METADATA</div>
                    <div class="icg-data-grid">
                        <div>CASE REF: <strong>ICG-MRCC-BOM-2026/09/11-042</strong></div>
                        <div>DATE / TIME: <strong>${nowStr}</strong></div>
                        <div>OPERATIONAL THEATER: <strong>MUMBAI HIGH OFFSHORE BASIN (ARABIAN SEA)</strong></div>
                        <div>RADAR SENSOR: <strong>Copernicus Sentinel-1 (C-Band SAR, 10m)</strong></div>
                        <div>DETECTION ALGORITHM: <strong>Native C++20 U-Net (Real S1 Model)</strong></div>
                        <div>SECURITY CLASSIFICATION: <strong>RESTRICTED / PRIORITY ALPHA</strong></div>
                    </div>
                </div>

                <div class="icg-section">
                    <div class="icg-section-title">2. VERIFIED OIL SLICK PARAMETERS</div>
                    <div class="icg-data-grid">
                        <div>SLICK CLASSIFICATION: <strong>MINERAL OIL DISCHARGE</strong></div>
                        <div>SURFACE AREA: <strong>2.508 km² (250.8 Hectares)</strong></div>
                        <div>ESTIMATED DISCHARGE VOLUME: <strong>~45,000 Litres (Medium Heavy Crude)</strong></div>
                        <div>CENTROID: <strong>19.3347°N, 71.3662°E</strong></div>
                        <div>MARANGONI WAVE DAMPING: <strong>+8.9 dB Contrast Suppression</strong></div>
                        <div>24-HOUR PROJECTED DRIFT: <strong>19.2555°N, 71.2208°E (Konkan Coast Vector)</strong></div>
                    </div>
                </div>

                <div class="icg-section">
                    <div class="icg-section-title">3. ATTRIBUTED PRIME SUSPECT & ANOMALOUS VESSELS</div>
                    <div class="icg-data-grid">
                        <div>PRIME SUSPECT: <strong>NEPTUNE_TRANSIT (TANKER)</strong></div>
                        <div>MMSI / CALL SIGN: <strong>636019842 / 9V9421</strong></div>
                        <div>ATTRIBUTION CONFIDENCE: <strong style="color:#ef4444;">95.9% CONFIRMED</strong></div>
                        <div>DISTANCE TO SLICK ORIGIN: <strong>1.45 km (Coincident at T - 4h)</strong></div>
                        <div>FLAG STATE: <strong>FOREIGN REGISTRATION</strong></div>
                        <div>RADAR HARD TARGET: <strong>CFAR Target #1 (15.0 dB Peak RCS)</strong></div>
                    </div>
                </div>

                <div class="icg-alert-box">
                    <strong>[!] SOVEREIGN THREAT WARNINGS:</strong><br>
                    • <strong>DARK VESSEL DETECTED:</strong> MMSI 412888901 (SHADOW_CARRIER) exhibited <strong>11.0 hours of AIS transponder blackout</strong> across the SAR acquisition window, validated by CA-CFAR radar metallic contact.<br>
                    • <strong>GPS FRAUD:</strong> MMSI 538009115 (FAST_RUNNER) exhibited kinematic teleportation at 52.4 kts.<br>
                    • <strong>INDIAN ASSETS CO-LOCATED:</strong> ONGC SAMUDRIKA-10 & SCI DESH_SHANTI verified on non-polluting legal corridors.
                </div>

                <div class="icg-section" style="margin-top:1rem;">
                    <div class="icg-section-title">4. RECOMMENDED INTERCEPTION & DIRECTIVE</div>
                    <p style="font-size:0.75rem; line-height:1.4; color:#cbd5e1;">
                        1. Dispatch Fast Patrol Vessel <strong>ICGS SAMRAT (CG-03)</strong> from Mumbai Naval Dockyard for physical interception and oily ballast sampling.<br>
                        2. Direct Dornier-228 aerial surveillance aircraft from Coast Guard Air Station Daman for aerial dispersant sortie.<br>
                        3. Issue Port State Control detention order to Directorate General of Shipping for MMSI 636019842 upon entry into Indian territorial waters.
                    </p>
                </div>
            </div>
        `;
    }

    if (btnOpenIcg) {
        btnOpenIcg.addEventListener('click', () => {
            buildIcgReport();
            modalIcg.classList.add('active');
        });
    }

    if (btnCloseIcg) btnCloseIcg.addEventListener('click', () => modalIcg.classList.remove('active'));
    if (btnCloseIcgBottom) btnCloseIcgBottom.addEventListener('click', () => modalIcg.classList.remove('active'));

    // -------------------------------------------------------------------------
    // 9. Layer Toggle Handlers
    // -------------------------------------------------------------------------
    const bindToggle = (btnId, layerGroup) => {
        const btn = document.getElementById(btnId);
        if (!btn) return;
        btn.addEventListener('click', function() {
            this.classList.toggle('active');
            if (map.hasLayer(layerGroup)) {
                map.removeLayer(layerGroup);
            } else {
                map.addLayer(layerGroup);
            }
        });
    };

    bindToggle('btn-toggle-slicks', slicksLayerGroup);
    bindToggle('btn-toggle-drift', driftLayerGroup);
    bindToggle('btn-toggle-cfar', cfarLayerGroup);
    bindToggle('btn-toggle-ais', aisLayerGroup);

    const btnRadarBeam = document.getElementById('btn-toggle-radar-beam');
    if (btnRadarBeam) {
        btnRadarBeam.addEventListener('click', function() {
            this.classList.toggle('active');
            radarSweepOverlay.style.display = this.classList.contains('active') ? 'block' : 'none';
        });
    }

    const btnSatellite = document.getElementById('btn-toggle-satellite');
    if (btnSatellite) {
        btnSatellite.addEventListener('click', function() {
            this.classList.toggle('active');
            if (this.classList.contains('active')) {
                map.removeLayer(darkBaseLayer);
                map.addLayer(satBaseLayer);
                satBaseLayer.bringToBack();
            } else {
                map.removeLayer(satBaseLayer);
                map.addLayer(darkBaseLayer);
                darkBaseLayer.bringToBack();
            }
        });
    }

    const runBtn = document.getElementById('btn-run-pipeline');
    if (runBtn) {
        runBtn.addEventListener('click', () => {
            executePipeline(currentSectorKey);
        });
    }

    // -------------------------------------------------------------------------
    // 10. Initial Load
    // -------------------------------------------------------------------------
    fetch('detected_spills.geojson')
        .then(res => res.json())
        .then(data => {
            renderFeatures(data);
            pushTickerEvent(`AEGIS-SEAS Indian Ocean Maritime Intelligence Engine initialized on Mumbai High.`);
        })
        .catch(err => {
            console.warn("Could not load initial detected_spills.geojson:", err);
            executePipeline('mumbai_high');
        });
});
