/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmEventCallbackParameter {
    /**< identifies the module which generated event */
    int sourceModuleId;
    /**< identifies the event */
    int eventId;
    /**< payload associated with the event if any */
    byte[] eventPayload;
}
