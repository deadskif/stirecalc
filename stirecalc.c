/*
 *   Copyright (C) 2018 Badaev Vladimir <dead.skif@gmail.com>
 *  
 *   This file is part of stirecalc.
 *
 *   stirecalc is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   stirecalc is distributed in the hope that it will be lteful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <sys/types.h>
#include <regex.h>

#include "stirecalc.h"
#include "oem_tires.h"

static struct {
    bool c; /* Compare */
    bool l; /* List OEM tires */
} opts = {
    .c = false,
    .l = false,
};
#define ASPECT_RATIO_ISO_DEFAULT    82      /* %  */
#define ASPECT_RATIO_LT_ZERO        92      /* %  */
#define APPECT_RATIO_LT_NZERO       82      /* %  */

#define MM_IN_INCH  25.4f
enum wheel_flags {
    WHEELF_PASSENGER = 0x1,
};

#define ISO_TIRE_WIDTH "([[:digit:]]{3})"
#define ISO_ASPECT_RATIO "([[:digit:]]{2,3})"
#define SPACES "[[:space:]]*"
#define SPACES_OR_SLASH "[[:space:]/]"
#define TIRE_CARCASS "([BRD-])"
#define RIM_DIAMETER "([[:digit:]]{1,2}(\\.[[:digit:]])?)"

#define LT_TIRE_DIAMETER "([[:digit:]]{2}(\\.[[:digit:]])?)"
#define LT_TIRE_WIDTH "([[:digit:]]{1,2}(\\.[[:digit:]]{2})?)"

#define ISO_REGEX \
    "^[P]?" ISO_TIRE_WIDTH "(" SPACES_OR_SLASH "+" \
    ISO_ASPECT_RATIO ")?" SPACES "(" SPACES_OR_SLASH "*" TIRE_CARCASS "?)" \
    SPACES RIM_DIAMETER "$"

#define LT_REGEX \
    "(" LT_TIRE_DIAMETER SPACES "x" ")?" SPACES LT_TIRE_WIDTH \
     SPACES TIRE_CARCASS "?" SPACES RIM_DIAMETER "$"

#define DEBUG_MATCH(STR, MATCHES, N) do { \
    char __buf[1024] = {0}; \
    strncpy(__buf, (STR) + (MATCHES)[(N)].rm_so, \
            (MATCHES)[(N)].rm_eo - (MATCHES)[(N)].rm_so); \
    fprintf(stderr, "DEBUG_MATCH %s() %zu: '%s'\n", __func__, (N), __buf); \
} while(0);
#define DEBUG_ALL_MATCH(STR, MATCHES, N) do { \
    for(size_t __i = 0; __i < (N); __i++) {\
        if ((MATCHES)[__i].rm_so == -1) continue; \
        DEBUG_MATCH(STR, MATCHES, __i); \
    }; \
} while(0)

static inline unsigned get_int(const char *str, regmatch_t *match, size_t n)
{
    //DEBUG_MATCH(str, match, n);
    if (match[n].rm_so == -1)
        return 0;
    return atoi(str + match[n].rm_so);
}
static inline float get_float(const char *str, regmatch_t *match, size_t n)
{
    //DEBUG_MATCH(str, match, n);
    if (match[n].rm_so == -1)
        return 0.;
    return atof(str + match[n].rm_so);
}
static inline int get_char(const char *str, regmatch_t *match, size_t n)
{
    //DEBUG_MATCH(str, match, n);
    const char *start, *end;

    if (match[n].rm_so == -1)
        return '\0';

    start = str + match[n].rm_so;
    end = str + match[n].rm_eo;

    if (start == end)
        return '\0';
    assert((start + 1) == end);
    return *start;
}

