import drjit as dr
import mitsuba as mi
import pytest


def make_scene(**sensor_kwargs):
    sensor_dict = {
        "type": "gonio",
        "precision": 1,
        "merge_l1_l2": False,
        "record_refraction": False,
        "sampler": {
            "type": "independent",
            "sample_count": 1,
        },
        "film": {
            "type": "hdrfilm",
            "width": 1,
            "height": 1,
            "rfilter": {"type": "box"},
        },
    }
    sensor_dict.update(sensor_kwargs)

    return mi.load_dict({
        "type": "scene",
        "integrator": {
            "type": "gtracer",
            "max_depth": 4,
            "rr_depth": 8,
        },
        "sensor": sensor_dict,
        "emitter": {
            "type": "directionalsimple",
            "direction": [0, 0, -1],
            "irradiance": {
                "type": "rgb",
                "value": [1.0, 1.0, 1.0],
            },
        },
        "shape": {
            "type": "rectangle",
            "bsdf": {
                "type": "diffuse",
                "reflectance": {
                    "type": "rgb",
                    "value": [1.0, 1.0, 1.0],
                },
            },
        },
    })


def test_render_reflective_chain(variant_scalar_rgb):
    scene = make_scene()
    integrator = scene.integrator()
    assert isinstance(integrator, mi.Integrator)

    image = mi.render(scene, integrator=integrator, spp=1024, seed=0)

    assert dr.shape(image) == (2, 1664, 3)
    assert dr.count(dr.ravel(image[0, 1:, :]) > 0) > 0
    assert dr.all(image[1, :, :] == 0)


def test_reject_non_gonio_sensor(variant_scalar_rgb):
    scene = mi.load_dict({
        "type": "scene",
        "integrator": {
            "type": "gtracer",
        },
        "sensor": {
            "type": "perspective",
            "sampler": {"type": "independent"},
            "film": {
                "type": "hdrfilm",
                "width": 4,
                "height": 4,
                "rfilter": {"type": "box"},
            },
        },
        "emitter": {
            "type": "directionalsimple",
            "direction": [0, 0, -1],
        },
        "shape": {
            "type": "rectangle",
            "bsdf": {"type": "diffuse"},
        },
    })

    with pytest.raises(RuntimeError, match="sensor of type 'gonio'"):
        mi.render(scene, spp=1, seed=0)
