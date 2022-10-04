# SPDX-License-Identifier: GPL-2.0-or-later

import bpy

from bpy.types import (
    Collection,
    Menu,
    Panel,
)

from rna_prop_ui import PropertyPanel


class CollectionButtonsPanel:
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "collection"

    @classmethod
    def poll(cls, context):
        return context.collection != context.scene.collection


def lineart_make_line_type_entry(col, line_type, text_disp, expand, search_from):
    col.prop(line_type, "use", text=text_disp)
    if line_type.use and expand:
        col.prop_search(line_type, "layer", search_from,
                        "layers", icon='GREASEPENCIL')
        col.prop_search(line_type, "material", search_from,
                        "materials", icon='SHADING_TEXTURE')


class COLLECTION_PT_collection_flags(CollectionButtonsPanel, Panel):
    bl_label = "Restrictions"

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False

        collection = context.collection
        vl = context.view_layer
        vlc = vl.active_layer_collection

        col = layout.column(align=True)
        col.prop(collection, "hide_select", text="Selectable", toggle=False, invert_checkbox=True)
        col.prop(collection, "hide_render", toggle=False)

        col = layout.column(align=True)
        col.prop(vlc, "holdout", toggle=False)
        col.prop(vlc, "indirect_only", toggle=False)


class COLLECTION_MT_context_menu_instance_offset(Menu):
    bl_label = "Instance Offset"

    def draw(self, _context):
        layout = self.layout
        layout.operator("object.instance_offset_from_cursor")
        layout.operator("object.instance_offset_from_object")
        layout.operator("object.instance_offset_to_cursor")


class COLLECTION_PT_instancing(CollectionButtonsPanel, Panel):
    bl_label = "Instancing"

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False
        collection = context.collection

        row = layout.row(align=True)
        row.prop(collection, "instance_offset")
        row.menu("COLLECTION_MT_context_menu_instance_offset", icon='DOWNARROW_HLT', text="")


class COLLECTION_PT_transform(CollectionButtonsPanel, Panel):
    bl_label = "Transform"

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False
        collection = context.collection

        col = layout.column()
        row = col.row(align=True)
        row.prop(collection, "location")
        row.use_property_decorate = False

        col = layout.column()
        row = col.row(align=True)
        row.prop(collection, "rotation_euler", text="Rotation")
        row.use_property_decorate = False

        col = layout.column()
        row = col.row(align=True)
        row.prop(collection, "scale")
        row.use_property_decorate = False


class COLLECTION_OP_collection_link_collection(bpy.types.Operator):
    """Link collection"""
    bl_idname = "collection.collection_blender_data_link_collection"
    bl_label = ""

    collection_name: bpy.props.StringProperty(default="")

    def execute(self, context):
        context.collection.referenced_collection = bpy.data.collections[self.collection_name]
        return {"FINISHED"}


class COLLECTION_OP_collection_unlink_collection(bpy.types.Operator):
    """Unlink collection"""
    bl_idname = "collection.collection_blender_data_unlink_collection"
    bl_label = ""

    def execute(self, context):
        context.collection.referenced_collection = None
        return {"FINISHED"}


class COLLECTION_MT_collection(bpy.types.Menu):
    bl_idname = "COLLECTION_MT_collection"
    bl_label = "Collection"

    def draw(self, context):
        layout = self.layout
        collections = bpy.data.collections

        for coll in collections:
            if coll.name == context.collection.name:
                continue

            row = layout.row()
            op = row.operator(COLLECTION_OP_collection_link_collection.bl_idname,
                              text=coll.name)
            op.collection_name = coll.name


class COLLECTION_OP_collection_link_object(bpy.types.Operator):
    """Link object"""
    bl_idname = "collection.collection_blender_data_link_object"
    bl_label = ""

    object_name: bpy.props.StringProperty(default="")

    def execute(self, context):
        context.collection.referenced_object = bpy.data.objects[self.object_name]
        return {"FINISHED"}


class COLLECTION_OP_collection_unlink_object(bpy.types.Operator):
    """Unlink object"""
    bl_idname = "collection.collection_blender_data_unlink_object"
    bl_label = ""

    def execute(self, context):
        context.collection.referenced_object = None
        return {"FINISHED"}


