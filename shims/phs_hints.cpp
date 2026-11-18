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

/*
 * libEGL_tegra sends PHS throughput hints from every process that uses GL.
 * In app processes the call into nvphsd fails at the transport level and
 * libnvphs aborts on the unchecked HIDL return (seen in the camera app's
 * RenderThread). Skip the hints in app processes and forward them unchanged
 * everywhere else.
 */

#include <dlfcn.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>

namespace {

constexpr uid_t kAppStart = 10000;   // AID_APP_START
constexpr uid_t kPerUser = 100000;   // AID_USER_OFFSET
constexpr uint32_t kUsecaseNull = 0; // NvUsecase_NULL terminates the hint list
constexpr int kMaxHints = 4;

typedef int (*SendHintsFn)(uint32_t, uint32_t, ...);

struct Hint {
    uint32_t usecase;
    uint32_t type;
    uint32_t value;
    uint32_t timeout;
};

}  // namespace

extern "C" int NvPHSSendThroughputHints(uint32_t client_tag, uint32_t flags, ...) {
    if (getuid() % kPerUser >= kAppStart) return 0;

    static SendHintsFn real =
            reinterpret_cast<SendHintsFn>(dlsym(RTLD_NEXT, "NvPHSSendThroughputHints"));
    if (real == nullptr) return -1;

    Hint h[kMaxHints];
    int n = 0;
    va_list ap;
    va_start(ap, flags);
    while (n < kMaxHints) {
        uint32_t usecase = va_arg(ap, uint32_t);
        if (usecase == kUsecaseNull) break;
        h[n].usecase = usecase;
        h[n].type = va_arg(ap, uint32_t);
        h[n].value = va_arg(ap, uint32_t);
        h[n].timeout = va_arg(ap, uint32_t);
        n++;
    }
    va_end(ap);

    switch (n) {
        case 0:
            return real(client_tag, flags, kUsecaseNull);
        case 1:
            return real(client_tag, flags, h[0].usecase, h[0].type, h[0].value, h[0].timeout,
                        kUsecaseNull);
        case 2:
            return real(client_tag, flags, h[0].usecase, h[0].type, h[0].value, h[0].timeout,
                        h[1].usecase, h[1].type, h[1].value, h[1].timeout, kUsecaseNull);
        case 3:
            return real(client_tag, flags, h[0].usecase, h[0].type, h[0].value, h[0].timeout,
                        h[1].usecase, h[1].type, h[1].value, h[1].timeout,
                        h[2].usecase, h[2].type, h[2].value, h[2].timeout, kUsecaseNull);
        default:
            return real(client_tag, flags, h[0].usecase, h[0].type, h[0].value, h[0].timeout,
                        h[1].usecase, h[1].type, h[1].value, h[1].timeout,
                        h[2].usecase, h[2].type, h[2].value, h[2].timeout,
                        h[3].usecase, h[3].type, h[3].value, h[3].timeout, kUsecaseNull);
    }
}
