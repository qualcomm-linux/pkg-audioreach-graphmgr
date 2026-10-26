/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

import android.hardware.common.NativeHandle;

@VintfStability
parcelable MmapBufInfo {
    NativeHandle dataFdHandle;
    int dataSize;
    NativeHandle positionFdHandle;
    int posSize;
}
