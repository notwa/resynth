# resynth

resynthesis is the process of taking a source texture and
creating similar (oft tiling) textures based on it.

resynth is a C rewrite of the original resynthesizer by [Paul Harrison.][pfh]
it does not require GIMP to function; just feed it image paths and it will save copies.
however, this program lacks the patching (masking) and texture transfer abilities of the original plugin.

resynth produces [tiling images like those seen here.][examples]

[pfh]: http://logarithmic.net/pfh/
[examples]: http://logarithmic.net/pfh/resynthesizer/more

## related projects

* [the original resynthesizer GIMP plugin in C++][orig] and [the associated paper][paper]
* [an updated resynthesizer GIMP plugin in C and C++][gimp]
* [syntex: similar algorithms based on resynthesizer in C#][syntex]

[orig]: http://logarithmic.net/pfh/resynthesizer
[paper]: http://www.logarithmic.net/pfh-files/thesis/dissertation.pdf
[gimp]: https://github.com/bootchk/resynthesizer/
[syntex]: https://github.com/mxgmn/SynTex/

## help

```
usage:
  -a  --autism
        sensitivity to outliers
        range: [0,256];     default: 32
  -N  --neighbors
        points to use when sampling
        range: [0,1024];    default: 29
  -R  --circle-radius
        circle neighborhood radius
        range: [1,128];     default: [n/a]
  -M  --tries
        random points added to candidates
        range: [0,65536];   default: 192
  -m  --magic
        magic constant, affects iterations
        range: [0,255];     default: 192
  -s  --scale
        output size multiplier; negative values set width and height
        range: [-8192,32];  default: 1
  -S  --seed
        initial RNG value
                            default: 0 [time(0)]
  {files...}
        image files to open, resynthesize, and save as {filename}.resynth.png
        required            default: [none]
```

### neighborhood

offsets are sorted in ascending distance from the center (the 0,0 point).

the order of equal distances is undefined.
this doesn't matter much in practice,
considering the algorithm handles non-circular
neighborhoods just fine — more on that later.

consider the first 29 offsets,
which is the default number of neighbors:

* distance of 0:  
    `{ 0, 0}`
* distance of 1:  
    `{ 0,-1}, {+1, 0}, {-1, 0}, { 0,+1}`
* distance of sqrt(2):  
    `{-1,-1}, {-1,+1}, {+1,+1}, {+1,-1}`
* distance of 2:  
    `{+2, 0}, {-2, 0}, { 0,+2}, { 0,-2}`
* distance of sqrt(5):  
    `{-1,-2}, {-2,+1}, {-2,-1}, {+1,+2}, {-1,+2}, {+2,-1}, {+1,-2}, {+2,+1}`
* distance of sqrt(8):  
    `{+2,-2}, {-2,-2}, {-2,+2}, {+2,+2}`
* distance of 3:  
    `{+3, 0}, { 0,-3}, {-3, 0}, { 0,+3}`

the default neighborhood of 29 yields a (pixelated) circle,
as in this crude ascii art:
```
   X
 XXXXX
 XXXXX
XXXXXXX
 XXXXX
 XXXXX
   X
```

resynth provides a portion of [the associated integer sequence A057961,][A057961]
allowing the neighborhood to be specified as a radius-like size,
with guaranteed symmetry about the X and Y axes.
this is the `-R` flag, and the first few values are visualized here:
```
   -R1     -R2     -R3     -R4     -R5     -R6     -R7     -R8

                                                    X      XXX
                            X      XXX    XXXXX   XXXXX   XXXXX
            X      XXX     XXX    XXXXX   XXXXX   XXXXX  XXXXXXX
    X      XXX     XXX    XXXXX   XXXXX   XXXXX  XXXXXXX XXXXXXX
            X      XXX     XXX    XXXXX   XXXXX   XXXXX  XXXXXXX
                            X      XXX    XXXXX   XXXXX   XXXXX
                                                    X      XXX
                      equivalent to
   -N1     -N5     -N9     -N13    -N21    -N25    -N29    -N37
```
[A057961]: http://oeis.org/A057961

## notes

resynth includes the following header libraries:

* [stb\_image.h by nothings et al.][stb] (public domain / MIT)
* [stb\_image\_write.h by nothings et al.][stb] (public domain / MIT)
* [kyaa.h by notwa][kyaa] (public domain)
* [rnd.h by Mattias Gustavsson][rnd] (public domain / MIT)

[stb]: https://github.com/nothings/stb
[kyaa]: https://gist.github.com/notwa/5d287d807ffe11bbb553462c9940445c#file-kyaa-md
[rnd]: https://github.com/mattiasgustavsson/libs

compile like
```
clang -std=gnu11 -lm -Ofast -DNDEBUG resynth.c -o resynth
```
