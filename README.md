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
        range: (0,256);     default: 32
  -N  --neighbors
        points to use when sampling
        range: (0,1024);    default: 29
  -r  --radius
        square neighborhood, always odd
        range: (0,32);      default: [n/a]
  -R  --circle-radius
        circle neighborhood radius
        range: (1,128);     default: [n/a]
  -M  --tries
        random points added to candidates
        range: (0,65536);   default: 192
  -p  --polish
        extra iterations
        range: (0,9);       default: 0
  -m  --magic
        magic constant, affects iterations
        range: (0,255);     default: 192
  -s  --scale
        output size multiplier; negative values set width and height
        range: (-8192,32);  default: 1
  -S  --seed
        initial RNG value
        range: (0,);        default: 0 [time(0)]
  {files...}
        image files to open, resynthesize, and save as {filename}.resynth.png
        required            default: [none]
```

apologies for the `radius`, `circle-radius`, and `polish` parameters.
they should be properly described or removed.

## notes

includes header libraries from [nothings (stb)][stb] and [notwa (kyaa).][kyaa]

[stb]: https://github.com/nothings/stb
[kyaa]: https://gist.github.com/notwa/5d287d807ffe11bbb553462c9940445c#file-kyaa-md

compile like
```
clang -std=gnu11 -lm -Ofast resynth.c -o resynth
```

extra flags that i use are
```
-Wall -Wextra -Werror=implicit-function-declaration -Winline -Wshadow
-Wno-unused -Wno-padded -Wno-missing-field-initializers -Wno-misleading-indentation -Wno-disabled-macro-expansion -Wno-language-extension-token -Wno-unknown-warning-option
```
