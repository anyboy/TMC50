/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <os_common_api.h>
#include <mem_manager.h>
#include <stdio.h>

#include <misc/printk.h>
#include "http_client.h"
#include "http_helper_cb.h"
#include "dns_client.h"
#if (CONFIG_NET_NBUF_INNER_COUNT == 4)
#define HTTP_KEEP_PKT_MAX		3
#else
#define HTTP_KEEP_PKT_MAX		4
#endif
#define HTTP_HRARD_FINISHED_TAG "\r\n\r\n"

static bool stop_all_connectting_http = false;

static bool support_chunked = true;

static char net_cache_pool[2048];

char *get_net_cache_pool(void)
{
	return net_cache_pool;
}

int get_net_cache_pool_size(void)
{
	return sizeof(net_cache_pool);
}


bool http_support_chunked(void)
{
	return support_chunked;
}

int stop_all_http_connectting(void)
{
	stop_all_connectting_http = true;
	return 0;
}

struct http_client_ctx *http_init(void)
{
	struct http_client_ctx *http_ctx = NULL;
	struct tcp_client_ctx *ctx = NULL;

	http_ctx = mem_malloc(sizeof(struct http_client_ctx));
	if (!http_ctx) {
		return NULL;
	}

	ctx = mem_malloc(sizeof(struct tcp_client_ctx));
	if (!ctx) {
		mem_free(http_ctx);
		return NULL;
	}

	memset(http_ctx, 0, sizeof(struct http_client_ctx));
	memset(ctx, 0, sizeof(struct tcp_client_ctx));

	http_ctx->settings.on_body = on_body;
	http_ctx->settings.on_chunk_complete = on_chunk_complete;
	http_ctx->settings.on_chunk_header = on_chunk_header;
	http_ctx->settings.on_headers_complete = on_headers_complete;
	http_ctx->settings.on_header_field = on_header_field;
	http_ctx->settings.on_header_value = on_header_value;
	http_ctx->settings.on_message_begin = on_message_begin;
	http_ctx->settings.on_message_complete = on_message_complete;
	http_ctx->settings.on_status = on_status;
	http_ctx->settings.on_url = on_url;

	os_sem_init(&http_ctx->net_data_sem, 0, 1);
	os_fifo_init(&http_ctx->netbuffifo);

	http_ctx->tcp_ctx = ctx;
	http_ctx->finished = 0;

	return http_ctx;
}

int http_release_resource(struct http_client_ctx *http_ctx)
{
	int prio;
	int cnt = 0;
	struct net_pkt *nbuf = NULL;

	prio = os_thread_priority_get(os_current_get());
	os_thread_priority_set(os_current_get(), -1);

	tcp_disconnect(http_ctx->tcp_ctx);

	os_thread_priority_set(os_current_get(), prio);

	os_sem_give(&http_ctx->net_data_sem);

	if(http_ctx->cur_nbuf != NULL)
	{
		net_pkt_unref(http_ctx->cur_nbuf);
		http_ctx->cur_nbuf = NULL;
		http_ctx->cur_off = 0;
	}

	while((nbuf = os_fifo_get(&http_ctx->netbuffifo, K_NO_WAIT))
			!= NULL)
	{
		net_pkt_unref(nbuf);
	}

	while(http_ctx->isworking)
	{
		cnt++;
		os_sem_give(&http_ctx->net_data_sem);
		os_sleep(10);
		if(cnt > 1000)
		{
			break;
		}
	}


	return 0;
}

void http_deinit(struct http_client_ctx *http_ctx)
{
	if(http_ctx)
	{
		os_sem_give(&http_ctx->net_data_sem);
		if(http_ctx->tcp_ctx)
		{
			mem_free(http_ctx->tcp_ctx);
		}
		mem_free(http_ctx);
	}
}

