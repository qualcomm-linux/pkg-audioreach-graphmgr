/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmSessionOpusDec {
    int bitStreamFormat;
    int type;
    byte version;
    byte channels;
    int preSkip;
    long sampleRate;
    int outputGain;
    byte mappingFamily;
    byte streamCount;
    byte coupledCount;
    byte [8] channelMap;
    byte [3] reserved;
}
