/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2018-10-13-AM10:52:46             1.0             build this file
 ********************************************************************************/
/*!
 * \file     soc_cache.h
 * \brief
 * \author
 * \version  1.0
 * \date  2018-10-13-AM10:52:46
 *******************************************************************************/

#ifndef SOC_CACHE_H_
#define SOC_CACHE_H_

#define     SPICACHE_CTL_CACHE_AUTO_INVALID_FLAG                              16
#define     SPICACHE_CTL_CACHE_SIZE_MODE                                      6
#define     SPICACHE_CTL_LOW_POWER_EN                                         5
#define     SPICACHE_CTL_PROF_EN                                              4
#define     SPICACHE_CTL_ABORT_EN                                             3
#define     SPICACHE_CTL_UNCACHE_EN                                           2
#define     SPICACHE_CTL_PREF_EN                                              1
#define     SPICACHE_CTL_EN                                                   0

#define     SPICACHE_CTL                                                      (SPICACHE_REG_BASE+0x0000)
#define     SPICACHE_INVALIDATE                                               (SPICACHE_REG_BASE+0x0004)
#define     SPICACHE_UNCACHE_ADDR_START                                       (SPICACHE_REG_BASE+0x0008)
#define     SPICACHE_UNCACHE_ADDR_END                                         (SPICACHE_REG_BASE+0x000C)
#define     SPICACHE_TOTAL_MISS_COUNT                                         (SPICACHE_REG_BASE+0x0010)
#define     SPICACHE_TOTAL_HIT_COUNT                                          (SPICACHE_REG_BASE+0x0014)
#define     SPICACHE_PROFILE_INDEX_START                                      (SPICACHE_REG_BASE+0x0018)
#define     SPICACHE_PROFILE_INDEX_END                                        (SPICACHE_REG_BASE+0x001C)
#define     SPICACHE_RANGE_INDEX_MISS_COUNT                                   (SPICACHE_REG_BASE+0x0020)
#define     SPICACHE_RANGE_INDEX_HIT_COUNT                                    (SPICACHE_REG_BASE+0x0024)
#define     SPICACHE_PROFILE_ADDR_START                                       (SPICACHE_REG_BASE+0x0028)
#define     SPICACHE_PROFILE_ADDR_END                                         (SPICACHE_REG_BASE+0x002C)
#define     SPICACHE_RANGE_ADDR_MISS_COUNT                                    (SPICACHE_REG_BASE+0x0030)
#define     SPICACHE_RANGE_ADDR_HIT_COUNT                                     (SPICACHE_REG_BASE+0x0034)


#endif /* SOC_CACHE_H_ */
