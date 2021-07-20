/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "battery_framework.h"
#include "battery_info.h"
#include <stdint.h>
#include <stdlib.h>
#include <ohos_errno.h>
#include <pthread.h>
#include <unistd.h>
#include "hilog_wrapper.h"
#include "battery_interface.h"
#include "ibattery.h"
#include "iunknown.h"

#define MAX_DATA_LEN    1024

typedef struct {
    INHERIT_IUNKNOWNENTRY(BatteryProxyInterface);
} BatteryProxyEntry;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static BatteryProxyInterface *g_intf = NULL;

static int32_t BatteryCallbackInt(IOwner owner, int32_t code, IpcIo *reply)
{
    if ((reply == NULL) || (owner == NULL)) {
        POWER_HILOGE("Invalid parameter");
        return EC_INVALID;
    }

    int32_t *ret = (int32_t *)owner;
    *ret = IpcIoPopInt32(reply);
    POWER_HILOGD("BatteryCallback():start");
    return EC_SUCCESS;
}

static int32_t GetBatSocProxy(IUnknown *iUnknown)
{

    POWER_HILOGD("static int32_t GetBatSocProxy(IUnknown *iUnknown):start");
    IpcIo request;
    char buffer[MAX_DATA_LEN];
    IpcIoInit(&request, buffer, MAX_DATA_LEN, 0);
    int32_t ret;

    BatteryProxyInterface *proxy = (BatteryProxyInterface *)iUnknown;
    proxy->Invoke((IClientProxy *)proxy, BATTERY_FUNCID_GETSOC, &request, &ret, BatteryCallbackInt);
    POWER_HILOGD("static int32_t GetBatSocProxy(IUnknown *iUnknown):end");

    return ret;
}
static BatteryChargeState GetChargingStatusProxy(IUnknown *iUnknown)
{
    IpcIo request;
    char buffer[MAX_DATA_LEN];
    IpcIoInit(&request, buffer, MAX_DATA_LEN, 0);
    int32_t ret;

    BatteryProxyInterface *proxy = (BatteryProxyInterface *)iUnknown;
    proxy->Invoke((IClientProxy *)proxy, BATTERY_FUNCID_GETCHARGING, &request, &ret, BatteryCallbackInt);
    POWER_HILOGD("GetChargingStatusProxy():start");

    return ret;
}
static BatteryHealthState GetHealthStatusProxy(IUnknown *iUnknown)
{
    IpcIo request;
    char buffer[MAX_DATA_LEN];
    IpcIoInit(&request, buffer, MAX_DATA_LEN, 0);
    int32_t ret;

    BatteryProxyInterface *proxy = (BatteryProxyInterface *)iUnknown;
    proxy->Invoke((IClientProxy *)proxy, BATTERY_FUNCID_GETHEALTH, &request, &ret, BatteryCallbackInt);
    POWER_HILOGD("GetHealthStatusProxy():start");

    return ret;
}
static BatteryPluggedType GetPluggedTypeProxy(IUnknown *iUnknown)
{
    IpcIo request;
    char buffer[MAX_DATA_LEN];
    IpcIoInit(&request, buffer, MAX_DATA_LEN, 0);
    int32_t ret;

    BatteryProxyInterface *proxy = (BatteryProxyInterface *)iUnknown;
    proxy->Invoke((IClientProxy *)proxy, BATTERY_FUNCID_GETPLUGTYPE, &request, &ret, BatteryCallbackInt);
    POWER_HILOGD("GetPluggedTypeProxy():start");

    return ret;
}
static int32_t GetBatVoltageProxy(IUnknown *iUnknown)
{
    IpcIo request;
    char buffer[MAX_DATA_LEN];
    IpcIoInit(&request, buffer, MAX_DATA_LEN, 0);
    int32_t ret;

    BatteryProxyInterface *proxy = (BatteryProxyInterface *)iUnknown;
    proxy->Invoke((IClientProxy *)proxy, BATTERY_FUNCID_GETVOLTAGE, &request, &ret, BatteryCallbackInt);
    POWER_HILOGD("GetBatVoltageProxy():start");
    return ret;
}

static int32_t BatteryCallbackBuff(IOwner owner, int32_t code, IpcIo *reply)
{
    size_t len = 0;

    if ((reply == NULL) || (owner == NULL)) {
        POWER_HILOGE("Invalid parameter");
        return EC_INVALID;
    }

    char **strbuff=(char **)owner;
    *strbuff = IpcIoPopString(reply, &len);
    if (strbuff == NULL || len == 0) {
        POWER_HILOGD("BatteryCallbackBuff():strbuff == NULL || len == 0 endl");
        return EC_INVALID;
    }
    POWER_HILOGD("BatteryCallback():start");
    return EC_SUCCESS;
}

static char* GetBatTechnologyProxy(IUnknown *iUnknown)
{
    IpcIo request;
    char buffer[MAX_DATA_LEN];
    IpcIoInit(&request, buffer, MAX_DATA_LEN, 0);
    char* string;

    BatteryProxyInterface *proxy = (BatteryProxyInterface *)iUnknown;
    proxy->Invoke((IClientProxy *)proxy, BATTERY_FUNCID_GETTECHNOLONY, &request, &string, BatteryCallbackBuff);
    return string;
}

static int32_t GetBatTemperatureProxy(IUnknown *iUnknown)
{
    IpcIo request;
    char buffer[MAX_DATA_LEN];
    IpcIoInit(&request, buffer, MAX_DATA_LEN, 0);
    int32_t ret;

    BatteryProxyInterface *proxy = (BatteryProxyInterface *)iUnknown;
    proxy->Invoke((IClientProxy *)proxy, BATTERY_FUNCID_GETTEMPERATURE, &request, &ret, BatteryCallbackInt);
    POWER_HILOGD("GetBatTemperatureProxy():start");

    return ret;
}


