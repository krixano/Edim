#include "edimcoder.h"

uint64_t hash_uint64(uint64_t x) {
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 32;
    return x;
}

uint64_t hash_ptr(const void *ptr) {
    return hash_uint64((uintptr_t)ptr);
}

uint64_t hash_mix(uint64_t x, uint64_t y) {
    x ^= y;
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 32;
    return x;
}

uint64_t hash_bytes(const void *ptr, size_t len) {
    uint64_t x = 0xcbf29ce484222325;
    const char *buf = (const char *)ptr;
    for (size_t i = 0; i < len; i++) {
        x ^= buf[i];
        x *= 0x100000001b3;
        x ^= x >> 32;
    }
    return x;
}

// ----------------------

uint64_t map_get_uint64_from_uint64(Map *map, uint64_t key) {
    if (map->len == 0) {
        return 0;
    }
    assert(IS_POW2(map->cap));
    size_t i = (size_t)hash_uint64(key);
    assert(map->len < map->cap);
    for (;;) {
        i &= map->cap - 1;
        if (map->keys[i] == key) {
            return map->vals[i];
        } else if (!map->keys[i]) {
            return 0;
        }
        i++;
    }
    return 0;
}

void map_grow(Map *map, size_t new_cap) {
    new_cap = CLAMP_MIN(new_cap, 16);
    Map new_map = {
        .keys = xcalloc(new_cap, sizeof(uint64_t)),
        .vals = xmalloc(new_cap * sizeof(uint64_t)),
        .cap = new_cap,
    };
    for (size_t i = 0; i < map->cap; i++) {
        if (map->keys[i]) {
            map_put_uint64_from_uint64(&new_map, map->keys[i], map->vals[i]);
        }
    }
    free((void *)map->keys);
    free(map->vals);
    *map = new_map;
}

void map_put_uint64_from_uint64(Map *map, uint64_t key, uint64_t val) {
    assert(key);
    if (!val) {
        return;
    }
    if (2*map->len >= map->cap) {
        map_grow(map, 2*map->cap);
    }
    assert(2*map->len < map->cap);
    assert(IS_POW2(map->cap));
    size_t i = (size_t)hash_uint64(key);
    for (;;) {
        i &= map->cap - 1;
        if (!map->keys[i]) {
            map->len++;
            map->keys[i] = key;
            map->vals[i] = val;
            return;
        } else if (map->keys[i] == key) {
            map->vals[i] = val;
            return;
        }
        i++;
    }
}

void *map_get(Map *map, const void *key) {
    return (void *)(uintptr_t)map_get_uint64_from_uint64(map, (uint64_t)(uintptr_t)key);
}

void map_put(Map *map, const void *key, void *val) {
    map_put_uint64_from_uint64(map, (uint64_t)(uintptr_t)key, (uint64_t)(uintptr_t)val);
}

void *map_get_from_uint64(Map *map, uint64_t key) {
    return (void *)(uintptr_t)map_get_uint64_from_uint64(map, key);
}

void map_put_from_uint64(Map *map, uint64_t key, void *val) {
    map_put_uint64_from_uint64(map, key, (uint64_t)(uintptr_t)val);
}

uint64_t map_get_uint64(Map *map, void *key) {
    return map_get_uint64_from_uint64(map, (uint64_t)(uintptr_t)key);
}

void map_put_uint64(Map *map, void *key, uint64_t val) {
    map_put_uint64_from_uint64(map, (uint64_t)(uintptr_t)key, val);
}

void map_test(void) {
    Map map = {0};
    enum { N = 1024 };
    for (size_t i = 1; i < N; i++) {
        map_put(&map, (void *)i, (void *)(i+1));
    }
    for (size_t i = 1; i < N; i++) {
        void *val = map_get(&map, (void *)i);
        assert(val == (void *)(i+1));
    }
}
