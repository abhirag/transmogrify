project('fe', 'c')
i = include_directories('.')
l = static_library('fe', 'fe.c', include_directories : i, install : false)

fe_dep = declare_dependency(include_directories : i,
  link_with : l)