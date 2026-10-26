/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmBufferConfig {
    /**< number of buffers */
    int count;
    /**< size of each buffer */
    int size;
    /**< max metadata size a client attaches to a buffer */
    int maxMetadataSize;
}
