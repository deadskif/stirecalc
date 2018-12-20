#include <stdio.h>
#include <string.h>

#include "oem_tires.h"
#include "stirecalc.h"

struct oem_wheel {
    const char *manufacturer;
    const char *model;
    const struct wheel **wheels;
};
#define OEM_WHEEL(MAN, MODEL, ...) { \
    .manufacturer = (MAN), \
    .model = (MODEL), \
    .wheels = (const struct wheel *[]){ \
        __VA_ARGS__, \
        NULL, \
    }, \
}
static const struct wheel iso205_70R15 = INIT_WHEEL_ISO_R(205, 70, 15);
static const struct wheel iso225_75R16 = INIT_WHEEL_ISO_R(225, 75, 16);

#define ISOR(TW, AR, RD) (struct wheel[]){ INIT_WHEEL_ISO_R((TW), (AR), (RD)) } 
const struct oem_wheel oem_wheels[] = {
    OEM_WHEEL("Suzuki", "Jimny III", ISOR(175, 80, 16)),
    OEM_WHEEL("Suzuki", "Jimny Sierra III", &iso205_70R15),
    OEM_WHEEL("Suzuki", "Jimny Sierra IV", ISOR(195, 80, 15)),
    OEM_WHEEL("Renault", "Duster", ISOR(215, 65, 16)),
    OEM_WHEEL("Lada", "4x4", ISOR(185, 75, 16)),
    OEM_WHEEL("Lada", "4x4 Bronto", ISOR(235, 75, 15)),
    OEM_WHEEL("UAZ", "Hunter", &iso225_75R16),
    OEM_WHEEL("UAZ", "Patriot", &iso225_75R16, ISOR(235, 70, 16), ISOR(245, 70, 16), ISOR(245, 60, 18)),
};

int print_all_oem_wheels(void)
{
    for (size_t m = 0; m < sizeof(oem_wheels)/sizeof(*oem_wheels); m++) {
        const struct oem_wheel *oem = oem_wheels + m;
        printf("%s %s: ", oem->manufacturer, oem->model);
        for (const struct wheel **w = oem->wheels; *w; w++) {
            print_wheel(*w, (*w)->type);
            printf(" ");
        }
        printf("\n");
    }
    return 0;
}
