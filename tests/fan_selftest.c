/* Self-test of runtime/nvn_vk_fan.h (MAC4: triangle fans as triangle lists on a device without fans). No GPU, no game.
 * Build and run from the repository root (any host):
 *   clang -O1 -I runtime tests/fan_selftest.c -o fan_selftest && ./fan_selftest
 * Every list triangle is checked against Vulkan's fan triangle (v[i+1], v[i+2], v[0]): the same three vertices in the same
 * winding (a rotation of that order), and the provoking vertex of the mode in use (first: v[i+1], last: v[i+2]). */
#include <stdio.h>
#include <string.h>
#include "nvn_vk_fan.h"

static int fails;
#define CHECK(c, what) do { if (!(c)) { printf("FAIL %s (line %d)\n", what, __LINE__); fails++; } else printf("ok   %s\n", what); } while (0)

/* the fans of an index list as Vulkan draws them with primitive restart: restart values split it, short fans draw nothing */
static uint32_t reference(const uint32_t *v, uint32_t count, int restart, uint32_t ones, uint32_t *tri) {
    uint32_t n = 0, start = 0;
    for (uint32_t i = 0; i <= count; i++) {
        if (i < count && !(restart && v[i] == ones)) continue;
        for (uint32_t k = start; k + 2 < i; k++) { tri[n++] = v[k + 1]; tri[n++] = v[k + 2]; tri[n++] = v[start]; }
        start = i + 1;
    }
    return n;
}
static int same_triangle(const uint32_t *got, const uint32_t *want, int last) {
    for (int r = 0; r < 3; r++)                      /* a rotation keeps the winding */
        if (got[0] == want[r] && got[1] == want[(r + 1) % 3] && got[2] == want[(r + 2) % 3])
            return last ? got[2] == want[1] : got[0] == want[0];
    return 0;
}
static int check_case(const void *idx, int t, uint32_t count, int restart, int last) {
    uint32_t v[64], want[192], got[192];
    const uint32_t ones = t == 0 ? 0xffu : t == 1 ? 0xffffu : 0xffffffffu;
    for (uint32_t i = 0; i < count; i++) v[i] = !idx ? i : t == 0 ? ((const uint8_t *)idx)[i] : t == 1 ? ((const uint16_t *)idx)[i] : ((const uint32_t *)idx)[i];
    uint32_t nw = reference(v, count, restart && idx, ones, want);
    memset(got, 0xcd, sizeof got);
    uint32_t ng = fan_to_list(idx, t, count, restart, last, got);
    if (ng != nw || ng > fan_list_cap(count)) { printf("     %u indices, %u expected, cap %u\n", ng, nw, fan_list_cap(count)); return 0; }
    for (uint32_t i = 0; i < ng; i += 3) if (!same_triangle(got + i, want + i, last)) { printf("     triangle %u: %u %u %u, fan %u %u %u\n", i / 3, got[i], got[i + 1], got[i + 2], want[i], want[i + 1], want[i + 2]); return 0; }
    return 1;
}

int main(void) {
    static const uint8_t i8[] = {3, 7, 1, 0xff, 9, 2, 5, 6, 0xff, 4, 0xff, 8, 1, 0xff, 0xff, 2, 3, 4};
    static const uint16_t i16[] = {10, 11, 12, 13, 0xffff, 20, 21, 22, 0xffff};
    static const uint32_t i32[] = {100, 0xffffffffu, 101, 102, 103, 104, 0xffffffffu, 105, 106};
    char what[96];
    for (int last = 0; last < 2; last++) {
        const char *pv = last ? "provoking last" : "provoking first";
        for (uint32_t n = 0; n <= 7; n++) { snprintf(what, sizeof what, "%u vertices without indices, %s", n, pv); CHECK(check_case(NULL, 2, n, 0, last), what); }
        snprintf(what, sizeof what, "8-bit indices with restarts and short fans, %s", pv); CHECK(check_case(i8, 0, sizeof i8, 1, last), what);
        snprintf(what, sizeof what, "8-bit indices, restart off (0xff is a vertex), %s", pv); CHECK(check_case(i8, 0, sizeof i8, 0, last), what);
        snprintf(what, sizeof what, "16-bit indices with restarts, %s", pv); CHECK(check_case(i16, 1, 9, 1, last), what);
        snprintf(what, sizeof what, "32-bit indices with restarts, %s", pv); CHECK(check_case(i32, 2, 9, 1, last), what);
        snprintf(what, sizeof what, "32-bit indices, restart off, %s", pv); CHECK(check_case(i32, 2, 9, 0, last), what);
    }
    {   uint32_t out[6];                              /* the order itself, as nvn_vk_fan.h documents it */
        CHECK(fan_to_list(NULL, 2, 4, 0, 0, out) == 6 && out[0] == 1 && out[1] == 2 && out[2] == 0 && out[3] == 2 && out[4] == 3 && out[5] == 0, "provoking first: (1 2 0) (2 3 0)");
        CHECK(fan_to_list(NULL, 2, 4, 0, 1, out) == 6 && out[0] == 0 && out[1] == 1 && out[2] == 2 && out[3] == 0 && out[4] == 2 && out[5] == 3, "provoking last: (0 1 2) (0 2 3)");
    }
    printf(fails ? "%d FAILED\n" : "all passed\n", fails);
    return fails != 0;
}
