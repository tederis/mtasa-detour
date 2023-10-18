# mtasa-detour
The server extension for MTA:SA that adds navigation mesh tools.
![social_mini](https://github.com/tederis/mtasa-detour/assets/12121551/0e0ab1af-3a07-4456-8934-4ab82acbc54f)

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

Installation
======

32 bit: Copy 32 bit navigation.dll into the server\mods\deathmatch\modules\ directory.

64 bit: Copy 64 bit navigation_64.dll into the server\x64\modules\ directory.

Copy(unzip) the contents of navmesh.zip into the server\navmesh\ directory. This directory must contain three files: cols.col, defs.xml and nodes.xml.

Then, add the following line in mtaserver.conf:
```xml
  <module src="navigation.dll" />
```

**NOTE:** The module is memory intensive and server must have at least 600 MB of free process memory. This limitation will be omitted later.

Description
======

This project aims at cover the wide spectrum of applications. Its kernel is the navigation mesh tools that allow to effectively find a path between world space points and query auxiliary data on a pregenerated data set.

It consists of two parts: *game assets aggregator* and *server module*. Сonsider them in more detail.

Game assets aggregator is responsible for loading game assets, their processing and creating compact files contain all data necessary for the navigation mesh generating. It is placed in a separate tool called **builder**. Before a server is available to generate a navigation mesh this tool should be used to build a game data. For generating a game data run builder.exe with the following arguments:
```
builder -g GTASA_DIRECTORY -o SERVER_DIRECTORY
```

**NOTE:** You do not have to build the data yourself. The pregenerated data is available in the Releases section(see navmesh.zip).

Server module is responsible for the actual navigation mesh processing, including the navigation mesh building. Immediately after the launch of server navigation mesh is unloaded. To use it you have to build it (see *navBuild* function) or load from a file(see *navLoad* function). Once that is done all navigation mesh functions become available. 

Videos
======
[https://youtu.be/8QdN8mQOP5w?si=TV2siFsuudmuf7r2](https://youtu.be/8QdN8mQOP5w?si=R34wqrx0_34XwEYp)

[https://youtu.be/40tvi-jHtNY?si=9wB57lJb-6nrqrma](https://youtu.be/40tvi-jHtNY?si=9wB57lJb-6nrqrma)

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
This function is used to build the navigation mesh. The function is not saving a navigation mesh into  a file, you can use *navSave* for this. Returns *true* if the navmesh is successfully built, *false* otherwise. Note that this function is CPU extensive and the building process can freeze your server for a while. In the next version the building process will be asynchronous.

```lua
table navFindPath(float startX, float startY, float startZ, float endX, float endY, float endZ)
```
This function is used to find a path between world space points. Returns table of points if the path was successfully found, *false* otherwise.

```lua
float, float, float navNearestPoint(float x, float y, float z)
```
This function is used to find the nearest point on the navigation mesh to a given point. Returns position if the point was successfully found, *false* otherwise.

```lua
bool navDump(string filename)
```
This function is used to dump the entire navigation mesh into a Wavefront *.obj* file. Can be used for the debug purposes. Returns true if the dump was successfully created, *false* otherwise.

```lua
table navCollisionMesh(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, float bias)
```
This function is used to dump collision vertices that are found in the region into a table. Can be used for the debug purposes. Returns a table of vertices if the dump was successfully created, *false* otherwise.

```lua
table navNavigationMesh(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, float bias)
```
This function is used to dump navigation mesh vertices that are found in the region into a table. Can be used for the debug purposes. Returns a table of vertices if the dump was successfully created, *false* otherwise.

```lua
table navScanWorld(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
```
This function is used to find objects in the specified region. Returns a table of model IDs in the following format: { { model }, { model }, ... }.

License
======
This repository is licensed under the GNU General Public License v3.0. Please review the license before using or contributing to this repository.
