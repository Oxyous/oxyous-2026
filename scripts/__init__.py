# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

bl_info = {
    "name": "OSceneExporter",
    "author": "Steven Baldwin",
    "description": "",
    "blender": (2, 80, 0),
    "version": (0, 0, 1),
    "location": "",
    "warning": "",
    "category": "Generic",
}

import os
import shutil

import bpy
import bmesh
import math
import mathutils
import struct
import xml.etree.ElementTree as ET
from xml.dom import minidom

from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty, FloatProperty, IntProperty
from bpy.types import Operator, Panel, PropertyGroup

#------------------------------------------------------------
# Global Variables
#------------------------------------------------------------

mesh_cache = {}
texture_cache = {}
material_cache = {}
collision_cache = {}

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

def quantize_float(v, scale=100000.0):
    return int(round(v * scale))


def make_vertex_key(pos, normal, tangent, uv, tangent_w):
    """
    The exported vertex must be unique by all attributes used by the engine.

    If you only merge by position + UV, hard edges and mirrored UV tangent
    seams can break. Including normal/tangent/sign preserves correct shading.
    """
    return (
        quantize_float(pos.x),
        quantize_float(pos.y),
        quantize_float(pos.z),

        quantize_float(normal.x),
        quantize_float(normal.y),
        quantize_float(normal.z),

        quantize_float(tangent.x),
        quantize_float(tangent.y),
        quantize_float(tangent.z),

        quantize_float(uv.x),
        quantize_float(uv.y),

        1 if tangent_w >= 0.0 else -1,
    )


def triangulate_mesh_copy(mesh):
    """
    Triangulates a mesh copy so the original Blender object is not modified.
    """
    bm = bmesh.new()
    bm.from_mesh(mesh)

    bmesh.ops.triangulate(
        bm,
        faces=bm.faces[:],
        quad_method="FIXED",
        ngon_method="BEAUTY"
    )

    bm.to_mesh(mesh)
    bm.free()

    mesh.update()

def compute_bounding_box(obj):
    """
    Computes the axis-aligned bounding box of the object in world space.
    """
    bbox = [obj.matrix_world @ mathutils.Vector(corner) for corner in obj.bound_box]
    min_corner = mathutils.Vector((min(v.x for v in bbox), min(v.y for v in bbox), min(v.z for v in bbox)))
    max_corner = mathutils.Vector((max(v.x for v in bbox), max(v.y for v in bbox), max(v.z for v in bbox)))
    return min_corner, max_corner


def get_export_basis():
    """
    Converts Blender's basis into the engine basis.
    """
    rotation_x_minus_90 = mathutils.Matrix.Rotation(math.radians(-90.0), 3, 'X')
    mirror_x = mathutils.Matrix.Diagonal((-1.0, 1.0, 1.0))
    return mirror_x @ rotation_x_minus_90


def convert_local_transform(obj, basis_change):
    """
    Converts an object's local transform into the export coordinate basis.
    """
    basis_change_4x4 = basis_change.to_4x4()
    converted_matrix = basis_change_4x4 @ obj.matrix_basis @ basis_change_4x4.inverted()
    location, rotation, scale = converted_matrix.decompose()
    converted_location = location.copy()
    converted_rotation = rotation.to_euler('XYZ')
    return converted_location, converted_rotation, scale


def convert_bounding_box(obj, basis_change):
    """
    Converts the object's world-space bounding box into the export basis.
    """
    min_corner, max_corner = compute_bounding_box(obj)
    corners = [
        mathutils.Vector((min_corner.x, min_corner.y, min_corner.z)),
        mathutils.Vector((min_corner.x, min_corner.y, max_corner.z)),
        mathutils.Vector((min_corner.x, max_corner.y, min_corner.z)),
        mathutils.Vector((min_corner.x, max_corner.y, max_corner.z)),
        mathutils.Vector((max_corner.x, min_corner.y, min_corner.z)),
        mathutils.Vector((max_corner.x, min_corner.y, max_corner.z)),
        mathutils.Vector((max_corner.x, max_corner.y, min_corner.z)),
        mathutils.Vector((max_corner.x, max_corner.y, max_corner.z)),
    ]
    converted_corners = [basis_change @ corner for corner in corners]
    converted_min = mathutils.Vector((
        min(v.x for v in converted_corners),
        min(v.y for v in converted_corners),
        min(v.z for v in converted_corners),
    ))
    converted_max = mathutils.Vector((
        max(v.x for v in converted_corners),
        max(v.y for v in converted_corners),
        max(v.z for v in converted_corners),
    ))
    return converted_min, converted_max
