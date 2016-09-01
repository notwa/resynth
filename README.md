## related projects

* [the original resynth gimp plugin in C++][orig]
* [an updated gimp plugin in C and C++][gimp]
* [syntex: similar algorithms based on resynth][syntex]

[orig]: http://logarithmic.net/pfh/resynthesizer
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