int http_reset_ctx(struct http_client_ctx *http_ctx)
{
	http_parser_init(&http_ctx->parser, HTTP_RESPONSE);

	memset(http_ctx->http_status, 0, sizeof(http_ctx->http_status));

	http_ctx->cl_present = 0;
	http_ctx->location_present = 0;
	http_ctx->content_length = 0;
	http_ctx->processed = 0;
	http_ctx->body_found = 0;
	http_ctx->responsed = 0;
	http_ctx->received_len = 0;

	return 0;
}
const int tcp_retry_timeout[HTTP_TCP_CONNECT_RETRY] = {1000, 1000, 1000, 2000, 4000};

static void defualt_http_receive_cb(struct tcp_client_ctx *tcp_ctx,
									struct net_pkt *rx)
{
	struct http_client_ctx * http_ctx = (struct http_client_ctx *)tcp_ctx->pIOhandle;
	uint16_t nbuffer_len = 0;
	uint16_t nbuffer_off = 0;
	char * cache_buffer = get_net_cache_pool();

	if (!rx) {
		return;
	}

	if(http_ctx->responsed == 0)
	{
		nbuffer_off = http_resp_parse_headers(tcp_ctx, rx);
		nbuffer_len = net_buf_frags_len(rx->frags) - nbuffer_off;
		if(http_ctx->parser.status_code == 411)
		{
			printk("not support chunked mode , switch to no chunked mode\n");
			support_chunked = false;
		}
		http_ctx->cur_nbuf = rx;
		http_ctx->cur_off = nbuffer_off;
		net_pktbuf_set_owner(rx);
		http_ctx->responsed = 1;
		os_sem_give(&http_ctx->net_data_sem);
		if(http_ctx->receive_cb == NULL)
		{
			return;
		}
	}
	else
	{
		nbuffer_len = net_pkt_appdatalen(rx);
		nbuffer_off = net_buf_frags_len(rx->frags) - nbuffer_len;
	}

	if(http_ctx->receive_cb != NULL)
	{
		if(((http_ctx->parser.status_code == 200) ||
			(http_ctx->parser.status_code == 206))
			&& cache_buffer != NULL)
		{
			if(http_ctx->received_len + nbuffer_len >= get_net_cache_pool_size())
			{
				nbuffer_len = get_net_cache_pool_size() - http_ctx->received_len;
			}

			if(nbuffer_len > 0)
			{
				memcpy(&cache_buffer[http_ctx->received_len], &rx->frags->data[nbuffer_off], nbuffer_len);
				http_ctx->received_len += nbuffer_len;
			}

			if(http_ctx->content_length != 0)
			{
				if(http_ctx->content_length <= http_ctx->received_len
					||  get_net_cache_pool_size() == http_ctx->received_len)
				{
					http_ctx->receive_cb(cache_buffer, http_ctx->received_len);
				}
			}
			else
			{
				if(memcmp(&rx->frags->data[nbuffer_off + nbuffer_len - 5],"0\r\n\r\n",5) == 0
					||  get_net_cache_pool_size() == http_ctx->received_len)
				{
					http_ctx->receive_cb(cache_buffer, http_ctx->received_len);
				}
			}
		}
		else
		{
			http_ctx->receive_cb(&rx->frags->data[nbuffer_off], nbuffer_len);
		}
		net_pkt_unref(rx);
		http_ctx->cur_nbuf = NULL;
#ifdef	CONFIG_NET_TCP_CTRL_ACK
		if (os_fifo_cnt_sum(&http_ctx->netbuffifo) < HTTP_KEEP_PKT_MAX) {
			net_context_ctrl_send_ack(tcp_ctx->app_ctx.default_ctx->ctx);
		}
#endif
		return;
	}
	else if(net_pkt_appdatalen(rx) != 0)
	{
		net_pktbuf_set_owner(rx);
		os_fifo_put(&http_ctx->netbuffifo, rx);
		os_sem_give(&http_ctx->net_data_sem);
	}
	else
	{
	    SYS_LOG_ERR("http_recv data %d bytes buf %p",
						net_pkt_appdatalen(rx), rx);
	    net_pkt_unref(rx);
#ifdef	CONFIG_NET_TCP_CTRL_ACK
		if (os_fifo_cnt_sum(&http_ctx->netbuffifo) < HTTP_KEEP_PKT_MAX) {
			net_context_ctrl_send_ack(tcp_ctx->app_ctx.default_ctx->ctx);
		}
#endif
	}
}