static void *CreatClient(const char *service, const char *feature, uint32_t size)
{
    (void)service;
    (void)feature;
    uint32_t len = size + sizeof(BatteryProxyEntry);
    uint8_t *client = malloc(len);
    if (client == NULL) {
        POWER_HILOGE("CreatClient():malloc return NULL");
        return NULL;
    }

    (void)memset_s(client, len, 0, len);
    BatteryProxyEntry *entry = (BatteryProxyEntry *)&client[size];
    entry->ver =  ((uint16)CLIENT_PROXY_VER | (uint16)DEFAULT_VERSION);
    entry->ref = 1;
    entry->iUnknown.QueryInterface = IUNKNOWN_QueryInterface;
    entry->iUnknown.AddRef = IUNKNOWN_AddRef;
    entry->iUnknown.Release = IUNKNOWN_Release;
    entry->iUnknown.Invoke = NULL;
    entry->iUnknown.GetBatSocFunc = GetBatSocProxy;
    entry->iUnknown.GetChargingStatusFunc = GetChargingStatusProxy;
    entry->iUnknown.GetHealthStatusFunc = GetHealthStatusProxy;
    entry->iUnknown.GetPluggedTypeFunc = GetPluggedTypeProxy;
    entry->iUnknown.GetBatVoltageFunc = GetBatVoltageProxy;
    entry->iUnknown.GetBatTechnologyFunc = GetBatTechnologyProxy;
    entry->iUnknown.GetBatTemperatureFunc = GetBatTemperatureProxy;
    return client;
}

static void DestroyClient(const char *service, const char *feature, void *iproxy)
{
    free(iproxy);
}

static BatteryProxyInterface *GetBatteryInterface(void)
{
    if (g_intf != NULL) {
        return g_intf;
    }
    pthread_mutex_lock(&g_mutex);
    if (g_intf != NULL) {
        pthread_mutex_unlock(&g_mutex);
        return g_intf;
    }
    SAMGR_RegisterFactory(BATTERY_SERVICE, BATTERY_INNER, CreatClient, DestroyClient);

    IUnknown *iUnknown = GetBatteryIUnknown();
    if (iUnknown == NULL) {
        POWER_HILOGE("Failed to get batterymgr iUnknown");
        pthread_mutex_unlock(&g_mutex);
        return NULL;
    }

    int ret = iUnknown->QueryInterface(iUnknown, DEFAULT_VERSION, (void **)&g_intf);
    if ((ret != EC_SUCCESS) || (g_intf == NULL)) {
        POWER_HILOGE("Failed to query batteryInterface interface");
        pthread_mutex_unlock(&g_mutex);
        return NULL;
    }
    pthread_mutex_unlock(&g_mutex);
    POWER_HILOGI("Succeed to get batteryInterface proxy interface");
    return g_intf;
}

int32_t GetBatSoc()
{
    POWER_HILOGI("GetBatSoc():start........");
    int32_t ret = EC_FAILURE;
    BatteryProxyInterface *intf = GetBatteryInterface();
    if (intf == NULL) {    
        POWER_HILOGI("intf == NULL:err........");
    }
    if (intf->GetBatSocFunc == NULL) {
        POWER_HILOGI("(intf->GetBatSocFunc:err........");
    }
    if ((intf != NULL) && (intf->GetBatSocFunc != NULL)) {
        ret = intf->GetBatSocFunc((IUnknown *)intf);
    }
    POWER_HILOGI("GetBatSoc():end........");
    POWER_HILOGI("GetBatSoc():ret = %d........", ret);
    return ret;
}

BatteryChargeState GetChargingStatus()
{
    BatteryChargeState state = CHARGE_STATE_NONE;
    BatteryProxyInterface *intf = GetBatteryInterface();
    if ((intf != NULL) && (intf->GetBatSocFunc != NULL)) {
        state = intf->GetChargingStatusFunc((IUnknown *)intf);
    }
    return state;
}

BatteryHealthState GetHealthStatus()
{
    BatteryHealthState state = HEALTH_STATE_UNKNOWN;
    BatteryProxyInterface *intf = GetBatteryInterface();
    if ((intf != NULL) && (intf->GetBatSocFunc != NULL)) {
        state = intf->GetHealthStatusFunc((IUnknown *)intf);
    }
    return state;
}

BatteryPluggedType GetPluggedType()
{
    BatteryPluggedType state = PLUGGED_TYPE_NONE;
    BatteryProxyInterface *intf = GetBatteryInterface();
    if ((intf != NULL) && (intf->GetBatSocFunc != NULL)) {
        state = intf->GetPluggedTypeFunc((IUnknown *)intf);
    }
    return state;
}

int32_t GetBatVoltage()
{
    int32_t ret = EC_FAILURE;
    BatteryProxyInterface *intf = GetBatteryInterface();
    if ((intf != NULL) && (intf->GetBatSocFunc != NULL)) {
        ret = intf->GetBatVoltageFunc((IUnknown *)intf);
    }
    return ret;
}

char* GetBatTechnology()
{
    char* strbuff = NULL;
    BatteryProxyInterface *intf = GetBatteryInterface();
    if ((intf != NULL) && (intf->GetBatSocFunc != NULL)) {
        strbuff = intf->GetBatTechnologyFunc((IUnknown *)intf);
    }
    return strbuff;
}

int32_t GetBatTemperature()
{
    int32_t ret = EC_FAILURE;
    BatteryProxyInterface *intf = GetBatteryInterface();
    if ((intf != NULL) && (intf->GetBatSocFunc != NULL)) {
        ret = intf->GetBatTemperatureFunc((IUnknown *)intf);
    }
    return ret;
}