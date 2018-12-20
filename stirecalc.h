#ifndef STIRECALC_H

#define STIRECALC_H
#include <stdio.h>

struct stirec_tire {
    enum { 
        TIRECODE_UNKNOWN, TIRECODE_ISO, TIRECODE_LT 
    } type;
    enum tire_type {
        TIRE_UNKNOWN,
        TIRE_RADIAL = 'R',
        TIRE_DIAGONAL = 'D',
        TIRE_BIAS_BELT = 'B',
        TIRE_DASH = '-',
    } tire_type;

    union {
        struct stirec_tire_iso {
            unsigned tire_width;   /* mm */
            unsigned aspect_ratio; /* %  */
            float rim_diameter; /* in */
        } iso;
        struct stirec_tire_lt {
            float diameter;     /* in */
            float tire_width;   /* in */
            float rim_diameter; /* in */
        } lt;
    };
    unsigned flags;
};

#define INIT_TIRECODE_ISO_R(TW, AR, RD) { \
    .type = TIRECODE_ISO, \
    .tire_type = TIRE_RADIAL, \
    .iso = { \
        .tire_width = (TW), \
        .aspect_ratio = (AR), \
        .rim_diameter = (RD), \
    }, \
}
#define INIT_TIRECODE_UNKNOWN { \
    .type = TIRECODE_UNKNOWN, \
}
int snprint_tire(char *str, size_t size, const struct stirec_tire *tire, unsigned system);
static inline int print_tire(const struct stirec_tire *tire, unsigned system) {
    char buf[512];
    int ret;
    if ((ret = snprint_tire(buf, sizeof(buf), tire, system)) > 0)
        ret = printf("%s", buf);
    return ret;
}

#endif /* end of include guard: STIRECALC_H */

