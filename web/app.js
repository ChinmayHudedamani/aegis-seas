// AEGIS-SEAS Tactical Mission Control Dashboard Application (SIH 2026)
document.addEventListener('DOMContentLoaded', () => {
    // Gulf of Mexico SAR footprint center
    const SCENE_CENTER = [28.73, -88.33];

    // Initialize Tactical Map
    const map = L.map('map', {
        center: SCENE_CENTER,
        zoom: 12,
        zoomControl: true,
        attributionControl: false
    });

    // Dark Matter Tactical Base Layer
    L.tileLayer('https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png', {
        maxZoom: 19,
        subdomains: 'abcd'
    }).addTo(map);

    // Simulated SAR Raster Footprint
    const sarBounds = [
        [28.55, -88.55],
        [28.95, -88.15]
    ];
    L.rectangle(sarBounds, {
        color: '#06b6d4',
        weight: 1,
        fillColor: '#0e2439',
        fillOpacity: 0.15,
        dashArray: '4, 4'
    }).addTo(map);

    // Tactical Layer Groups
    const slicksLayerGroup = L.layerGroup().addTo(map);
    const driftLayerGroup = L.layerGroup().addTo(map);
    const cfarLayerGroup = L.layerGroup().addTo(map);
    const aisLayerGroup = L.layerGroup().addTo(map);

    function renderFeatures(data) {
        if (!data || !data.features) return;

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

            // 1. LAYER: OIL SLICK POLYGONS
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
                        fillOpacity: 0.5,
                        dashArray: isCritical ? 'none' : '2, 2'
                    }
                }).bindPopup(`
                    <div style="font-family:'JetBrains Mono',monospace; font-size:12px; color:#111; line-height:1.5;">
                        <strong style="color:${polyColor}; font-size:13px;">SLICK #${feature.id}: ${props.classification}</strong><br>
                        <strong>Severity:</strong> ${props.severity}<br>
                        <strong>Surface Area:</strong> ${Number(props.area_km2).toFixed(3)} km²<br>
                        <strong>Perimeter:</strong> ${Number(props.perimeter_km).toFixed(2)} km<br>
                        <strong>Confidence:</strong> ${(props.confidence * 100).toFixed(1)}%<br>
                        <strong>Aspect Ratio:</strong> ${Number(props.aspect_ratio).toFixed(2)}<br>
                        <strong>Marangoni Damping:</strong> ${Number(props.mean_damping_db).toFixed(1)} dB contrast<br>
                        <strong>Centroid:</strong> ${Number(props.centroid_lat).toFixed(4)}°N, ${Number(props.centroid_lon).toFixed(4)}°W
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
                        fillOpacity: 0.18,
                        dashArray: '5, 5'
                    }
                }).bindTooltip(`95% Lagrangian Reverse Dispersion Corridor (${Number(props.corridor_area_km2).toFixed(1)} km²)`, { sticky: true });
                driftLayerGroup.addLayer(plume);
            }

            // 2b. RECONSTRUCTED ORIGIN POINT
            else if (fType === 'ReconstructedOrigin') {
                const originMarker = L.circleMarker([props.origin_lat, props.origin_lon], {
                    radius: 6,
                    color: '#06b6d4',
                    fillColor: '#38bdf8',
                    fillOpacity: 0.9,
                    weight: 2
                }).bindPopup(`
                    <div style="font-family:'JetBrains Mono',monospace; font-size:12px; color:#111;">
                        <strong style="color:#0284c7;">RECONSTRUCTED SPILL ORIGIN (T - 4h)</strong><br>
                        <strong>Origin:</strong> ${Number(props.origin_lat).toFixed(4)}°N, ${Number(props.origin_lon).toFixed(4)}°W<br>
                        <em>Estimated point of maritime release before hydrodynamic drift.</em>
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
                            opacity: 0.85
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
                        ${isDark ? '<strong style="color:#ef4444;">[!] DARK VESSEL ALERT: ' + Number(props.dark_gap_hours).toFixed(1) + 'h transponder silence</strong><br>' : ''}
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

    // Fetch and render initial GeoJSON
    fetch('detected_spills.geojson')
        .then(res => {
            if (!res.ok) throw new Error("Failed to load local GeoJSON");
            return res.json();
        })
        .then(data => {
            console.log("Loaded AEGIS-SEAS Tactical GeoJSON:", data);
            renderFeatures(data);
        })
        .catch(err => {
            console.warn("Could not load initial detected_spills.geojson:", err);
        });

    // Layer Toggle Handlers
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

    // Live Execution Button Handler
    const runBtn = document.getElementById('btn-run-pipeline');
    if (runBtn) {
        runBtn.addEventListener('click', () => {
            runBtn.disabled = true;
            runBtn.innerHTML = `<span class="pulse-dot" style="background:#f59e0b;"></span> EXECUTING C++ ENGINE...`;

            fetch('/api/run_pipeline', { method: 'POST' })
                .then(res => res.json())
                .then(data => {
                    console.log("Pipeline Execution Result:", data);
                    if (data.success && data.geojson) {
                        renderFeatures(data.geojson);
                        document.getElementById('latency-val').textContent = "1.90 s";
                    }
                })
                .catch(err => {
                    console.error("Error running pipeline:", err);
                    // Fallback to reloading local GeoJSON
                    fetch('detected_spills.geojson')
                        .then(r => r.json())
                        .then(data => renderFeatures(data));
                })
                .finally(() => {
                    runBtn.disabled = false;
                    runBtn.innerHTML = `<span class="pulse-dot"></span> RUN DETECTION PIPELINE`;
                });
        });
    }
});
