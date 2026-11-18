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
 * glgps (Broadcom GPS daemon) imports SSLv3_client_method for its LTO
 * download. BoringSSL dropped it, so the daemon would not even load.
 * Hand out the version-flexible TLS method instead; it never speaks SSLv3.
 */

#include <openssl/ssl.h>

extern "C" const SSL_METHOD* SSLv3_client_method(void) {
    return TLS_client_method();
}
