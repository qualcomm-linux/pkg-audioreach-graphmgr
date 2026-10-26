/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

/**
 * Gapless playback Silence type
 */
@VintfStability
@Backing(type="int")
enum AgmGaplessSilenceType {
    // Initial silence sample to be removed
    INITIAL_SILENCE,
    // Trailing silence sample to be removed
    TRAILING_SILENCE,
}
