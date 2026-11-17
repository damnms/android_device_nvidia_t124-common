/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <malloc.h>
#include <stdlib.h>

/*
 * libglcore parses /proc/self/maps when a context is created and grows the
 * result buffer by 4 KiB per realloc. With scudo every growth of a large
 * (secondary) block maps a new region and copies the whole buffer, so a
 * process with thousands of mappings (WebView/Chromium) spends 10+ seconds
 * in eglCreateContext. Grow large blocks geometrically instead.
 */

static constexpr size_t kGeometricMin = 64 * 1024;

extern "C" void* nvrealc(void* ptr, size_t size) {
    if (ptr != nullptr && size != 0) {
        size_t usable = malloc_usable_size(ptr);
        if (size <= usable && size >= usable / 2) {
            return ptr;
        }
        if (size > usable && size >= kGeometricMin) {
            size_t grown = usable + usable / 2;
            if (grown > size) {
                size = grown;
            }
        }
    }
    return realloc(ptr, size);
}
