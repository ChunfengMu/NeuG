
from building import *

cwd     = GetCurrentDir()
src     = Glob('src/*.c') + Glob('ports/*.c')
CPPPATH = [cwd + '/inc'] + [cwd + '/ports']

if GetDepend('PKG_USING_NEUG_EXAMPLE'):
    src += ['examples/neug_sample.c']

group = DefineGroup('NeuG', src, depend = ['PKG_USING_NEUG'], CPPPATH = CPPPATH)

Return('group')
