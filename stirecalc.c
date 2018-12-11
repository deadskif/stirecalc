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
 *   stirecalc is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#define WHEEL_DIAMETER_MAX      50.0f   /* in */
#define WHEEL_DIAMETER_MIN      10.0f   /* in */
#define TIRE_WIDTH_MAX          1000.0f /* mm */
#define ASPECT_RATIO_MAX        120     /* %  */
#define ASPECT_RATIO_MIN        20      /* %  */
#define ASPECT_RATIO_DEFAULT    80      /* %  */

#define MM_IN_INCH  25.4f
struct wheel {
    enum { 
        WHEEL_UNKNOWN, WHEEL_METRIC, WHEEL_US 
    } type;
    enum tire_type {
        TIRE_UNKNOWN, TIRE_RADIAL = 'R', TIRE_DIAGONAL = 'D'
    } tire_type;

    union {
        struct wheel_metric {
            float tire_width;   /* mm */
            unsigned aspect_ratio; /* %  */
            float rim_diameter; /* in */
        } metric;
        struct wheel_us {
            float diameter;     /* in */
            float tire_width;   /* in */
            float rim_diameter; /* in */
        } us;
    };
};

#define SKIP_SPACE(P) do { \
    while (isspace(*(P)))\
        (P)++; \
} while(false)

#define SKIP_CHAR(P, C) (\
    (*(P) == (C)) ? ((P)++, true) : false \
)

static const char *parse_wheel(const char *str, struct wheel *wheel) {
    char *end = NULL;
    memset(wheel, 0, sizeof(struct wheel));
    
    SKIP_SPACE(str);
    SKIP_CHAR(str, 'P');

    float val = strtof(str, &end);
    if (end == str) {
        fprintf(stderr, "Expected tirewidth or wheel diameter.\n");
        return NULL;
    };
    str = end;
    if ((val > TIRE_WIDTH_MAX) || (val < WHEEL_DIAMETER_MIN)) {
        fprintf(stderr, "Value out of range.\n");
        return NULL;
    }

    if (val > WHEEL_DIAMETER_MAX) {
        /* Metric */
        long ar;
        wheel->type = WHEEL_METRIC;
        wheel->metric.tire_width = val;

        SKIP_SPACE(str);
        SKIP_CHAR(str, '/');

        if (*str != '-') {
            ar = strtol(str, &end, 10);
            if (str == end)
                ar = ASPECT_RATIO_DEFAULT;
            if ((ar > ASPECT_RATIO_MAX) || (ar < ASPECT_RATIO_MIN)) {
                fprintf(stderr, "Aspect ratio out of range.\n");
                return NULL;
            }
            str = end;
        } else {
            /* - */
            wheel->tire_type = TIRE_DIAGONAL;
            ar = ASPECT_RATIO_DEFAULT;
        }
        wheel->metric.aspect_ratio = ar;

        SKIP_SPACE(str);
        SKIP_CHAR(str, '/');
        SKIP_SPACE(str);

        if (SKIP_CHAR(str, 'R')) {
            wheel->tire_type = TIRE_RADIAL;
        } else if (SKIP_CHAR(str, 'D') || SKIP_CHAR(str, '-')) {
            wheel->tire_type = TIRE_DIAGONAL;
        }
        if (wheel->tire_type != TIRE_UNKNOWN)
            SKIP_SPACE(str);

        val = strtof(str, &end);
        if (end == str) {
            fprintf(stderr, "Rim diameter expected.\n");
            return NULL;
        }
        str = end;
        wheel->metric.rim_diameter = val;

    } else {
        wheel->type = WHEEL_US;
        wheel->us.diameter = val;

        SKIP_SPACE(str);
    }

    return str;
}

static const char *tire_type_char(enum tire_type type) {
    switch (type) {
        case TIRE_RADIAL:
            return "R";
            break;
        case TIRE_DIAGONAL:
            return "D";
            break;
        case TIRE_UNKNOWN:
            return "";
            break;
    }
    return "Error";
}
static float calc_us_diameter(const struct wheel *src)
{
    float tire_h = src->metric.tire_width * (src->metric.aspect_ratio / 100.) / MM_IN_INCH;
    float d = tire_h * 2. + src->metric.rim_diameter;
    return roundf(d * 10.) / 10.;
}
static float calc_us_width(const struct wheel *src)
{
    float w = src->metric.tire_width / MM_IN_INCH;
    return roundf(w * 10.) / 10.;
}
static bool convert_wheel(struct wheel *dst, const struct wheel *src, unsigned to)
{
    if (src->type == to) {
        memcpy(dst, src, sizeof(struct wheel));
        return true;
    }
    switch (to) {
        case WHEEL_METRIC:
            assert(src->type == WHEEL_US);
            break;
        case WHEEL_US:
            assert(src->type == WHEEL_METRIC);
            dst->type = to;
            dst->tire_type = src->tire_type;
            dst->us.diameter = calc_us_diameter(src);
            dst->us.tire_width = calc_us_width(src);
            dst->us.rim_diameter = src->metric.rim_diameter;
            break;
        default:
            return false;
            break;
    }
    return true;
}
static int print_wheel(const struct wheel *wheel, unsigned system)
{
    struct wheel tmp = {0};
    if (!convert_wheel(&tmp, wheel, system))
        return 0;

    if (tmp.type == WHEEL_METRIC) {
        printf("%.0f/%u %s%.0f", tmp.metric.tire_width,
                tmp.metric.aspect_ratio,
                tire_type_char(tmp.tire_type),
                tmp.metric.rim_diameter);
    } else {
        printf("%.1fx%.1f %s%.0f",tmp.us.diameter,
                tmp.us.tire_width,
                tire_type_char(tmp.tire_type),
                tmp.us.rim_diameter);
    }
    return 0;
}

static void usage(void) {
    printf("Usage: stirecalc <tire_size>\n");
    exit(1);
}
int main(int argc, const char *argv[])
{
    struct wheel wheel;

    if (argc != 2)
        usage();
    
    if (parse_wheel(argv[1], &wheel) != NULL) {
        print_wheel(&wheel, WHEEL_METRIC);
        printf(" -> ");
        print_wheel(&wheel, WHEEL_US);
        printf("\n");
    }
    return 0;
}
