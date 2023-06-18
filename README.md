# vrc-rtrti

THIS PROJECT IS ABANDONED SORRY.




## Notes on optimization:

```
 * Using CN's 2070, i7-8750H CPU @ 2.20GHz laptop
 * Only profiler visible. 
 * Mirror forced off.
 * Power Management set to "Consistent Performance"
 * General Power MAnager set to Power Saving
 * Disabled RTGI
 * 4096x2048 Target Texture
 
First test: 33.0ms
Runtime: 2m17.04s

Tying early abort for z-less: 31ms
Early sphere-too-big: 1m48.9 baketime.
Only compare volumes, triangularly: 0m50.764s
Change Memory structure to use pixel quad groups: 32ms.
Aggressively optimizing hulls (pull spheres tight): DOWN TO 16ms!!!
Now, bounding boxes.
Doing BB's yields: 16ms.
Trying W*H*L as the heuristic --> BAD --> 50ms.
A few more tweaks gets me to 14ms.

Today start at 45ms?
Switch to .Load() with floats: 45ms.
Swithched to integer .Load()s: 48ms.

Switch it all up: 20.7ms
