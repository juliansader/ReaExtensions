# ReaExtensions

Extensions for REAPER.

The easiest way to install these extensions is via [ReaPack](https://reapack.com) -- a package manager for Reaper.  If ReaTeam/Extensions is not listed among your default repositories, you can add the repository manually with this [URL](https://github.com/ReaTeam/Extensions/raw/master/index.xml).  Alternatively, you can download the extensions directly from [GitHub](https://github.com/juliansader/ReaExtensions/tree/master/js_ReaScriptAPI/) and copy the file to REAPER's /UserPlugins directory.

The purpose of the ReaScriptAPI extension is to make all the useful and powerful functions that are available to C++ extensions available to ReaScripts too.  Most -- but not all -- of the functions are just an interface to the corresponding C++ functions of the Win32 (for Windows) or WDL/swell (for Linux and macOS) API, so the online documentation for Win32 programming is probably the best place to learn how to use these functions.

Any questions can be posted in the [js_ReaScriptAPI extension thread](https://forum.cockos.com/showthread.php?t=212174) on the Cockos forums.
