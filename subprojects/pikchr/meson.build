project('pikchr', 'c')
i = include_directories('.')
l = static_library('pikchr', 'pikchr.c', include_directories : i, install : false)

pikchr_dep = declare_dependency(include_directories : i,
  link_with : l)