/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ResponderHelper.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/RDMEnums.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::string;
using std::vector;

template<typename T>
static bool GenericExtractValue(const RDMRequest *request, T *output) {
  T value;
  if (request->ParamDataSize() != sizeof(value)) {
    return false;
  }

  memcpy(reinterpret_cast<uint8_t*>(&value), request->ParamData(),
         sizeof(value));
  *output = NetworkToHost(value);
  return true;
}

bool ResponderHelper::ExtractUInt8(const RDMRequest *request,
                                   uint8_t *output) {
  return GenericExtractValue(request, output);
}

bool ResponderHelper::ExtractUInt16(const RDMRequest *request,
                                    uint16_t *output) {
  return GenericExtractValue(request, output);
}

bool ResponderHelper::ExtractUInt32(const RDMRequest *request,
                                    uint32_t *output) {
  return GenericExtractValue(request, output);
}


const RDMResponse *ResponderHelper::GetDeviceInfo(
    const RDMRequest *request,
    uint16_t device_model,
    rdm_product_category product_category,
    uint32_t software_version,
    uint16_t dmx_footprint,
    uint8_t current_personality,
    uint8_t personality_count,
    uint16_t dmx_start_address,
    uint16_t sub_device_count,
    uint8_t sensor_count,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  struct device_info_s {
    uint16_t rdm_version;
    uint16_t model;
    uint16_t product_category;
    uint32_t software_version;
    uint16_t dmx_footprint;
    uint8_t current_personality;
    uint8_t personality_count;
    uint16_t dmx_start_address;
    uint16_t sub_device_count;
    uint8_t sensor_count;
  } __attribute__((packed));

  struct device_info_s device_info;
  device_info.rdm_version = HostToNetwork(static_cast<uint16_t>(0x100));
  device_info.model = HostToNetwork(device_model);
  device_info.product_category = HostToNetwork(
      static_cast<uint16_t>(product_category));
  device_info.software_version = HostToNetwork(software_version);
  device_info.dmx_footprint = HostToNetwork(dmx_footprint);
  device_info.current_personality = current_personality;
  device_info.personality_count = personality_count;
  device_info.dmx_start_address = HostToNetwork(dmx_start_address);
  device_info.sub_device_count = HostToNetwork(sub_device_count);
  device_info.sensor_count = sensor_count;
  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&device_info),
      sizeof(device_info),
      RDM_ACK,
      queued_message_count);
}

/**
 * Get the clock response.
 */
const RDMResponse *ResponderHelper::GetRealTimeClock(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  struct clock_s {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
  } __attribute__((packed));

  time_t now;
  now = time(NULL);
  struct tm tm_now;
  localtime_r(&now, &tm_now);

  struct clock_s clock;
  clock.year = HostToNetwork(static_cast<uint16_t>(1900 + tm_now.tm_year));
  clock.month = tm_now.tm_mon + 1;
  clock.day = tm_now.tm_mday;
  clock.hour = tm_now.tm_hour;
  clock.minute = tm_now.tm_min;
  clock.second = tm_now.tm_sec;

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&clock),
      sizeof(clock));
}

/*
 * Handle a request that returns a string
 */
const RDMResponse *ResponderHelper::GetString(
    const RDMRequest *request,
    const std::string &value,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  return GetResponseFromData(
        request,
        reinterpret_cast<const uint8_t*>(value.data()),
        value.size(),
        RDM_ACK,
        queued_message_count);
}

const RDMResponse *ResponderHelper::GetBoolValue(const RDMRequest *request,
                                                 bool value,
                                                 uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  uint8_t param = value ? 1 : 0;

  return GetResponseFromData(request, &param, sizeof(param),
                             RDM_ACK, queued_message_count);
}

const RDMResponse *ResponderHelper::SetBoolValue(const RDMRequest *request,
                                                 bool *value,
                                                 uint8_t queued_message_count) {
  uint8_t arg;
  if (!ResponderHelper::ExtractUInt8(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  if (arg == 0 || arg == 1) {
    *value = arg;
    return new RDMSetResponse(
      request->DestinationUID(),
      request->SourceUID(),
      request->TransactionNumber(),
      RDM_ACK,
      queued_message_count,
      request->SubDevice(),
      request->ParamId(),
      NULL,
      0);
  } else {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, queued_message_count);
  }
}

template<typename T>
static const RDMResponse *GenericGetIntValue(const RDMRequest *request,
                                             T value) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }
  T param = HostToNetwork(value);
  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&param),
    sizeof(param));
}

const RDMResponse *ResponderHelper::GetUInt8Value(const RDMRequest *request,
                                                 uint8_t value) {
  return GenericGetIntValue(request, value);
}

const RDMResponse *ResponderHelper::GetUInt16Value(const RDMRequest *request,
                                                 uint16_t value) {
  return GenericGetIntValue(request, value);
}

const RDMResponse *ResponderHelper::GetUInt32Value(const RDMRequest *request,
                                                 uint32_t value) {
  return GenericGetIntValue(request, value);
}

template<typename T>
static const RDMResponse *GenericSetIntValue(const RDMRequest *request,
                                             T *value) {
  if (!GenericExtractValue(request, value)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  return new RDMSetResponse(
    request->DestinationUID(),
    request->SourceUID(),
    request->TransactionNumber(),
    RDM_ACK,
    0,
    request->SubDevice(),
    request->ParamId(),
    NULL,
    0);
}

const RDMResponse *ResponderHelper::SetUInt8Value(const RDMRequest *request,
                                                 uint8_t *value) {
  return GenericSetIntValue(request, value);
}

const RDMResponse *ResponderHelper::SetUInt16Value(const RDMRequest *request,
                                                 uint16_t *value) {
  return GenericSetIntValue(request, value);
}

const RDMResponse *ResponderHelper::SetUInt32Value(const RDMRequest *request,
                                                 uint32_t *value) {
  return GenericSetIntValue(request, value);
}
}  // namespace rdm
}  // namespace ola