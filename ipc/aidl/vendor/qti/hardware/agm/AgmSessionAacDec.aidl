/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

/**
 * AAC decoder parameters
 */
@VintfStability
parcelable AgmSessionAacDec {
    // AAC format flag
    int formatFlag;
    // AAC obj type
    int objectType;
    // number of channels
    int channels;
    // PCE bits size
    int sizeOfPCEBits;
    // sample rate
    int sampleRate;
}
