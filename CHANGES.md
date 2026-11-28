# SOLARIUM-23A – changes

The newest first. To release a new version: raise the number in VERSION and put its notes here under
"## <the new number>". After the merge into main the release is made by itself (auto-release.yml), the
downloads are built into it, and the website and the Discord post follow.

## 1.4.0
Sharing sounds made easy: the SAVE window has two new buttons.
- **Folder** opens the folder with your own presets in Explorer, Finder or the file manager.
- **Import...** takes in presets from someone else: pick one or more `.s23` files, or a `.zip` of them. They show up in the list right away; a preset you already have is not replaced.

## 1.3.5
After saving a sound of your own, the plugin now stays on it (before, Cubase, FL Studio and other VST3 hosts switched to another patch, so the new one had to be loaded again).

## 1.3.4
New: zoom into any section. Click a section's name tab (with the small magnifier) and that section fills the window, sharp and fully playable. Great on small laptop screens. Click outside it, on the X, or on the tab again to see the whole panel.

## 1.3.3
Resizing the window is smooth now. Before, the whole panel was redrawn at every step while you dragged its corner, so it stuttered badly. Now the panel is only stretched while you drag and drawn sharp again the moment you let go.

## 1.3.2
New: SOLARIUM-23A for Linux - VST3, LV2 and a standalone app (64-bit, built on Ubuntu 22.04). Checked with pluginval like the Windows version.

## 1.3.1
The window opens as large as you left it - also after closing and reopening the DAW (before, it came back at the smallest size). A new instance starts at the size you used last.
Saving your own sounds fixed: the save window now opens inside the plugin (before, it could open hidden behind the DAW's plugin window, so nothing got saved). If Windows keeps apps out of the Documents folder, your sounds are saved in AppData instead. A failed save now says so.

## 1.3
(see the release page on GitHub)
