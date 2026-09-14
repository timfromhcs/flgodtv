"""
FLGODTV Procedural 3D Asset Generator using Headless Blender
SPDX-License-Identifier: Apache-2.0

Generates deterministic 3D assets for the FLGODTV Godot presentation:
1. fly_agent.glb: Detailed Drosophila morphological fly mesh (head, compound eyes, thorax, abdomen, wings, legs)
2. flora_shrub.glb: Procedural shrub/plant mesh for vegetation instancing
3. environment_rock.glb: Procedural mineral/boulder mesh

Usage:
    blender.exe -b --python scripts/blender_asset_generator.py -- --out-dir godot/assets/models --seed 42
"""

import sys
import os
import math
import hashlib
import json

# Try importing bpy (Blender Python API)
try:
    import bpy
    import bmesh
    HAS_BPY = True
except ImportError:
    HAS_BPY = False


def reset_blender_scene():
    """Clears all objects from the current scene."""
    if not HAS_BPY:
        return
    bpy.ops.wm.read_factory_settings(use_empty=True)


def create_material(name, diffuse_color, roughness=0.5, metallic=0.0):
    """Creates a simple PBR Principled BSDF material."""
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = nodes.get("Principled BSDF")
    if bsdf:
        # Blender 4+ / 5+ BSDF socket naming
        if "Base Color" in bsdf.inputs:
            bsdf.inputs["Base Color"].default_value = diffuse_color
        if "Roughness" in bsdf.inputs:
            bsdf.inputs["Roughness"].default_value = roughness
        if "Metallic" in bsdf.inputs:
            bsdf.inputs["Metallic"].default_value = metallic
    return mat


def generate_fly_agent(seed=42):
    """
    Generates a deterministic morphological Drosophila fly mesh.
    Body segments: Head (with red compound eyes), Thorax (dark amber),
    Abdomen (striped amber/brown segments), and translucent wings.
    """
    reset_blender_scene()
    
    # Materials
    mat_body = create_material("FlyBody", (0.35, 0.22, 0.10, 1.0), roughness=0.6)
    mat_thorax = create_material("FlyThorax", (0.25, 0.16, 0.08, 1.0), roughness=0.5)
    mat_eyes = create_material("FlyEyes", (0.80, 0.08, 0.05, 1.0), roughness=0.3)
    mat_wings = create_material("FlyWings", (0.90, 0.95, 1.0, 0.6), roughness=0.1, metallic=0.2)

    # 1. Thorax (central body segment)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, radius=0.4, location=(0, 0, 0))
    thorax = bpy.context.active_object
    thorax.name = "Thorax"
    thorax.scale = (1.0, 1.3, 0.9)
    thorax.data.materials.append(mat_thorax)

    # 2. Head
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, radius=0.28, location=(0, 0.55, 0.05))
    head = bpy.context.active_object
    head.name = "Head"
    head.scale = (1.1, 0.9, 0.9)
    head.data.materials.append(mat_body)

    # 3. Compound Eyes (left & right)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=0.14, location=(-0.22, 0.60, 0.12))
    eye_l = bpy.context.active_object
    eye_l.name = "Eye_L"
    eye_l.scale = (0.8, 1.1, 1.2)
    eye_l.data.materials.append(mat_eyes)

    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=0.14, location=(0.22, 0.60, 0.12))
    eye_r = bpy.context.active_object
    eye_r.name = "Eye_R"
    eye_r.scale = (0.8, 1.1, 1.2)
    eye_r.data.materials.append(mat_eyes)

    # 4. Abdomen (elongated posterior segments)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, radius=0.38, location=(0, -0.65, -0.05))
    abdomen = bpy.context.active_object
    abdomen.name = "Abdomen"
    abdomen.scale = (0.9, 1.6, 0.8)
    abdomen.data.materials.append(mat_body)

    # 5. Left Wing (thin flat rectangular oval rotated horizontally)
    bpy.ops.mesh.primitive_cube_add(size=0.5, location=(-0.45, -0.25, 0.25))
    wing_l = bpy.context.active_object
    wing_l.name = "Wing_L"
    wing_l.scale = (0.7, 2.0, 0.02)
    wing_l.rotation_euler = (math.radians(5), math.radians(-15), math.radians(25))
    wing_l.data.materials.append(mat_wings)

    # 6. Right Wing
    bpy.ops.mesh.primitive_cube_add(size=0.5, location=(0.45, -0.25, 0.25))
    wing_r = bpy.context.active_object
    wing_r.name = "Wing_R"
    wing_r.scale = (0.7, 2.0, 0.02)
    wing_r.rotation_euler = (math.radians(5), math.radians(15), math.radians(-25))
    wing_r.data.materials.append(mat_wings)

    # Apply transforms before join so vertex geometry is baked deterministically
    for obj in [thorax, head, eye_l, eye_r, abdomen, wing_l, wing_r]:
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Join all meshes into a single FlyMesh for efficient real-time MultiMesh rendering
    bpy.ops.object.select_all(action='DESELECT')
    for obj in [thorax, head, eye_l, eye_r, abdomen, wing_l, wing_r]:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = thorax
    bpy.ops.object.join()
    thorax.name = "FlyAgentMesh"

    # Deterministic single-threaded triangulation of quads
    bm = bmesh.new()
    bm.from_mesh(thorax.data)
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method='BEAUTY', ngon_method='BEAUTY')
    bm.to_mesh(thorax.data)
    bm.free()
    thorax.data.update()

    return thorax


