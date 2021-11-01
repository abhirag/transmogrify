project('transmogrify', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 
                     'c_std=c11'])

md4c_sp = subproject('md4c')
md4c = md4c_sp.get_variable('md4c_dep')

sds_sp = subproject('sds')
sds = sds_sp.get_variable('sds_dep')

fe_sp = subproject('fe')
fe = fe_sp.get_variable('fe_dep')

log_sp = subproject('log')
log = log_sp.get_variable('log_dep')

inc = include_directories('include')
transmogrify_src = files('src/transmogrify.c')
lisp_src = files('src/lisp.c')

subdir('tests')

transmogrify = executable('transmogrify', 
                          [transmogrify_src, lisp_src, 'src/main.c'],
                          include_directories: inc,
                          dependencies: [md4c, sds, fe, log])







