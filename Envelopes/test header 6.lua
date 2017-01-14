--[[
ReaScript name: js_LFO Tool (MIDI editor version, insert CCs in time selection in lane under mouse).lua
Version: 2.00
Author: juliansader    
]]

--[[
  Changelog:
  v2.00 (2017-01-15)
    + Much faster execution in large takes with hundreds aof thousands of events.
    + Keyboard shortcuts "a", "c" and "r" to switch GUI views.
    + LFO can be applied to existing events - including velocities - instead of inserting new CCs.
    + Requires REAPER v5.32 or later.  
]]
