/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file lib mqtt interface 
 */

#ifndef __MQTT_API_H__
#define __MQTT_API_H__

/**
 * @defgroup mqtt_procotol_apis Mqtt Procotol APIs
 * @ingroup mem_managers
 * @{
 */

/**
 * player mode
 * This enum is used in send_loop_mode as a argument
 */
typedef enum {
	/**  repeat one */
	REPEAT_ONE,
	/**  repeat all */
	REPEAT_ALL
}loop_mode_t;

/**
 * play_status_t - player status
 * This enum is used in send_play_status as a argument
 */
typedef enum {
   /** PLAY: playing status */
	PLAY,
	/** PAUSE: pause status */
	PAUSE
}play_status_t;


/** This struct describes a music or story to play or download */
typedef struct{
    /**  track id of the music or story */
	int track_id;
	/**  url of the music or story*/
	char *play_url;
	/**  download url of the music or story */
	char *download_url;
}music_track_info_t;

/*track_list_info_t - the struct describes a track list info
 *  @size: track list size
 *  @trackIds: always null
 *  @name: track list name
 *  @id: track list id
 */
typedef struct{
	/** track list size  */
	int size;
	/** trackIds always null, so not used */
	//char *trackIds;
	/** track name */
	char *name;
	/** track id */
	int id;
}track_list_info_t;

/**
 * This struct describes a music or story on demand
 * the struct describes a track(music or story) on demand
 */
typedef struct {
	/** url of the notify voice */
	char *voice_url;
	/** url of the music or story */
	char *url;
	/** track id of the music or story */
	int track_id;
}demand_music_info_t;


/** This struct describes a track list. It is used for play list operations. */
typedef struct {
    /**  list name */
	char *name;
    /**  list id */
	int id;
	/**  list size */
	int size;
	/**  track id array in this list */
	int *trackIDs;
}track_list_t;

/**
 * command_type_t - received server message , command type
 * This enum describes simple command types.
 */
typedef enum {
	/**  command play */
	Play,
	/**  command Pause */
	Pause,
	/**  command Resume */
	Resume,
	/**  command Next */
	Next,
	/**  command Prev */
	Prev,
	/**  command SetVolume */
	SetVolume,
	/**  command GetVolume */
	GetVolume,
	/**  command SetLoopMode */
	SetLoopMode,
	/**  command GetLoopMode */
	GetLoopMode,
	/**  command GetCurrentTrack */
	GetCurrentTrack,
	/**  command GetSysInfo */
	GetSysInfo,
    /**  command SetPowerOff */
	SetPowerOff,
	/**  command GetPlayStatus */
	GetPlayStatus,
	/**  command GetOnlineStatus */
	GetOnlineStatus,
	/**  command SetPlayTrackListId */
	SetPlayTrackListId,
	/**  command GetPlayTrackListId */
	GetPlayTrackListId,
	/**  command GetTracksById */
	GetTracksById,
}command_type_t;

#ifdef CONFIG_EXTEND_MQTT_PROTOCOL
typedef enum
{
#ifdef CONFIG_ROBOO_EXTEND_MQTT_PROTOCOL
	SetCollectTrackListId,
	PlayText,
	Off_Upgrade,
	DemandList,
	SetEarLight,
	GetEarLight,
	SetChildLock,
	GetChildLock,
	GetPlayProgress,
	SetPlayProgress,
#endif
} extend_command_type_t;
#endif
#ifdef CONFIG_ROBOO_EXTEND_MQTT_PROTOCOL


typedef struct {
    /**  list id */
	int list_id;
	/** trackids */
	int* trackids;
	/** trackids */
	int trackid_size;

	char* list_name;

	int demandId;

	int trackId;

	/**  download url */
	char *play_url;
	/** */
	char *download_url;
}demand_list_arg_t;

typedef struct {

	int type;

	int bright_value;

}earlight_arg_t;

typedef enum
{
	STATUS_ON,
	STATUS_ON_UPGRADE_FAILED,
	STATUS_OFF,
	STATUS_ABNORMAL,
	STATUS_OFF_FOR_UPGRADE,
}device_status_e;

void set_device_status(device_status_e status);
int send_earlight(earlight_arg_t arg);
int send_clildblock(int value);
int send_collect_list_track(int listid,int* trackids,int size);
#endif


