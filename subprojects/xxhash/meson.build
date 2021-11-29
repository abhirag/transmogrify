project('xxhash', 'c')
i = include_directories('.')
l = static_library('xxhash', 'xxhash.c', include_directories : i, install : false)

xxhash_dep = declare_dependency(include_directories : i,
  link_with : l)