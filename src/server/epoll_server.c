/*
 * NOTE1: on Linux, if the LINGER option is
 * enabled on a socket, a close system call
 * issued on that socket will block until
 * all pending data is drained REGARDLESS
 * if it's in a non blocking mode or not.
 *
 * NOTE2: as both connections and services will
 * be used in the same epoll instance, they both
 * need to inherit from EPOLLDATA which adds a
 * tag field to separate them in the work loop.
 *
 * NOTE3: functions that return int here (except
 * the epoll wrappers), will return 1 for success,
 * 0 for EAGAIN, or -1 for error.
 */


//#include "server.h"
#if 0
//#ifdef PLATFORM_LINUX
#include "../log.h"
#include "message.h"
#include "outputmessage.h"

//#include <queue>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <netinet/in.h>
#include <unistd.h>

/*
 * FORWARD DECLARATIONS
 */

/* structs */
struct connection;
struct service;

/* socket helpers */
static bool fd_set_linger(int fd, int seconds);
static bool fd_set_non_blocking(int fd);

/* connection functions */
static void connection_lock(struct connection *c);
static void connection_unlock(struct connection *c);
static void __connection_close(struct connection *c);
static void __connection_dispatch_input_message(struct connection *c);
static int __connection_read_partial_length(struct connection *c);
static int __connection_read_length(struct connection *c);
static int __connection_read_data(struct connection *c);
static int __connection_read(struct connection *c);
static bool __connection_resume_read(struct connection *c);
static bool connection_resume_read(struct connection *c);
static int __connection_write(struct connection *c);
static bool __connection_resume_write(struct connection *c);
static bool connection_resume_write(struct connection *c);
static void connection_start(struct connection *c);
void connection_close(struct connection *c);
void connection_send(struct connection *c, OutputMessage *out);

/* service functions */
static bool service_open(struct service *s);
static void service_close(struct service *s);
static int service_accept(struct service *s, struct connection **c);
static bool service_accept_all(struct service *s);
static Protocol *service_first_protocol(struct service *s);
static Protocol *service_identify_protocol(struct service *s, Message *msg);

/* server functions */
static bool server_is_epoll_open(void);
static int server_epoll_add(int fd, struct epoll_event *ev);
static int server_epoll_del(int fd);
static void server_close_services(void);
static bool server_open_services(void);
static void server_close_interrupt(void);
static bool server_create_interrupt(void);
static void server_close_epoll(void);
static bool server_create_epoll(void);
static void server_do_work(void);
void server_run(void);
void server_stop(void);
bool server_add_protocol(Protocol *protocol, int port);

/*
 * EPOLLDATA
 */
enum epoll_tag{
	TAG_NONE = 0,
	TAG_INTERRUPT,
	TAG_CONNECTION,
	TAG_SERVICE,
};

struct epoll_data{
	enum epoll_tag tag;
};

// connection settings
#define CONNECTION_MAX_INPUT		(1<<15) // 32K
#define CONNECTION_MAX_OUTPUT		(1<<15) // 32K

// connection flags
#define CONNECTION_NEW			0x00
#define CONNECTION_READY		0x01
#define CONNECTION_CLOSING		0x02
#define CONNECTION_FIRST_MSG		0x04
#define CONNECTION_READING_LENGTH	0x08
#define CONNECTION_PARTIAL_LENGTH	0x10

struct connection{
	enum epoll_tag		tag;
	int			fd;
	uint32			flags;
	uint32			ref_count;
	uint32			rdwr_count;
	struct sockaddr_in	addr;
	pthread_mutex_t		mtx;
	struct service		*service;
	struct protocol		*protocol;
	void			*protocol_handle;
	//void			*input;
	//void			*output;
};

// service settings
#define SERVICE_MAX_PROTOCOLS 4

struct service{
	enum epoll_tag	tag;
	int		fd;
	int		port;
	int		protocol_count;
	Protocol	protocols[SERVICE_MAX_PROTOCOLS];
};

/*
 * Socket Helpers
 */

