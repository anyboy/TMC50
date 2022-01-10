/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file tts manager interface
 */

#ifndef __TTS_MANAGER_H__
#define __TTS_MANAGER_H__

/**
 * @defgroup tts_manager_apis App tts Manager APIs
 * @ingroup system_apis
 * @{
 */
/** tts mode */
enum tts_mode_e
{
	BLOCK_INTERRUPTABLE = 1,
	BLOCK_UNINTERRUPTABLE = 2,
	UNBLOCK_UNINTERRUPTABLE = 4,
	UNBLOCK_INTERRUPTABLE = 8,
};

/** tts file storage medium */
enum tts_storage_medium_e
{
	/** tts file storage in sdfs on nor*/
	TTS_SOTRAGE_SDFS = 1,
	/** tts file storage in fatfs on sdcard*/
	TTS_SOTRAGE_SDCARD = 2,
};

/** tts event */
enum tts_event_e
{
	/** tts event start play */
	TTS_EVENT_START_PLAY = 1,
	/** tts event stop play */
	TTS_EVENT_STOP_PLAY = 2,
};

/** tts ui event map */
typedef struct
{
	/** ui event */
	u16_t ui_event;
	/** tts mode */
	u16_t mode;
	/** name of tts file*/
	const u8_t *tts_file_name;
}tts_ui_event_map_t;

/** tts config */
struct tts_config_t
{
	/** tts media storage type link:tts_storage_medium_e */
	u32_t tts_storage_media;
	/** tts and ui event map*/
	const tts_ui_event_map_t *tts_map;
	/** tts and ui event map size*/
	u32_t tts_map_size;
};

/**
 * @brief lock tts manager
 *
 * This routine calls to lock tts manager, tts will drop when if tts manager locked
 * If the user does not want to play TTS for a period of time, then can used this interface to lock tts mangaer.
 *
 * @return N/A
 */

typedef void (*tts_event_nodify)(u8_t *, u32_t);
/**
 * @brief tts manager init funcion
 *
 * This routine calls init tts manager ,called by main
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int tts_manager_init(void);

/**
 * @brief tts manager deinit funcion
 *
 * This routine calls deinit tts manager ,called by main ,release all tts manager resource.
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int tts_manager_deinit(void);
/**
 * @brief lock tts manager
 *
 * This routine calls to lock tts manager, tts will drop when if tts manager locked
 * If the user does not want to play TTS for a period of time, then can used this interface to lock tts mangaer.
 *
 * @return N/A
 */

void tts_manager_lock(void);

/**
 * @brief unlock tts manager
 *
 * This routine calls to unlock tts manager
 *
 * @return N/A
 */

void tts_manager_unlock(void);

/**
 * @brief get tts manager state
 *
 * This routine calls to unlock tts manager,
 *
 * @return true tts manager is locked
 * @return false tts manager is unlocked
 */

bool tts_manager_is_locked(void);

/**
 * @brief start to play tts
 *
 * This routine calls to play tts 
 *
 * @param tts_name url for tts witch tts user want to play
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int tts_manager_play(u8_t *tts_name, u32_t mode);

/**
 * @brief start to play tts
 *
 * This routine calls to play tts blocked
 *
 * @param tts_name url for tts witch tts user want to play, this blocked caller thread
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int tts_manager_play_block(u8_t *tts_name);

void tts_manager_play_process(void);
/**
 * @brief stop to play tts
 *
 * This routine calls to interrupt to stop tts
 *
 * @param tts_name url for tts witch want to stop
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
 
int tts_manager_stop(u8_t *tts_name);

/**
 * @brief wait tts finised
 *
 * This routine calls to wait tts fininsed
 *
 * @param clean_all true clean all tts ,false not clean all tts
 *
 * @return N/A
 */

void tts_manager_wait_finished(bool clean_all);
/**
 * @brief tts manager process ui event 
 *
 * This routine calls tts manager procees ui event, 
 * find tts according tts config map id and tts file name
 *
 * @param ui_event ui event id
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int tts_manager_process_ui_event(int ui_event);

/**
 * @brief tts manager register tts config 
 *
 * tts config map id and tts file name , and config tts mode 
 * tts manager receive tts id and find tts information according tts config
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int tts_manager_register_tts_config(const struct tts_config_t *config);

/**
 * @cond INTERNAL_HIDDEN
 */
/**
 * @brief tts manager add event lisener
 *
 * This routine calls add event lisener to tts manager lisener list
 * tts manager will notify lisener when tts is start and tts is stop
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int tts_manager_add_event_lisener(tts_event_nodify lisener);

/**
 * @brief tts manager remove event lisener
 *
 * This routine calls remove event lisener from tts manager lisener list
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int tts_manager_remove_event_lisener(tts_event_nodify lisener);

int key_tone_manager_init(void);

int key_tone_play(void);

/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @} end defgroup tts_manager_apis
 */

#endif

