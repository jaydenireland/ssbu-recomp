/* MAC4: triangle fans as triangle lists, for a GPU that cannot draw fans (Metal under MoltenVK: the portability subset's
 * triangleFans is 0). Included by nvn_vk_draw.c (macOS) and by tests/fan_selftest.c: no Vulkan, no game state here.
 *
 * Fan triangle i is (v[i+1], v[i+2], v[0]) in Vulkan's order. The list keeps that winding and the provoking vertex of the
 * mode in use: first vertex -> (i+1, i+2, 0), last vertex (VK_EXT_provoking_vertex, nvn_vk enables LAST) -> (0, i+1, i+2),
 * a rotation of the same triangle whose last vertex is v[i+2]. */
#ifndef NVN_VK_FAN_H
#define NVN_VK_FAN_H
#include <stdint.h>

/* The most indices fan_to_list writes for a fan of count vertices. */
static uint32_t fan_list_cap(uint32_t count) { return count >= 3 ? (count - 2) * 3 : 0; }

/* idx: the fan's indices, t: their width (0 8-bit, 1 16-bit, 2 32-bit); NULL = a draw without indices (vertex numbers 0..count-1,
 * the first vertex goes in the draw's vertex offset). restart: an all-ones index of that width ends the fan and the next index
 * starts a new one (as primitive restart does). last: the provoking vertex is the last one. out: fan_list_cap(count) entries.
 * Returns the number of indices written. */
static uint32_t fan_to_list(const void *idx, int t, uint32_t count, int restart, int last, uint32_t *out) {
    const uint32_t ones = t == 0 ? 0xffu : t == 1 ? 0xffffu : 0xffffffffu;
    uint32_t n = 0, have = 0, v0 = 0, prev = 0;     /* have: vertices of the current fan so far */
    for (uint32_t i = 0; i < count; i++) {
        uint32_t v = !idx ? i : t == 0 ? ((const uint8_t *)idx)[i] : t == 1 ? ((const uint16_t *)idx)[i] : ((const uint32_t *)idx)[i];
        if (restart && idx && v == ones) { have = 0; continue; }
        if (have == 0) v0 = v;
        else if (have >= 2) {
            if (last) { out[n] = v0; out[n + 1] = prev; out[n + 2] = v; }
            else { out[n] = prev; out[n + 1] = v; out[n + 2] = v0; }
            n += 3;
        }
        prev = v; have++;
    }
    return n;
}
#endif
