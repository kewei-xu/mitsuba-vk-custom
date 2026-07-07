from pathlib import Path

from gonio.python import render_gonio_bundle_from_obj


def test_export_from_obj(variant_scalar_rgb):
    obj_path = Path("F:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/gonio/tests/resources/quad.obj")
    bundle = render_gonio_bundle_from_obj(
        str(obj_path),
        variant="scalar_rgb",
        sample_count=256,
        precision=1,
        image_size=64,
    )

    summary = bundle["summary"]
    assert summary["surface_hits"] > 0
    assert bundle["preview_shape"][2] == 3
    assert len(bundle["preview_pixels"]) == bundle["preview_shape"][0] * bundle["preview_shape"][1] * 3
    assert len(bundle["theta_values"]) == 1663
    assert len(bundle["phi_values"]) == 1663
    assert len(bundle["layers"]) == 2
    assert bundle["layers"][0]["name"] == "sensor_L1_H+"
    assert bundle["layers"][1]["name"] == "sensor_L2_H+"
    assert len(bundle["layers"][0]["data"]) == 1663
    assert len(bundle["layers"][0]["projection_pixels"]) == 64 * 64 * 3
    assert summary["layers"][0]["sensor_hits"] > 0