/** This struct describes system information.*/
typedef struct SysInfo {
	/** wifi connecctd ap ssid */
	char ssid[64];
	/** fw version */
	char fw_version[64];
	/** charging state */
	bool is_charging;
	/** battery vol */
	int battery_vol;
	/** total storage size */
	int storage_total;
    /** free size of  storage */
	int storage_free;
}Sys_info_t;

/** mqtt message info */
typedef struct MsgInfo {
	   /** len of message */
       uint16_t msg_len;
	   /** pointer of message */
       char msg[0];
}Msg_info_t;

/** 
 * argument for SetPlayTrackListId command
 * SetPlayTrackListId can set one or more lists at one time
 */
typedef struct {
	 /**  id array */
	int *ids;
	/**  id array size */
	int size;
}play_list_arg_t;

/** argument for Play command */
typedef struct {
    /**  list id */
	int list_id;
	/** play url */
	int track_id;
	/**  download url */
	char *play_url;
	/** */
	char *download_url;
}play_track_arg_t;

/** This struct is used to transform fw information */
typedef struct {
	/** firmware version */
	char *version;
    /** firmware download url */
	char *url;
}fw_info_t;

/** This enum is used in send_current_track as an argument */
typedef enum {
    /** music or story normal type */
	MUSIC_TYPE,
	/** on demand type */
	DEMAND_TYPE,
	/** voice type */
	VOICE_TYPE
}track_type_t;

/** This enum is used in send_download_state as an argument */
typedef enum {
	/** download success */
	DOWNLOAD_STATE_SUCCESS,
	/** download failed */
	DOWNLOAD_STATE_ERROR,
	/** system storage is full*/
	DOWNLOAD_STATE_FULL
}download_state_t;

/**
 * @brief json parse, find key string in the data string
 *
 * this routine provides find one string in another
 *
 * @param data source string
 * @param length date string length
 * @param key the string want to find
 *
 * @return char *, find string pointer or NULL
 */
char *json_get_string(char* data, int length, char *key);

/**
 * @brief json parse, parse key int value in the data string
 *
 * this routine provides parse key int value in the data string
 *
 * @param data source string
 * @param length date string length
 * @param key the string want to find
 *
 * @return int key int value
 * @return -1 error
 */
int json_get_int(char* data, int length, char *key);

/**
 * @brief json parse, parse int array value
 *
 * this routine provides parse int array value
 *
 * @param data source string
 * @param length date string length
 * @param key the string want to find
 * @param out find int array
 *
 * @return 0 success
 * @return -1 error
 */
int json_get_int_array(char* data, int length,char *key,int * out);

/**
 * @brief json parse, parse int array size
 *
 * this routine provides parse int array size
 *
 * @param data source string
 * @param length date string length
 * @param key the string want to find
 *
 * @return int array size
 * @return -1 error
 */
int json_get_int_array_size(char* data, int length, char *key);

/**
 * @brief json parse, parse object array size
 *
 * this routine provides parse object array size
 *
 * @param data source string
 * @param length date string length
 * @param key the string want to find
 *
 * @return int array size
 * @return -1 error
 */
int json_get_obj_array_size(char *data, int length, char *key);

/**
 * @brief Send current play track info to the server
 *
 * this routine provides Send current play track info to the server
 *
 * @param list_id current track list id
 * @param track_id current track id
 * @param type current track typec
 *
 * @return N/A
 */
int send_current_track(int list_id, int track_id, track_type_t type);

/**
 * @brief Send demand music broadcast to the server
 *
 * this routine provides  Send demand music broadcast to the server
 *
 * @param track_id demand music track id
 *
 * @return 0 success ,others failed
 */
int send_demand_music_broadcast(int track_id);

/**
 * @brief Send download success info to the server
 *
 * this routine provides  Send download success info to the server
 *
 * @param track_id download track id
 * @param url url of track id
 *
 * @return 0 success ,others failed
 */
int send_download_success(int track_id, char *url);
/**
 * @brief Send download error info to the server
 *
 * this routine provides  Send download error info to the server
 *
 * @param track_id download track id
 * @param url url of track id
 *
 * @return 0 success ,others failed
 */