int http_send_request(struct http_client_ctx *http_ctx, const char *method, struct request_info * request)
{
	#define SIZE_OF_TEMP_BUF 64
	struct net_pkt *tx;
	char * temp_buffer;
	uint16_t off = 0;
	uint16_t port;
	struct http_parser_url parser;
	int rc, i;
	u8_t set_ctrl_flag = 0;

	temp_buffer = mem_malloc(SIZE_OF_TEMP_BUF);
	if(temp_buffer == NULL) {
		return -1;
	}

	rc = http_parse_url(&parser,request->url,temp_buffer, &port);
	if (rc != 0) {
		SYS_LOG_ERR("http_parse_url error:%d\n", rc);
		goto free_exit;
	}

	os_sem_reset(&http_ctx->net_data_sem);
	http_reset_ctx(http_ctx);
	if (http_ctx->tcp_ctx->receive_cb == NULL) {
		http_ctx->tcp_ctx->receive_cb = defualt_http_receive_cb;
		set_ctrl_flag = 1;
	}

	for (i = 0; i < HTTP_TCP_CONNECT_RETRY; i++) {
		http_ctx->tcp_ctx->timeout = tcp_retry_timeout[i];
		rc = tcp_connect(&http_ctx->tcp_ctx, temp_buffer, port);
		if (rc == 0) {
			break;
		}
		os_sleep(10);

		if (stop_all_connectting_http) {
			/* Try one time more */
			http_ctx->tcp_ctx->timeout = HTTP_NETWORK_TIMEOUT;
			rc = tcp_connect(&http_ctx->tcp_ctx, temp_buffer,port);
			break;
		}
	}

	SYS_LOG_DBG("serverip:  %s port %d ,request->url:%s\n",temp_buffer,port,request->url);
	if (rc) {
		SYS_LOG_ERR("tcp_connect error\n");

		if (i >= HTTP_TCP_CONNECT_RETRY) {
			SYS_LOG_ERR("del dns cached:%s", temp_buffer);
			del_dns_cached(temp_buffer);
		}

		if(stop_all_connectting_http) {
			stop_all_connectting_http = false;
		}
		goto free_exit;
	}

#ifdef	CONFIG_NET_TCP_CTRL_ACK
	if (set_ctrl_flag) {
		net_context_set_ctrl_ack_flag(http_ctx->tcp_ctx->app_ctx.default_ctx->ctx, true);
	}
#endif

	stop_all_connectting_http = false;
	tx = net_pkt_get_tx(http_ctx->tcp_ctx->app_ctx.default_ctx->ctx, K_FOREVER);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto free_exit;
	}

	if(request->http_head != NULL)
	{
		if (!net_pkt_append(tx, strlen(request->http_head), (uint8_t *)request->http_head,
			     K_FOREVER))
		{
			rc = -ENOMEM;
			goto lb_exit;
		}
	}
	else
	{
		/*append method */
		if (!net_pkt_append(tx, strlen(method), (uint8_t *)method,
				     K_FOREVER)) {
			rc = -ENOMEM;
			goto lb_exit;
		}

		/*append url field data */
		if (parser.field_set & (0x01 << UF_PATH)) {
			if (!net_pkt_append(tx, parser.field_data[UF_PATH].len,
					 (uint8_t *)&request->url[parser.field_data[UF_PATH].off], K_FOREVER)) {
				rc = -ENOMEM;
				goto lb_exit;
			}

			if (parser.field_set & (0x01 << UF_QUERY)) {
				if (!net_pkt_append(tx, 1, "?", K_FOREVER)) {
					rc = -ENOMEM;
					goto lb_exit;
				}

				if (!net_pkt_append(tx, parser.field_data[UF_QUERY].len,
						 (uint8_t *)&request->url[parser.field_data[UF_QUERY].off], K_FOREVER)) {
					rc = -ENOMEM;
					goto lb_exit;
				}
			}
		} else {
			if (!net_pkt_append(tx, 1, "/", K_FOREVER)) {
				rc = -ENOMEM;
				goto lb_exit;
			}
		}

		/* append http protocol */
		if (!net_pkt_append(tx, strlen(PROTOCOL), (uint8_t *)PROTOCOL, K_FOREVER)) {
			rc = -ENOMEM;
			goto lb_exit;
		}

		/*append ipaddr and port*/
		memset(temp_buffer,0,SIZE_OF_TEMP_BUF);
		memcpy(temp_buffer,HOST,strlen(HOST));
		off += strlen(HOST);

		memcpy(&temp_buffer[off], &request->url[parser.field_data[UF_HOST].off],
				parser.field_data[UF_HOST].len);
		off += parser.field_data[UF_HOST].len;

		if (parser.field_set & (0x01 << UF_PORT)) {
			temp_buffer[off++] = ':';
			memcpy(&temp_buffer[off], &request->url[parser.field_data[UF_PORT].off],
				parser.field_data[UF_PORT].len);
			off += parser.field_data[UF_PORT].len;
		}

		if (!net_pkt_append(tx, off,(uint8_t *)temp_buffer, K_FOREVER)) {
			rc = -ENOMEM;
			goto lb_exit;
		}

		/*append HEADER_FIELDS User-Agent: AND Connection*/
		if (!net_pkt_append(tx, strlen(HEADER_FIELDS),
				     (uint8_t *)HEADER_FIELDS, K_FOREVER)) {
			rc = -ENOMEM;
			goto lb_exit;
		}

		/* append HTTP_RANGE if need */
		if (request->seek_offset != 0) {
			memset(temp_buffer, 0, SIZE_OF_TEMP_BUF);
			sprintf(temp_buffer, HTTP_RANGE, request->seek_offset);
			if (!net_pkt_append(tx, strlen(temp_buffer),
					     temp_buffer, K_FOREVER)) {
				rc = -ENOMEM;
				goto lb_exit;
			}
		}

		/* append http end NULL line */
		if (!net_pkt_append(tx, strlen(HTTP_END_LINE),
				     (uint8_t *)HTTP_END_LINE, K_FOREVER)) {
			rc = -ENOMEM;
			goto lb_exit;
		}
	}

	rc = net_app_send_pkt(&http_ctx->tcp_ctx->app_ctx, tx, NULL, 0,
						http_ctx->tcp_ctx->timeout, NULL);
	if (rc < 0) {
		rc = -EIO;
	} else {
		/* send success, app can't unref buf */
		tx = NULL;
	}

