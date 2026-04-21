"""
Agrabah Maze Level Generator (v2 - Greedy Meshing)
Generates a maze-based level for the Aladdin game engine.
Merges adjacent walls into large rectangles to prevent z-fighting.

Usage:
    python generate_maze.py [--rows 7] [--cols 7] [--seed 42] [--write-app-config]
"""

import json
import random
import argparse
import os
import sys

# ---- Maze Generation (Iterative Backtracking) ----

def generate_maze(rows, cols, seed=None):
    """
    Generate a perfect maze using iterative backtracking.
    Returns a 2D grid where 0 = path, 1 = wall.
    Grid dimensions: (2*rows+1) x (2*cols+1).
    """
    if seed is not None:
        random.seed(seed)

    grid_h = 2 * rows + 1
    grid_w = 2 * cols + 1
    grid = [[1] * grid_w for _ in range(grid_h)]

    # Iterative backtracking (avoids recursion limit for large mazes)
    stack = [(0, 0)]
    grid[1][1] = 0  # Mark start cell

    while stack:
        r, c = stack[-1]
        directions = [(0, 1), (1, 0), (0, -1), (-1, 0)]
        random.shuffle(directions)

        found = False
        for dr, dc in directions:
            nr, nc = r + dr, c + dc
            if 0 <= nr < rows and 0 <= nc < cols and grid[2*nr+1][2*nc+1] == 1:
                # Knock down wall between current and neighbor
                grid[2*r+1+dr][2*c+1+dc] = 0
                grid[2*nr+1][2*nc+1] = 0
                stack.append((nr, nc))
                found = True
                break

        if not found:
            stack.pop()

    return grid


# ---- Greedy Rectangular Merging ----

def greedy_merge_walls(grid):
    rows = len(grid)
    cols = len(grid[0])
    rectangles = []
    consumed = [[False] * cols for _ in range(rows)]

    for r in range(rows):
        for c in range(cols):
            if grid[r][c] != 1 or consumed[r][c]:
                continue

            width = 0
            while c + width < cols and grid[r][c + width] == 1 and not consumed[r][c + width]:
                width += 1

            height = 1
            while r + height < rows:
                can_expand = True
                for cc in range(c, c + width):
                    if grid[r + height][cc] != 1 or consumed[r + height][cc]:
                        can_expand = False
                        break
                if not can_expand:
                    break
                height += 1

            for rr in range(r, r + height):
                for cc in range(c, c + width):
                    consumed[rr][cc] = True

            rectangles.append((r, c, width, height))

    return rectangles


# ---- Constants ----

WALL_HEIGHT = 4.0
CELL_SIZE = 3.0
# The cube.obj has vertices from -1 to +1, so native size is 2x2x2.
# Scale of S produces a cube of size 2*S on each axis.
CUBE_NATIVE = 2.0  # cube.obj spans 2 units per axis


# ---- Coordinate Conversion ----

def grid_center(grid):
    """Return the center offset for the grid."""
    rows = len(grid)
    cols = len(grid[0])
    return (cols - 1) / 2.0, (rows - 1) / 2.0

def grid_to_world(row, col, grid):
    """Convert grid (row, col) to world (x, z) centered at origin."""
    cx, cz = grid_center(grid)
    x = (col - cx) * CELL_SIZE
    z = -(row - cz) * CELL_SIZE
    return x, z


# ---- Entity Builders ----

def make_merged_wall(name, row, col, w, h, grid):
    """Create a single wall entity from a merged rectangle of grid cells."""
    # Center of the rectangle in grid coords
    center_col = col + w / 2.0 - 0.5
    center_row = row + h / 2.0 - 0.5
    # But grid_to_world expects integer-like coords, so compute directly
    cx, cz = grid_center(grid)
    x = (col + w / 2.0 - 0.5 - cx) * CELL_SIZE
    z = -(row + h / 2.0 - 0.5 - cz) * CELL_SIZE

    # Scale = desired_size / CUBE_NATIVE
    scale_x = (w * CELL_SIZE) / CUBE_NATIVE
    scale_y = WALL_HEIGHT / CUBE_NATIVE
    scale_z = (h * CELL_SIZE) / CUBE_NATIVE

    return {
        "name": name,
        "position": [round(x, 3), WALL_HEIGHT / 2, round(z, 3)],
        "scale": [round(scale_x, 3), round(scale_y, 3), round(scale_z, 3)],
        "components": [
            {"type": "Mesh Renderer", "mesh": "cube", "material": "lit-wall"}
        ]
    }