int send_download_error(int track_id, char *url);
/**
 * @brief Send download storage full info to the server
 *
 * this routine provides  Send download storage full info to the server
 *
 * @param track_id download track id
 * @param url url of track id
 *
 * @return 0 success ,others failed
 */
int send_download_full(int track_id, char *url);
/**
 * @brief Send download status to the server
 *
 * this routine provides Send download status to the server
 *
 * @param type download status
 * @param ids download list id
 * @param size size of download list id
 *
 * @return 0 success ,others failed
 */
int send_download_state(download_state_t type, int *ids, int size);
/**
 * @brief Send system volume to the server
 *
 * this routine provides Send system volume to the server
 *
 * @param vol current system volume
 *
 * @return 0 success ,others failed
 */
int send_volume(int vol);
/**
 * @brief Send play loop mode status to the server
 *
 * this routine provides Send play loop mode status to the server
 *
 * @param mode play loop mode
 *
 * @return 0 success ,others failed
 */
int send_loop_mode(loop_mode_t mode);

/**
 * @brief Send play status to the server
 *
 * this routine provides Send play status to the server
 *
 * @param status current play status
 *
 * @return 0 success ,others failed
 */
int send_play_status(play_status_t status);

/**
 * @brief Send system info to the server
 *
 * this routine provides Send system info to the server
 *
 * @param info  pointer of system info
 *
 * @return 0 success ,others failed
 */

int send_sys_info(Sys_info_t *info);

/**
 * @brief Send ip addr to the server
 *
 * this routine provides Send ip addr to the server
 *
 *
 * @return 0 success ,others failed
 */

int send_ip_info(void);

/**
 * @brief Send predefine list info to the server
 *
 * this routine provides Send predefine list info to the server
 *
 * @param list  array of list ids
 * @param from  start index of list ids
 * @param to  end index of list ids
 * @param listsize total size of list
 *
 * @return 0 success ,others failed
 */

int send_predefine_list(track_list_t *list,int from ,int to,int listsize);
/**
 * @brief Send the preset list info to the server
 *
 * this routine provides Send the preset list info to the server
 *
 * @param list  array of list ids
 * @param from  start index of list ids
 * @param to  end index of list ids
 * @param listsize total size of list
 *
 * @return 0 success ,others failed
 */

int send_preset_list(track_list_t *list,int from ,int to,int listsize);

/**
 * @brief Send offline information to the server
 *
 * this routine provides Send offline information to the server
 *
 * @return 0 success ,others failed
 */
int send_offline_status(void);
/**
 * @brief Send online information to the server
 *
 * this routine provides Send online information to the server
 *
 * @return 0 success ,others failed
 */
int send_online_status(void);

/**
 * @brief Send a synchronization list request to the server
 *
 * this routine provides Send a synchronization list request to the server
 *
 * @return 0 success ,others failed
 */
int send_sync_list_request(void);

/**
 * @brief Send playlist information to the server
 *
 * this routine provides Send playlist information to the server
 * the reply used response receives the GetPlayTrackListId command
 *
 * @param listIDs  array of list ids
 * @param size  sizeof array of listids
 *
 * @return 0 success ,others failed
 */

int send_play_list_id(int *listIDs, int size);
/**
 * @brief sends a message to the server to tell server wechat media info
 *
 * this routine provides sends a message to the server to tell server wechat media info
 *
 * @param media_id  media id generated  by server
 *
 * @return 0 success ,others failed
 */
int send_voice_media_id(char *media_id);

/**
 * @brief sends a message to the server to get list info associated with the id
 *
 * this routine provides sends a message to the server to get list info
 * associated with the id
 *
 * @param list_id  track list id
 *
 * @return 0 success ,others failed
 */
int get_list_by_id(int list_id);
/**
 * @brief Send a message telling the server that an id's tracklist has changed
 *
 * this routine provides  Send a message telling the server
 * that tracklist associated with the id has changed associated with the id
 *
 * @param listid  track list id
 *
 * @return 0 success ,others failed
 */
int send_change_listid(int listid);

/**
 * @brief sends a message to the server to get the track info associated with the id
 *
 * this routine provides  sends a message to the server to 
 * get the track info associated with the id
 *
 * @param track_id track id
 *
 * @return 0 success ,others failed
 */

