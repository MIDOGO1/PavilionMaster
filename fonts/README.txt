FONTS BUNDLED INTO THE APP
==========================

Every .ttf / .otf file in this folder is embedded in the executable at build time
(see CMakeLists.txt) and registered at startup.

Inter  - all general UI text (static files, 18pt / 24pt / 28pt optical sizes).
         The QML uses Light, Medium, DemiBold and Bold. Full license: Inter-OFL.txt.
AKONY  - "TURBO ON" / "AUTO" and large wattage numbers. License not verified; check before publishing.
Angled - CPU/GPU temperature and "TOTAL" labels (file: Angled 1.otf, family "Angled").
         License not verified; check before publishing.

Inter variable fonts (Inter-VariableFont_*.ttf) are not included: they register
overlapping family names with the static files.

After changing this folder: Build > Run CMake, rebuild, then check control_center.log
for "Loaded bundled font" lines to confirm the family names.
