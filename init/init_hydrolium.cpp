/*
   Copyright (c) 2015, The Linux Foundation. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "log.h"
#include "property_service.h"
#include "util.h"

#include <android-base/strings.h>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

using android::base::Trim;

static std::string build_code_name;

std::string property_get(const std::string& key)
{
  const std::string default_value = "";
  const prop_info* pi = __system_property_find(key.c_str());

  if (pi == nullptr) return default_value;

  char buf[PROP_VALUE_MAX];
  if (__system_property_read(pi, nullptr, buf) > 0) return buf;

  // If the property exists but is empty, also return the default value.
  // Since we can't remove system properties, "empty" is traditionally
  // the same as "missing" (this was true for cutils' property_get).
  return default_value;
}

void property_override(char const prop[], char const value[])
{
    prop_info *pi;

    pi = (prop_info*) __system_property_find(prop);
    if (pi)
        __system_property_update(pi, value, strlen(value));
    else
        __system_property_add(prop, strlen(prop), value, strlen(value));
}

void code_name_property_override(const std::string& key,
                               const std::string& code_name)
{
    std::string value = property_get(key);
    size_t pos = value.find(build_code_name);

    if (value != "" && pos != value.npos) {
        while(pos != value.npos) {
            value.erase(pos, build_code_name.size());
            value.insert(pos, code_name);
            pos = value.find(build_code_name);
        }
        property_override(key.c_str(), value.c_str());
    }
}

void vendor_load_properties()
{
    build_code_name = property_get("ro.build.product");
    const char *soc_id_file = "/sys/devices/soc0/soc_id";
    std::string soc_id = "-1";
    std::string code_name = build_code_name;

    /* get raw SOC_ID */
    if (read_file(soc_id_file, &soc_id)) {
        if (Trim(soc_id) == "266" /* hydrogen */) {
            code_name = "hydrogen";
        } else if (Trim(soc_id) == "278" /* helium */) {
            code_name = "helium";
        } else {
            ERROR("init_hydrolium: unexpected soc_id = '%s'\n", soc_id.c_str());
            return;
        }
    } else {
        ERROR("init_hydrolium: failed to read soc_id\n");
        return;
    }

    NOTICE("init_hydrolium: Device SOC_ID is %s", soc_id.c_str());
    NOTICE("init_hydrolium: Setting up system properties for %s...\n", code_name.c_str());

    property_override("ro.product.device", code_name.c_str());
    property_override("ro.build.product", code_name.c_str());
    property_override("ro.cm.device", code_name.c_str());
    property_override("ro.product.name", code_name.c_str());
    code_name_property_override("ro.build.fingerprint", code_name);
    code_name_property_override("ro.build.flavor", code_name);
    code_name_property_override("ro.build.description", code_name);
    code_name_property_override("ro.build.display.id", code_name);
}

