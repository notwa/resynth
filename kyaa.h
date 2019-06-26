/* kyaa.h - macro hacks for handling main() arguments
    This is free and unencumbered software released into the public domain.
    For more information, please refer to <http://unlicense.org>
*/

#ifndef KYAA_ONCE
#define KYAA_ONCE

/* set some sane defaults. */
#ifndef KYAA_OKAY
#define KYAA_OKAY 0
#endif
#ifndef KYAA_FAIL
#define KYAA_FAIL 1
#endif
#ifndef KYAA_OUT
#define KYAA_OUT(...) printf(__VA_ARGS__)
#endif
#ifndef KYAA_ERR
#define KYAA_ERR(...) fprintf(stderr, __VA_ARGS__)
#endif

#define KYAA_IS_LONG(arg, name) \
    (strncmp(arg, "--" name, strlen("--" name)) == 0 && \
     (arg[strlen("--" name)] == '\0' || arg[strlen("--" name)] == '='))

#define KYAA_SETUP \
    if (argc <= 0 || argv == NULL || argv[0] == NULL) { \
        KYAA_ERR("malformed argc/argv\n"); \
        return KYAA_FAIL; \
    } \
    const char *kyaa_name = argv[0]; \
    bool kyaa_read_stdin = false; \
    char kyaa_char = '\0'; \
    bool kyaa_parsing = true; \
    bool kyaa_skip = false; \

#define KYAA_LOOP \
    KYAA_SETUP \
    for (int kyaa_iter = 1; kyaa_iter < argc; kyaa_iter++) \

#define KYAA_BEGIN \
    const char *kyaa_arg = argv[kyaa_iter]; \
    if (kyaa_parsing && (kyaa_skip || kyaa_arg[0] == '-')) { \
        if (!kyaa_skip && kyaa_arg[1] == '-' && kyaa_arg[2] == '\0') { \
            kyaa_parsing = false; \
            continue; \
        } \
        if (!kyaa_skip && kyaa_arg[1] == '\0') kyaa_read_stdin = true; \
        else { \
            /* case:  kyaa_skip: kyaa_arg is at least 1 char  long. */ \
            /* case: !kyaa_skip: kyaa_arg is at least 3 chars long. */ \
            const char *kyaa_etc = kyaa_skip ? kyaa_arg : kyaa_arg + 2; \
            const char *kyaa_equals = kyaa_skip ? NULL : strchr(kyaa_etc, '='); \
            bool kyaa_helping = false; \
            bool kyaa_any = false; \
            if (!kyaa_skip) { \
                if (kyaa_arg[1] != '-') { \
                    kyaa_char = kyaa_arg[1]; \
                    if (kyaa_arg[2] == '\0') kyaa_etc = NULL; \
                } else if (kyaa_equals == NULL) kyaa_etc = NULL; \
                else kyaa_etc = kyaa_equals + 1; \
                if (strcmp(kyaa_arg, "--help") == 0) kyaa_char = 'h'; \
                if (kyaa_char == 'h') { \
                    KYAA_OUT("usage:\n"); \
                    kyaa_helping = true; \
                } \
            } \
            if (0) { \

#define KYAA_END \
            } \
            if (!kyaa_any && !kyaa_helping) { \
                if (kyaa_char) KYAA_ERR("unknown flag: -%c\n", kyaa_char); \
                else KYAA_ERR("unknown flag: %s\n", kyaa_arg); \
                return KYAA_FAIL; \
            } \
            if (kyaa_helping) { \
                return KYAA_OKAY; \
            } \
            kyaa_skip = false; \
            kyaa_char = '\0'; \
            continue; \
        } \
    } \

#define KYAA_DESCRIBE(c, name, description) \
    KYAA_OUT("  -%c  --%s\n%s\n", c, name, description) \

#define KYAA_FLAG(c, name, description) \
    } \
    if (kyaa_helping) KYAA_DESCRIBE(c, name, description); \
    else if (kyaa_char == c || KYAA_IS_LONG(kyaa_arg, name)) { \
        kyaa_char = c; \
        kyaa_any = true; \

#define KYAA_FLAG_ARG(c, name, description) \
    } \
    if (kyaa_helping) KYAA_DESCRIBE(c, name, description); \
    else if (kyaa_char == c || KYAA_IS_LONG(kyaa_arg, name)) { \
        if (kyaa_etc == NULL) { \
            kyaa_skip = true; \
            if (kyaa_iter + 1 == argc || argv[kyaa_iter + 1] == NULL) { \
                KYAA_ERR("expected an argument for --%s (-%c)\n", name, c); \
                return KYAA_FAIL; \
            } \
            kyaa_char = c; \
            continue; \
        } \
        kyaa_char = c; \
        kyaa_any = true; \

#define KYAA_HELP(description) \
    } \
    if (kyaa_helping) { \
        if (description != NULL) KYAA_OUT("%s\n", description); \

#endif /* KYAA_ONCE */