#------------------------------------------------------------
#
#------------------------------------------------------------

def compute_aabb(vertices):
    """
    Computes the axis-aligned bounding box (AABB) for a list of vertices.
    """
    if not vertices:
        return None, None

    min_corner = mathutils.Vector((float('inf'), float('inf'), float('inf')))
    max_corner = mathutils.Vector((float('-inf'), float('-inf'), float('-inf')))

    for v in vertices:
        min_corner.x = min(min_corner.x, v.x)
        min_corner.y = min(min_corner.y, v.y)
        min_corner.z = min(min_corner.z, v.z)

        max_corner.x = max(max_corner.x, v.x)
        max_corner.y = max(max_corner.y, v.y)
        max_corner.z = max(max_corner.z, v.z)

    return min_corner, max_corner

# ------------------------------------------------------------
# Mesh Export
# ------------------------------------------------------------

def export_mesh(self, context, filepath, resource_key, obj):
    """
    Exports the mesh to a custom binary format.
    """

    if obj is None or obj.type != 'MESH':
        self.report({'ERROR'}, "No active mesh object to export.")
        return {'CANCELLED'}
    
    src_mesh = obj.data

    if not src_mesh.uv_layers.active:
        self.report({'ERROR'}, "Mesh has no active UV map.")
        return {'CANCELLED'}
    
    mesh = src_mesh.copy()

    try:
        triangulate_mesh_copy(mesh)

        uv_layer = mesh.uv_layers.active
        uv_layer_name = uv_layer.name

        # Required to calculate tangents
        mesh.calc_tangents(uvmap=uv_layer_name)
        mesh.calc_loop_triangles()

        vertices = [] 
        indices = []

        vertex_lookup = {}  # Maps vertex keys to their index in the vertices list

        export_basis = get_export_basis()

        aabb_min, aabb_max = compute_aabb([v.co for v in mesh.vertices])

        for tri in mesh.loop_triangles:
            tri_indices = []

            for loop_index in tri.loops:
                loop = mesh.loops[loop_index]
                vertex = mesh.vertices[loop.vertex_index]

                pos = vertex.co.copy()
                normal = loop.normal.copy()
                tangent = loop.tangent.copy()

                uv = uv_layer.data[loop_index].uv.copy()
                uv.y = 1.0 - uv.y  # Flip V coordinate
                tangent_w = loop.bitangent_sign

                key = make_vertex_key(pos, normal, tangent, uv, tangent_w)

                existing_index = vertex_lookup.get(key)

                if existing_index is not None:
                    tri_indices.append(existing_index)
                    continue

                new_index = len(vertices)
                vertex_lookup[key] = new_index
                tri_indices.append(new_index)

                rotated_pos = export_basis @ pos
                rotated_normal = export_basis @ normal
                rotated_tangent = export_basis @ tangent

                if rotated_normal.length > 0.0:
                    rotated_normal.normalize()
                if rotated_tangent.length > 0.0:
                    rotated_tangent.normalize()

                vertices.append({
                    'position': rotated_pos,
                    'normal': rotated_normal,
                    'tangent': rotated_tangent,
                    'tangent_w': -tangent_w,
                    'uv': uv,
                })

            indices.extend((tri_indices[0], tri_indices[2], tri_indices[1]))
        with open(filepath + resource_key, 'wb') as f:
            # Write header
            f.write(struct.pack("i", len(vertices)))
            f.write(struct.pack("i", len(indices)))
            f.write(struct.pack("fff", aabb_min.x, aabb_min.y, aabb_min.z))
            f.write(struct.pack("fff", aabb_max.x, aabb_max.y, aabb_max.z))

            print(f"Exporting {len(vertices)} vertices and {len(indices)} indices to {filepath + resource_key}")

            # vertex buffer
            for v in vertices:
                p = v['position']
                n = v['normal']
                t = v['tangent']
                tw = v['tangent_w']
                uv = v['uv']

                f.write(struct.pack("fff", p.x, p.y, p.z))
                f.write(struct.pack("fff", n.x, n.y, n.z))
                f.write(struct.pack("ffff", t.x, t.y, t.z, tw))
                f.write(struct.pack("ff", uv.x, uv.y))
            
            # index buffer
            for index in indices:
                f.write(struct.pack("I", index))
        self.report({'INFO'}, f"Mesh exported successfully to {filepath + resource_key}")
    except Exception as e:
        self.report({'ERROR'}, f"Failed to export mesh: {str(e)}")
        return {'CANCELLED'}
    
    finally:
        try:
            mesh.free_tangents()
        except Exception:
            pass

        bpy.data.meshes.remove(mesh)