static bool fd_set_linger(int fd, int seconds){
	struct linger l;
	l.l_onoff = (seconds > 0);
	l.l_linger = seconds;
	if(setsockopt(fd, SOL_SOCKET, SO_LINGER,
			&l, sizeof(struct linger)) == -1){
		LOG_ERROR("fd_set_linger: failed to"
			" set linger option (errno = %d)", errno);
		return false;
	}
	return true;
}

static bool fd_set_non_blocking(int fd){
	int flags = fcntl(fd, F_GETFL);
	if(flags == -1){
		LOG_ERROR("fd_set_non_blocking: failed to"
			" retrieve socket flags (errno = %d)", errno);
		return false;
	}
	if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1){
		LOG_ERROR("fd_set_non_blocking: failed to"
			" set non blocking mode (errno = %d)", errno);
		return false;
	}
	return true;
}

/*
 * Connection Interface
 */

static void connection_ctor(struct connection *c, int fd,
		struct sockaddr_in *addr,
		struct service *service){
	c->tag = TAG_CONNECTION;
	c->fd = fd;
	c->flags = CONNECTION_NEW;
	c->ref_count = 1;
	c->rdwr_count = 0;
	if(addr != NULL)
		memcpy(&c->addr, addr, sizeof(struct sockaddr_in));
	else
		memset(&c->addr, 0, sizeof(struct sockaddr_in));
	pthread_mutex_init(&c->mtx, NULL);
	c->service = service;
	c->protocol = NULL;
	c->protocol_handle = NULL;
	// init c->input;
	// init c->output;
	//
}

static void connection_dtor(struct connection *c){
	if(c->fd != -1){
		close(c->fd);
		c->fd = -1;
	}
	pthread_mutex_destroy(&c->mtx);
	if(c->protocol != NULL){
		c->protocol->destroy_handle(c, c->protocol_handle);
		c->protocol = NULL;
		c->protocol_handle = NULL;
	}
	// destroy c->input;
	// destroy c->output;
}

static void connection_lock(struct connection *c){
	pthread_mutex_lock(&c->mtx);
}

static void connection_unlock(struct connection *c){
	pthread_mutex_unlock(&c->mtx);
}

static void __connection_close(struct connection *c){
	// @TODO
	// set the connection into a closing state
	// shutdown reading but keep sending queued messages
	// call protocol->on_close
	// close fd when all queued output messages have been sent
	// delete connection
}

static void __connection_dispatch_input_message(struct connection *c){
	// @TODO
	struct service *svc = c->service;
	Protocol *proto = c->protocol;
	Message *input = c->input;

	// create protocol handle if there's none
	if(proto == NULL){
		// proto = service_identify_protocol(svc, input);
	}

	// dispatch message
	if((c->flags & CONNECTION_FIRST_MSG) == 0){}
}

static int __connection_read_partial_length(struct connection *c){
/*
 * REM: this partial length may not be a problem at all
 * but if it is, this workaround should account for it
 */
	Message *input = c->input;
	int ret = read(c->fd, (void*)(input->buffer + 1), 1);
	if(ret <= 0) return (errno == EAGAIN) ? 0 : -1;
	c->flags &= ~CONNECTION_PARTIAL_LENGTH;
	c->flags &= ~CONNECTION_READING_LENGTH;
	input->readpos = 0;
	input->length = input->get_u16() + 2;
	return 1;
}

static int __connection_read_length(struct connection *c){
	Message *input = c->input;
	int ret = read(c->fd, (void*)input->buffer, 2);
	if(ret <= 0) return (errno == EAGAIN) ? 0 : -1;
	if(ret == 1){
		ret = read(c->fd, (void*)(input->buffer + 1), 1);
		if(ret <= 0){
			c->flags |= CONNECTION_PARTIAL_LENGTH;
			return (errno == EAGAIN) ? 0 : -1;
		}
	}
	c->flags &= ~CONNECTION_READING_LENGTH;
	input->readpos = 0;
	input->length = input->get_u16() + 2;
	return 1;
}