class COLLECTION_MT_collection_object(bpy.types.Menu):
    bl_idname = "COLLECTION_MT_collection_object"
    bl_label = "Object"

    def draw(self, context):
        layout = self.layout
        objects = bpy.data.objects

        for obj in objects:
            # if (obj.type == 'CAMERA' and obj.name == USD_CAMERA) or obj.hdusd.is_usd or obj.type not in SUPPORTED_TYPES:
            #     continue

            row = layout.row()
            op = row.operator(COLLECTION_OP_collection_link_object.bl_idname, text=obj.name)
            op.object_name = obj.name


class COLLECTION_PT_reference(CollectionButtonsPanel, Panel):
    bl_label = "Reference"

    collection: bpy.props.PointerProperty(
        type=bpy.types.Collection,
        name="Collection",
        description="",
    )

    def draw(self, context):
        collection = context.collection
        layout = self.layout

        col = layout.column(align=True)
        col.prop(collection, 'reference_usage')

        if collection.reference_usage == 'COLLECTION':
            split = layout.row(align=True).split(factor=0.25)
            col = split.column()
            col.label(text="Collection")
            col = split.column()
            row = col.row(align=True)

            if collection.referenced_collection:
                row.menu(COLLECTION_MT_collection.bl_idname, text=collection.referenced_collection.name,
                 icon='OUTLINER_COLLECTION')
                row.operator(COLLECTION_OP_collection_unlink_collection.bl_idname, icon='X')
            else:
                row.menu(COLLECTION_MT_collection.bl_idname,
                         text=" ", icon='OUTLINER_COLLECTION')

        else:
            split = layout.row(align=True).split(factor=0.25)
            col = split.column()
            col.label(text="Object")
            col = split.column()
            row = col.row(align=True)

            if collection.referenced_object:
                row.menu(COLLECTION_MT_collection_object.bl_idname, text=collection.referenced_object.name,
                         icon='OBJECT_DATAMODE')
                row.operator(COLLECTION_OP_collection_unlink_object.bl_idname, icon='X')
            else:
                row.menu(COLLECTION_MT_collection_object.bl_idname, text=" ", icon='OBJECT_DATAMODE')


class COLLECTION_PT_lineart_collection(CollectionButtonsPanel, Panel):
    bl_label = "Line Art"
    bl_order = 10

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False
        collection = context.collection

        row = layout.row()
        row.prop(collection, "lineart_usage")

        layout.prop(collection, "lineart_use_intersection_mask", text="Collection Mask")

        col = layout.column(align=True)
        col.active = collection.lineart_use_intersection_mask
        row = col.row(align=True, heading="Masks")
        for i in range(8):
            row.prop(collection, "lineart_intersection_mask", index=i, text=" ", toggle=True)
            if i == 3:
                row = col.row(align=True)

        row = layout.row(heading="Intersection Priority")
        row.prop(collection, "use_lineart_intersection_priority", text="")
        subrow = row.row()
        subrow.active = collection.use_lineart_intersection_priority
        subrow.prop(collection, "lineart_intersection_priority", text="")


class COLLECTION_PT_collection_custom_props(CollectionButtonsPanel, PropertyPanel, Panel):
    _context_path = "collection"
    _property_type = Collection


classes = (
    COLLECTION_MT_context_menu_instance_offset,
    COLLECTION_PT_collection_flags,
    COLLECTION_OP_collection_unlink_collection,
    COLLECTION_MT_collection,
    COLLECTION_OP_collection_link_object,
    COLLECTION_OP_collection_unlink_object,
    COLLECTION_MT_collection_object,
    COLLECTION_OP_collection_link_collection,
    COLLECTION_PT_reference,
    COLLECTION_PT_instancing,
    COLLECTION_PT_transform,
    COLLECTION_PT_lineart_collection,
    COLLECTION_PT_collection_custom_props,
)

if __name__ == "__main__":  # only for live edit.
    from bpy.utils import register_class
    for cls in classes:
        register_class(cls)
