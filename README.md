# **Std-Raytracer:** a multi-threaded ray-traced/path-traced renderer built from scratch with C++ standard library and nlohmann's json parser

This project was a **Computer Graphics: Rendering** coursework at the University of Edinburgh for the year 2023, with some small bug fixes added after submission. To build the project, use `Code/Makefile`, code has been tested on Windows 11, Ubuntu and MacOS. The file format for scenarios to render is JSON, will upload instructions later...

### File formats
- **Scenes:**   JSON (Loads animation data from another JSON file)
- **Textures:** PPM (P6 format, binary)
- **Models:**   PLY (ASCII 1.0, plain text, for texturing: supports Blender's S-H coordinates and a custom format of my own)


### Don't have time to write a readme currently, but here's an animation rendered with it (object position/rotation data extracted from Blender):

https://github.com/user-attachments/assets/fb152ac1-2d47-4b62-9231-f097cd12511b

### And an image:

![](scene_render_RAW_0.png)

### Example Blender script for exporting animation data (can't guarantee that it has no bug):
```python
import bpy
from numpy import degrees # can be replaced with math module

# Note: this only extract key-framed animations
# 1. bake all simulations
# 2. select the objects to bake
# 3. in Object menu at top-left, select [Object > Rigid Body > Bake to Keyframes]
# 4. now, run this script

# This brings up console for debug:
# bpy.ops.wm.console_toggle() 

# name of objects to exclude
exclude = ["0", "836"]

# path to save the json
file = open("path\\to\\your\\animscene.json", 'w')

# load scene, objects and frame numbers
scene = bpy.context.scene
frame_num = scene.frame_end - scene.frame_start + 1
objects = [o for o in bpy.context.scene.objects if o.name not in exclude]

file.write("{\n\t\"frames\" : %d,\n\t\"animated objects\" : %d,\n\t\"animations\" : [\n"%(frame_num, len(objects)))
template = "\t\t\t{\n\t\t\t\"pos\":[%.9f, %.9f, %.9f],\n\t\t\t\"rot\":[%.9f, %.9f, %.9f]\n\t\t\t},\n"
content = ""
for obj in objects:
    object_animation = "\t{\n\t\t\"id\" : \"%s\",\n\t\t\"transform\" : [\n"%obj.name
    animated_frames = ""
    for frame in range(scene.frame_start, scene.frame_end+1):
        scene.frame_set(frame)
        loc = obj.location
        rot = degrees(obj.rotation_euler)
        # print(frame, loc)
        animated_frames += template%(loc.x, loc.y, loc.z, rot[0], rot[1], rot[2])
    object_animation += (animated_frames[:-2] + "\n\t\t]\n\t},\n")
    content += object_animation
file.write(content[:-2] + "\n\t]\n}\n")
file.close()
```
