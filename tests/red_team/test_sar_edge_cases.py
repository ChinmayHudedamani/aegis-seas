import json
import math
import random
from pathlib import Path

def calculate_ccr_and_look_alike(vv_db, candidate_mask):
    """Python reference implementation of the physical polarimetric CCR check."""
    slick_pixels = [vv_db[r][c] for r in range(len(vv_db)) for c in range(len(vv_db[0])) if candidate_mask[r][c] == 1]
    clutter_pixels = [vv_db[r][c] for r in range(len(vv_db)) for c in range(len(vv_db[0])) if candidate_mask[r][c] == 0]

    # Sanitize NaNs
    slick_pixels = [v for v in slick_pixels if not math.isnan(v) and not math.isinf(v)]
    clutter_pixels = [v for v in clutter_pixels if not math.isnan(v) and not math.isinf(v)]

    if len(slick_pixels) == 0 or len(clutter_pixels) == 0:
        return 0.0, True, 0.0

    mean_slick = sum(slick_pixels) / len(slick_pixels)
    mean_clutter = sum(clutter_pixels) / len(clutter_pixels)
    ccr_db = mean_clutter - mean_slick

    if mean_clutter < -21.0:
        return ccr_db, True, 0.20 # Look alike
    elif ccr_db < 3.5:
        return ccr_db, True, 0.35 # Low contrast anomaly
    elif ccr_db >= 6.0:
        prob = 1.0 / (1.0 + math.exp(-0.8 * (ccr_db - 5.5)))
        return ccr_db, False, min(0.99, max(0.5, prob))
    else:
        return ccr_db, False, 0.55

def test_dead_calm_low_wind_rejection():
    """Test Vector 1.1: Dead calm water (< 2.5 m/s) must be rejected without false positive alerts."""
    shape = (128, 128)
    vv_db = [[-24.5 for _ in range(shape[1])] for _ in range(shape[0])]
    mask = [[0 for _ in range(shape[1])] for _ in range(shape[0])]
    for r in range(40, 80):
        for c in range(40, 80):
            mask[r][c] = 1
            vv_db[r][c] = -26.0

    ccr, is_look_alike, prob = calculate_ccr_and_look_alike(vv_db, mask)
    assert is_look_alike is True, "Dead calm low-wind area was incorrectly classified as crude oil"
    assert prob <= 0.35, f"Confidence score {prob} too high for low-wind false positive"
    print("  [OK] PASSED: Python test_dead_calm_low_wind_rejection")

def test_biogenic_look_alike_discrimination():
    """Test Vector 1.2: Weak biogenic surfactant slicks must be flagged as look-alikes."""
    shape = (128, 128)
    vv_db = [[random.gauss(-11.0, 1.0) for _ in range(shape[1])] for _ in range(shape[0])]
    mask = [[0 for _ in range(shape[1])] for _ in range(shape[0])]
    for r in range(40, 80):
        for c in range(40, 80):
            mask[r][c] = 1
            vv_db[r][c] -= 2.8 # Weak damping

    ccr, is_look_alike, prob = calculate_ccr_and_look_alike(vv_db, mask)
    assert is_look_alike is True, "Biogenic film with CCR < 3.5 dB was not flagged as look-alike"
    assert ccr < 3.5, f"Expected CCR < 3.5 dB for biogenic film, got {ccr} dB"
    print("  [OK] PASSED: Python test_biogenic_look_alike_discrimination")

def test_nan_and_inf_resilience():
    """Test Vector 1.3: Pipeline must not crash when encountering sensor corrupted pixels."""
    shape = (64, 64)
    vv_db = [[random.gauss(-11.0, 1.0) for _ in range(shape[1])] for _ in range(shape[0])]
    vv_db[10][10] = float('nan')
    vv_db[20][20] = float('inf')
    mask = [[0 for _ in range(shape[1])] for _ in range(shape[0])]

    ccr, is_look_alike, prob = calculate_ccr_and_look_alike(vv_db, mask)
    assert not math.isnan(ccr)
    print("  [OK] PASSED: Python test_nan_and_inf_resilience")

if __name__ == "__main__":
    test_dead_calm_low_wind_rejection()
    test_biogenic_look_alike_discrimination()
    test_nan_and_inf_resilience()