static int __connection_read_data(struct connection *c){
	Message *input = c->input;
	size_t dataleft = input->length - input->readpos;
	int ret;
	do{
		ret = read(c->fd, (void*)(input->buffer + input->readpos), dataleft);
		if(ret <= 0) return (errno == EAGAIN) ? 0 : -1;
		input->readpos += ret;
		dataleft -= ret;
	} while(dataleft > 0);
	__connection_dispatch_input_message(c);
	c->rdwr_count += 1;
	return 1;
}

static int __connection_read(struct connection *c){
	if(c->flags & CONNECTION_READING_LENGTH){
		if(c->flags & CONNECTION_PARTIAL_LENGTH)
			return __connection_read_partial_length(c);
		else
			return __connection_read_length(c);
	}else{
		return __connection_read_data(c);
	}
}

static bool __connection_resume_read(struct connection *c){
	int ret;
	if((c->flags & CONNECTION_READY) == 0)
		return false;
	do {
		ret = __connection_read(c);
	} while(ret == 1);
	return ret == 0;
}

static bool connection_resume_read(struct connection *c){
	bool ret;
	connection_lock(c);
	ret = __connection_resume_read(c);
	connection_unlock(c);
	return ret;
}

static int __connection_write(struct connection *c){
	OutputMessage *out;
	Message *msg;
	size_t dataleft;
	int ret;

	if(c->output.empty())
		return 0;
	out = &c->output.front();
	msg = out->msg;
	dataleft = msg->length - msg->readpos;
	do{
		ret = write(c->fd, (void*)(msg->buffer + msg->readpos), dataleft);
		if(ret <= 0) return (errno == EAGAIN) ? 0 : -1;
		msg->readpos += ret;
		dataleft -= ret;
	} while(dataleft > 0);
	output_message_release(out);
	c->output.pop();
	c->rdwr_count += 1;
	return 1;
}

static bool __connection_resume_write(struct connection *c){
	int ret;
	if((c->flags & CONNECTION_READY) == 0)
		return false;
	do {
		ret = __connection_write(c);
	} while(ret == 1);
	return ret == 0;
}

static bool connection_resume_write(struct connection *c){
	bool ret;
	connection_lock(c);
	ret = __connection_resume_write(c);
	connection_unlock(c);
	return ret == 0;
}

static void connection_start(struct connection *c){
	//struct service *svc = c->service;
	connection_lock(c);
	c->flags |= CONNECTION_READY;
	c->flags |= CONNECTION_READING_LENGTH;
	/*if(service_sends_first(svc)){
		c->protocol = service_first_protocol(svc);
		c->protocol_handle = c->protocol->create_handle(c);
		c->protocol->on_connect(c, c->protocol_handle);

	}*/
	__connection_resume_read(c);
	connection_unlock(c);
}

void connection_close(struct connection *c){
	connection_lock(c);
	__connection_close(c);
	connection_unlock(c);
}

void connection_send(struct connection *c, OutputMessage *out){
	connection_lock(c);
	out->msg->readpos = 0;
	c->output.push(*out);
	__connection_resume_write(c);
	connection_unlock(c);
}

/*
 * Service Interface
 */

static bool service_open(struct service *s){
	if(!server_is_epoll_open()){
		LOG_ERROR("service_open: server's epoll"
			" instance hasn't been created yet");
		return false;
	}

	if(s->fd != -1){
		LOG_WARNING("service_open: service is already open");
		return true;
	}

	// create socket
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1){
		LOG_ERROR("service_open: failed to create"
			" service socket (errno = %d)", errno);
		return false;
	}

	// set socket options
	if(!fd_set_linger(fd, 0) || !fd_set_non_blocking(fd)){
		LOG_ERROR("service_open: failed to set socket options");
		close(fd);
		return false;
	}

	// bind socket to port
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(s->port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1){
		LOG_ERROR("service_open: failed to bind service"
			" to port %d (errno = %d)", s->port, errno);
		close(fd);
		return false;
	}

	// start listening for connections
	if(listen(fd, SOMAXCONN) == -1){
		LOG_ERROR("service_open: failed to start "
			" listening (errno = %d)", errno);
		close(fd);
		return false;
	}

	// add socket to epoll in level-triggered mode
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.ptr = s;
	if(server_epoll_add(fd, &ev) != 0){
		LOG_ERROR("service_open: failed to"
			" add socket to epoll (errno = %d)", errno);
		close(fd);
		return false;
	}

	s->fd = fd;
	return true;
}

