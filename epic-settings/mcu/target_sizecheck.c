/* Cross-compile sizecheck for epic-settings. */

#include "epic_settings.h"

typedef struct {
    unsigned char a;
    unsigned char b;
} tiny_settings_t;

/**
 * @brief Cross-compile sizecheck: touch every epic-settings entry point.
 *
 * @return the last settings byte, so the call results are observable
 */
int main(void)
{
    tiny_settings_t cfg = { 1u, 2u };
    tiny_settings_t def = { 3u, 4u };

    (void)epic_settings_save(0x10u, &cfg, (unsigned char)sizeof(cfg));
    (void)epic_settings_load(0x10u, &cfg, (unsigned char)sizeof(cfg));
    (void)epic_settings_load_or_default(0x20u, &cfg, (unsigned char)sizeof(cfg), &def);
    return (int)cfg.a;
}
