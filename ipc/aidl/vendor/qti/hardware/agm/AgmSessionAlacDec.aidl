/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmSessionAlacDec {
    // Frame length
    int frameLength;
    // version
    byte compatibleVersion;
    // bit depth
    byte bitDepth;
    // tuning parameters
    byte pb;
    // tuning parameters
    byte mb;
    // tuning parameters
    byte kb;
    // number of channels
    byte channels;
    // max run
    int maxRun;
    // max frame bytes
    int maxFrameBytes;
    // average bit rate
    int averageBitRate;
    // sample rate
    int sampleRate;
    // channel layout tag
    int channelLayoutTag;
}
