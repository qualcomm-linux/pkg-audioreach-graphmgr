/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmSessionAacEnc {
    // encoder bitrate
    int bitRate;
    // global cutoff frequency
    int globalCutOffFrequency;
    // encoder mode
    int mode;
    // format flags
    int formatFlags;
}
