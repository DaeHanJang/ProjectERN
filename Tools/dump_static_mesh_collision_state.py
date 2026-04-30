import json
from pathlib import Path

import unreal


ASSET_PATH = "/Game/Assets/FC_MedievalMonastery/BackgroundLandscape/SM_Background_Landscape"
OUTPUT_PATH = Path(
    r"C:\Users\KGA\Documents\Unreal Projects\ProjectERN\Saved\CurrentStaticMeshCollisionState.json"
)


def safe_enum_name(value):
    try:
        return str(value)
    except Exception:
        return repr(value)


def safe_get(obj, prop):
    try:
        return obj.get_editor_property(prop)
    except Exception as exc:
        return f"<error:{exc}>"


def summarize_convex(convex_elem):
    summary = {"vertex_count": None, "bounding_box": None}
    try:
        verts = convex_elem.get_editor_property("vertex_data")
        summary["vertex_count"] = len(verts)
        if verts:
            xs = [v.x for v in verts]
            ys = [v.y for v in verts]
            zs = [v.z for v in verts]
            summary["bounding_box"] = {
                "min": {"x": min(xs), "y": min(ys), "z": min(zs)},
                "max": {"x": max(xs), "y": max(ys), "z": max(zs)},
            }
    except Exception as exc:
        summary["error"] = str(exc)
    return summary


def try_get_lod_stats(mesh):
    stats = {}
    for fn_name in ("get_num_triangles", "get_num_sections", "get_num_vertices"):
        fn = getattr(mesh, fn_name, None)
        if not callable(fn):
            continue
        try:
            stats[fn_name] = fn(0)
        except Exception as exc:
            stats[fn_name] = f"<error:{exc}>"
    return stats


mesh = unreal.load_asset(ASSET_PATH)
if not mesh:
    raise RuntimeError(f"Failed to load asset: {ASSET_PATH}")

body_setup = safe_get(mesh, "body_setup")
complex_collision_mesh = safe_get(mesh, "complex_collision_mesh")

result = {
    "asset_path": ASSET_PATH,
    "asset_name": mesh.get_name(),
    "complex_collision_mesh": None,
    "lod_for_collision": safe_get(mesh, "lod_for_collision"),
    "nanite_settings": str(safe_get(mesh, "nanite_settings")),
    "lod0_stats": try_get_lod_stats(mesh),
}

if hasattr(complex_collision_mesh, "get_path_name"):
    result["complex_collision_mesh"] = {
        "path": complex_collision_mesh.get_path_name(),
        "name": complex_collision_mesh.get_name(),
        "lod0_stats": try_get_lod_stats(complex_collision_mesh),
    }
else:
    result["complex_collision_mesh"] = complex_collision_mesh

if hasattr(body_setup, "get_editor_property"):
    agg_geom = safe_get(body_setup, "agg_geom")
    default_instance = safe_get(body_setup, "default_instance")
    convex_elems = []
    box_elems = []
    sphere_elems = []
    capsule_elems = []
    tapered_capsule_elems = []

    if hasattr(agg_geom, "get_editor_property"):
        convex_elems = safe_get(agg_geom, "convex_elems")
        box_elems = safe_get(agg_geom, "box_elems")
        sphere_elems = safe_get(agg_geom, "sphere_elems")
        capsule_elems = safe_get(agg_geom, "sphyl_elems")
        tapered_capsule_elems = safe_get(agg_geom, "tapered_capsule_elems")

    result["body_setup"] = {
        "collision_trace_flag": safe_enum_name(safe_get(body_setup, "collision_trace_flag")),
        "double_sided_geometry": safe_get(body_setup, "double_sided_geometry"),
        "default_instance_collision_enabled": safe_enum_name(
            safe_get(default_instance, "collision_enabled")
        )
        if hasattr(default_instance, "get_editor_property")
        else default_instance,
        "simple_collision_counts": {
            "boxes": len(box_elems) if isinstance(box_elems, list) else box_elems,
            "spheres": len(sphere_elems) if isinstance(sphere_elems, list) else sphere_elems,
            "capsules": len(capsule_elems) if isinstance(capsule_elems, list) else capsule_elems,
            "convex": len(convex_elems) if isinstance(convex_elems, list) else convex_elems,
            "tapered_capsules": len(tapered_capsule_elems)
            if isinstance(tapered_capsule_elems, list)
            else tapered_capsule_elems,
        },
        "convex_summaries": [
            summarize_convex(elem) for elem in convex_elems
        ]
        if isinstance(convex_elems, list)
        else convex_elems,
    }
else:
    result["body_setup"] = body_setup

OUTPUT_PATH.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log(f"Wrote collision state report: {OUTPUT_PATH}")
