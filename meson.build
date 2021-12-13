project(
	'nson',
	['c'],
	default_options : [
		'c_std=c18',
		'optimization=2',
		'werror=true',
		'warning_level=1'
	],
	version: '0.0.0',
	meson_version: '>=0.57'
)

header = [
	'src/config.h',
	'src/nson.h',
	'src/internal.h',
]

source = [
	'src/ini.c',
	'src/nson.c',
	'src/scalar.c',
	'src/stack.c',
	'src/pointer.c',
	'src/buf.c',
	'src/plist.c',
	'src/util.c',
	'src/map_reduce.c',
	'src/json.c',
	'src/object.c',
	'src/array.c',
	'src/data.c',
]

test = [
'test/ini.c',
'test/map_reduce.c',
'test/plist.c',
'test/pointer.c',
'test/data.c',
'test/json.c',
]

build_args = [
	'-DVERSION="' + meson.project_version() + '"',
]

dependencies = [
	dependency('threads')
]

nson = library(
  'nson',
  source,
  install: not meson.is_subproject(),
  c_args : build_args,
  include_directories : 'src',
  dependencies : dependencies,
)

if get_option('test')
	foreach p : test
		t = executable(p.underscorify(),
			[ p ],
			install : false,
			c_args : build_args,
			link_with : nson
		)
		test(p, t)
	endforeach
endif
