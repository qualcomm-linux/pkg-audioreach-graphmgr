/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause
 */

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.Direction;

@VintfStability
parcelable AifInfo {
    // AIF name
    String aifName;
    // direction Rx or Tx
    Direction direction;
}
