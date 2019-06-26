/*
    resynth - A program for resynthesizing textures.
    modified by Connor Olding, 2016
    Copyright (C) 2000 2008  Paul Francis Harrison
    Copyright (C) 2002  Laurent Despeyroux
    Copyright (C) 2002  David Rodríguez García

    This program is licensed under the terms of the GNU General Public
    License (version 2), and is distributed without any warranty.
    You should have received a copy of the license with the program.
    If not, visit <http://gnu.org/licenses/> to obtain one.
*/

#include <limits.h>
#include <math.h> // for log (neglog_cauchy) used in pixel diff. calculations
#include <stdbool.h> // we're targetting C11 anyway, may as well use it
#include <stdint.h> // for uint8_t for pixel data
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for file extension mangling
#include <time.h> // for time(0) as a random seed

// decide which features we want from stb_image.
// this should cover the most common formats.
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#include "stb_image.h"

// likewise for stb_image_write. by using the static keyword,
// any unused formats and functions can be stripped from the resulting binary.
// we only use png (stbi_write_png) in this case.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#include "stb_image_write.h"
#pragma GCC diagnostic pop

// this isn't the prettiest way of handling memory errors,
// but it should suffice for our one-thing one-shot program.
#define STRETCHY_BUFFER_OUT_OF_MEMORY \
    fprintf(stderr, "fatal error: ran out of memory in stb__sbgrowf\n"); \
    exit(1);
// provides vector<>-like arrays of variable size.
#include "stretchy_buffer.h"

// for command-line argument parsing
#include "kyaa.h"
#include "kyaa_extra.h"

// rand() is neither consistent across platforms
// nor guaranteed to have desirable properties,
// so we use this random number generator instead.
#define RND_U32 uint32_t
#define RND_U64 uint64_t
#define RND_IMPLEMENTATION
#include "rnd.h"
rnd_pcg_t pcg;

// convenience macros. hopefully these names don't interfere
// with any defined in the standard library headers on any system.
// it's technically not an error to redefine macros anyway.
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, l, u) (MIN(MAX(x, l), u))
#define CLAMPV(x, l, u) x = CLAMP(x, l, u)
#define LEN(a) (sizeof(a) / sizeof((a)[0]))

// this macro lets us state how much memory we want a pointer to employ,
// while freeing its old memory as needed.
// note that this expects the given variable to be initialized to NULL
// for its first allocation; using an uninitialized pointer is erroneous.
// (this should probably be renamed to something more meaningful)
#define MEMORY(a, size) \
    do { \
        if (a) (a) = (free(a), NULL); \
        if (size > 0) (a) = (typeof(a))(calloc(size, sizeof((a)[0]))); \
    } while (0) \

// a simple extension to stretchy_buffer.h:
// a macro that frees and nulls a pointer at the same time,
// which should be safer and easier to program with.
#define sb_freeset(a) ((a) = (sb_free(a), NULL))

// in some cases, we must declare functions as static to prevent
// their symbols being exported, thus truly allowing them to be inlined.
#define INLINE static inline

// end of generic boilerplate, here's the actual program:

typedef struct coord {
    int x, y;
} Coord;

INLINE Coord coord_add(const Coord a, const Coord b) {
    return (Coord){a.x + b.x, a.y + b.y};
}

INLINE Coord coord_sub(const Coord a, const Coord b) {
    return (Coord){a.x - b.x, a.y - b.y};
}

static int coord_compare(const void *v_a, const void *v_b) {
    const Coord *a = (Coord *) v_a;
    const Coord *b = (Coord *) v_b;
    return (a->x * a->x + a->y * a->y) - (b->x * b->x + b->y * b->y);
}

typedef uint8_t Pixel;

typedef union {
    Pixel v[4];
    struct {
        Pixel r, g, b, a;
    };
} Pixel32;

typedef struct {
    bool has_value, has_source;
    Coord source;
} Status;

typedef struct {
    int width, height, depth;
} Image;

