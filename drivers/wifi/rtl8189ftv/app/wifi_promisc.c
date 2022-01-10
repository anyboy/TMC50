#include <zephyr.h>
#include <stdlib.h>
#include <mem_manager.h>
#include "wifi_conf.h"


#if CONFIG_WLAN

struct eth_frame {
	struct eth_frame *prev;
	struct eth_frame *next;
	unsigned char da[6];
	unsigned char sa[6];
	unsigned int len;
	unsigned char type;
	signed char rssi;
};

struct eth_buffer {
	struct eth_frame *head;
	struct eth_frame *tail;
};

extern int _promisc_get_fixed_channel(void *fixed_bssid,u8 *ssid,int *ssid_length);

static struct eth_buffer eth_buffer;
#ifdef CONFIG_PROMISC
#define MAX_PACKET_FILTER_INFO 5
#define FILTER_ID_INIT_VALUE 10
rtw_packet_filter_info_t paff_array[MAX_PACKET_FILTER_INFO];
static u8 packet_filter_enable_num = 0;

void promisc_init_packet_filter()
{
	int i = 0;
	for(i=0; i<MAX_PACKET_FILTER_INFO; i++){
		paff_array[i].filter_id = FILTER_ID_INIT_VALUE;
		paff_array[i].enable = 0;
		paff_array[i].patt.mask_size = 0;
		paff_array[i].rule = RTW_POSITIVE_MATCHING;
		paff_array[i].patt.mask = NULL;
		paff_array[i].patt.pattern = NULL;
	}
	packet_filter_enable_num = 0;
}

int promisc_add_packet_filter(u8 filter_id, rtw_packet_filter_pattern_t *patt, rtw_packet_filter_rule_t rule)
{
	int i = 0;
	while(i < MAX_PACKET_FILTER_INFO){
		if(paff_array[i].filter_id == FILTER_ID_INIT_VALUE){
			break;
		}
		i++;
	}

	if(i == MAX_PACKET_FILTER_INFO)
		return -1;

	paff_array[i].filter_id = filter_id;

	paff_array[i].patt.offset= patt->offset;
	paff_array[i].patt.mask_size = patt->mask_size;
	paff_array[i].patt.mask = pvPortMalloc(patt->mask_size);
	memcpy(paff_array[i].patt.mask, patt->mask, patt->mask_size);
	paff_array[i].patt.pattern= pvPortMalloc(patt->mask_size);
	memcpy(paff_array[i].patt.pattern, patt->pattern, patt->mask_size);

	paff_array[i].rule = rule;
	paff_array[i].enable = 0;

	return 0;
}

int promisc_enable_packet_filter(u8 filter_id)
{
	int i = 0;
	while(i < MAX_PACKET_FILTER_INFO){
		if(paff_array[i].filter_id == filter_id)
			break;
		i++;
	}

	if(i == MAX_PACKET_FILTER_INFO)
		return -1;

	paff_array[i].enable = 1;
	packet_filter_enable_num++;
	return 0;
}

int promisc_disable_packet_filter(u8 filter_id)
{
	int i = 0;
	while(i < MAX_PACKET_FILTER_INFO){
		if(paff_array[i].filter_id == filter_id)
			break;
		i++;
	}

	if(i == MAX_PACKET_FILTER_INFO)
		return -1;

	paff_array[i].enable = 0;
	packet_filter_enable_num--;
	return 0;
}

int promisc_remove_packet_filter(u8 filter_id)
{
	int i = 0;
	while(i < MAX_PACKET_FILTER_INFO){
		if(paff_array[i].filter_id == filter_id)
			break;
		i++;
	}

	if(i == MAX_PACKET_FILTER_INFO)
		return -1;

	paff_array[i].filter_id = FILTER_ID_INIT_VALUE;
	paff_array[i].enable = 0;
	paff_array[i].patt.mask_size = 0;
	paff_array[i].rule = 0;
	if(paff_array[i].patt.mask){
		vPortFree(paff_array[i].patt.mask);
		paff_array[i].patt.mask = NULL;
	}

	if(paff_array[i].patt.pattern){
		vPortFree(paff_array[i].patt.pattern);
		paff_array[i].patt.pattern = NULL;
	}
	return 0;
}
#endif