def export_collision_mesh(self, context, filepath, obj):
    """
    Export collision mesh to a custom binary format.
    
    This function is similar to export_mesh but may have different requirements for collision meshes.
    """
    export_basis = get_export_basis()

    vertices = []
    indices = []
    normals = []

    for obj in bpy.data.objects:
        if obj.type != 'MESH':
            continue
        if obj is not None and obj.get("collision_resource") != None:
            mesh = obj.data.copy()
            mesh.transform(obj.matrix_world)
            
            mesh.calc_loop_triangles()

            for tri in mesh.loop_triangles:
                for loop_index in tri.loops:
                    loop = mesh.loops[loop_index]
                    vertex = mesh.vertices[loop.vertex_index]
                    pos = vertex.co.copy()
                    vertex = export_basis @ pos

                    vertices.append(vertex)
                    normal = export_basis @ loop.normal
                    normals.append(normal.normalized())
                    indices.append(len(vertices) - 1)
    return {
        "vertices": vertices,
        "indices": indices,
        "normals": normals,
    }



def export_scene_meshes(self, context, filepath):
    """
    Exports the entire scene to a custom XML format.
    """
    # Export All Objects in the Scene
    objects = bpy.data.objects

    for obj in objects:

        # Check if custom property exists for the object mesh resource key
        for key, value in obj.items():
            # Skip Blender internal properties
            if key.startswith("_"):
                continue

            if key == "mesh_resource":
                resource_key = f"{value}.osm"
                
                # Mesh not exported yet, export it now
                if resource_key not in mesh_cache and obj.type == 'MESH':
                    export_mesh(self, context, filepath, resource_key, obj)
                    mesh_cache[resource_key] = 1

#------------------------------------------------------------
# export materials  
#------------------------------------------------------------

