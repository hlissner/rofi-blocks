# SUBDIRS = src tests
ACLOCAL_AMFLAGS=-I m4
plugindir=$(XDG_CONFIG_HOME)/rofi/plugins
plugin_LTLIBRARIES=blocks.la
blocks_la_SOURCES=\
	src/blocks.c\
	src/blocks_mode_data.c\
	src/page_data.c\
	src/json_glib_extensions.c\
	src/string_utils.c\
	src/render_state.c
blocks_la_CFLAGS=$(glib_CFLAGS) $(pango_CFLAGS) $(cairo_CFLAGS)
blocks_la_LIBADD=$(glib_LIBS) $(pango_LIBS) $(cairo_LIBS)
blocks_la_LDFLAGS= -module -avoid-version
