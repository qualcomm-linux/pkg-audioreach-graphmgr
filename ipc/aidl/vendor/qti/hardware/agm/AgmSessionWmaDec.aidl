/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmSessionWmaDec {
    int formatTag; /**< Format Tag */
    int channels; /**< Number of channels */
    int sampleRate; /**< Sample rate */
    int averageBytesPerSecond; /**< Avg bytes per sec */
    int blockAlign; /**< Block align */
    int bitsPerSample; /**< Bits per sample */
    int channelMask; /**< Channel mask */
    int encoderOption; /**< Encoder options */
    int reserved; /**< reserved */
}
