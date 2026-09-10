import pytest
import numpy as np

@pytest.fixture
def synthetic_sar_generator():
    """Generates synthetic multi-polarized SAR arrays for red-team fuzzing."""
    def _generate(scenario="normal", shape=(512, 512)):
        rows, cols = shape
        if scenario == "dead_calm_low_wind":
            # Very low backscatter everywhere with zero wave damping contrast
            vv_db = np.full(shape, -25.0, dtype=np.float32)
            vh_db = np.full(shape, -33.0, dtype=np.float32)
            mask = np.zeros(shape, dtype=np.uint8)
            mask[100:200, 100:200] = 1 # Candidate box
            return vv_db, vh_db, mask

        elif scenario == "edge_clipped_slick":
            # Slick sliced directly along the tile boundary [0:64, 0:512]
            vv_db = np.random.normal(-10.5, 1.5, size=shape).astype(np.float32)
            vh_db = np.random.normal(-20.0, 1.5, size=shape).astype(np.float32)
            mask = np.zeros(shape, dtype=np.uint8)
            # Slick touching the top border
            vv_db[0:30, 100:400] -= 10.0 # Damping
            mask[0:30, 100:400] = 1
            return vv_db, vh_db, mask

        elif scenario == "biogenic_look_alike":
            # Weak damping (~3.0 dB) with high VH variation
            vv_db = np.random.normal(-11.0, 1.2, size=shape).astype(np.float32)
            vh_db = np.random.normal(-20.0, 1.2, size=shape).astype(np.float32)
            mask = np.zeros(shape, dtype=np.uint8)
            vv_db[150:220, 150:220] -= 2.8 # Weak damping
            mask[150:220, 150:220] = 1
            return vv_db, vh_db, mask

        elif scenario == "corrupted_nans":
            vv_db = np.random.normal(-11.0, 1.2, size=shape).astype(np.float32)
            vh_db = np.random.normal(-20.0, 1.2, size=shape).astype(np.float32)
            vv_db[50:60, 50:60] = np.nan
            vh_db[70:80, 70:80] = np.inf
            mask = np.zeros(shape, dtype=np.uint8)
            return vv_db, vh_db, mask

        return np.zeros(shape), np.zeros(shape), np.zeros(shape)
    return _generate