static void service_close(struct service *s){
	if(s->fd == -1) return;
	server_epoll_del(s->fd);
	close(s->fd);
	s->fd = -1;
}

static int service_accept(struct service *s, struct connection **c){
	struct epoll_event ev;
	struct sockaddr_in addr;
	socklen_t addrlen;
	int ret;

	// accept new socket
	addrlen = sizeof(struct sockaddr_in);
	ret = accept(s->fd, (struct sockaddr*)&addr, &addrlen);
	if(ret < 0){
		if(errno == EAGAIN)
			return 0;
		else
			return -1;
	}

	// set new socket options
	if(fd_set_linger(ret, 0) == -1 ||
		fd_set_non_blocking(ret) == -1){
		LOG_ERROR("service_accept: failed to"
			"set new socket options");
		close(ret);
		return -1;
	}

	// create connection handle
	// @TODO
	// allocate connection
	// connection_ctor(*c, ret, &addr, s);

	// add it to epoll in edge-triggered mode
	ev.events = EPOLLET | EPOLLIN | EPOLLOUT;
	ev.data.ptr = *c;
	if(server_epoll_add(ret, &ev) != 0){
		LOG_ERROR("service_accept: failed to"
			" add socket to epoll (errno = %d)", errno);
		// @TODO
		// release connection
		// connection_dtor(*c);
		// connection_release(*c);
		close(ret);
		return -1;
	}
	return 1;
}

static bool service_accept_all(struct service *s){
	struct connection *c;
	int ret;
	while(1){
		ret = service_accept(s, &c);
		if(ret != 1) return (ret == 0);
		connection_start(c);
	}
}

static Protocol *service_first_protocol(struct service *s){
	return &s->protocols[0];
}

static Protocol *service_identify_protocol(struct service *s, Message *msg){
	for(int i = 0; i < s->protocol_count; i += 1){
		if(s->protocols[i].identify(msg))
			return &s->protocols[i];
	}
	return NULL;
}

/*
 * Server Interface
 */

#define MAX_SERVICES	4
static bool running = false;
static int epfd = -1;
static int interrupt_fd = -1;
static struct epoll_data interrupt_data = { TAG_INTERRUPT };
static int service_count = 0;
static struct service services[MAX_SERVICES];

static bool server_is_epoll_open(void){
	return epfd != -1;
}

static int server_epoll_add(int fd, struct epoll_event *ev){
	return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, ev);
}

static int server_epoll_del(int fd){
	/*
	 * from epoll_ctl(2):
	 *	In kernel versions before 2.6.9, the EPOLL_CTL_DEL
	 *	operation required a non-NULL pointer in event, even
	 *	though this argument is ignored.
	 */
	struct epoll_event ev;
	return epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
}

static void server_close_services(void){
	for(int i = 0; i < service_count; i += 1)
		service_close(&services[i]);
}

static bool server_open_services(void){
	for(int i = 0; i < service_count; i += 1){
		if(!service_open(&services[i])){
			server_close_services();
			return false;
		}
	}
	return true;
}

static void server_close_interrupt(void){
	if(interrupt_fd == -1) return;
	server_epoll_del(interrupt_fd);
	close(interrupt_fd);
	interrupt_fd = -1;
}

static bool server_create_interrupt(void){
	if(interrupt_fd != -1) return true;
	// create interrupt eventfd
	interrupt_fd = eventfd(0, 0);
	if(interrupt_fd == -1){
		LOG_ERROR("server_create_interrupt: failed to create"
			" interrupt eventfd (errno = %d)", errno);
		return false;
	}
	// set it as nonblocking
	if(fd_set_non_blocking(interrupt_fd) != 0){
		LOG_ERROR("server_create_interrupt: failed to set"
			" interrupt eventfd as nonblocking");
		goto clup;
	}
	// add it to epoll in level-triggered mode
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.ptr = &interrupt_data;
	if(server_epoll_add(interrupt_fd, &ev) != 0){
		LOG_ERROR("server_create_interrupt: failed to add"
			" interrupt eventfd to epoll (errno = %d)", errno);
		goto clup;
	}
	return true;

clup:	close(interrupt_fd);
	interrupt_fd = -1;
	return false;
}