def make_ground(grid):
    total_w = len(grid[0]) * CELL_SIZE
    total_h = len(grid) * CELL_SIZE
    # Plane is 2x2 natively, so scale = desired_size / 2
    return {
        "name": "ground",
        "position": [0, -0.01, 0],
        "rotation": [-90, 0, 0],
        "scale": [total_w / CUBE_NATIVE, total_h / CUBE_NATIVE, 1],
        "components": [
            {"type": "Mesh Renderer", "mesh": "plane", "material": "lit-ground"}
        ]
    }

def make_coin(name, x, z, y=1):
    return {
        "name": name,
        "position": [round(x, 3), y, round(z, 3)],
        "rotation": [0, 0, 0],
        "scale": [2.0, 2.0, 2.0],
        "components": [
            {"type": "Mesh Renderer", "mesh": "coin_mesh", "material": "coin-mat"},
            {"type": "Movement", "angularVelocity": [0, 120, 0]}
        ]
    }

def make_enemy(name, x, z):
    # Want a 1x2x1 enemy → scale = size / CUBE_NATIVE
    return {
        "name": name,
        "position": [round(x, 3), 1.0, round(z, 3)],
        "scale": [0.75, 0.75, 0.75],
        "components": [
            {"type": "Mesh Renderer", "mesh": "monkey_mesh", "material": "lit-guard"}
        ]
    }

def make_player(x, z):
    # Want a 0.8x1.5x0.8 player → scale = size / CUBE_NATIVE
    return {
        "name": "aladdin",
        "position": [round(x, 3), 0.0, round(z, 3)], # Models usually have feet at origin
        "rotation": [0, 180, 0],
        "scale": [0.175, 0.175, 0.175],
        "components": [
            {"type": "Mesh Renderer", "mesh": "aladdin_mesh", "material": "lit-aladdin"}
        ]
    }

def make_portal(x, z):
    return {
        "name": "end-portal",
        # The portal model spans from Z = -0.512 to +0.512 locally.
        # Scaled by 0.75, its back is at -0.384. 
        # The wall front is at z - 1.5. To make the back touch the wall,
        # we place the center at z - 1.11.
        "position": [round(x, 3), 0.0, round(z - 1.1, 3)],
        "scale": [0.75, 0.75, 0.75], # Model is ~4.8 units tall, scaling down to fit the 4.0 height wall
        "children": [
            {
                "name": "portal-arch",
                "components": [
                    {"type": "Mesh Renderer", "mesh": "portal_arch_mesh", "material": "portal-arch-mat"}
                ]
            },
            {
                "name": "portal-core",
                "components": [
                    {"type": "Mesh Renderer", "mesh": "portal_core_mesh", "material": "portal-core-mat"}
                ]
            }
        ]
    }

def make_camera():
    return {
        "name": "main-camera",
        "position": [0, 40, 35],
        "rotation": [-50, 0, 0],
        "components": [
            {"type": "Camera", "fov": 60},
            {"type": "Free Camera Controller"}
        ]
    }

