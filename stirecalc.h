#ifndef STIRECALC_H

#define STIRECALC_H
#include <stdio.h>

struct wheel {
    enum { 
        WHEEL_UNKNOWN, WHEEL_ISO, WHEEL_LT 
    } type;
    enum tire_type {
        TIRE_UNKNOWN,
        TIRE_RADIAL = 'R',
        TIRE_DIAGONAL = 'D',
        TIRE_BIAS_BELT = 'B',
        TIRE_DASH = '-',
    } tire_type;

    union {
        struct wheel_iso {
            unsigned tire_width;   /* mm */
            unsigned aspect_ratio; /* %  */
            float rim_diameter; /* in */
        } iso;
        struct wheel_lt {
            float diameter;     /* in */
            float tire_width;   /* in */
            float rim_diameter; /* in */
        } lt;
    };
    unsigned flags;
};

#define INIT_WHEEL_ISO_R(TW, AR, RD) { \
    .type = WHEEL_ISO, \
    .tire_type = TIRE_RADIAL, \
    .iso = { \
        .tire_width = (TW), \
        .aspect_ratio = (AR), \
        .rim_diameter = (RD), \
    }, \
}
#define INIT_WHEEL_UNKNOWN { \
    .type = WHEEL_UNKNOWN, \
}
int snprint_wheel(char *str, size_t size, const struct wheel *wheel, unsigned system);
static inline int print_wheel(const struct wheel *wheel, unsigned system) {
    char buf[512];
    int ret;
    if ((ret = snprint_wheel(buf, sizeof(buf), wheel, system)) > 0)
        ret = printf("%s", buf);
    return ret;
}

#endif /* end of include guard: STIRECALC_H */