def export_materials(self, context, filepath):
    """
    Export Materials
    """
    copied = set()
    
    for mat in bpy.data.materials:
        if mat is None:
            continue
        if not mat.use_nodes:
            continue

        albedo_texture = None
        normal_texture = None

        self.report({'INFO'}, f"Exporting material: {mat.name}")
        
        Principled_node = None
        for node in mat.node_tree.nodes:
            if node.type == 'BSDF_PRINCIPLED':
                Principled_node = node
                break

        if Principled_node:
            # Export Base Color
            base_input = Principled_node.inputs["Base Color"]

            if base_input.is_linked:
                link = base_input.links[0]
                from_node = link.from_node

                if from_node.type == 'TEX_IMAGE' and from_node.image:
                    img = from_node.image
                    if img.packed_file:
                        # Unpack the image to a temporary location
                        temp_path = bpy.path.abspath("//temp_texture.png")
                        img.filepath_raw = temp_path
                        img.save()
                        src_path = temp_path
                    else:
                        src_path = bpy.path.abspath(img.filepath)
                        
                    if src_path and os.path.exists(src_path):
                        filename = os.path.basename(src_path)

                        if filename not in copied:
                            dest_path = os.path.join(filepath, filename)
                            shutil.copy2(src_path, dest_path)
                            copied.add(filename)
                            texture_cache[filename] = dest_path
                        albedo_texture = filename
                            
            
            normal_input = Principled_node.inputs["Normal"]
            
            if normal_input.is_linked:
                normal_node = normal_input.links[0].from_node

                if normal_node.type == 'NORMAL_MAP' and normal_node.inputs["Color"].is_linked:
                    tex_node = normal_node.inputs["Color"].links[0].from_node

                    if tex_node.type == 'TEX_IMAGE' and tex_node.image:
                        img = tex_node.image
                        src_path = bpy.path.abspath(img.filepath)
                        
                        if src_path and os.path.exists(src_path):
                            filename = os.path.basename(src_path)

                            if filename not in copied:
                                dest_path = os.path.join(filepath, filename)
                                shutil.copy2(src_path, dest_path)
                                copied.add(filename)
                                texture_cache[filename] = dest_path
                            normal_texture = filename
        material_cache[mat.name] = {
            'albedo_texture': albedo_texture,
            'normal_texture': normal_texture
        }
                                

def export_scene_graph(self, context, filepath):
    """
    Exports the scene graph to a custom XML format.
    """
    root = ET.Element("Scene")

    for material in material_cache:
        mat_elem = ET.SubElement(root, "Material")
        mat_elem.set("name", material)

        albedo_texture = material_cache[material].get('albedo_texture')
        normal_texture = material_cache[material].get('normal_texture')

        self.report({'INFO'}, f"Exporting material: {material}")
        self.report({'INFO'}, f"Albedo Texture: {albedo_texture}")
        self.report({'INFO'}, f"Normal Texture: {normal_texture}")

        if albedo_texture:
            mat_elem.set('albedo', albedo_texture)
        if normal_texture:
            mat_elem.set('normal', normal_texture)
    
    for obj in bpy.data.objects:

        if obj.type != 'MESH':
            continue

        export_basis = get_export_basis()
        converted_location, converted_rotation, converted_scale = convert_local_transform(obj, export_basis)

        obj_elem = ET.SubElement(root, "Object")

        obj_elem.set("name", obj.name)
        obj_elem.set("type", obj.type)

        # Location
        loc_elem = ET.SubElement(obj_elem, "Location")
        loc_elem.set("x", str(round(converted_location.x,3)))
        loc_elem.set("y", str(round(converted_location.y,3)))
        loc_elem.set("z", str(round(converted_location.z,3)))
        
        # Rotation (Euler)
        rot_elem = ET.SubElement(obj_elem, "Rotation")
        rot_elem.set("x", str(converted_rotation.x))
        rot_elem.set("y", str(converted_rotation.y))
        rot_elem.set("z", str(converted_rotation.z))

        # Scale
        scale_elem = ET.SubElement(obj_elem, "Scale")
        scale_elem.set("x", str(converted_scale.x))
        scale_elem.set("y", str(converted_scale.y))
        scale_elem.set("z", str(converted_scale.z))

        for key, value in obj.items():
            if key.startswith("_"):
                continue

            prop_elem = ET.SubElement(obj_elem, "MeshResource")
            prop_elem.set("name", key)
            prop_elem.set("value", str(value))
            
        materal = obj.active_material
        if materal:
            mat_elem = ET.SubElement(obj_elem, "Material")
            mat_elem.set("name", materal.name)
        
        bbox_elem = ET.SubElement(obj_elem, "BoundingBox")
        min_corner, max_corner = convert_bounding_box(obj, export_basis)
        bbox_elem.set("min", f"{round(min_corner.x,3)},{round(min_corner.y,3)},{round(min_corner.z,3)}")
        bbox_elem.set("max", f"{round(max_corner.x,3)},{round(max_corner.y,3)},{round(max_corner.z,3)}")
        
    # Pretty print XML
    def prettify(elem):
        rough_string = ET.tostring(elem, 'utf-8')
        reparsed = minidom.parseString(rough_string)
        return reparsed.toprettyxml(indent="  ")

    xml_str = prettify(root)

    # Save to file
    output_path = bpy.path.abspath(filepath + "\\scene_graph.xml")

    with open(output_path, "w") as f:
        f.write(xml_str)

    print(f"Exported XML to: {output_path}")    

