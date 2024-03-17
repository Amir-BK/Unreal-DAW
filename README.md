## V0.0.1! First public release! Currently only the Midi Editor Widget is substantially developed, can also use the Decent Sampler importer to quickly make fusion patches. More to come! 

# Community & Support

https://discord.gg/hTKjSfcbEn


# ALPHA WARNING AND VERSIONING
As Unreal 5.4 is not even in early access to use this plugin you must sync to the Epic repo 5.4 branch, it is expected that this plugin will not maintain a stable API until full 5.4 release, expect changes to be breaking.

This plugin and repo will adhere to semantic versioning - https://semver.org/ - until we reach a V1.0 release all changes on main have the potential to break blueprints and existing APIs. 

## Packaging and Platform Status

Currently the plugin has packaging issues preventing it from being used in a packaged game or in mobile builds, this is (likely) due to warnings that need to be resolved in the codebase, by v1.0 and 5.4 release this plugin is meant to be cross-platform with no packaging issues for release. 

# Modules
## Runtime - BK Musical Widgets 
a collection of widgets that can be used both in Editor and in game (not yet!) to visualize and edit musical information, currently consisting of a Piano Roll graph and some basic transport widgets, planned to include full musical engraving capabilities using SMUFL fonts.

## Runtime - BK Music Core
Interactions between metasounds and the unreal world, the purpose of unreal daw is after all to create a dynamic musical session that can is controlled by the game world.

## Editor - BK Editor Utilities
Editor widget for editing midifiles and creating Unreal DAW musical scenes in the editor, factory classes for mass importing Decent Sampler and SFZ libraries into Fusion Sampler Patches 

# Current Features - 




# Roadmap Features

# Depenencies & third parties
All third party resources include their original licensing documents in the resources folder for the plugin, these should be included when used as per the specifications of the original licenses 
- SMUFL Fonts - Bravura
- SIMPLE UI icons 


