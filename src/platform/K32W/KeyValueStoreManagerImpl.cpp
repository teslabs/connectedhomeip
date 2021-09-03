/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Platform-specific key value storage implementation for K32W
 */
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <lib/support/CHIPMem.h>
#include <list>
#include <platform/K32W/K32WConfig.h>
#include <platform/KeyValueStoreManager.h>
#include <string>

#include "PDM.h"

#include <unordered_map>

namespace chip {
namespace DeviceLayer {
namespace PersistedStorage {

/* TODO: adjust this value */
#define MAX_NO_OF_KEYS 255

KeyValueStoreManagerImpl KeyValueStoreManagerImpl::sInstance;

/* hashmap having:
 * 	- the matter key value as key;
 * 	- internal PDM identifier as value;
 */
std::unordered_map<std::string, uint8_t> kvs;

/* list containing the key identifiers */
std::list<uint8_t> key_ids;

CHIP_ERROR KeyValueStoreManagerImpl::_Get(const char * key, void * value, size_t value_size, size_t * read_bytes_size,
                                          size_t offset_bytes)
{
    CHIP_ERROR err     = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    uint8_t pdm_id_kvs = chip::DeviceLayer::Internal::K32WConfig::kPDMId_KVS;
    std::unordered_map<std::string, uint8_t>::const_iterator it;
    size_t read_bytes = 0;
    uint8_t key_id    = 0;

    VerifyOrExit((key != NULL) && (value != NULL), err = CHIP_ERROR_INVALID_ARGUMENT);

    if ((it = kvs.find(key)) != kvs.end())
    {
        key_id = it->second;

        ChipLogProgress(DeviceLayer, "KVS, get key id:: %i", key_id);
        err = chip::DeviceLayer::Internal::K32WConfig::ReadConfigValueBin(
            chip::DeviceLayer::Internal::K32WConfigKey(pdm_id_kvs, key_id), (uint8_t *) value, value_size, read_bytes);
        *read_bytes_size = read_bytes;
    }

exit:
    return err;
}

CHIP_ERROR KeyValueStoreManagerImpl::_Put(const char * key, const void * value, size_t value_size)
{
    CHIP_ERROR err = CHIP_ERROR_INVALID_ARGUMENT;
    uint8_t key_id;
    uint8_t pdm_id_kvs = chip::DeviceLayer::Internal::K32WConfig::kPDMId_KVS;

    VerifyOrExit((key != NULL) && (value != NULL), err = CHIP_ERROR_INVALID_ARGUMENT);

    if (kvs.find(key) == kvs.end())
    {
        for (key_id = 0; key_id < MAX_NO_OF_KEYS; key_id++)
        {
            std::list<uint8_t>::iterator iter = std::find(key_ids.begin(), key_ids.end(), key_id);

            if (iter == key_ids.end())
            {
                key_ids.push_back(key_id);

                ChipLogProgress(DeviceLayer, "KVS, put key id:: %i", key_id);
                err = chip::DeviceLayer::Internal::K32WConfig::WriteConfigValueBin(
                    chip::DeviceLayer::Internal::K32WConfigKey(pdm_id_kvs, key_id), (uint8_t *) value, value_size);
                break;
            }
        }
    }

exit:
    return err;
}

CHIP_ERROR KeyValueStoreManagerImpl::_Delete(const char * key)
{
    CHIP_ERROR err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    std::unordered_map<std::string, uint8_t>::const_iterator it;
    uint8_t pdm_id_kvs = chip::DeviceLayer::Internal::K32WConfig::kPDMId_KVS;
    uint8_t key_id     = 0;

    VerifyOrExit(key != NULL, err = CHIP_ERROR_INVALID_ARGUMENT);

    if ((it = kvs.find(key)) != kvs.end())
    {
        key_id = it->second;
        key_ids.remove(key_id);
        kvs.erase(it);

        ChipLogProgress(DeviceLayer, "KVS, delete key id:: %i", key_id);
        err = chip::DeviceLayer::Internal::K32WConfig::ClearConfigValue(
            chip::DeviceLayer::Internal::K32WConfigKey(pdm_id_kvs, key_id));
    }

exit:
    return err;
}

} // namespace PersistedStorage
} // namespace DeviceLayer
} // namespace chip
