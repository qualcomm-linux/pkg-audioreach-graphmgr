/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.AgmEventReadWriteDonePayload;

@VintfStability
parcelable AgmReadWriteEventCallbackParams {
    // identifies the module which generated event
    int sourceModuleId;
    // identifies the event
    int eventId;
    // payload associated with the event if any
    AgmEventReadWriteDonePayload payload;
}
