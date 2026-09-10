import math
import random

def run_lagrangian_backward_drift(origin_lon, origin_lat, hours, u_current, v_current, u_wind, v_wind, diff_coeff, n_particles=200):
    dt = 60.0
    total_sec = abs(hours) * 3600.0
    steps = int(total_sec / dt)

    u_adv = -(u_current + 0.03 * u_wind)
    v_adv = -(v_current + 0.03 * v_wind)

    rw_std = math.sqrt(2.0 * diff_coeff * dt)

    R_earth = 6378137.0
    deg_to_rad = math.pi / 180.0
    rad_to_deg = 180.0 / math.pi

    lons = [origin_lon for _ in range(n_particles)]
    lats = [origin_lat for _ in range(n_particles)]

    for i in range(n_particles):
        p_u = random.gauss(u_adv, 0.35)
        p_v = random.gauss(v_adv, 0.35)

        for _ in range(steps):
            dx = p_u * dt + random.gauss(0, rw_std)
            dy = p_v * dt + random.gauss(0, rw_std)

            lat_rad = lats[i] * deg_to_rad
            lons[i] += (dx / (R_earth * math.cos(lat_rad))) * rad_to_deg
            lats[i] += (dy / R_earth) * rad_to_deg

    center_lon = sum(lons) / n_particles
    center_lat = sum(lats) / n_particles

    dx_m = [(lon - center_lon) * (deg_to_rad * R_earth * math.cos(center_lat * deg_to_rad)) for lon in lons]
    dy_m = [(lat - center_lat) * (deg_to_rad * R_earth) for lat in lats]

    var_x = sum(x * x for x in dx_m) / n_particles
    var_y = sum(y * y for y in dy_m) / n_particles
    cov_xy = sum(x * y for x, y in zip(dx_m, dy_m)) / n_particles

    common = math.sqrt((var_x - var_y)**2 + 4.0 * cov_xy**2)
    lambda1 = (var_x + var_y + common) / 2.0
    lambda2 = max(1.0, (var_x + var_y - common) / 2.0)

    k_95 = 2.4477
    major_m = k_95 * math.sqrt(lambda1)
    minor_m = k_95 * math.sqrt(lambda2)

    area_km2 = (math.pi * major_m * minor_m) / 1e6
    return center_lon, center_lat, area_km2

def test_extreme_shear_corridor_expansion():
    """Test Vector 2.1: Under severe current/wind shear, search corridor expansion must trigger threshold alert."""
    origin_lon, origin_lat = -88.35, 28.75
    _, _, area_km2 = run_lagrangian_backward_drift(
        origin_lon, origin_lat, hours=-12.0,
        u_current=1.5, v_current=0.0,
        u_wind=-16.0, v_wind=0.0,
        diff_coeff=20.0
    )

    max_search_area_limit = 500.0 # km^2
    assert area_km2 > max_search_area_limit, f"Corridor area {area_km2} km2 expected > {max_search_area_limit} km2"
    print(f"  [OK] PASSED: Python test_extreme_shear_corridor_expansion (Area = {area_km2:.1f} km2)")

if __name__ == "__main__":
    test_extreme_shear_corridor_expansion()
