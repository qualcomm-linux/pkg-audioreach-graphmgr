/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.AgmDataMode;
import vendor.qti.hardware.agm.AgmSessionCodec;
import vendor.qti.hardware.agm.AgmSessionMode;
import vendor.qti.hardware.agm.Direction;

@VintfStability
parcelable AgmSessionConfig {
    Direction direction; /**< TX or RX */
    AgmSessionMode sessionMode; /**< indicates mode of agm sesison, non-tunnel, or hostless */
    int startThreshold; /**< start_threshold: number of buffers * buffer size */
    int stopThreshold; /**< stop_threshold: number of buffers * buffer size */
    @nullable AgmSessionCodec codec; /**< codec configuration */
    AgmDataMode dataMode; /**< data mode */
    int flags; /**< pass session specific flags e.g enable inband SRCM event*/
}
