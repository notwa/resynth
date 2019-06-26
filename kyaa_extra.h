/* kyaa_extra.h - extensions to kyaa for parsing arguments.
    This is free and unencumbered software released into the public domain.
    For more information, please refer to <http://unlicense.org>
*/

#ifndef KYAA_EXTRA
#define KYAA_EXTRA

static const char *kyaa_skip_spaces(const char *str) {
    /* iterates str to first non-space character according to the C locale */
    while (*str != '\0') {
        switch (*str) {
        case ' ':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
        case '\v': { str++; } break;
        default: return str;
        }
    }
    return str;
}

static const char *kyaa__base_2(const char **p_str, long *p_out) {
    const char *str = *p_str;
    long out = *p_out;
    for (char c; (c = *str) != '\0'; str++) {
        switch (c) {
        case '0': case '1': {
            long digit = (long)(c - '0');
            if (out < (LONG_MIN + digit) / 2) {
                return "out of range for long integer";
            }
            out = out * 2 - digit;
        } break;

        case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case '.': return "invalid character for base 2 integer";

        default: goto exit;
        }
    }
exit:
    *p_str = str;
    *p_out = out;
    return NULL;
}

static const char *kyaa__base_8(const char **p_str, long *p_out) {
    const char *str = *p_str;
    long out = *p_out;
    for (char c; (c = *str) != '\0'; str++) {
        switch (c) {
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7': {
            long digit = (long)(c - '0');
            if (out < (LONG_MIN + digit) / 8) {
                return "out of range for long integer";
            }
            out = out * 8 - digit;
        } break;

        case '8': case '9':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case '.': return "invalid character for base 8 integer";

        default: goto exit;
        }
    }
exit:
    *p_str = str;
    *p_out = out;
    return NULL;
}

static const char *kyaa__base_10(const char **p_str, long *p_out) {
    const char *str = *p_str;
    long out = *p_out;
    for (char c; (c = *str) != '\0'; str++) {
        switch (c) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': {
            long digit = (long)(c - '0');
            if (out < (LONG_MIN + digit) / 10) {
                return "out of range for long integer";
            }
            out = out * 10 - digit;
        } break;

        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case '.': return "invalid character for base 10 integer";

        default: goto exit;
        }
    }
exit:
    *p_str = str;
    *p_out = out;
    return NULL;
}

static const char *kyaa__base_16(const char **p_str, long *p_out) {
    const char *str = *p_str;
    long out = *p_out;
    for (char c; (c = *str) != '\0'; str++) {
        switch (c) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': {
            long digit = (long)(c - '0');
            if (out < (LONG_MIN + digit) / 16) {
                return "out of range for long integer";
            }
            out = out * 16 - digit;
        } break;

        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': {
            long digit = (long)(c - 'A') + 10;
            if (out < (LONG_MIN + digit) / 16) {
                return "out of range for long integer";
            }
            out = out * 16 - digit;
        } break;

        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': {
            long digit = (long)(c - 'a') + 10;
            if (out < (LONG_MIN + digit) / 16) {
                return "out of range for long integer";
            }
            out = out * 16 - digit;
        } break;

        case '.': return "invalid character for base 16 integer";

        default: goto exit;
        }
    }
exit:
    *p_str = str;
    *p_out = out;
    return NULL;
}

static const char *kyaa_str_to_long(const char *str, long *p_out) {
    /* returns error message or NULL */
    long out = 0;
    int base = 10;
    bool negated = false;

    str = kyaa_skip_spaces(str);

    switch (*str) {
    case '-': { negated = true; str++; } break;
    case '+': { str++; } break;
    default: break;
    }

    switch (*str) {
    case '#': { base = 10; str++; } break;
    case '$': { base =  8; str++; } break;
    case '%': { base =  2; str++; } break;
    case '0': { base = -1; str++; } break;
    default: break;
    }

    if (base == -1) {
        switch (*str) {
        case '\0': { *p_out = 0; } return NULL;
        case 'b': { base =  2; str++; } break;
        case 'h': { base = 16; str++; } break;
        case 'o': { base =  8; str++; } break;
        case 'x': { base = 16; str++; } break;
        default:  { base =  8; } break;
        }
    }

    if (*str == '\0') return "no number given";

    // NOTE: we actually subtract each digit from the result instead of summing.
    //       this lets us represent LONG_MIN without overflowing.
    const char *err;
    switch (base) {
        case  2: { err = kyaa__base_2( &str, &out); } break;
        case  8: { err = kyaa__base_8( &str, &out); } break;
        case 10: { err = kyaa__base_10(&str, &out); } break;
        case 16: { err = kyaa__base_16(&str, &out); } break;
        default: return "internal error";
    }
    if (err != NULL) return err;

    str = kyaa_skip_spaces(str);
    if (*str != '\0') return "unexpected character";

    // NOTE: out is negative here; see above comment.
    // assuming two's complement
    if (!negated && out == LONG_MIN) return "out of range for long integer";
    *p_out = negated ? out : -out;
    return NULL;
}

#define KYAA_FLAG_LONG(c, name, description) \
    KYAA_FLAG_ARG(c, name, description) \
        long kyaa_long_value; \
        const char *err = kyaa_str_to_long(kyaa_etc, &kyaa_long_value); \
        if (err) { \
            KYAA_ERR("%s\n", err); \
            return KYAA_FAIL; \
        } \

#endif /* KYAA_EXTRA */