lb_exit:
	if (tx) {
		net_pkt_unref(tx);
	}
free_exit:
	mem_free(temp_buffer);
	return rc;
}

int http_resp_parse_headers(struct tcp_client_ctx *tcp_ctx, struct net_pkt *nbuf)
{
	struct http_client_ctx *http_ctx;
	struct net_buf *buf;
	int parser_len = 0;
	uint16_t offset;
	uint8_t * data_ptr = NULL;
	uint16_t data_len = 0;
	uint16_t i = 0;
	uint16_t temp_off = 0;
	uint16_t head_end_off = 0;
	char temp_buffer[4];

	if (!nbuf) {
		return parser_len;
	}

	http_ctx = (struct http_client_ctx *)tcp_ctx->pIOhandle;

	buf = nbuf->frags;
	offset = net_buf_frags_len(buf) - net_pkt_appdatalen(nbuf);
	head_end_off += offset;
	/* find the fragment */
	while (buf && offset >= buf->len) {
		offset -= buf->len;
		buf = buf->frags;
	}

	while (buf) {

		data_ptr = buf->data + offset;
		data_len =  buf->len - offset;

		parser_len += http_parser_execute(&http_ctx->parser,
					  &http_ctx->settings,
					  data_ptr,
					  data_len);

		for(i = 0; i < data_len; i++)
		{
			if(data_ptr[i] == HTTP_HRARD_FINISHED_TAG[0])
			{
				if(data_len - i >= sizeof(HTTP_HRARD_FINISHED_TAG) - 1)
				{
					temp_off = sizeof(HTTP_HRARD_FINISHED_TAG) - 1;
				}
				else
				{
					temp_off = data_len - i;
				}
				memcpy(temp_buffer,&data_ptr[i],temp_off);
				i += sizeof(HTTP_HRARD_FINISHED_TAG) - 1;
			}
			else if(temp_off != 0 && i == 0)
			{
				memcpy(&temp_buffer[temp_off],&data_ptr[i],
						sizeof(HTTP_HRARD_FINISHED_TAG) - 1 - temp_off);

				i += sizeof(HTTP_HRARD_FINISHED_TAG) - 1 - temp_off;

				temp_off = sizeof(HTTP_HRARD_FINISHED_TAG) - 1;
			}

			if(memcmp(&temp_buffer, HTTP_HRARD_FINISHED_TAG,
						sizeof(HTTP_HRARD_FINISHED_TAG) - 1) == 0)
			{
				head_end_off += i;
				goto lb_exit;
			}
		}
		head_end_off += data_len;
		/* after the first iteration, we set offset to 0 */
		offset = 0;

		/* The parser's error can be catched outside, reading the
		 * http_errno struct member
		 */
		if (http_ctx->parser.http_errno != HPE_OK) {
			goto lb_exit;
		}

		buf = buf->frags;
	}
lb_exit:
	return head_end_off;
}

