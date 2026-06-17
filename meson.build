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
  'demo2_1': ['src/demo2_1.cpp', 'src/lib/terrain.cpp', 'src/lib/bruteforce.cpp'],
  'demo2_2': ['src/demo2_2.cpp', 'src/lib/terrain.cpp', 'src/lib/bruteforce.cpp'],
}

foreach name, sources : demos
  executable(name, sources,
    dependencies: dependencies,
    cpp_args: cpp_args,
    link_args: link_args)
endforeach
