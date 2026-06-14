/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.agm;
/* AgmCshmCachetype type of memory to be allocated*/
@Backing(type="int") @VintfStability
enum AgmCshmCacheType {
    /**
    * Indicates that the allocated shared memory is cacheable.
    * This typically offers better performance due to faster access,
    * but may require explicit cache management
    * to ensure data consistency between CPU and hardware devices.
    */
    CACHED = 1,
    /**
    * Indicates that the allocated shared memory is non-cacheable.
    * This avoids issues with cache coherency,
    * though it may result in slower access speeds.
    */
    UNCACHED = 2,
}
