# LadderRambos
A L4D2 Sourcemod extension that allows Survivors to shoot their guns on ladders. Works only on Linux. Nuff said.

# Compilation
You will need any distro/version of Linux with any version of GCC in it.

You will also need Metamod, Sourcemod and L4D2 SDK headers. Get the from [here](https://github.com/alliedmodders).

Ignore the compiler warnings.

# Installation
Throw the extension and the `autoload` file into your `addons/sourcemod/extensions` folder.

Throw the gamedata into `addons/sourcemod/gamedata`.

Restart your server.

# Configuration
Just a couple cvars to get you started.

* **cssladders_version** - you don't really need to change this
* **cssladders_enabled** - 1 to enable, 0 to disable the extension
* **cssladders_allow_m2** - 1 to allow M2(shoves) on ladders, 0 to block
* **cssladders_allow_reload** - 1 to allow reloading on ladders, 0 to block. Keep in mind that shotguns are broken and won't reload on ladders no matter what.

# Credits
All credit for this amazing extension goes solely to [**Ilya 'Visor' Komarov**](http://steamcommunity.com/id/Marauder_Shields/) aka **Attano**.

(c) 2015
Designed for [Equilibrium 3.0 Competitive Mod](http://www.l4dnation.com/community-news/equilibrium-3-0/)
