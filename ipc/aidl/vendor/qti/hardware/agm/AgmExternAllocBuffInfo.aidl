/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

import android.hardware.common.NativeHandle;
/**
 * Externally allocated buffer info
 */
@VintfStability
parcelable AgmExternAllocBuffInfo {
    // unique native handle identifying extern memory allocation
    NativeHandle allocHandle;
    // size of external allocation
    int allocatedSize;
    //  offset of buffer within extern allocation
    int offset;
}
