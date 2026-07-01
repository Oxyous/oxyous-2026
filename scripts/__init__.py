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

        rotation_x_minus_90 = mathutils.Matrix.Rotation(math.radians(-90.0), 3, 'X')

        for tri in mesh.loop_triangles:
            for loop_index in tri.loops:
                loop = mesh.loops[loop_index]
                vertex = mesh.vertices[loop.vertex_index]

                pos = vertex.co.copy()
                normal = loop.normal.copy()
                tangent = loop.tangent.copy()

                uv = uv_layer.data[loop_index].uv.copy()
                tangent_w = loop.bitangent_sign

                key = make_vertex_key(pos, normal, tangent, uv, tangent_w)

                existing_index = vertex_lookup.get(key)

                if existing_index is not None:
                    indices.append(existing_index)
                    continue

                new_index = len(vertices)
                vertex_lookup[key] = new_index
                indices.append(new_index)

                rotated_pos = rotation_x_minus_90 @ pos
                rotated_normal = rotation_x_minus_90 @ normal
                rotated_tangent = rotation_x_minus_90 @ tangent

                if rotated_normal.length > 0.0:
                    rotated_normal.normalize()
                if rotated_tangent.length > 0.0:
                    rotated_tangent.normalize()

                vertices.append({
                    'position': rotated_pos,
                    'normal': rotated_normal,
                    'tangent': rotated_tangent,
                    'tangent_w': tangent_w,
                    'uv': uv,
                })
        with open(filepath + resource_key, 'wb') as f:
            # Write header
            f.write(struct.pack("i", len(vertices)))
            f.write(struct.pack("i", len(indices)))

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

        obj_elem = ET.SubElement(root, "Object")

        obj_elem.set("name", obj.name)
        obj_elem.set("type", obj.type)

        # Location
        loc_elem = ET.SubElement(obj_elem, "Location")
        rot_correction = mathutils.Matrix.Rotation(math.radians(-90.0), 3, 'X')
        corrected_location = rot_correction @ obj.location

        loc_elem.set("x", str(corrected_location.x))
        loc_elem.set("y", str(corrected_location.y))
        loc_elem.set("z", str(corrected_location.z))

        # Rotation (Euler)
        rot_elem = ET.SubElement(obj_elem, "Rotation")
        rot_elem.set("x", str(obj.rotation_euler.x))
        rot_elem.set("y", str(obj.rotation_euler.y))
        rot_elem.set("z", str(obj.rotation_euler.z))

        # Scale
        scale_elem = ET.SubElement(obj_elem, "Scale")
        scale_elem.set("x", str(obj.scale.x))
        scale_elem.set("y", str(obj.scale.y))
        scale_elem.set("z", str(obj.scale.z))

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

        



#------------------------------------------------------------
# Operator
#------------------------------------------------------------

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
        export_scene_meshes(self, context, self.filepath)
        export_materials(self, context, self.filepath + "\\textures\\")
        export_scene_graph(self, context, self.filepath)

        mesh_cache.clear()
        texture_cache.clear()
        material_cache.clear()

        return {'FINISHED'}
    
    def invoke(self, context, event):
        wm = context.window_manager
        return wm.invoke_props_dialog(self)
    
#------------------------------------------------------------
# Menu  
#------------------------------------------------------------

def menu_func_export(self, context):
    self.layout.operator(ObjectExport.bl_idname, text="Oxyous Scene Export (.oss)")

def register():
    bpy.utils.register_class(ObjectExport)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)

def unregister():
    bpy.utils.unregister_class(ObjectExport)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)