# UrFelt - deformable surfaces in the Urho3D game engine
This project combines my [Felt](https://github.com/feltech/Felt) deformable surface library with
the excellent open source [Urho3D](https://urho3d.github.io/) game engine.

A demo video is available at https://www.youtube.com/watch?v=KwpXzBb6Nzs

The MRI brain scan asset was obtained from [BrainWeb](http://www.bic.mni.mcgill.ca/brainweb/).

The business loging is written in Lua via [moonscript](https://moonscript.org/).  The key addition
to the [Felt](https://github.com/feltech/Felt) library are the additional
[Bullet](https://pybullet.org) collision shape classes, allowing for interaction with the physics
engine.

## Demo scripts

The default demo is of a surface in the shape of a brain, which can be deformed with mouse clicks
and have boxes thrown at it.

Press SPACE to enter/leave mouse capture mode, allowing the viewport to rotate as the mouse is
moved.

Use W, S, A, D, X, and Z keys to move around.

In mouse capture mode, left-click to throw a box.

In normal mouse mode, left-click on the surface to destroy and right-click to raise the surface.

Edit the `main.moon` (or `main.lua`, to avoid transpiling) file to show different demos.

Currently the only other demo is `segment`, which constructs the surface segmentation of grey
matter from an MRI image, and does not include any physics.