// convenience macros to simplify image handling.
// note that these secretly expect an image##_array variable to exist.
#define IMAGE_RESIZE(image, w, h, d) \
    do { \
        image.width = w; \
        image.height = h; \
        image.depth = d; \
        MEMORY(image##_array, w * h * d); \
    } while (0) \

#define image_at(image, x, y) (image##_array + (y * image.width + x) * image.depth)
#define image_atc(image, coord) image_at(image, coord.x, coord.y)

typedef struct {
    bool h_tile, v_tile;
    double autism;
    int neighbors, tries;
    int magic;
} Parameters;

INLINE bool wrap_or_clip(const Parameters parameters, const Image image,
                         Coord *point) {
    // like modulo, with optional bounds checking.
    // (perhaps we should just use modulo if h_tile and v_tile are both true?)
    while (point->x < 0) {
        if (parameters.h_tile) point->x += image.width;
        else return false;
    }
    while (point->x >= image.width) {
        if (parameters.h_tile) point->x -= image.width;
        else return false;
    }
    while (point->y < 0) {
        if (parameters.v_tile) point->y += image.height;
        else return false;
    }
    while (point->y >= image.height) {
        if (parameters.v_tile) point->y -= image.height;
        else return false;
    }
    return true;
}

typedef struct {
    int input_bytes;
    // note that these variables must exist alongside their "_array"s
    // for the image macros to work.
    Image data, corpus, status;
    Pixel *data_array, *corpus_array;
    Status *status_array;
    Coord *data_points, *corpus_points, *sorted_offsets;
    Image tried;
    int *tried_array;

    Coord *neighbors;
    Pixel32 *neighbor_values;
    Status **neighbor_statuses;
    int n_neighbors;

    int *diff_table; // (might be more efficient to store as uint16_t?)

    int best;
    Coord best_point;
} Resynth_state;

static void state_free(Resynth_state *s) {
    sb_freeset(s->data_points);
    sb_freeset(s->corpus_points);
    sb_freeset(s->sorted_offsets);
    MEMORY(s->diff_table, 0);
    MEMORY(s->neighbors, 0);
    MEMORY(s->neighbor_values, 0);
    MEMORY(s->neighbor_statuses, 0);
    MEMORY(s->data_array, 0);
    MEMORY(s->corpus_array, 0);
    MEMORY(s->status_array, 0);
    MEMORY(s->tried_array, 0);
}

static double neglog_cauchy(double x) {
    return log(x * x + 1.0);
}

static void make_offset_list(Resynth_state *s) {
    // generate a vector of x,y offsets used to search around any given pixel.
    // this is constrained by the minimum image size to prevent overlapping.
    int width = MIN(s->corpus.width, s->data.width);
    int height = MIN(s->corpus.height, s->data.height);

    sb_freeset(s->sorted_offsets);
    for (int y = -height + 1; y < height; y++) {
        for (int x = -width + 1; x < width; x++) {
            Coord c = {x, y};
            sb_push(s->sorted_offsets, c);
        }
    }

    // sort offsets in ascending distance from the center zero-point.
    // the order of equal distances is undefined. (it doesn't seem to matter)
    // (for a more detailed description of what's going on, refer to the README)
    qsort(s->sorted_offsets, sb_count(s->sorted_offsets),
          sizeof(Coord), coord_compare);
}

INLINE void try_point(Resynth_state *s, const Coord point) {
    // consider a candidate pixel for the best-fit by considering its neighbors.
    int sum = 0;

    for (int i = 0; i < s->n_neighbors; i++) {
        Coord off_point = coord_add(point, s->neighbors[i]);

        int diff = 0;
        if (off_point.x < 0 || off_point.y < 0 ||
            off_point.x >= s->corpus.width || off_point.y >= s->corpus.height) {
            // penalize edges, assuming the corpus image doesn't wrap cleanly.
            diff = s->diff_table[0] * s->input_bytes;
        } else if (i) {
            const Pixel *corpus_pixel = image_atc(s->corpus, off_point);
            const Pixel *data_pixel = s->neighbor_values[i].v;
            for (int j = 0; j < s->input_bytes; j++) {
                diff += s->diff_table[256 + data_pixel[j] - corpus_pixel[j]];
            }
        }

#ifdef NDEBUG
        sum += diff;
#else
        if (__builtin_add_overflow(sum, diff, &sum)) {
            fprintf(stderr, "integer overflow at (%i,%i) + (%i,%i)\n",
                    point.x, point.y, s->neighbors[i].x, s->neighbors[i].y);
            fprintf(stderr, "diff: %i\n", diff);
            exit(1);
        }
#endif
        if (sum >= s->best) return;
    }

    s->best = sum;
    s->best_point = point;
}

INLINE void resynth__init(Resynth_state *s, Parameters parameters) {
    sb_freeset(s->data_points);
    sb_freeset(s->corpus_points);
    sb_freeset(s->sorted_offsets);

    // (iirc i opted to put diff_table on heap to keep Resynth_state small)
    MEMORY(s->diff_table, 512);
    MEMORY(s->neighbors, parameters.neighbors);
    MEMORY(s->neighbor_values, parameters.neighbors);
    MEMORY(s->neighbor_statuses, parameters.neighbors);

    IMAGE_RESIZE(s->status, s->data.width, s->data.height, 1);

    // set default values and allocate points to shuffle later.
    for (int y = 0; y < s->status.height; y++) {
        for (int x = 0; x < s->status.width; x++) {
            // (this might be redundant since this memory is calloc'd)
            image_at(s->status, x, y)->has_source = false;
            image_at(s->status, x, y)->has_value = false;

            Coord coord = {x, y};
            sb_push(s->data_points, coord);
        }
    }

    // likewise for the corpus.
    for (int y = 0; y < s->corpus.height; y++) {
        for (int x = 0; x < s->corpus.width; x++) {
            Coord coord = {x, y};
            sb_push(s->corpus_points, coord);
        }
    }

    if (!sb_count(s->corpus_points) || !sb_count(s->data_points)) {
        fprintf(stderr, "invalid sizes\n");
        fprintf(stderr, "corpus: %i\n", sb_count(s->corpus_points));
        fprintf(stderr, "data: %i\n", sb_count(s->data_points));
        return;
    }

    make_offset_list(s);

    // precompute how "different" a pixel value is from another.
    // this greatly affects how apparent any seams are in the synthesized image.
    // this is done per 8-bit channel, so only 256 * 2 values are needed.
    // since we can't use negative indices, we pretend index 256 is 0 instead.
    // (you could try adding CIELAB heuristics, but this seems robust enough)
    if (parameters.autism > 0) for (int i = -256; i < 256; i++) {
        double value = neglog_cauchy(i / 256.0 / parameters.autism) /
                       neglog_cauchy(1.0 / parameters.autism) * 65536.0;
        s->diff_table[256 + i] = (int)(value);
    } else for (int i = -256; i < 256; i++) {
        s->diff_table[256 + i] = (int)(i != 0) * 65536;
    }

    const int data_area = sb_count(s->data_points);

    // shuffle the data points in-place.
    for (int i = 0; i < data_area; i++) {
        int j = rnd_pcg_range(&pcg, 0, data_area - 1);
        Coord temp = s->data_points[i];
        s->data_points[i] = s->data_points[j];
        s->data_points[j] = temp;
    }

    // polishing improves pixels chosen early in the algorithm
    // by reconsidering them after the output image has been filled.
    // this greatly reduces the "sparklies" in the resulting image.
    // this is achieved by appending the first n data points to the end.
    // n is reduced exponentially by "magic" until it's less than 1.
    if (parameters.magic) for (int n = data_area; n > 0;) {
        n = n * parameters.magic / 256;
        for (int i = 0; i < n; i++) {
            sb_push(s->data_points, s->data_points[i]);
        }
    }

    // prepare an array of neighbors we've already computed the difference of.
    // this is a simple optimization and isn't critical to the algorithm.
    // (tried_array is referred to implicitly by macros)
    IMAGE_RESIZE(s->tried, s->corpus.width, s->corpus.height, 1);
    const int corpus_area = s->corpus.width * s->corpus.height;
    for (int i = 0; i < corpus_area; i++) s->tried_array[i] = -1;
}

static void resynth(Resynth_state *s, Parameters parameters) {
    // "resynthesize" an output image from a given input image.
    resynth__init(s, parameters);

    for (int i = sb_count(s->data_points) - 1; i >= 0; i--) {
        Coord position = s->data_points[i];

        // this point is guaranteed to have a value after this iteration.
        image_atc(s->status, position)->has_value = true;

        // collect neighboring pixels as candidates for best-fit.
        // the order we check and collect is relevant, thus "sorted_offsets".
        s->n_neighbors = 0;
        const int sorted_offsets_size = sb_count(s->sorted_offsets);
        for (int j = 0; j < sorted_offsets_size; j++) {
            Coord point = coord_add(position, s->sorted_offsets[j]);

            if (wrap_or_clip(parameters, s->data, &point) &&
                image_atc(s->status, point)->has_value) {
                s->neighbors[s->n_neighbors] = s->sorted_offsets[j];
                s->neighbor_statuses[s->n_neighbors] =
                    image_atc(s->status, point);
                for (int k = 0; k < s->input_bytes; k++) {
                    s->neighbor_values[s->n_neighbors].v[k] =
                        image_atc(s->data, point)[k];
                }
                s->n_neighbors++;
                if (s->n_neighbors >= parameters.neighbors) break;
            }
        }

        s->best = INT_MAX;

        // consider each neighboring pixel collected as a best-fit.
        for (int j = 0; j < s->n_neighbors && s->best != 0; j++) {
            if (s->neighbor_statuses[j]->has_source) {
                Coord point = coord_sub(s->neighbor_statuses[j]->source,
                                        s->neighbors[j]);
                if (point.x < 0 || point.y < 0 ||
                    point.x >= s->corpus.width || point.y >= s->corpus.height) {
                    continue;
                }
                // skip computing differences of points
                // we've already done this iteration. not mandatory.
                if (*image_atc(s->tried, point) == i) continue;
                try_point(s, point);
                *image_atc(s->tried, point) = i;
            }
        }

        // try some random points in the corpus. this is required for
        // choosing the first couple pixels, since they have no neighbors.
        // after that, this step is optional. it can improve subjective quality.
        for (int j = 0; j < parameters.tries && s->best != 0; j++) {
            int random = rnd_pcg_range(&pcg, 0, sb_count(s->corpus_points) - 1);
            try_point(s, s->corpus_points[random]);
        }

        // finally, copy the best pixel to the output image.
        for (int j = 0; j < s->input_bytes; j++) {
            image_atc(s->data, position)[j] =
                image_atc(s->corpus, s->best_point)[j];
        }
        image_atc(s->status, position)->has_source = true;
        image_atc(s->status, position)->source = s->best_point;
    }
}

static char *manipulate_filename(const char *fn,
                                 const char *new_extension) {
#define MAX_LENGTH 256
    int length = strlen(fn);
    if (length > MAX_LENGTH) length = MAX_LENGTH;
    // out_fn must be freed by the caller.
    char *out_fn = (char *)calloc(2 * MAX_LENGTH, 1);
    strncpy(out_fn, fn, length);

    char *hint = strrchr(out_fn, '.');
    if (hint == NULL) strcpy(out_fn + length, new_extension);
    else strcpy(hint, new_extension);

    return out_fn;
#undef MAX_LENGTH
}

static const int disc00[] = {
    // http://oeis.org/A057961
    1,    5,    9,    13,   21,   25,   29,   37,
    45,   49,   57,   61,   69,   81,   89,   97,
    101,  109,  113,  121,  129,  137,  145,  149,
    161,  169,  177,  185,  193,  197,  213,  221,
    225,  233,  241,  249,  253,  261,  277,  285,
    293,  301,  305,  317,  325,  333,  341,  349,
    357,  365,  373,  377,  385,  401,  405,  421,
    429,  437,  441,  457,  465,  473,  481,  489,
    497,  505,  509,  517,  529,  545,  553,  561,
    569,  577,  593,  601,  609,  613,  621,  633,
    641,  657,  665,  673,  681,  697,  709,  717,
    725,  733,  741,  749,  757,  761,  769,  777,
    793,  797,  805,  821,  829,  845,  853,  861,
    869,  877,  885,  889,  901,  917,  925,  933,
    941,  949,  965,  973,  981,  989,  997,  1005,
    1009, 1033, 1041, 1049, 1057, 1069, 1085, 1093
};

int main(int argc, char *argv[]) {
    Resynth_state state = {0};
    Resynth_state *s = &state; // (just for consistency across functions)

    Parameters parameters = {0};
    parameters.v_tile = true;
    parameters.h_tile = true;
    // blah = our default;          // original resynthesizer default
    parameters.magic = 192;         // 192 (3/4)
    parameters.autism = 32. / 256.; // 30. / 256.
    parameters.neighbors = 29;      // 30
    parameters.tries = 192;         // 200 (or 80 in the paper)

    int scale = 1;
    unsigned long seed = 0;

    // our main() return value. subtracted by one for each failed image.
    int ret = 0;

    KYAA_LOOP {
        KYAA_BEGIN

        KYAA_FLAG_LONG('a', "autism",
"        sensitivity to outliers\n"
"        range: [0,256];     default: 32")
            parameters.autism = (double)(kyaa_long_value) / 256.;

        KYAA_FLAG_LONG('N', "neighbors",
"        points to use when sampling\n"
"        range: [0,1024];    default: 29")
            parameters.neighbors = kyaa_long_value;

        KYAA_FLAG_LONG('R', "circle-radius",
"        circle neighborhood radius\n"
"        range: [1,128];     default: [n/a]")
            int radius = kyaa_long_value;
            radius = CLAMP(radius, 1, (int)(LEN(disc00)));
            parameters.neighbors = disc00[radius - 1];

        KYAA_FLAG_LONG('M', "tries",
"        random points added to candidates\n"
"        range: [0,65536];   default: 192")
            parameters.tries = kyaa_long_value;

        KYAA_FLAG_LONG('m', "magic",
"        magic constant, affects iterations\n"
"        range: [0,255];     default: 192")
            parameters.magic = kyaa_long_value;

        KYAA_FLAG_LONG('s', "scale",
"        output size multiplier; negative values set width and height\n"
"        range: [-8192,32];  default: 1")
            scale = kyaa_long_value;

        KYAA_FLAG_LONG('S', "seed",
"        initial RNG value\n"
"                            default: 0 [time(0)]")
            seed = (unsigned long) kyaa_long_value;

        KYAA_HELP("  {files...}\n"
"        image files to open, resynthesize, and save as {filename}.resynth.png\n"
"        required            default: [none]")

        KYAA_END

        if (kyaa_read_stdin) {
            fprintf(stderr, "fatal error: reading from stdin is unsupported\n");
            exit(1);
        }

        CLAMPV(parameters.magic, 0, 255);
        CLAMPV(parameters.autism, 0., 1.);
        CLAMPV(parameters.neighbors, 0, disc00[LEN(disc00) - 1]);
        CLAMPV(parameters.tries, 0, 65536);
        CLAMPV(scale, -8192, 32);

        char *fn = kyaa_arg;

        int w, h, d;
        uint8_t *image = stbi_load(fn, &w, &h, &d, 0);
        if (image == NULL) {
            fprintf(stderr, "invalid image: %s\n", fn);
            ret--;
            continue;
        }

        IMAGE_RESIZE(s->corpus, w, h, d);
        memcpy(s->corpus_array, image, w * h * d);

        s->input_bytes = MIN(d, 3);

        {
            int data_w = 256, data_h = 256;
            if (scale > 0) data_w = scale * w, data_h = scale * h;
            if (scale < 0) data_w = -scale, data_h = -scale;
            IMAGE_RESIZE(s->data, data_w, data_h, s->input_bytes);
        }

        stbi_image_free(image);

        if (seed) rnd_pcg_seed(&pcg, seed);
        else rnd_pcg_seed(&pcg, time(0));
        resynth(s, parameters);

        char *out_fn = manipulate_filename(fn, ".resynth.png");
        puts(out_fn);
        int result = stbi_write_png(out_fn, s->data.width, s->data.height,
                                    s->data.depth, s->data_array, 0);
        if (!result) {
            fprintf(stderr, "failed to write: %s\n", out_fn);
            ret--;
        }

        free(out_fn);
    }

    state_free(s);

    return ret;
}
