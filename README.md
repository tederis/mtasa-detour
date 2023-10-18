# mtasa-detour
The server extension for MTA:SA that adds a navigation mesh
![mtasa-detour-preview](https://github.com/tederis/mtasa-detour/assets/12121551/1cf56319-6555-4253-aa5d-2ae5072c2a51)

Status
======

Work in progress.

Build instructions
======

NOTE: At the moment, Windows servers are only supported by this module. Linux support will be added later.
* Clone the repository.
* Download premake5 executable and put it in the project's root folder.
* Generate a project using the premake5 executable.
* Build a project

Functions
======
```lua
bool navState()
```
This function is used to return the current state of the navigation mesh. Returns *true* if the navmesh is loaded, *false* otherwise.

```lua
bool navLoad(string filename)
```
This function is used to load(reload) the navigation mesh from a previously generated file. Returns *true* if the navmesh is successfully loaded(reloaded), *false* otherwise.

```lua
bool navSave(string filename)
```
This function is used to save the navigation mesh to a file. Returns *true* if the navmesh is successfully saved, *false* otherwise.

```lua
bool navBuild()
```
This function is used to build the navigation mesh. The function is not saving a navigation mesh into a file, you can use *navSave* for this. Returns *true* if the navmesh is successfully built, *false* otherwise.

```lua
bool navFindPath(float startX, float startY, float startZ, float endX, float endY, float endZ)
```
This function is used to find a path between world space points. Returns *true* if the path was successfully found, *false* otherwise.

```lua
bool navNearestPoint(float x, float y, float z)
```
This function is used to find the nearest point on the navigation mesh to a given point. Returns *true* if the point was successfully found, *false* otherwise.

```lua
bool navDump(float x, float y, float z)
```
This function is used to dump the entire navigation mesh into a Wavefront *.obj* file. Can be used for the debug purposes. Returns a table of vertices if the dump was successfully created, *false* otherwise.

```lua
bool navCollisionMesh(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, float bias)
```
This function is used to dump collisions that are found in the region into a Wavefront *.obj* file. Can be used for the debug purposes. Returns a table of vertices if the dump was successfully created, *false* otherwise.

```lua
bool navNavigationMesh(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, float bias)
```
This function is used to dump navigation mesh tiles that are found in the region into a Wavefront *.obj* file. Can be used for the debug purposes. Returns a table of vertices if the dump was successfully created, *false* otherwise.

```lua
bool navScanWorld(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
```
This function is used to find objects in the specified region. Returns a table of model IDs in the following format: { { model }, { model }, ... }.

License
======
This repository is licensed under the GNU General Public License v3.0. Please review the license before using or contributing to this repository.