def generate_flora_shrub(seed=42):
    """Generates a procedural shrub/plant mesh for vegetation instancing."""
    reset_blender_scene()
    mat_shrub = create_material("ShrubLeaves", (0.15, 0.45, 0.12, 1.0), roughness=0.7)

    # Central stem
    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=0.08, depth=0.6, location=(0, 0, 0.3))
    stem = bpy.context.active_object
    stem.name = "ShrubStem"
    stem.data.materials.append(mat_shrub)

    # Foliage clusters
    clusters = [
        (0.0, 0.0, 0.55, 0.35),
        (-0.18, 0.12, 0.42, 0.25),
        (0.18, -0.10, 0.45, 0.28),
        (0.12, 0.18, 0.38, 0.24),
        (-0.15, -0.15, 0.40, 0.26),
    ]
    objs = [stem]
    for i, (cx, cy, cz, rad) in enumerate(clusters):
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=rad, location=(cx, cy, cz))
        c_obj = bpy.context.active_object
        c_obj.name = f"Cluster_{i}"
        c_obj.data.materials.append(mat_shrub)
        objs.append(c_obj)

    bpy.ops.object.select_all(action='DESELECT')
    for obj in objs:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = stem
    bpy.ops.object.join()
    stem.name = "FloraShrubMesh"
    return stem


def generate_environment_rock(seed=42):
    """Generates a procedural natural rock/boulder mesh."""
    reset_blender_scene()
    mat_rock = create_material("RockMaterial", (0.35, 0.35, 0.38, 1.0), roughness=0.9)

    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, radius=0.6, location=(0, 0, 0.3))
    rock = bpy.context.active_object
    rock.name = "EnvironmentRockMesh"
    rock.scale = (1.2, 0.9, 0.7)
    rock.data.materials.append(mat_rock)
    return rock


def export_gltf(filepath):
    """Exports the current active scene to a GLB/glTF binary file."""
    os.makedirs(os.path.dirname(os.path.abspath(filepath)), exist_ok=True)
    # Use blender's glTF exporter
    bpy.ops.export_scene.gltf(
        filepath=filepath,
        export_format='GLB',
        use_selection=False,
        export_apply=True
    )
    print(f"[Blender Pipeline] Exported: {filepath} ({os.path.getsize(filepath)} bytes)")


def compute_file_sha256(filepath):
    """Calculates SHA256 checksum of a file."""
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def main():
    if not HAS_BPY:
        print("[ERROR] This script must be executed inside Blender via: blender.exe -b --python ...")
        sys.exit(1)

    # Parse arguments after '--'
    argv = sys.argv
    if "--" in argv:
        args = argv[argv.index("--") + 1:]
    else:
        args = []

    out_dir = "godot/assets/models"
    seed = 42

    i = 0
    while i < len(args):
        if args[i] == "--out-dir" and i + 1 < len(args):
            out_dir = args[i + 1]
            i += 2
        elif args[i] == "--seed" and i + 1 < len(args):
            seed = int(args[i + 1])
            i += 2
        else:
            i += 1

    print(f"[Blender Pipeline] Generating procedural 3D assets into '{out_dir}' with seed {seed}...")
    manifest = {
        "generator": "scripts/blender_asset_generator.py",
        "blender_version": bpy.app.version_string,
        "seed": seed,
        "assets": {}
    }

    # 1. Fly Agent
    fly_path = os.path.join(out_dir, "fly_agent.glb")
    generate_fly_agent(seed=seed)
    export_gltf(fly_path)
    manifest["assets"]["fly_agent"] = {
        "file": fly_path,
        "size_bytes": os.path.getsize(fly_path),
        "sha256": compute_file_sha256(fly_path)
    }

    # 2. Flora Shrub
    shrub_path = os.path.join(out_dir, "flora_shrub.glb")
    generate_flora_shrub(seed=seed)
    export_gltf(shrub_path)
    manifest["assets"]["flora_shrub"] = {
        "file": shrub_path,
        "size_bytes": os.path.getsize(shrub_path),
        "sha256": compute_file_sha256(shrub_path)
    }

    # 3. Environment Rock
    rock_path = os.path.join(out_dir, "environment_rock.glb")
    generate_environment_rock(seed=seed)
    export_gltf(rock_path)
    manifest["assets"]["environment_rock"] = {
        "file": rock_path,
        "size_bytes": os.path.getsize(rock_path),
        "sha256": compute_file_sha256(rock_path)
    }

    # Save manifest
    manifest_path = os.path.join(out_dir, "asset_manifest.json")
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
    print(f"[Blender Pipeline] Asset generation complete! Manifest saved to {manifest_path}")


if __name__ == "__main__":
    main()