def make_lights(grid):
    half_w = (len(grid[0]) * CELL_SIZE) / 2
    half_h = (len(grid) * CELL_SIZE) / 2
    return [
        {
            "name": "sun",
            "rotation": [-55, 35, 0],
            "components": [{
                "type": "Light",
                "light_type": "directional",
                "color": [1.0, 0.92, 0.75],
                "intensity": 1.4
            }]
        },
        {
            "name": "torch-1",
            "position": [-half_w + 3, 6, half_h - 3],
            "components": [{
                "type": "Light",
                "light_type": "point",
                "color": [1.0, 0.6, 0.2],
                "intensity": 3.0,
                "attenuation_linear": 0.07,
                "attenuation_quadratic": 0.017
            }]
        },
        {
            "name": "torch-2",
            "position": [half_w - 3, 6, -half_h + 3],
            "components": [{
                "type": "Light",
                "light_type": "point",
                "color": [1.0, 0.55, 0.15],
                "intensity": 3.0,
                "attenuation_linear": 0.07,
                "attenuation_quadratic": 0.017
            }]
        },
        {
            "name": "portal-light",
            "position": [half_w - 6, 8, -half_h + 3],
            "rotation": [-70, 0, 0],
            "components": [{
                "type": "Light",
                "light_type": "spot",
                "color": [0.5, 0.7, 1.0],
                "intensity": 4.0,
                "inner_angle": 20.0,
                "outer_angle": 35.0,
                "attenuation_linear": 0.045,
                "attenuation_quadratic": 0.0075
            }]
        }
    ]


# ---- Placement Helpers ----

def count_neighbors(grid, r, c):
    count = 0
    for dr, dc in [(0,1),(0,-1),(1,0),(-1,0)]:
        nr, nc = r+dr, c+dc
        if 0 <= nr < len(grid) and 0 <= nc < len(grid[0]) and grid[nr][nc] == 0:
            count += 1
    return count

def categorize_cells(grid):
    dead_ends = []
    corridors = []
    intersections = []
    for r in range(len(grid)):
        for c in range(len(grid[0])):
            if grid[r][c] == 0:
                n = count_neighbors(grid, r, c)
                if n == 1:
                    dead_ends.append((r,c))
                elif n == 2:
                    corridors.append((r,c))
                elif n >= 3:
                    intersections.append((r,c))
    return dead_ends, corridors, intersections


# ---- Main Generator ----

