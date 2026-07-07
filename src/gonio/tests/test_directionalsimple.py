import drjit as dr
import mitsuba as mi
import pytest


def make_emitter(direction=None):
    emitter_dict = {
        "type": "directionalsimple",
        "irradiance": {
            "type": "d65",
        },
    }

    if direction is not None:
        emitter_dict["direction"] = direction

    return mi.load_dict(emitter_dict)


def make_scene(direction=None):
    return mi.load_dict({
        "type": "scene",
        "emitter": {
            "type": "directionalsimple",
            "direction": [0, 0, -1] if direction is None else direction,
        },
        "shape": {
            "type": "rectangle",
        },
    })


def test_construct(variant_scalar_rgb):
    emitter = make_emitter()
    assert not emitter.bbox().valid()
    assert dr.allclose(
        emitter.world_transform().matrix,
        [[1, 0, 0, 0],
         [0, 1, 0, 0],
         [0, 0, 1, 0],
         [0, 0, 0, 1]]
    )

    emitter = make_emitter(direction=[0, 0, -1])
    assert dr.allclose(
        emitter.world_transform().matrix,
        [[0, 1, 0, 0],
         [1, 0, 0, 0],
         [0, 0, -1, 0],
         [0, 0, 0, 1]]
    )


def test_requires_scene_bbox(variant_scalar_rgb):
    emitter = make_emitter(direction=[0, 0, -1])
    with pytest.raises(RuntimeError):
        emitter.sample_ray(0.0, 0.5, [0.3, 0.7], [0.2, 0.4])


def test_sample_ray_from_scene(variant_scalar_rgb):
    scene = make_scene(direction=[0, 0, -1])
    emitter = scene.emitters()[0]

    ray, weight = emitter.sample_ray(0.0, 0.5, [0.25, 0.75], [0.2, 0.4])

    assert dr.allclose(ray.d, [0, 0, -1], atol=1e-6)
    assert ray.o.z > 0.0
    assert -1.1 <= dr.slice(ray.o.x) <= 1.1
    assert -1.1 <= dr.slice(ray.o.y) <= 1.1
    assert dr.all(weight != 0)