int http_parse_url(struct http_parser_url * u,const char *url, char * ip_addr ,uint16_t * port)
{
	struct in_addr  in_addr_t = { { { 0 } } };
	int ret;

	http_parser_url_init(u);

	ret = http_parser_parse_url(url, strlen(url), false,u);

	if (ret) {
		SYS_LOG_ERR("http_parser error!\n");
		return -1;
	}

	if (u->field_set & (0x01 << UF_PORT))
	{
		*port = u->port;
	}
	else
	{
		*port = HTTP_SERVICE_PORT;
	}

	memcpy(ip_addr, &url[u->field_data[UF_HOST].off], u->field_data[UF_HOST].len);

	ret = net_addr_pton(AF_INET, ip_addr, &in_addr_t);

	if (ret < 0) {
		ret = net_dns_resolve(ip_addr, &in_addr_t);
		if (ret == 0) {
			net_addr_ntop(AF_INET, &in_addr_t, ip_addr, NET_IPV4_ADDR_LEN);
		}
	}

	return ret;
}

int http_wait_response_code(struct http_client_ctx *http_ctx, int timeout)
{
	int response_code = 0;

	if(!os_sem_take(&http_ctx->net_data_sem, OS_MSEC(timeout)))
	{
		response_code = http_ctx->parser.status_code;
	}

	return response_code;
}

