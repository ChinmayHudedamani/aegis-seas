import json
from pathlib import Path

FIXTURES_DIR = Path(__file__).parent / "fixtures"

def test_dark_vessel_fixture_ingestion():
    fixture_path = FIXTURES_DIR / "dark_vessel_ais_feed.json"
    assert fixture_path.exists(), "Dark vessel fixture missing"

    with open(fixture_path, "r") as f:
        data = json.load(f)

    transmissions = data["ais_transmissions"]
    assert len(transmissions) == 2

    t1 = transmissions[0]["timestamp_epoch_sec"]
    t2 = transmissions[1]["timestamp_epoch_sec"]
    gap_hours = (t2 - t1) / 3600.0

    assert gap_hours >= 4.0, f"Expected dark gap > 4h, got {gap_hours} hours"
    assert len(data["radar_hard_targets"]) > 0, "Expected CFAR radar hard target for dark vessel cross-validation"
    print(f"  [OK] PASSED: Python test_dark_vessel_fixture_ingestion (Gap = {gap_hours:.1f}h)")

def test_mmsi_speed_spoofing_fixture():
    fixture_path = FIXTURES_DIR / "spoofed_ais_feed.json"
    assert fixture_path.exists(), "Spoofed AIS fixture missing"

    with open(fixture_path, "r") as f:
        data = json.load(f)

    msg = data["ais_transmissions"][0]
    assert msg["vessel_type"] == "CARGO"
    assert msg["speed_knots"] > 40.0, "Spoofed speed threshold check failed"
    print(f"  [OK] PASSED: Python test_mmsi_speed_spoofing_fixture (Speed = {msg['speed_knots']} kts)")

if __name__ == "__main__":
    test_dark_vessel_fixture_ingestion()
    test_mmsi_speed_spoofing_fixture()