/*	Make callback simple to prevent latency to wlan rx when promiscuous mode */
static void promisc_callback(unsigned char *buf, unsigned int len, void* userdata)
{
	struct eth_frame *frame = (struct eth_frame *) pvPortMalloc(sizeof(struct eth_frame));

	if(frame) {
		frame->prev = NULL;
		frame->next = NULL;
		memcpy(frame->da, buf, 6);
		memcpy(frame->sa, buf+6, 6);
		frame->len = len;
		frame->rssi = ((ieee80211_frame_info_t *)userdata)->rssi;
		taskENTER_CRITICAL();

#if 0		/* debug */
		unsigned short frame_corl;
		unsigned short adress1 = "6C59408B135E";
		unsigned short adress2 = "CC2D835A981D";
		unsigned short adress3 = "FFFFFFFFFFFF";
		u16 sequence;
		rtw_memcpy(&sequence, buf+22, 2);
		printf("\n\rSDK:promisc_callback");
		frame_corl = (((ieee80211_frame_info_t *)userdata)->i_fc) && 1;
		if((frame_corl == 1) && (!strncmp(adress1,((ieee80211_frame_info_t *)userdata)->i_addr1,6)) &&(!strncmp(adress2,((ieee80211_frame_info_t *)userdata)->i_addr2,6)) &&(!strncmp(adress3,((ieee80211_frame_info_t *)userdata)->i_addr3,6)))
			printf("\n\r====>SDK:promisc_callback:frame->len = %d,sequence = %d\n",frame->len,sequence);
#endif

		if(eth_buffer.tail) {
			eth_buffer.tail->next = frame;
			frame->prev = eth_buffer.tail;
			eth_buffer.tail = frame;
		}
		else {
			eth_buffer.head = frame;
			eth_buffer.tail = frame;
		}

		taskEXIT_CRITICAL();
	}
}

struct eth_frame* retrieve_frame(void)
{
	struct eth_frame *frame = NULL;

	taskENTER_CRITICAL();

	if(eth_buffer.head) {
		frame = eth_buffer.head;

		if(eth_buffer.head->next) {
			eth_buffer.head = eth_buffer.head->next;
			eth_buffer.head->prev = NULL;
		}
		else {
			eth_buffer.head = NULL;
			eth_buffer.tail = NULL;
		}
	}

	taskEXIT_CRITICAL();

	return frame;
}

void wifi_enter_promisc_mode();
static void promisc_test(int duration, unsigned char len_used)
{
	int ch;
	unsigned int start_time;
	struct eth_frame *frame;
	eth_buffer.head = NULL;
	eth_buffer.tail = NULL;

	wifi_enter_promisc_mode();
	wifi_set_promisc(RTW_PROMISC_ENABLE, promisc_callback, len_used);

	for(ch = 1; ch <= 13; ch ++) {
		if(wifi_set_channel(ch) == 0)
			printf("\n\n\rSwitch to channel(%d)", ch);

		start_time = xTaskGetTickCount();

		while(1) {
			unsigned int current_time = xTaskGetTickCount();

			if((current_time - start_time) < (duration * configTICK_RATE_HZ)) {
				frame = retrieve_frame();

				if(frame) {
					int i;
					printf("\n\rDA:");
					for(i = 0; i < 6; i ++)
						printf(" %02x", frame->da[i]);
					printf(", SA:");
					for(i = 0; i < 6; i ++)
						printf(" %02x", frame->sa[i]);
					printf(", len=%d", frame->len);
					printf(", RSSI=%d", frame->rssi);

					vPortFree((void *) frame);
				}
				else
					vTaskDelay(1);	//delay 1 tick
			}
			else
				break;
		}
	}

	wifi_set_promisc(RTW_PROMISC_DISABLE, NULL, 0);

	while((frame = retrieve_frame()) != NULL)
		vPortFree((void *) frame);
}

