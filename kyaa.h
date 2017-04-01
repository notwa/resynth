/* kyaa.h - macro hacks for handling main() arguments
    license: public domain or whatever.
    documentation is kept separate in kyaa.md
*/

#pragma once

#ifndef KYAA_OKAY
#define KYAA_OKAY 0
#endif
#ifndef KYAA_ERROR
#define KYAA_ERROR 1
#endif
#ifndef KYAA_ITER
#define KYAA_ITER i
#endif

#define KYAA_SETUP \
    /* sanity checks */ \
    if (argc <= 0 || argv == NULL || argv[0] == NULL) { \
        fprintf(stderr, "You've met with a terrible fate.\n"); \
        return KYAA_ERROR; \
    } \
    char *kyaa_name = argv[0]; \
    bool kyaa_read_stdin = false; \
    char kyaa_flag = '\0'; \
    bool kyaa_keep_parsing = true; \
    bool kyaa_parse_next = false; \

#define KYAA_LOOP \
    KYAA_SETUP \
    for (int KYAA_ITER = 1; KYAA_ITER < argc; KYAA_ITER++) \

#define KYAA_BEGIN \
    char *kyaa_arg = argv[KYAA_ITER]; \
    if (kyaa_keep_parsing && (kyaa_parse_next || kyaa_arg[0] == '-')) { \
        if (!kyaa_parse_next && kyaa_arg[1] == '-' && kyaa_arg[2] == '\0') { \
            kyaa_keep_parsing = false; \
            continue; \
        } \
        if (!kyaa_parse_next && kyaa_arg[1] == '\0') { \
            kyaa_read_stdin = true; \
        } else { \
            /* case: kyaa_parse_next: arg is at least 1 char initialized. */ \
            /* case: !kyaa_parse_next: arg is at least 3 chars initialized. */ \
            char *kyaa_etc = kyaa_parse_next ? kyaa_arg : kyaa_arg + 2; \
            bool kyaa_no_more = false; \
            bool kyaa_helping = false; \
            bool kyaa_any = false; \
            if (!kyaa_parse_next && kyaa_arg[1] != '-') { \
                kyaa_flag = kyaa_arg[1]; \
                kyaa_no_more = kyaa_arg[2] == '\0'; \
            } \
            if (kyaa_flag == 'h' || !strcmp(kyaa_arg, "--help")) { \
                printf("usage:\n"); \
                kyaa_helping = true; \
            } \
            if (0) { \

#define KYAA_END \
            } \
            if (!kyaa_any && !kyaa_helping) { \
                if (kyaa_flag) { \
                    fprintf(stderr, "unknown flag: -%c\n", kyaa_flag); \
                } else { \
                    fprintf(stderr, "unknown flag: %s\n", kyaa_arg); \
                } \
                return KYAA_ERROR; \
            } \
            if (kyaa_helping) { \
                return KYAA_OKAY; \
            } \
            kyaa_parse_next = false; \
            kyaa_flag = '\0'; \
            continue; \
        } \
    } \

#define KYAA_DESCRIBE(c, name, description) \
    printf("  -%c  --%s\n%s\n", c, name, description) \

#define KYAA_FLAG(c, name, description) \
    } \
    if (kyaa_helping) { \
        KYAA_DESCRIBE(c, name, description); \
    } else if (kyaa_flag == c || !strcmp(kyaa_arg, "--"name)) { \
        kyaa_flag = c; \
        kyaa_any = true; \

#define KYAA_FLAG_ARG(c, name, description) \
    } \
    if (kyaa_helping) { \
        KYAA_DESCRIBE(c, name, description); \
    } else if (kyaa_flag == c || !strcmp(kyaa_arg, "--"name)) { \
        if (kyaa_no_more || kyaa_flag != c) { \
            kyaa_parse_next = true; \
            if (KYAA_ITER + 1 == argc || argv[KYAA_ITER + 1][0] == '\0') { \
                fprintf(stderr, "expected an argument for --%s (-%c)\n", name, c); \
                return KYAA_ERROR; \
            } \
            kyaa_flag = c; \
            continue; \
        } \
        kyaa_flag = c; \
        kyaa_any = true; \

#define KYAA_HELP(description) \
    } \
    if (kyaa_helping) { \
        if (description != NULL) { \
            printf("%s\n", description); \
        } \