int get_track_info_by_id(int track_id);
/**
 * @brief sends a message to the server to get the track info associated with the ids
 *
 * this routine provides sends a message to the server to
 * get the track info associated with the ids , Get information about multiple ids at once
 *
 * @param ids pointer of track list ids
 * @param size size of array ids
 *
 * @return 0 success ,others failed
 */

int get_track_info_array_by_id(int *ids, int size);

/**
 * @brief send ota upgrade start message
 *
 * this routine provides send ota upgrade start message
 *
 * @return 0 success ,others failed
 */

int send_upgrade_start(void);

/**
 * @brief send ota upgrade stop message
 *
 * this routine provides send ota upgrade stop message
 *
 * @return 0 success ,others failed
 */

int send_upgrade_stop(void);

/**
 * @brief send customer-extending message
 *
 * this routine provides send customer-extending message
 *
 * @param data pointer of user message 
 * @param length len of user message
 *
 * @return 0 success ,others failed
 */

int send_customer_message(char *data, int length);

/**
 * @brief send storyapp tracks list info to server
 *
 * this routine provides send storyapp tracks list info to server
 * when received server command getTracks
 *
 * @param trackListId track list id
 * @param trackIds array of trackids of target track list
 * @param size size of array trackids
 *
 * @return 0 success ,others failed
 */

int send_tracks_list_content(int trackListId, int *trackIds, int size);

/**
 * @brief send get track lists command
 *
 * this routine provides send get tracks list command to server
 *
 * @param N/A
 *
 * @return 0 success ,others failed
 */
int send_get_track_lists(void);

/**
 * @brief send customer play seek
 *
 * this routine provides customer play seek command to server
 *
 * @param value seek position
 *
 * @return 0 success ,others failed
 */
int send_customer_seek(int value);


struct mqtt_callback_api
{
	int verison;
	/** system function , user can't removed*/
	void (* on_mqtt_connect)(void);
	void (* on_mqtt_disconnect)(void);
	int (* unpack_message)(char* data, int length);

	/** user function */
	void (* on_receive_command)(command_type_t cmd, void *arg);

	void (* on_receive_emoji)(char *url);
	void (* on_receive_voice_message)(char *url);
	void (* on_receive_demand_music)(demand_music_info_t *musicInfo);
	void (* on_receive_demand_music_online)(demand_music_info_t *musicInfo);

	void (* on_receive_track_info)(music_track_info_t *info);
	void (* on_receive_track_info_array)(music_track_info_t *info, int size);
	void (* on_receive_list)(track_list_t *list, int from, int to, int listSize);
	void (* on_receive_predefine_list_id)(char *name, int list_id);
	void (* on_receive_update_list_id)(int *listIDs, int size);
	void (* on_receive_add_list_id)(int *listID, int size, char *name);
	void (* on_receive_deleted_list_id)(int *listIDs, int size);
	void (* on_receive_track_lists_array)(track_list_info_t *track, int size);

	void (* on_delete_track_by_id)(int list_id, int *trackIDs, int size, int from, int to, int listSize);
	void (* on_add_track_by_id)(int list_id, int *trackIDs, int size, int from, int to, int listSize);

	void (* on_receive_upgrade_firmware)(fw_info_t *fw);
	void (* on_receive_customer_message)(char *data, int length);

	void (* on_receive_test_mode)(char *url);

	void (* on_receive_get_initial_track_list)(int tracklistId);
	void (* on_receive_play_tracks)(int *trackIds,int size, int from, int to, int listSize);
	void (* on_receive_track_info_for_collector)(int sourceId, int duration, int customerId);

#ifdef CONFIG_EXTEND_MQTT_PROTOCOL
	void (* on_receive_command_extend)(extend_command_type_t cmd, void *arg);
#endif

};

/**
 * @brief registered mqtt callback
 *
 * this routine provides app can registered users mqtt callback to system
 *
 * @param callback stucture of  mqtt_callback_api , user callback interace
 *
 * @return true registered success
 * @return false  registered failed
 */
bool mqtt_callback_registered(const struct mqtt_callback_api * callback);

/**
 * @brief get mqtt callback
 *
 * this routine provides get the mqtt callback interface
 *
 * @return NULL if no callback registered otherwise return callback handle
 */
struct mqtt_callback_api * get_mqtt_cb(void);

/**
 * @} end defgroup mqtt_procotol_apis
 */

#endif
