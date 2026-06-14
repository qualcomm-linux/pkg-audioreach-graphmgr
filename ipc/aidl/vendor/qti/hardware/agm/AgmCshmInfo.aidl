/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.agm;

import android.hardware.common.NativeHandle;

import vendor.qti.hardware.agm.AgmCshmCacheType;

/* memID memID of the memory allocated
 * type type of the memory allocated
 * fd fd of the memory allocated
 * flags reserved flag for future usage*/
@VintfStability
parcelable AgmCshmInfo {
    int memID;
    AgmCshmCacheType type;
    NativeHandle allocHandle;
    int flags;
}
