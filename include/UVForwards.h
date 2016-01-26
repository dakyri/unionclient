/*
 * UVForwards.h
 *
 *  Created on: May 13, 2014
 *      Author: dak
 */

#ifndef UVFORWARDS_H_
#define UVFORWARDS_H_

/**
 * forward decls to remove dependency of classes using uv dependent classes from requiring the full uv headers ...
 */
#ifndef UV_H
struct sockaddr;
struct uv_buf_t;
/* Request types. */
typedef struct uv_req_s uv_req_t;
typedef struct uv_getaddrinfo_s uv_getaddrinfo_t;
typedef struct uv_shutdown_s uv_shutdown_t;
typedef struct uv_write_s uv_write_t;
typedef struct uv_connect_s uv_connect_t;
typedef struct uv_udp_send_s uv_udp_send_t;
typedef struct uv_fs_s uv_fs_t;
typedef struct uv_work_s uv_work_t;

/* Handle types. */
typedef struct uv_loop_s uv_loop_t;
typedef struct uv_handle_s uv_handle_t;
typedef struct uv_stream_s uv_stream_t;
typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_udp_s uv_udp_t;
typedef struct uv_pipe_s uv_pipe_t;
typedef struct uv_tty_s uv_tty_t;
typedef struct uv_poll_s uv_poll_t;
typedef struct uv_timer_s uv_timer_t;
typedef struct uv_prepare_s uv_prepare_t;
typedef struct uv_check_s uv_check_t;
typedef struct uv_idle_s uv_idle_t;
typedef struct uv_async_s uv_async_t;
typedef struct uv_process_s uv_process_t;
typedef struct uv_fs_event_s uv_fs_event_t;
typedef struct uv_fs_poll_s uv_fs_poll_t;
typedef struct uv_signal_s uv_signal_t;
#endif


class UVTCPClient
{
	friend class UVEventLoop;
public:
	UVTCPClient();
	virtual ~UVTCPClient();
	std::string IP4Addr() const;

protected:
	uv_tcp_t* socket;
	struct sockaddr* address;
};

#endif /* UVFORWARDS_H_ */
