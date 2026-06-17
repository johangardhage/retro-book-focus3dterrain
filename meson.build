project('retro-book-focus3dterrain', 'cpp',
  default_options: ['warning_level=1', 'buildtype=debug'])

sdl3 = dependency('sdl3')

dependencies = [sdl3]
cpp_args = []
link_args = []

if host_machine.system() == 'darwin'
  cpp_args += ['-DGL_SILENCE_DEPRECATION']
  link_args += ['-framework', 'OpenGL', '-Wl,-undefined,dynamic_lookup']
else
  dependencies += [
    dependency('gl'),
    dependency('glu'),
  ]
endif

demos = {
  'demo1_1': ['src/demo1_1.cpp', 'src/lib/roam.cpp'],
  'demo2_1': ['src/demo2_1.cpp', 'src/lib/terrain.cpp', 'src/lib/bruteforce.cpp', 'src/lib/image.cpp'],
  'demo2_2': ['src/demo2_2.cpp', 'src/lib/terrain.cpp', 'src/lib/bruteforce.cpp', 'src/lib/image.cpp'],
  'demo3_1': ['src/demo3_1.cpp', 'src/lib/terrain.cpp', 'src/lib/bruteforce.cpp', 'src/lib/image.cpp'],
  'demo3_2': ['src/demo3_2.cpp', 'src/lib/terrain.cpp', 'src/lib/bruteforce.cpp', 'src/lib/image.cpp'],
  'demo3_3': ['src/demo3_3.cpp', 'src/lib/terrain.cpp', 'src/lib/bruteforce.cpp', 'src/lib/image.cpp'],
  'demo4_1': ['src/demo4_1.cpp', 'src/lib/terrain.cpp', 'src/lib/bruteforce.cpp', 'src/lib/image.cpp'],
  'demo4_2': ['src/demo4_2.cpp', 'src/lib/terrain.cpp', 'src/lib/bruteforce.cpp', 'src/lib/image.cpp'],
  'demo4_3': ['src/demo4_3.cpp', 'src/lib/terrain.cpp', 'src/lib/bruteforce.cpp', 'src/lib/image.cpp'],
  'demo5_1': ['src/demo5_1.cpp', 'src/lib/terrain.cpp', 'src/lib/geomipmapping.cpp', 'src/lib/image.cpp'],
  'demo5_2': ['src/demo5_2.cpp', 'src/lib/terrain.cpp', 'src/lib/geomipmapping.cpp', 'src/lib/image.cpp'],
  'demo6_1': ['src/demo6_1.cpp', 'src/lib/terrain.cpp', 'src/lib/quadtree.cpp', 'src/lib/image.cpp'],
  'demo6_2': ['src/demo6_2.cpp', 'src/lib/terrain.cpp', 'src/lib/quadtree.cpp', 'src/lib/image.cpp'],
  'demo6_3': ['src/demo6_3.cpp', 'src/lib/terrain.cpp', 'src/lib/quadtree.cpp', 'src/lib/image.cpp'],
  'demo7_1': ['src/demo7_1.cpp', 'src/lib/roamlod.cpp', 'src/lib/terrain.cpp', 'src/lib/image.cpp'],
  'demo7_2': ['src/demo7_2.cpp', 'src/lib/terrain.cpp', 'src/lib/roamfrustum.cpp', 'src/lib/image.cpp'],
  'demo7_3': ['src/demo7_3.cpp', 'src/lib/terrain.cpp', 'src/lib/roamdiamond.cpp', 'src/lib/image.cpp'],
  'demo7_4': ['src/demo7_4.cpp', 'src/lib/terrain.cpp', 'src/lib/roamsplitmerge.cpp', 'src/lib/image.cpp'],
  'demo8_1': ['src/demo8_1.cpp', 'src/lib/terrain.cpp', 'src/lib/roamsplitmerge.cpp', 'src/lib/water.cpp', 'src/lib/image.cpp'],
  'demo8_2': ['src/demo8_2.cpp', 'src/lib/terrain.cpp', 'src/lib/roamsplitmerge.cpp', 'src/lib/watersim.cpp', 'src/lib/image.cpp'],
  'demo8_3': ['src/demo8_3.cpp', 'src/lib/terrain.cpp', 'src/lib/roamsplitmerge.cpp', 'src/lib/watersim.cpp', 'src/lib/skybox.cpp', 'src/lib/image.cpp'],
  'demo8_4': ['src/demo8_4.cpp', 'src/lib/terrain.cpp', 'src/lib/roamsplitmerge.cpp', 'src/lib/watersim.cpp', 'src/lib/skydome.cpp', 'src/lib/image.cpp'],
  'demo8_5': ['src/demo8_5.cpp', 'src/lib/terrain.cpp', 'src/lib/roamsplitmerge.cpp', 'src/lib/watersim.cpp', 'src/lib/skydome.cpp', 'src/lib/image.cpp'],
  'demo8_6': ['src/demo8_6.cpp', 'src/lib/terrain.cpp', 'src/lib/roamsplitmerge.cpp', 'src/lib/watersim.cpp', 'src/lib/skydome.cpp', 'src/lib/image.cpp'],
  'demo8_7': ['src/demo8_7.cpp', 'src/lib/terrain.cpp', 'src/lib/roamsplitmerge.cpp', 'src/lib/watersim.cpp', 'src/lib/skydome.cpp', 'src/lib/image.cpp'],
  'demo8_8a': ['src/demo8_8a.cpp', 'src/lib/terrain.cpp', 'src/lib/geomipmapping.cpp', 'src/lib/watersim.cpp', 'src/lib/skydome.cpp', 'src/lib/image.cpp'],
  'demo8_8b': ['src/demo8_8b.cpp', 'src/lib/terrain.cpp', 'src/lib/geomipmapping.cpp', 'src/lib/watersim.cpp', 'src/lib/skydome.cpp', 'src/lib/image.cpp'],
  'demo8_9': ['src/demo8_9.cpp', 'src/lib/particle.cpp'],
  'demo8_10': ['src/demo8_10.cpp', 'src/lib/particlesystem.cpp', 'src/lib/image.cpp'],
  'demo8_11': ['src/demo8_11.cpp', 'src/lib/particlesystem.cpp', 'src/lib/image.cpp'],
  'demo8_12': ['src/demo8_12.cpp', 'src/lib/terrain.cpp', 'src/lib/geomipmapping.cpp', 'src/lib/watersim.cpp', 'src/lib/skydome.cpp', 'src/lib/particlesystem.cpp', 'src/lib/image.cpp'],
}

foreach name, sources : demos
  executable(name, sources,
    dependencies: dependencies,
    cpp_args: cpp_args,
    link_args: link_args)
endforeach