static void server_close_epoll(void){
	if(epfd == -1) return;
	close(epfd);
	epfd = -1;
}

static bool server_create_epoll(void){
	if(epfd != -1) return true;
	epfd = epoll_create1(0);
	if(epfd == -1){
		LOG_ERROR("server_create_epoll: failed to"
			" create epoll instance (errno = %d)", errno);
		return false;
	}
	return true;
}

static void server_do_work(void){
	static constexpr int max_work = 256;
	struct epoll_event evs[max_work];
	int ev_count, error_count, i;
	double failure;
	EPOLLDATA *data;

	error_count = 0;
	ev_count = epoll_wait(epfd, evs, max_work, -1);
	for(i = 0; i < ev_count; i += 1){
		data = (struct epoll_data*)evs[i].data.ptr;
		if(data->tag == TAG_CONNECTION){
			struct connection *conn = (struct connection*)data;
			int conn_error = 0;
			if(evs[i].events & EPOLLIN)
				conn_error += !connection_resume_read(conn);
			if(evs[i].events & EPOLLOUT)
				conn_error += !connection_resume_write(conn);
			error_count += (conn_error != 0);

		}else if(data->tag == TAG_SERVICE){
			struct service *svc = (struct service*)data;
			DEBUG_CHECK(evs[i].events & EPOLLIN,
				"server_do_work: invalid service event");
			error_count += !service_accept_all(svc);
		}else if(data->tag == TAG_INTERRUPT){
			uint64 x;
			DEBUG_CHECK(evs[i].events & EPOLLIN,
				"server_do_work: invalid interrupt event");
			read(interrupt_fd, &x, sizeof(uint64));
		}else{
			LOG_ERROR("server_do_work: invalid epoll event data");
			error_count += 1;
		}
	}

	failure = (double)(error_count) / ev_count;
	if(failure >= 0.90){
		LOG_ERROR("server_do_work: failure levels over 90%");
		LOG_ERROR("server_do_work: shutting down...");
		running = false;
	}else if(failure >= 0.75){
		LOG_WARNING("server_do_work: failure levels over 75%");
	}else if(failure >= 0.50){
		LOG_WARNING("server_do_work: failure levels over 50%");
	}
}

void server_run(void){
	if(running) return;
	if(!server_create_epoll())
		return;
	if(!server_create_interrupt())
		goto clup1;
	if(!server_open_services())
		goto clup2;
	running = true;
	while(running)
		server_do_work();

	server_close_services();
clup2:	server_close_interrupt();
clup1:	server_close_epoll();
}

void server_stop(void){
	uint64 x = 1;
	running = false;
	write(interrupt_fd, &x, sizeof(uint64));
}

bool server_add_protocol(Protocol *protocol, int port){
	int i;
	struct service *s = NULL;
	if(running || protocol == NULL)
		return false;
	for(i = 0; i < service_count; i += 1){
		if(services[i].port == port){
			s = &services[i];
			break;
		}
	}

	if(s == NULL){
		if(service_count >= MAX_SERVICES)
			return false;
		s = &services[service_count];
		service_count += 1;

		s->tag = TAG_SERVICE;
		s->fd = -1;
		s->port = port;
		s->protocol_count = 1;
		memcpy(&s->protocols[0], protocol, sizeof(Protocol));
	}else{
		if(protocol->sends_first ||
		  s->protocol_count >= SERVICE_MAX_PROTOCOLS ||
		  s->protocols[0].sends_first)
			return false;
		i = s->protocol_count;
		s->protocol_count += 1;
		memcpy(&s->protocols[i], protocol, sizeof(Protocol));
	}
	return true;
}

#endif