static const char *parse_wheel(const char *str, struct wheel *w) {
    regex_t iso;
    regex_t lt;
    int ret;
#define MAX_MATCH 128
    regmatch_t match[MAX_MATCH];
    bool parsed = false;

    memset(w, 0, sizeof(struct wheel));
    if ((ret = regcomp(&iso, ISO_REGEX, REG_EXTENDED)) != 0)
    {
        char errbuf[1024];
        regerror(ret, &iso, errbuf, sizeof(errbuf));
        fprintf(stderr, "Compiling ISO regex '%s' failed: %s\n",
                ISO_REGEX, errbuf);
        return false;
    }
    if ((ret = regcomp(&lt, LT_REGEX, REG_EXTENDED)) != 0)
    {
        char errbuf[1024];
        regerror(ret, &lt, errbuf, sizeof(errbuf));
        fprintf(stderr, "Compiling LT regex '%s' failed: %s\n",
                LT_REGEX, errbuf);
        goto clean_iso;
    }

    if ((ret = regexec(&iso, str, MAX_MATCH, match, 0)) == 0) {
        //printf("ISO match\n");
        parsed = true;
        w->type = WHEEL_ISO;
        //DEBUG_ALL_MATCH(str, match, MAX_MATCH);
        w->iso.tire_width = get_int(str, match, 1);
        w->iso.aspect_ratio = get_int(str, match, 3);
        if (w->iso.aspect_ratio == 0)
            w->iso.aspect_ratio = ASPECT_RATIO_ISO_DEFAULT;
        w->iso.rim_diameter = get_float(str, match, 6);
        w->tire_type = get_char(str, match, 5);
    } else if ((ret = regexec(&lt, str, MAX_MATCH, match, 0)) == 0) {
        //printf("LT match\n");
        parsed = true;
        w->type = WHEEL_LT;
        //DEBUG_ALL_MATCH(str, match, MAX_MATCH);
        w->lt.diameter = get_float(str, match, 2);
        w->lt.tire_width = get_float(str, match, 4);
        w->lt.rim_diameter = get_float(str, match, 7);
        w->tire_type = get_char(str, match, 6);
        if (w->lt.diameter == 0.0) {
            double tmp;
            if (modf(w->lt.tire_width * 10., &tmp) == 0.0) {
                tmp = ASPECT_RATIO_LT_ZERO;
            } else {
                tmp = APPECT_RATIO_LT_NZERO;
            }
            w->lt.diameter = w->lt.rim_diameter + 
                2 * w->lt.tire_width * (tmp / 100.);
        }
    } else {
    }
//  printf("echo '%s' | egrep '%s'\n", str, ISO_REGEX);
//clean_lt:
    regfree(&lt);
clean_iso:
    regfree(&iso);
    return parsed ? str : NULL;
}
static const char *tire_type_char(const struct wheel *w) {
    switch (w->tire_type) {
        case TIRE_RADIAL:
            return "R";
            break;
        case TIRE_DIAGONAL:
            return "D";
            break;
        case TIRE_BIAS_BELT:
            return "B";
            break;
        case TIRE_DASH:
            return "-";
            break;
        case TIRE_UNKNOWN:
            return (w->type == WHEEL_ISO) ? "/" : " ";
            break;
    }
    fprintf(stderr, "Bad tire type: %c\n", w->tire_type);
    return "Error";
}
static float calc_lt_diameter(const struct wheel *src)
{
    float tire_h = src->iso.tire_width * (src->iso.aspect_ratio / 100.) / MM_IN_INCH;
    float d = tire_h * 2. + src->iso.rim_diameter;
    return roundf(d * 10.) / 10.;
}
static float calc_lt_width(const struct wheel *src)
{
    float w = src->iso.tire_width / MM_IN_INCH;
    return roundf(w * 10.) / 10.;
}
static float calc_iso_width(const struct wheel *src)
{
    if (src->type == WHEEL_ISO)
        return src->iso.tire_width;
    float w = src->lt.tire_width * MM_IN_INCH;
    return roundf(w / 5.) * 5.; 
}
static float calc_iso_aspect_ratio(const struct wheel *src, float tire_width)
{
    if (src->type == WHEEL_ISO)
        return src->iso.aspect_ratio;
    float h = (src->lt.diameter - src->iso.rim_diameter) / 2.;
    h *= MM_IN_INCH;
    float ar = (h / tire_width) * 100.;
    return roundf(ar / 5.) * 5.;
}
#if 0
static float calc_rim_diameter(const struct wheel *src)
{
    if (src->type == WHEEL_ISO)
        return src->iso.rim_diameter;
    return src->lt.rim_diameter;
}
static float calc_tire_height_mm(const struct wheel *src)
{
    if (src->type == WHEEL_ISO)
        return src->iso.tire_width * src->iso.aspect_ratio / 100.;
    return (src->lt.diameter - src->lt.rim_diameter) / 2. / MM_IN_INCH;
}
#endif
static bool convert_wheel(struct wheel *dst, const struct wheel *src, unsigned to)
{
    if (src->type == to) {
        memcpy(dst, src, sizeof(struct wheel));
        return true;
    }
    dst->type = to;
    dst->tire_type = src->tire_type;
    switch (to) {
        case WHEEL_ISO:
            assert(src->type == WHEEL_LT);
            dst->iso.tire_width = calc_iso_width(src);
            dst->iso.aspect_ratio = calc_iso_aspect_ratio(src,
                    dst->iso.tire_width);
            dst->iso.rim_diameter = src->lt.rim_diameter;
            break;
        case WHEEL_LT:
            assert(src->type == WHEEL_ISO);
            dst->lt.diameter = calc_lt_diameter(src);
            dst->lt.tire_width = calc_lt_width(src);
            dst->lt.rim_diameter = src->iso.rim_diameter;
            break;
        default:
            return false;
            break;
    }
    return true;
}
int snprint_wheel(char *str, size_t size, const struct wheel *wheel, unsigned system)
{
    struct wheel tmp = {0};
    int ret = 0;
    if (!convert_wheel(&tmp, wheel, system))
        return 0;

    if (tmp.type == WHEEL_ISO) {
        ret = snprintf(str, size, "%u/%u%s%.0f",
                tmp.iso.tire_width,
                tmp.iso.aspect_ratio,
                tire_type_char(&tmp),
                tmp.iso.rim_diameter);
    } else {
        ret = snprintf(str, size, "%.1fx%.2f%s%.0f",
                tmp.lt.diameter,
                tmp.lt.tire_width,
                tire_type_char(&tmp),
                tmp.lt.rim_diameter);
    }
    return ret;
}

