ReactiViz Read Me Instructions
==========================

Windows
-------

First, download **Visual Studio Community 2019**.
Make sure to install the C++ development tools.

At this point, you can choose to follow the "Lab machines (Windows)" instructions above.

Or, if you would rather do CMake (or if you have a different version of Visual Studio and don't want to change),
follow these steps:

Download these:

- CMake (<http://cmake.org/download/>). Make sure to add CMake to the system
  path when asked to do so.
- [vcpkg](https://github.com/Microsoft/vcpkg)

Make sure the command `cmake` works from the command prompt.

Now use `vcpkg` to install `glm` and `glfw3`


Building and Running the Lab/Assignment
=======================================

All platforms (except Lab windows)
----------------------------------

We'll perform an "out-of- source" build, which means that the binary files
will not be in the same directory as the source files. In the folder that
contains CMakeLists.txt, run the following.

	> mkdir build
	> cd build

Then run one of the following *from the build folder*, depending on your
choice of platform and IDE.


OSX & Linux Makefile
--------------------

	> cmake ..

This will generate a Makefile that you can use to compile your code. To
compile the code, run the generated Makefile.

	> make -j4

The `-j` argument speeds up the compilation by multithreading the compiler.
This will generate an executable, which you can run by typing

	> ./Lab3 ../resources

To build in release mode, use `ccmake ..` and change `CMAKE_BUILD_TYPE` to
`Release`. Press 'c' to configure then 'g' to generate. Now `make -j4` will
build in release mode.

To change the compiler, read [this
page](http://cmake.org/Wiki/CMake_FAQ#How_do_I_use_a_different_compiler.3F).
The best way is to use environment variables before calling cmake. For
example, to use the Intel C++ compiler:

	> which icpc # copy the path
	> CXX=/path/to/icpc cmake ..


OSX Xcode
---------

	> cmake -G Xcode ..

This will generate `Lab3.xcodeproj` project that you can open with Xcode.

- To run, change the target to `Lab00` by going to Product -> Scheme -> Lab00.
  Then click on the play button or press Command+R to run the application.
- Edit the scheme to add command-line arguments (`../../resources`) or to run
  in release mode.


Windows Visual Studio 2017
--------------------------

The provided `.sln` should work out of the box, provided you installed the two dependencies.

- To build and run the project, right-click on `Lab3` in the project explorer
  and then click on "Set as Startup Project." Then press F7 (Build Solution)
  and then F5 (Start Debugging).
- To add a commandline argument, right-click on `Lab3` in
  the project explorer and then click on "Properties" and then click on
  "Debugging."


Windows Visual Studio 2015
--------------------------

Or on your own machine, if you would prefer to use CMake.

Use [vcpkg](https://github.com/Microsoft/vcpkg) to install `glfw3` and `glm`.

	> cmake ..

By default on Windows, CMake will generate a Visual Studio solution file,
`Lab00.sln`, which you can open by double-clicking. If you get a version
mismatch, you may have to specify your visual studio version, for example:

	> cmake -G "Visual Studio 14 2015" ..

Other versions of Visual Studio are listed on the CMake page
(<http://cmake.org/cmake/help/latest/manual/cmake-generators.7.html>).

- To build and run the project, right-click on `Lab3` in the project explorer
  and then click on "Set as Startup Project." Then press F7 (Build Solution)
  and then F5 (Start Debugging).
- To add a commandline argument, right-click on `Lab3` in
  the project explorer and then click on "Properties" and then click on
  "Debugging."