def generate_level(rows=7, cols=7, seed=42):
    grid = generate_maze(rows, cols, seed)
    dead_ends, corridors, intersections = categorize_cells(grid)

    entities = []

    # Camera
    entities.append(make_camera())

    # Lights
    entities.extend(make_lights(grid))

    # Ground (slightly below y=0 to avoid z-fighting with wall bottoms)
    entities.append(make_ground(grid))

    # Walls - GREEDY MERGED (no z-fighting!)
    rectangles = greedy_merge_walls(grid)
    for i, (r, c, w, h) in enumerate(rectangles):
        entities.append(make_merged_wall(f"wall-{i}", r, c, w, h, grid))

    # Player at top-left
    start_cell = (1, 1)
    sx, sz = grid_to_world(start_cell[0], start_cell[1], grid)
    entities.append(make_player(sx, sz))

    # Portal at bottom-right
    end_cell = (len(grid)-2, len(grid[0])-2)
    ex, ez = grid_to_world(end_cell[0], end_cell[1], grid)
    entities.append(make_portal(ex, ez))

    # Coins at dead-ends (skip start)
    coin_count = 0
    for r, c in dead_ends:
        if (r,c) != start_cell:
            x, z = grid_to_world(r, c, grid)
            entities.append(make_coin(f"coin-{coin_count}", x, z))
            coin_count += 1

    # Coins at ~25% of corridors
    random.shuffle(corridors)
    for r, c in corridors[:max(1, len(corridors)//4)]:
        x, z = grid_to_world(r, c, grid)
        entities.append(make_coin(f"coin-{coin_count}", x, z))
        coin_count += 1

    # Enemies at ~40% of intersections
    enemy_count = 0
    random.shuffle(intersections)
    for r, c in intersections[:max(1, len(intersections)*2//5)]:
        if (r,c) != start_cell and (r,c) != end_cell:
            x, z = grid_to_world(r, c, grid)
            entities.append(make_enemy(f"guard-{enemy_count}", x, z))
            enemy_count += 1

    # Print summary
    print(f"+======================================+")
    print(f"|   Agrabah Maze Level Generated! (v2) |")
    print(f"+======================================+")
    print(f"|  Maze cells:     {rows}x{cols}")
    print(f"|  Grid size:      {len(grid)}x{len(grid[0])}")
    print(f"|  Wall rects:     {len(rectangles)} (merged from {sum(r[2]*r[3] for r in rectangles)} cells)")
    print(f"|  Total coins:    {coin_count}")
    print(f"|  Total enemies:  {enemy_count}")
    print(f"|  Total entities: {len(entities)}")
    print(f"|  Seed:           {seed}")
    print(f"+======================================+")
    print()

    # ASCII preview
    print("Maze preview (S=start, E=end, .=path, #=wall):")
    for r in range(len(grid)):
        row_str = ""
        for c in range(len(grid[0])):
            if (r,c) == start_cell:
                row_str += "S"
            elif (r,c) == end_cell:
                row_str += "E"
            elif grid[r][c] == 1:
                row_str += "#"
            else:
                row_str += "."
        print(row_str)
    print()

    return entities, coin_count, enemy_count


def build_scene_json(entities, coin_count, enemy_count, t3=60, t2=90, t1=120, grid_rows=15, grid_cols=15):
    return {
        "start-scene": "menu",
        "window": {
            "title": "Disney's Aladdin in Nasira's Revenge",
            "size": {"width": 1280, "height": 720},
            "fullscreen": False
        },
        "game": {
            "total_coins": coin_count,
            "total_enemies": enemy_count,
            "time_3star": t3,
            "time_2star": t2,
            "time_1star": t1
        },
        "scene": {
            "renderer": {
                "sky": "assets/textures/sky_desert.png",
                "postprocess": "assets/shaders/postprocess/vignette.frag"
            },
            "assets": {
                "shaders": {
                    "tinted":   {"vs": "assets/shaders/tinted.vert",   "fs": "assets/shaders/tinted.frag"},
                    "textured": {"vs": "assets/shaders/textured.vert", "fs": "assets/shaders/textured.frag"},
                    "light":    {"vs": "assets/shaders/light.vert",    "fs": "assets/shaders/light.frag"}
                },
                "textures": {
                    "agrabah_ground": "assets/textures/agrabah_ground.png",
                    "agrabah_wall":   "assets/textures/agrabah_wall.png",
                    "coin_tex":       "assets/models/Coin/Item_Coin_Texture.png",
                    "aladdin_skin":   "assets/models/Aladdin/aladdin_diff.png",
                    "guard_diffuse":  "assets/textures/guard_diffuse.jpg",
                    "portal_texture_0": "assets/models/Portal/0.png",
                    "portal_texture_1": "assets/models/Portal/1.png",
                    "moon":           "assets/textures/moon.jpg"
                },
                "meshes": {
                    "cube":         "assets/models/cube.obj",
                    "plane":        "assets/models/plane.obj",
                    "sphere":       "assets/models/sphere.obj",
                    "aladdin_mesh": "assets/models/Aladdin/aladdin_costume_basic.obj",
                    "monkey_mesh":  "assets/models/monkey.obj",
                    "coin_mesh":    "assets/models/Coin/Coin.obj",
                    "portal_arch_mesh": "assets/models/Portal/mtl15.obj",
                    "portal_core_mesh": "assets/models/Portal/mtl21.obj"
                },
                "samplers": {
                    "default": {},
                    "repeat":  {"WRAP_S": "GL_REPEAT", "WRAP_T": "GL_REPEAT"}
                },
                "materials": {
                    "lit-ground": {
                        "type": "lit", "shader": "light",
                        "pipelineState": {"faceCulling": {"enabled": False}, "depthTesting": {"enabled": True}},
                        "albedo_map": "agrabah_ground", "sampler": "repeat",
                        "shininess": 8.0, "ambient": 0.15,
                        "uv_multiplier": [grid_cols, grid_rows]
                    },
                    "lit-wall": {
                        "type": "lit", "shader": "light",
                        "pipelineState": {"faceCulling": {"enabled": True}, "depthTesting": {"enabled": True}},
                        "albedo_map": "agrabah_wall", "sampler": "repeat",
                        "shininess": 16.0, "ambient": 0.12
                    },
                    "lit-aladdin": {
                        "type": "lit", "shader": "light",
                        "pipelineState": {"faceCulling": {"enabled": True}, "depthTesting": {"enabled": True}},
                        "albedo_map": "aladdin_skin", "sampler": "default",
                        "shininess": 32.0, "ambient": 0.1
                    },
                    "lit-guard": {
                        "type": "lit", "shader": "light",
                        "pipelineState": {"faceCulling": {"enabled": True}, "depthTesting": {"enabled": True}},
                        "albedo_map": "guard_diffuse", "sampler": "default",
                        "shininess": 24.0, "ambient": 0.1
                    },
                    "coin-mat": {
                        "type": "lit", "shader": "light",
                        "pipelineState": {"faceCulling": {"enabled": True}, "depthTesting": {"enabled": True}},
                        "albedo_map": "coin_tex", "sampler": "default",
                        "shininess": 16.0, "ambient": 0.2
                    },
                    "portal-arch-mat": {
                        "type": "lit", "shader": "light",
                        "pipelineState": {"faceCulling": {"enabled": False}, "depthTesting": {"enabled": True}},
                        "albedo_map": "portal_texture_0", "sampler": "default",
                        "shininess": 8.0, "ambient": 0.3
                    },
                    "portal-core-mat": {
                        "type": "lit", "shader": "light",
                        "pipelineState": {"faceCulling": {"enabled": False}, "depthTesting": {"enabled": True}},
                        "albedo_map": "portal_texture_1", "sampler": "default",
                        "shininess": 32.0, "ambient": 0.8
                    },
                    "sandstone": {
                        "type": "tinted", "shader": "tinted",
                        "pipelineState": {"faceCulling": {"enabled": False}, "depthTesting": {"enabled": True}},
                        "tint": [0.85, 0.72, 0.53, 1]
                    }
                }
            },
            "world": entities
        }
    }


def main():
    parser = argparse.ArgumentParser(description="Generate Agrabah maze level")
    parser.add_argument("--rows",  type=int, default=7,   help="Maze rows (default: 7)")
    parser.add_argument("--cols",  type=int, default=7,   help="Maze cols (default: 7)")
    parser.add_argument("--seed",  type=int, default=42,  help="Random seed (default: 42)")
    parser.add_argument("--time3", type=int, default=60,  help="3-star time limit (sec)")
    parser.add_argument("--time2", type=int, default=90,  help="2-star time limit (sec)")
    parser.add_argument("--time1", type=int, default=120, help="1-star time limit (sec)")
    parser.add_argument("--write-app-config", action="store_true",
                        help="Overwrite config/app.jsonc with generated scene config")
    args = parser.parse_args()

    entities, coins, enemies = generate_level(args.rows, args.cols, args.seed)
    scene = build_scene_json(entities, coins, enemies, args.time3, args.time2, args.time1, args.rows, args.cols)

    # Write level1.jsonc
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    
    level_dir = os.path.join(root_dir, "config", "agrabah-level")
    os.makedirs(level_dir, exist_ok=True)
    level_path = os.path.join(level_dir, "level1.jsonc")
    with open(level_path, "w", encoding="utf-8") as f:
        f.write(f"// Auto-generated Agrabah Maze Level (v2 - greedy meshing)\n")
        f.write(f"// Maze: {args.rows}x{args.cols} | Coins: {coins} | Enemies: {enemies}\n")
        f.write(f"// Stars: 3-star < {args.time3}s | 2-star < {args.time2}s | 1-star < {args.time1}s\n")
        json.dump(scene, f, indent=4)
    print(f"Wrote: {level_path}")

    if args.write_app_config:
        app_path = os.path.join(root_dir, "config", "app.jsonc")
        with open(app_path, "w", encoding="utf-8") as f:
            f.write(f"// Auto-generated Agrabah Maze - App Config\n")
            f.write(f"// Maze: {args.rows}x{args.cols} | Coins: {coins} | Enemies: {enemies}\n")
            json.dump(scene, f, indent=4)
        print(f"Wrote: {app_path}")
    else:
        print("Skipped writing config/app.jsonc (use --write-app-config to enable).")


if __name__ == "__main__":
    main()