static void usage(void)
{
    printf("Usage: %s [-chl] <tire_size> [<tire_sizes>]\n", program_invocation_name);
    exit(1);
}
static int translate(const char *s)
{
    struct wheel wheel = {0};
    if (parse_wheel(s, &wheel) != NULL) {
        print_wheel(&wheel, wheel.type);
        printf(" -> ");
        print_wheel(&wheel, (wheel.type == WHEEL_LT) ? WHEEL_ISO : WHEEL_LT);
        printf("\n");
        return 0;
    } 
    printf("Unknown tire: %s\n", s);
    return -1;
}
static int compare(size_t n, char *s[])
{
    struct wheel wheel[n];
    float diam0, diam_diff;
    char buf[21];
    for (size_t i = 0; i < n; i++)
    {
        if (parse_wheel(s[i], wheel + i) == NULL) {
            printf("Unknown tire: %s\n", s[i]);
        }
    }

    assert(n > 1);

    for (size_t i = 0; i < n; i++)
    {
        snprint_wheel(buf, sizeof(buf), wheel + i, WHEEL_ISO);
        printf("%-20s", buf);
    }
    printf("\n");
    for (size_t i = 0; i < n; i++)
    {
        snprint_wheel(buf, sizeof(buf), wheel + i, WHEEL_LT);
        printf("%-20s", buf);
    }
    printf("\n");
    diam0 = calc_lt_diameter(wheel);

    printf("%-20s",  "Diam");
    for (size_t i = 1; i < n; i++)
    {
        diam_diff = calc_lt_diameter(wheel+i) - diam0;
        snprintf(buf, sizeof(buf), "%+.1f in %+.1f%%", 
                diam_diff, diam_diff / diam0 * 100.);
        printf("%-20s", buf);
    }
    printf("\n");

    printf("%-20s",  "Clearance");
    for (size_t i = 1; i < n; i++)
    {
        diam_diff = calc_lt_diameter(wheel+i) - diam0;
        snprintf(buf, sizeof(buf), "%+.1f mm",
                diam_diff / 2. * MM_IN_INCH);
        printf("%-20s", buf);
    };
    printf("\n");

    printf("%-20s",  "Tire width");
    for (size_t i = 1; i < n; i++)
    {
        snprintf(buf, sizeof(buf), "%+.1f mm",
                calc_iso_width(wheel+i) - calc_iso_width(wheel));
        printf("%-20s", buf);
    }
    printf("\n");
    return 0;
}
int main(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "chl")) != -1) {
        switch (opt) {
            case 'c':
                opts.c = true;
                break;
            case 'l':
                opts.l = true;
                break;
            case 'h':
            case '?':
            default:
                usage();
                break;
        }
    }

    argc -= optind;
    argv += optind;

    if (opts.c) {
        if (argc <= 1)
            usage();

        compare((size_t)argc, argv);
    } else if (opts.l) {
        print_all_oem_wheels();
    } else {
        printf("%s() argc %d, argv %s\n", __func__, argc, *argv);
        for (; argc; argc--, argv++) {
            translate(*argv);
        }
    };
    
    return 0;
}