int http_receive_data(struct http_client_ctx *http_ctx, u8_t *data, u32_t len, u32_t timeout)
{
    u8_t * destbuf = data;
	struct net_pkt *nbuf = NULL;
    int remain_len = len;
	int nbuffer_off = 0;
	u16_t pos;
	u16_t data_len;
	http_ctx->isworking = 1;

	if(http_ctx->received_len + remain_len >= http_ctx->content_length)
	{
		remain_len = http_ctx->content_length - http_ctx->received_len;
		len = remain_len;
		http_ctx->finished = 1;
	}

	/*read from net buffer */
    while(remain_len > 0)
    {
		/*read from net buffer */
        if(http_ctx->cur_nbuf != NULL)
        {
            nbuf = http_ctx->cur_nbuf;
            nbuffer_off = http_ctx->cur_off;
        }
        else
        {
			/* clear net_data_sem, get a new net buffer from fifo  */
			os_sem_reset(&http_ctx->net_data_sem);
			nbuf = os_fifo_get(&http_ctx->netbuffifo, K_NO_WAIT);
			if(nbuf == NULL)
			{
				if(os_sem_take(&http_ctx->net_data_sem, OS_MSEC(timeout)))
				{
					SYS_LOG_DBG("wait ...timeout \n");
				}
				nbuf = os_fifo_get(&http_ctx->netbuffifo, K_NO_WAIT);
				if(nbuf == NULL)
				{
					http_ctx->isworking = 0;
					return len - remain_len;
				}
			}

			http_ctx->cur_nbuf = nbuf;
			nbuffer_off = 0;

			/*first frag , we must skip the tcp head */
			nbuffer_off = net_buf_frags_len(nbuf->frags) - net_pkt_appdatalen(nbuf);
        }

		data_len = net_buf_frags_len(nbuf->frags) - nbuffer_off;

		if(data_len == 0)
		{
			net_pkt_unref(http_ctx->cur_nbuf);
            http_ctx->cur_nbuf = NULL;
#ifdef	CONFIG_NET_TCP_CTRL_ACK
			if (os_fifo_cnt_sum(&http_ctx->netbuffifo) < HTTP_KEEP_PKT_MAX) {
				net_context_ctrl_send_ack(http_ctx->tcp_ctx->app_ctx.default_ctx->ctx);
			}
#endif
			continue;
		}

		if(data_len >= remain_len)
		{
			data_len = remain_len;
		}

		if(destbuf != NULL){
			net_frag_read(nbuf->frags, nbuffer_off, &pos, data_len, destbuf);
			destbuf += data_len;
		}

		remain_len -= data_len;
		nbuffer_off += data_len;
		http_ctx->received_len += data_len;
		if(remain_len == 0)
        {
            http_ctx->cur_off = nbuffer_off;
            break;
        }
		else
		{
			net_pkt_unref(http_ctx->cur_nbuf);
			http_ctx->cur_nbuf = NULL;
#ifdef	CONFIG_NET_TCP_CTRL_ACK
			if (os_fifo_cnt_sum(&http_ctx->netbuffifo) < HTTP_KEEP_PKT_MAX) {
				net_context_ctrl_send_ack(http_ctx->tcp_ctx->app_ctx.default_ctx->ctx);
			}
#endif
		}
    }
	http_ctx->isworking = 0;
	return len;
}

int http_send_data(struct http_client_ctx *http_ctx, u8_t *data, u32_t len, bool chunked, bool last_block)
{
	int rc = 0;
	struct net_pkt * tx = NULL;
	struct tcp_client_ctx * ctx;
	char temp_head[10];

	http_ctx->isworking = 1;
	ctx = http_ctx->tcp_ctx;

	tx = net_pkt_get_tx(ctx->app_ctx.default_ctx->ctx, K_FOREVER);

	if(tx == NULL)
	{
		SYS_LOG_ERR(" tx is null");
		rc = -1;
		goto exit;
	}

	net_pktbuf_set_owner(tx);

	if(chunked)
	{
		rc = snprintf(temp_head, sizeof(temp_head), "%x\r\n", len);
		if (!(rc = net_pkt_append(tx, rc , temp_head, K_FOREVER)))
		{
			SYS_LOG_ERR("net_nbuf_append error %d",rc);
			goto send_failed;
		}
	}

	if (!(rc = net_pkt_append(tx, len,	data, K_FOREVER)))
	{
		SYS_LOG_ERR("net_nbuf_append error %d",rc);
		goto send_failed;
	}

	if(chunked)
	{
		if(last_block)
		{
			if(len != 0)
			{
				rc = snprintf(temp_head, sizeof(temp_head),"\r\n%x\r\n\r\n", 0);
			}
			else
			{
				rc = snprintf(temp_head, sizeof(temp_head),"%x\r\n\r\n", 0);
			}
		}
		else
		{
			rc = snprintf(temp_head, sizeof(temp_head),"\r\n");
		}

		if (!(rc = net_pkt_append(tx, rc , temp_head, K_FOREVER)))
		{
			SYS_LOG_ERR("net_nbuf_append error %d",rc);
			goto send_failed;
		}
	}

	rc = net_app_send_pkt(&ctx->app_ctx, tx, NULL, 0, ctx->timeout, NULL);

	if(rc < 0)
	{
		SYS_LOG_ERR("http send failed ... %d",rc);
	}
	else
	{
		SYS_LOG_DBG("http send ok ... rc %d",rc);
		/* send success, app can't unref buf */
		tx = NULL;
		rc = len;
	}

send_failed:
	if(tx != NULL)
	{
		net_pkt_unref(tx);
	}
exit:
	http_ctx->isworking = 0;
	return rc;
}
