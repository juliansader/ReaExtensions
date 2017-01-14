--[[
ReaScript name: Script: js_LFO Tool (MIDI editor version, insert CCs in time selection in lane under mouse).lua
Version: v2.00
Author: juliansader
Website: http://forum.cockos.com/showthread.php?t=177437
Screenshot: http://stash.reaper.fm/27716/LFO%20tool%20-%20MIDI%20editor%20%28default%29%20-%20Copy.gif
REAPER version: v5.32 or later
Extensions: None required
Donation: https://www.paypal.me/juliansader
About:
  # Description
  
  LFO generator and shaper - MIDI editor version
  
  Draw fancy LFO curves in REAPER's piano roll.
  
  This version applies the LFO to existing events -- velocities or CCs -- in the lane under mouse.
  
  # Instructions
  
  DRAWING ENVELOPES
  
  Leftclick in open space in the envelope drawing area to add an envelope node.
  
  Shift + Leftdrag to add multiple envelope nodes.
  
  Alt + Leftclick (or -drag) to delete nodes.
  
  Rightclick on an envelope node to open a dialog box in which a precise custom value can be entered.
  
  Move mousewheel while mouse hovers above node for fine adjustment.
  
  Use a Ctrl modifier to edit all nodes simultaneously:
  
  Ctrl + Leftclick (or -drag) to set all nodes to the mouse Y position.
  
  Ctrl + Rightclick to enter a precise custom value for all nodes.
  
  Ctrl + Mousewheel for fine adjustment of all nodes simultaneously.
  
  The keyboard shortcuts "a", "c" and "r" can be used to switch the envelope view between Amplitude, Center and Rate.
  

  VALUE AND TIME DISPLAY
  
  The precise Rate, Amplitude or Center of the hot node, as well as the precise time position, can be displayed above the node.
  
  Rightclick in open space in the envelope area to open a menu in which the Rate and time display formats can be selected.
  

  LOADING AND SAVING CURVES
  
  Right-click (outside envelope area) to open the Save/Load/Delete curve menu.
  
  One of the saved curves can be loaded automatically at startup. By default, this curve must be named "default".
  
                    
  FURTHER CUSTOMIZATION
  
  Further customization is possible - see the instructions in the script's USER AREA.
  
  This include:
  - Easily adding custom LFO shapes.
  - Specifying the resolution of LFO shapes' phase steps.
  - Specifying the resolution of the mousewheel fine adjustment.
  - Changing interface colors.
  - Changing the default curve name.
  - etc...      
]]

--[[
  Changelog:
  v2.00 (2017-01-15)
    + Much faster execution in large takes with hundreds aof thousands of events.
    + Keyboard shortcuts "a", "c" and "r" to switch GUI views.
    + LFO can be applied to existing events - including velocities - instead of inserting new CCs.
    + Requires REAPER v5.32 or later.  
]]
