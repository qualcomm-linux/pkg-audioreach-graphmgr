/**
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause
*/

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.AgmKeyValue;

@VintfStability
parcelable AgmCalConfig {
    /**< tag key vector*/
    AgmKeyValue[] kv;
}
