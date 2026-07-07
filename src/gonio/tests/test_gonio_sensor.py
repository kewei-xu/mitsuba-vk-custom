import pytest
import drjit as dr
import mitsuba as mi


def make_sensor(**kwargs):
    d = {
        "type": "gonio",
        "film": {
            "type": "hdrfilm",
            "width": 1,
            "height": 1,
            "rfilter": {"type": "box"}
        }
    }
    d.update(kwargs)
    return mi.load_dict(d)


def test_construct_default_layout(variant_scalar_rgb):
    sensor = make_sensor(precision=1)
    assert not sensor.bbox().valid()
    assert dr.all(sensor.film().size() == [1664, 2])

    desc = str(sensor)
    assert "precision = 1" in desc
    assert "patches = 1663" in desc
    assert "rings = 26" in desc


def test_construct_analytic_layout(variant_scalar_rgb):
    sensor = make_sensor(
        precision=2,
        analytic_measurement=True,
        record_refraction=True,
        merge_l1_l2=False
    )
    assert dr.all(sensor.film().size() == [26279, 2])


@pytest.mark.parametrize("precision", [0, 5])
def test_reject_invalid_precision(variant_scalar_rgb, precision):
    with pytest.raises(RuntimeError):
        make_sensor(precision=precision)