static void promisc_callback_all(unsigned char *buf, unsigned int len, void* userdata)
{
	struct eth_frame *frame = (struct eth_frame *) pvPortMalloc(sizeof(struct eth_frame));

	if(frame) {
		frame->prev = NULL;
		frame->next = NULL;
		memcpy(frame->da, buf+4, 6);
		memcpy(frame->sa, buf+10, 6);
		frame->len = len;
		frame->type = *buf;
		frame->rssi = ((ieee80211_frame_info_t *)userdata)->rssi;

#if 0	/* debug */
		unsigned short frame_corl;
		unsigned short adress1 = "6C59408B135E";
		unsigned short adress2 = "CC2D835A981D";
		unsigned short adress3 = "FFFFFFFFFFFF";
		u16 sequence;
		rtw_memcpy(&sequence, buf+22, 2);
		printf("\n\rSDK:promisc_callback_all");
		frame_corl = (((ieee80211_frame_info_t *)userdata)->i_fc) && 1;
		if((frame_corl == 1) && (!strncmp(adress1,((ieee80211_frame_info_t *)userdata)->i_addr1,6)) &&(!strncmp(adress2,((ieee80211_frame_info_t *)userdata)->i_addr2,6)) &&(!strncmp(adress3,((ieee80211_frame_info_t *)userdata)->i_addr3,6)))
			printf("\n\r====>SDK:promisc_callback_all:frame->len = %d,sequence = %d\n",frame->len,sequence);
#endif

		taskENTER_CRITICAL();

		if(eth_buffer.tail) {
			eth_buffer.tail->next = frame;
			frame->prev = eth_buffer.tail;
			eth_buffer.tail = frame;
		}
		else {
			eth_buffer.head = frame;
			eth_buffer.tail = frame;
		}

		taskEXIT_CRITICAL();
	}
}
static void promisc_test_all(int duration, unsigned char len_used)
{
	int ch;
	unsigned int start_time;
	struct eth_frame *frame;
	eth_buffer.head = NULL;
	eth_buffer.tail = NULL;

	wifi_enter_promisc_mode();
	wifi_set_promisc(RTW_PROMISC_ENABLE_2, promisc_callback_all, len_used);

	for(ch = 1; ch <= 13; ch ++) {
		if(wifi_set_channel(ch) == 0)
			printf("\n\n\rSwitch to channel(%d)", ch);

		start_time = xTaskGetTickCount();

		while(1) {
			unsigned int current_time = xTaskGetTickCount();

			if((current_time - start_time) < (duration * configTICK_RATE_HZ)) {
				frame = retrieve_frame();

				if(frame) {
					int i;
					printf("\n\rTYPE: 0x%x, ", frame->type);
					printf("DA:");
					for(i = 0; i < 6; i ++)
						printf(" %02x", frame->da[i]);
					printf(", SA:");
					for(i = 0; i < 6; i ++)
						printf(" %02x", frame->sa[i]);
					printf(", len=%d", frame->len);
					printf(", RSSI=%d", frame->rssi);

					vPortFree((void *) frame);
				}
				else
					vTaskDelay(1);	//delay 1 tick
			}
			else
				break;
		}
	}

	wifi_set_promisc(RTW_PROMISC_DISABLE, NULL, 0);

	while((frame = retrieve_frame()) != NULL)
		vPortFree((void *) frame);
}

void cmd_promisc(int argc, char **argv)
{
	int duration;
	#ifdef CONFIG_PROMISC
	wifi_init_packet_filter();
	#endif
	if((argc == 2) && ((duration = atoi(argv[1])) > 0))
		//promisc_test(duration, 0);
		promisc_test_all(duration, 0);
	else if((argc == 3) && ((duration = atoi(argv[1])) > 0) && (strcmp(argv[2], "with_len") == 0))
		promisc_test(duration, 1);
	else
		printf("\n\rUsage: %s DURATION_SECONDS [with_len]", argv[0]);
}

#endif	//#if CONFIG_WLAN

int promisc_get_fixed_channel(void *fixed_bssid,u8 *ssid,int *ssid_length)
{
//#ifdef CONFIG_PROMISC
	return _promisc_get_fixed_channel(fixed_bssid,ssid,ssid_length);
//#else
//	return 0;
//#endif
}
