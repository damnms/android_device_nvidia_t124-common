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
 * The t124 camera stack (libscf, libnvfnet, libnvmm_camera_v3) requires
 * EGL_EXT_device_base and EGL_EXT_platform_device and opens its display
 * through an EGL device.
 * Neither the rel24 nvgpu driver nor the Android EGL loader provide them.
 * Their imports of eglQueryString and eglGetProcAddress are renamed to
 * scfQueryString and scfGetProcAddress (extract/patches.sh); these report
 * the extensions and emulate a single device whose display is the default
 * Android display. Everything else goes to the Android EGL loader.
 */

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <string.h>

#include <mutex>
#include <string>
#include <vector>

namespace {

const char kDeviceExtensions[] =
        " EGL_EXT_device_base EGL_EXT_device_enumeration EGL_EXT_device_query"
        " EGL_EXT_platform_base EGL_EXT_platform_device";

int sDeviceTag;
EGLDeviceEXT const kDevice = static_cast<EGLDeviceEXT>(&sDeviceTag);

EGLBoolean queryDevices(EGLint max_devices, EGLDeviceEXT* devices, EGLint* num_devices) {
    if (num_devices == nullptr) return EGL_FALSE;
    if (devices == nullptr) {
        *num_devices = 1;
        return EGL_TRUE;
    }
    if (max_devices < 1) return EGL_FALSE;
    devices[0] = kDevice;
    *num_devices = 1;
    return EGL_TRUE;
}

const char* queryDeviceString(EGLDeviceEXT device, EGLint name) {
    if (device != kDevice) return nullptr;
    return name == EGL_EXTENSIONS ? "" : nullptr;
}

EGLBoolean queryDeviceAttrib(EGLDeviceEXT, EGLint, EGLAttrib*) {
    return EGL_FALSE;
}

EGLBoolean queryDisplayAttrib(EGLDisplay dpy, EGLint attribute, EGLAttrib* value) {
    if (dpy == EGL_NO_DISPLAY || value == nullptr || attribute != EGL_DEVICE_EXT) {
        return EGL_FALSE;
    }
    *value = reinterpret_cast<EGLAttrib>(kDevice);
    return EGL_TRUE;
}

EGLDisplay getPlatformDisplay(EGLenum platform, void* native_display, const EGLint* attrib_list) {
    if (platform == EGL_PLATFORM_DEVICE_EXT) {
        if (native_display != kDevice) return EGL_NO_DISPLAY;
        return eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    std::vector<EGLAttrib> attribs;
    if (attrib_list != nullptr) {
        for (const EGLint* a = attrib_list; *a != EGL_NONE; a += 2) {
            attribs.push_back(a[0]);
            attribs.push_back(a[1]);
        }
        attribs.push_back(EGL_NONE);
    }
    return eglGetPlatformDisplay(platform, native_display,
                                 attribs.empty() ? nullptr : attribs.data());
}

}  // namespace

extern "C" const char* scfQueryString(EGLDisplay dpy, EGLint name) {
    const char* str = eglQueryString(dpy, name);
    if (dpy != EGL_NO_DISPLAY || name != EGL_EXTENSIONS) return str;

    static std::once_flag once;
    static std::string extensions;
    std::call_once(once, [str] {
        extensions = str != nullptr ? str : "EGL_EXT_client_extensions";
        extensions += kDeviceExtensions;
    });
    return extensions.c_str();
}

extern "C" __eglMustCastToProperFunctionPointerType scfGetProcAddress(const char* procname) {
    if (procname != nullptr) {
        if (!strcmp(procname, "eglQueryDevicesEXT"))
            return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(queryDevices);
        if (!strcmp(procname, "eglQueryDeviceStringEXT"))
            return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(queryDeviceString);
        if (!strcmp(procname, "eglQueryDeviceAttribEXT"))
            return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(queryDeviceAttrib);
        if (!strcmp(procname, "eglQueryDisplayAttribEXT"))
            return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(queryDisplayAttrib);
        if (!strcmp(procname, "eglGetPlatformDisplayEXT"))
            return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(getPlatformDisplay);
    }
    return eglGetProcAddress(procname);
}
