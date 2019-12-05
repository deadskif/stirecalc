#include <stdio.h>
#include <string.h>

#include "oem_tires.h"
#include "stirecalc.h"

struct oem_tire {
    const char *manufacturer;
    const char *model;
    const struct stirec_tire **tires;
};
#define OEM_TIRES(MAN, MODEL, ...) { \
    .manufacturer = (MAN), \
    .model = (MODEL), \
    .tires = (const struct stirec_tire *[]){ \
        __VA_ARGS__, \
        NULL, \
    }, \
}
static const struct stirec_tire iso205_70R15 = INIT_TIRECODE_ISO_R(205, 70, 15);
static const struct stirec_tire iso215_65R16 = INIT_TIRECODE_ISO_R(215, 65, 16);
static const struct stirec_tire iso225_75R16 = INIT_TIRECODE_ISO_R(225, 75, 16);

#define ISOR(TW, AR, RD) (struct stirec_tire[]){ INIT_TIRECODE_ISO_R((TW), (AR), (RD)) } 
const struct oem_tire oem_tires[] = {
    OEM_TIRES("Suzuki", "Jimny III", ISOR(175, 80, 16)),
    OEM_TIRES("Suzuki", "Jimny Sierra III", &iso205_70R15),
    OEM_TIRES("Suzuki", "Jimny Sierra IV", ISOR(195, 80, 15)),
    OEM_TIRES("Renault", "Duster", &iso215_65R16),
    OEM_TIRES("Lada", "4x4", ISOR(185, 75, 16)),
    OEM_TIRES("Lada", "4x4 Bronto", ISOR(235, 75, 15)),
    OEM_TIRES("UAZ", "Hunter", &iso225_75R16),
    OEM_TIRES("UAZ", "Patriot", &iso225_75R16, ISOR(235, 70, 16), ISOR(245, 70, 16), ISOR(245, 60, 18)),
    OEM_TIRES("Chevrolet", "Niva", ISOR(205, 75, 15), &iso205_70R15, &iso215_65R16),
    OEM_TIRES("Toyota", "Fortuner", ISOR(265, 65, 17), ISOR(265, 60, 80)),
};

int print_all_oem_tires(void)
{
    for (size_t m = 0; m < sizeof(oem_tires)/sizeof(*oem_tires); m++) {
        const struct oem_tire *oem = oem_tires + m;
        printf("%s %s: ", oem->manufacturer, oem->model);
        for (const struct stirec_tire **w = oem->tires; *w; w++) {
            print_tire(*w, (*w)->type);
            printf(" ");
        }
        printf("\n");
    }
    return 0;
}
