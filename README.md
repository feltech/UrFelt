# UrFelt - deformable surfaces in the Urho3D game engine
This project combines my [Felt](https://github.com/feltech/Felt) deformable surface library with
the excellent open source [Urho3D](https://urho3d.github.io/) game engine.

A demo video is available at https://www.youtube.com/watch?v=KwpXzBb6Nzs

The MRI brain scan asset was obtained from [BrainWeb](http://www.bic.mni.mcgill.ca/brainweb/).

The business logic is written in Lua via [moonscript](https://moonscript.org/).  The key addition
to the [Felt](https://github.com/feltech/Felt) library are the additional
[Bullet](https://pybullet.org) collision shape classes, allowing for interaction with the physics
engine.

Edit the `main.moon` (or `main.lua`, to avoid transpiling) file to show different demos.
