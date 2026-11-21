Third-party runtime files (not built from this repo) go in THIS folder.
The build copies them into the release bundle next to the app. Required:

  ryzenadj-real.exe   <- RyzenAdj's ryzenadj.exe, RENAMED to ryzenadj-real.exe
  libryzenadj.dll     <- from RyzenAdj
  WinRing0x64.dll
  WinRing0x64.sys

Easiest source: your current working install in C:\Program Files\Pavilion Master
(rename ryzenadj.exe there to ryzenadj-real.exe ONLY if that file is the real RyzenAdj,
not the shim built from extras-launcher-shim/ryzenadj_shim.cpp).
