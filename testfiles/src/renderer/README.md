
# Pixel Filter Tests

These test the specific functionality of the filters, but not their integration.

# Surface Filter Tests

These test how well the filters integrate into the surface filter runner.

# Context Tests

Each of the main context drawinng instructions are tested here.

# Drawing Tests

Anything specific to feeding an SVG into the drawing engine.

# Debugging

The tests provide a way to get debugging information during a run. Add the environment variable `DEBUG_PNG` to produce png files which can be visually inspected.

# PNG 16

Tests will often depend on a PNG output, if you use PNG8 images which is the default, Cairo will load them as INT8 RGB surfaces and this will change what the tests do and how fast they are able to run. To improve the speed or change what is being tested, convert the PNGs to INT16 (ImageMagick can do this for you) which forces Cairo to load them into a Float128 surface instead.