def export_collision(self, context, filepath):
    """
    Exports collision data for the scene.
    """
    polygons = export_collision_mesh(self, context, filepath, None)
    vertices = polygons["vertices"]
    normals = polygons["normals"]
    indices = polygons["indices"]

    with open(filepath + '/world.osc', 'wb') as f:
        # Write header
        f.write(struct.pack("iii", len(vertices), len(normals), len(indices)))
        f.write(struct.pack("iii", 0, 0, 0))  # Placeholder for offsets

        print(f"Exporting {len(indices) // 3} collision polygons to {filepath}")
        vertexOffset = 0
        normalsOffset = 0
        indicesOffset = 0
        vertexOffset = f.tell()
        for vertex in vertices:
            f.write(struct.pack("fff", vertex.x, vertex.y, vertex.z))

        normalsOffset = f.tell()
        for normal in normals:
            f.write(struct.pack("fff", normal.x, normal.y, normal.z))

        indicesOffset = f.tell()
        for index in indices:
            f.write(struct.pack("i", index))

        f.seek(0 + 12)  # Move to the offset position in the header
        f.write(struct.pack("iii", vertexOffset, normalsOffset, indicesOffset))

#------------------------------------------------------------
# Operator
#------------------------------------------------------------

def createTextureDirectory(filepath):
    texture_dir = os.path.join(filepath, "textures")
    if not os.path.exists(texture_dir):
        os.makedirs(texture_dir)
    return texture_dir

class ObjectExport(Operator, ExportHelper):
    """
    Export all scene assets to custom xml format.
    """
    bl_idname = "export_scene.oxyous"
    bl_label = "Export Oxyous Scene"
    bl_options = {'PRESET'}

    filename_ext = ".oss"
    filter_glob: str = StringProperty(
        default="*.oss",
        options={'HIDDEN'},
        maxlen=255
    )

    def execute(self, context):
        createTextureDirectory(self.filepath)
        export_scene_meshes(self, context, self.filepath)
        export_materials(self, context, self.filepath + "textures\\")
        export_scene_graph(self, context, self.filepath)

        mesh_cache.clear()
        texture_cache.clear()
        material_cache.clear()

        return {'FINISHED'}
    
    def invoke(self, context, event):
        wm = context.window_manager
        return wm.invoke_props_dialog(self)

class OxyousSceneCollisionExport(Operator, ExportHelper):
    """
    Export collision meshes for the scene.
    """
    bl_idname = "export_scene.oxyous_collision"
    bl_label = "Export Oxyous Collision Meshes"
    bl_options = {'PRESET'}

    filename_ext = ".osc"
    filter_glob: str = StringProperty(
        default="*.osc",
        options={'HIDDEN'},
        maxlen=255
    )

    def execute(self, context):
        export_collision(self, context, self.filepath)
        return {'FINISHED'}
    
    def invoke(self, context, event):
        wm = context.window_manager
        return wm.invoke_props_dialog(self)    

#------------------------------------------------------------
# Menu  
#------------------------------------------------------------

def menu_func_export(self, context):
    self.layout.operator(ObjectExport.bl_idname, text="Oxyous Scene Export (.oss)")
    self.layout.operator(OxyousSceneCollisionExport.bl_idname, text="Oxyous Collision Export (.osc)")

def register():
    bpy.utils.register_class(ObjectExport)
    bpy.utils.register_class(OxyousSceneCollisionExport)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)

def unregister():
    bpy.utils.unregister_class(ObjectExport)
    bpy.utils.unregister_class(OxyousSceneCollisionExport)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)