/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  Stream Server with Reverse Ajax
 *
 *       Version:  0.1
 *       Created:  2009-XX-XX 9:10:03
 *       Revision:  none
 *       Compiler:  g++
 *
 *       Author:  Vadim Popov - zion.vad(at)gmail.com
 *         
 *
 * =====================================================================================
 */
 
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <time.h>

#include <string>
#include <map>
#include <signal.h>

//unix sockets
#include <sys/un.h>
#include <sys/uio.h>

//#include "JSON_parser.h"
//#include <yajl/yajl_parse.h>
//#include <yajl/yajl_gen.h>

// --------------------------------------------------------------- //


#define COUNT_FS 9
class Pool {
	public:
		Pool() {
			for(int i=0; i <= COUNT_FS; i++)
			{
				this->msg[i] = new char[10740];
				strcpy(this->msg[i]," ");
			}
			this->pcount = 0;
			this->cur_pos = 0;
		}
		~Pool() {
			for(int i=0; i <= COUNT_FS; i++) {
				delete[] this->msg[i];
			}
		}
		void next() {
			this->pcount++;
			if(this->pcount > COUNT_FS)
			{
			    this->pcount = 0;
			}
		}
		char* get() {
			unsigned int tmp_pc = this->pcount;
			/*if(this->cur_pos > COUNT_FS)
			{
			    std::cout << std::endl  << " ---------------------------- :" << std::endl;
			    this->cur_pos = 0;
			    return NULL;
			}*/
			this->next();
			//this->cur_pos++;
			return this->msg[tmp_pc];
		}
		bool set(char* str, int len) {
			char* tmp_buf;
			strncpy(this->msg[this->pcount], str, len);
			tmp_buf = this->msg[this->pcount] + len;
			*tmp_buf = '\x00';
			this->next();
		}
	private:
		char* msg[COUNT_FS+1];
		unsigned int pcount;
		unsigned int cur_pos;
};

// ---------------------------- MYSQL ---------------------------- //
#include <mysql.h>
MYSQL mysql;
MYSQL_RES *mysql_result;
MYSQL_ROW mysql_row;
char mysql_server[] = "localhost";
char mysql_user[] = "love";
char mysql_password[] = "asdcxz";
char mysql_db[] = "dating";
char mysql_table_prefix[] = "pro_";
#define TABLE "pro_active_sessions"
#define TABLE_PM_CL "pro_pm_contactlist"
#define TABLE_PM_OM "pro_pm_offline_messages"
#define TABLE_USER "pro_user"
// ---------------------------- MYSQL ---------------------------- //

//#define SUPPORT_MOD_COMM 1
char modcomm_socket [] = "/tmp/comm";

#define PORT 24078
#define BIND_O "0.0.0.0"
#define STACK_LISTEN 10
#define MAXEVENTS 150
#define SESSION_LEN 32
#define NICK_LEN 100

#define LF (char)10
#define CR (char)13
#define CRLF "\x0d\x0a"


#define zi_str3cmp(m, c0, c1, c2) m[0] == c0 && m[1] == c1 && m[2] == c2
#define zi_str4cmp(m, c0, c1, c2, c3) m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3

#define H2B(a) ((a) <= '9' ? (a) - '0' : (a) - 55);
#define ISXDIGIT(a) (((a) >= '0' && (a) <= '9') || ((a) >= 'A' && (a) <= 'F') || ((a) >= 'a' && (a) <= 'f'))
#define ISDEG(a) (((a) >= '0' && (a) <= '9')


enum event_method 
{ 
	CONNECTION,
	CONNECTED,
	METHOD_GET,
	METHOD_POST,
	CROSSDOMAIN_SEND
};

class Server;
//static int  ppr(void*, int, const JSON_value*);

Server* srv;
char* bufz;
int epoll_fd;
struct epoll_event ev;

void daemonize()
{
        int i=0;
        if(getppid()==1) return; /* already a daemon */
        if (getpid()!=1){
                i=fork();
                if (i<0) {perror("cannont fork"); exit (-1);} /* fork error */
                if (i>0) exit (0);/* parent exits */
                setsid(); /* obtain a new process group */
                for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */
                i=open("/dev/null",O_RDWR); dup(i); dup(i); /* handle standart I/O */

                printf("daemon started with pid >%d<\n", getpid());

        }
}



int url_ndecode(const char *src, char *dst, int lencpy)
{
    int state = 0, code;
    char *start = dst;
    const char *slim = src + lencpy;
    char *dlim = dst + lencpy;

    if (dst >= dlim) {
        return 0;
    }
    dlim--; /* ensure spot for '\0' */

    while (src < slim && dst < dlim) {
    	if(*src == '+') {
    		*dst++ = ' ';
    		//src++;
		}
        else 
        switch (state) {
            case 0:
                if (*src == '%') {
                    state = 1;
                } else {
                    *dst++ = *src;
                }
                break;
            case 1:
                code = H2B(*src);
            case 2:    
                if (ISXDIGIT(*src) == 0) {
                    errno = EILSEQ;
                    return -1;
                }
                if (state == 2) {
                    *dst++ = (code * 16) + H2B(*src);
                    state = 0;
                } else {
                    state = 2;
                }
                break;
        }
        src++;
    }
    *dst = '\0'; 

    return dst - start;
}

#ifdef SUPPORT_MOD_COMM
#define MAXLINE 259
static struct cmsghdr *cmptr = NULL;        /* malloc'ed first time */
#define CONTROLLEN (sizeof(struct cmsghdr) + sizeof(int))
int recv_fd (int servfd, char *query)
{
        int newfd, nread, status;
        char buf[MAXLINE];
        struct iovec iov[1];
        struct msghdr msg;

        status = -1;
        for (;;)
        {
                iov[0].iov_base = buf;
                iov[0].iov_len = sizeof (buf);

                iov[1].iov_base = query;
                iov[1].iov_len = sizeof(query);

                msg.msg_iov = iov;
                msg.msg_iovlen = 2;

                msg.msg_name = NULL;
                msg.msg_namelen = 0;
               // if (cmptr == NULL && (cmptr = (cmsghdr *) malloc (CONTROLLEN)) == NULL)
                 if (cmptr == NULL && (cmptr = new cmsghdr[CONTROLLEN]) == NULL)
                        return (-1);
                msg.msg_control = (caddr_t) cmptr;
                msg.msg_controllen = CONTROLLEN;
                if ((nread = recvmsg (servfd, &msg, 0)) < 0)
                        printf("recvmsg error\n");
                else if (nread == 0) {
                        printf("connection closed by server\n");
                        return (-1);
                }
                strcpy(query, "");
                strncpy(query, &buf[2], (nread-2 > 255)? 255: nread-2);
                query[(nread-2 > 255)? 255: nread-2] = 0;
                newfd = *(int *) CMSG_DATA (cmptr);
                return (newfd);
        }
}
#endif

void
dbping()
{
	if (mysql_ping(&mysql)!=0)
	{
		fprintf(stderr,"lost connection to the database, Error:> %s < Restore\n", mysql_error(&mysql));

		if (!mysql_real_connect(&mysql, mysql_server, mysql_user, mysql_password, mysql_db, 0, NULL, 0))
		{
		  fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(&mysql));
		  exit(1);
		}
		else
		{
		    if (mysql_real_query(&mysql, "SET names 'utf8'", 17) != 0)
			{
			    fprintf(stderr,"Bad sql query, Error: %s\n", mysql_error(&mysql));
			}
		}
	}
}

// ============================================== connections.cpp ===========================================================

class Connection {
	public:
		Connection(int);
		~Connection();
		int user_socket_fd;
		char* inc_buffer, * inc_buffer_cur, * inc_buffer_end, * content;
		char* out_buffer, * out_buffer_start, * out_buffer_end;
		char* out_buffer_user_start,* out_buffer_user, * out_buffer_user_end;
        char* session;
        int   id, send_count;
        char* nickname;
        char* to;
		
		int content_len;
		event_method method;
		
		//bool parse_header();
		bool parse_header(char*);
		bool parse_post_command(char*);
		bool post_parsed;
		bool add_pool;
		int  fill_buffer();
		bool send();
		void add_to_out(const char*, size_t&);
	private:
		void set_next(Connection*);
		void set_prev(Connection*);
		
};


Connection::Connection(int fd)
{

	this->user_socket_fd = fd;
	this->id = 0;
	this->send_count = 0;
	/*this->nextU = NULL;
	this->prevU = NULL;
	this->next  = NULL;
	this->prev  = NULL;*/

	this->post_parsed = false;
	this->content_len = -1;
	this->add_pool = false;
	this->method = CONNECTION;

	this->to = new char[NICK_LEN];

	this->out_buffer = this->out_buffer_start = this->out_buffer_end = NULL;
	//this->out_buffer = new char[10240];
	//this->out_buffer_start = this->out_buffer; // начало буфера
	//this->out_buffer_end   = this->out_buffer; // конец  буфера

	this->inc_buffer = new char[11340];
    this->inc_buffer_cur = this->inc_buffer; 
    this->inc_buffer_end = this->inc_buffer + 10240;

    this->nickname = new char[100];
    this->session = new char[41];

    strcpy(this->session,"...");
    strcpy(this->nickname,"...");
    strcpy(this->inc_buffer, "");
    strcpy(this->to, "");
    /*strcpy(out_buffer,  "HTTP/1.1 200 OK\r\n"
						"Server: StreamServer v0.1\r\n"
						"Content-Type: text/html; charset=UTF-8\r\n"
						"Cache-Control: no-store, no-cache\r\n"
						"Pragma: no-cache\r\n"
						"Expires: Mon, 24 Jul 1988 09:00:00 GMT\r\n"
						"Connection: close\r\n"
						//"Content-Length: 300000\r\n" 
						"Keep-Alive: max=0\r\n"
						"\r\n"
						"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">"
						"<html><head>"
						"<META http-equiv=\"Content-Type\" content=\"text/html; UTF-8\" /><META http-equiv=\"Cache-Control\" content=\"no-store\" /><META http-equiv=\"Cache-Control\" content=\"no-cache\" /><META http-equiv=\"Pragma\" content=\"no-cache\" /><META http-equiv=\"Expires\" content=\"Mon, 24 Jul 1988 09:00:00 GMT\" />"
						" </head> <body>");*/
	
}

Connection::~Connection()
{
	if(this->out_buffer) delete[] this->out_buffer;
	this->out_buffer = NULL;
	if(this->inc_buffer)
	{
		delete[] this->inc_buffer;
		this->inc_buffer = NULL;
	}
	delete[] this->nickname;
	this->nickname = NULL;
	delete[] this->session;
	this->session = NULL;
	delete[] this->to;
	this->to = NULL;
	
	ev.data.fd = user_socket_fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, user_socket_fd, &ev);
	::close(user_socket_fd);
	std::cout << "============================|= Соединение закрыто =|============================" << std::endl;
}

void
Connection::set_next(Connection* nxt)
{
	//this->next = nxt;
}

void
Connection::set_prev(Connection* prv)
{
	//this->prev = prv;
}

bool
Connection::send() 
{
	int ret, lenb;
	
	lenb = (this->out_buffer_end - this->out_buffer);
	if(lenb > 0) 
	{
	
		ret = ::send( this->user_socket_fd, this->out_buffer, lenb, 0);
		
		*this->out_buffer_end++ = 0;
		//std::cout << std::endl << "+++ " << this->out_buffer << " +++ " << std::endl;
		
		if(ret <= 0)
		{
			if(errno == EAGAIN)
			{
			    std::cout << std::endl << "****************** EAGAIN **********************" << std::endl;
			}
			goto free_buf;
		}
		//std::cout << "len before: " << (this->out_buffer_end - this->out_buffer) << std::endl;
		//std::cout << "in buf: " << this->out_buffer << std::endl;
		memmove(this->out_buffer, this->out_buffer + ret, ret);// + 1);
		this->out_buffer_end = this->out_buffer_end - ret;// + 1;
		this->out_buffer_end[0] = 0;
		//std::cout << "len after: " << (this->out_buffer_end - this->out_buffer) << std::endl;
	}

free_buf:	
	if(this->out_buffer)
	{
		delete[] this->out_buffer;
		this->out_buffer_start = this->out_buffer = this->out_buffer_end = NULL;
	}
	return true;
}

void
Connection::add_to_out(const char* addition, size_t& len)
{
	if(len == 0) return;
	
	if(!this->out_buffer)
	{
		this->out_buffer = new char[10240];
		this->out_buffer_start = this->out_buffer; 
		this->out_buffer_end   = this->out_buffer;
	}

	if ((int)(this->out_buffer_end - this->out_buffer) + len < 10240)
	{
		//strncpy(this->out_buffer_end, addition, len);
		//strcat(this->out_buffer, addition);
		for(int i = 0; i <= len; i++)
		{
			if(*addition == 0)break;
			*out_buffer_end++ = *addition++;
		}
		//*this->out_buffer_end = 0;
		//this->out_buffer_end = this->out_buffer_end + len;
	}
}

void
remove_conn(Connection*& cl, Connection*& firstC, Connection*& lastC, bool del_opt)
{
}


// ============================================== user.cpp ==================================================================
class PMFriend;

class User {
	public:
		User(char*,int);
		~User(void);
		
		//User *next, *prev;
		Connection *fC, *lC, *cC;
		std::map<int,Connection*> conns;
		std::map<int,Connection*>::iterator current_conn_it;
		
		char* nick, * nickname;
		int   id;
		char* session;
		char* out_buffer, *out_buffer_start, *out_buffer_end;
		
		PMFriend * my_friend_first, * me_friend_first;
		
		void fix_pm_list_shm(User*, bool, bool);
		void add_connection(Connection*);
		bool add_to_out(const char*, size_t);
		void add_to_out_escape(const char*, size_t);
		void remove_conn();
		void set_nickname(char*);
		char* get_nickname();
		
		void set_id(int);
		int get_id();
		
		bool is_valid();
		bool send();
};

User::User(char* sess, int nn)
{
	this->id = nn;
	this->my_friend_first = NULL;
	this->me_friend_first = NULL;
	this->nickname = NULL;
	this->session = new char[42];
	this->out_buffer_start = NULL;
	this->out_buffer = NULL;
	this->out_buffer_end = NULL;
	//this->nick = new char[100];
	strcpy(this->session, sess);
	//strcpy(this->nick, nn);
	this->lC = NULL;
	this->fC = NULL;
	this->cC = NULL;
	std::cout << nn << "=== New User :) ===" << this->id << std::endl;
}

User::~User()
{
	if(this->out_buffer_start) delete[] this->out_buffer_start;
	if(session) delete[] session;
	if(nickname) delete[] nickname;
	if(this->my_friend_first) delete my_friend_first;
	if(this->me_friend_first) delete me_friend_first;
	
	std::cout << "============================|= Пользователь удален =|============================" << std::endl;
}

void
User::set_nickname(char* n)
{
	this->nickname = new char[strlen(n)+6];
	strcpy(this->nickname, n);
}

char*
User::get_nickname()
{
	return this->nickname;
}

void
User::set_id(int n)
{
	this->id = n;
}

int
User::get_id()
{
	return this->id;
}


bool
User::send()
{
	int to_send, to_sended;
	bool all_sended = true;
	
	for( this->current_conn_it = this->conns.begin() ; this->current_conn_it != this->conns.end(); this->current_conn_it++ )
	{
		if(this->current_conn_it->second->send_count < 10)
		{
			if(this->current_conn_it->second->out_buffer_user == this->current_conn_it->second->out_buffer_user_end) continue;
			else all_sended = false;
			
			to_sended = ::send( this->current_conn_it->second->user_socket_fd, 
								this->current_conn_it->second->out_buffer_user,
								(this->current_conn_it->second->out_buffer_user_end - this->current_conn_it->second->out_buffer_user),
								0 );
			
			//*this->current_conn_it->second->out_buffer_user_end++ = 0;
			//std::cout << std::endl << "+++ " << this->current_conn_it->second->out_buffer_user << " +++ " << std::endl;
			//this->current_conn_it->second->out_buffer_user_end--;
			
			
			if(to_sended < 0)
			{
				if(errno == EAGAIN)
				{
					std::cout << std::endl << "*-*-**************** EAGAIN **********************" << std::endl;
					return true;
				}
				else return false;
			}
			
			this->current_conn_it->second->out_buffer_user += to_sended;
			
			if(this->current_conn_it->second->out_buffer_user == this->current_conn_it->second->out_buffer_user_end)
			{
				all_sended = true;
				this->current_conn_it->second->send_count = 0;
			} else {
				this->current_conn_it->second->send_count++;
				std::cout << std::endl << "######################### not all send #######################" << std::endl;
			}
		} else {
			conns.erase(this->current_conn_it);
			delete this->current_conn_it->second;
		}
	}
	
	if(all_sended)
	{
		if(this->out_buffer_start)
		{
			//delete[] this->out_buffer_start;
			//this->out_buffer_start = NULL;
			this->out_buffer = this->out_buffer_start;
		}
	}

	return true;
}

void
User::add_to_out_escape(const char* addition, size_t len)
{
	const char* cur = addition;
	const char* finder = addition;
	
	if(len != 0)
	{
		for(size_t i = 0; i<len; i++)
		{
			if(*finder == '"')
			{
				this->add_to_out(cur, (finder - cur));
				this->add_to_out("\\\"", 2);
				cur = finder + 1;
			}
			else if(*finder == '\'')
			{
				this->add_to_out(cur, (finder - cur));
				this->add_to_out("\\\'", 2);
				cur = finder + 1;
			}
			else if(*finder == '\n')
			{
				this->add_to_out(cur, (finder - cur));
				this->add_to_out("<br/>", 5);
				cur = finder + 1;
			}
			else if(*finder == '<')
			{
				this->add_to_out(cur, (finder - cur));
				this->add_to_out("&lt;", 4);
				cur = finder + 1;
			}
			else if(*finder == '>')
			{
				this->add_to_out(cur, (finder - cur));
				this->add_to_out("&gt;", 4);
				cur = finder + 1;
			}
			else if(*finder == '*')
			{
				this->add_to_out(cur, (finder - cur));
				this->add_to_out("&#42;", 5);
				cur = finder + 1;
			}
			else if(*finder == '/')
			{
				this->add_to_out(cur, (finder - cur));
				this->add_to_out("\\/", 2);
				cur = finder + 1;
			}
			finder++;
		}
		
		this->add_to_out(cur, (finder - cur));
	}
	
}


bool
User::add_to_out(const char* addition, size_t len)
{
	
	if(len != 0)
	{	
		if(!this->out_buffer_start)
		{
			this->out_buffer_start = new char[10240];
			this->out_buffer = this->out_buffer_start;
		}
		if(((this->out_buffer - this->out_buffer_start) + len) < 10240)
		{
			memcpy(this->out_buffer, addition, len);
			
			//*(this->out_buffer + len + 1) = 0;
			//std::cout << std::endl << "# add_to_out: " << this->out_buffer << std::endl;
			
			this->out_buffer += len;
			
			for ( this->current_conn_it = this->conns.begin() ; this->current_conn_it != this->conns.end(); this->current_conn_it++ )
			{
				if(this->current_conn_it->second)
				{
					this->current_conn_it->second->out_buffer_user_start = this->out_buffer_start;
					this->current_conn_it->second->out_buffer_user       = this->out_buffer_start;
					this->current_conn_it->second->out_buffer_user_end   = this->out_buffer;
				}
			}
		} else return false;
	}
	return true;
}


/*
bool
User::send()
{
	for( this->current_conn_it = this->conns.begin() ; this->current_conn_it != this->conns.end(); this->current_conn_it++ )
	{
		if( !this->current_conn_it->second->send() )
		{
			conns.erase(this->current_conn_it);
			delete this->current_conn_it->second;
		}
	}
	return true;
}

void
User::add_to_out(const char* addition, size_t& len)
{
	if(len == 0) return;
	
	for ( this->current_conn_it = this->conns.begin() ; this->current_conn_it != this->conns.end(); this->current_conn_it++ )
	{
		if(this->current_conn_it->second) this->current_conn_it->second->add_to_out(addition, len);
	}
}*/

bool
User::is_valid()
{
	dbping();
	
	char* sql_query = new char[326];
	strcpy(sql_query,"");
	snprintf(sql_query, 321, "SELECT count(*), u.login FROM " TABLE " "
							 "LEFT JOIN pro_user u ON u.id = id_user "
							 "WHERE session='%s' AND id_user='%d'", this->session, this->id);
	
	if (mysql_real_query(&mysql, sql_query, strlen(sql_query)) != 0) {
		delete[] sql_query;
		mysql_free_result(mysql_result);
		fprintf(stderr,"Bad sql query, Error: %s\n", mysql_error(&mysql));
		return false;
	} else {
		mysql_result = mysql_store_result(&mysql);
		mysql_row = mysql_fetch_row(mysql_result);
		
	}
	
	std::cout << sql_query << std::endl;
	
	delete[] sql_query;
	mysql_free_result(mysql_result);
	if(mysql_row[1]) this->set_nickname(mysql_row[1]);
	return (atoi(mysql_row[0]) > 0);
}

void
User::add_connection(Connection* curC)
{
	
}

void
User::remove_conn()
{
	this->conns.erase(this->current_conn_it);
	delete this->current_conn_it->second;
}

/*
void
remove_user(User*& cl, User*& firstC, User*& lastC)
{
	User *nxt, *prv;

	if (cl == firstC) firstC = cl->next;
	if (cl == lastC) lastC = cl->prev;
	nxt = cl->next;
	prv = cl->prev;
	if (prv!=NULL) prv->next = nxt;
	if (nxt!=NULL) nxt->prev = prv;
	delete cl;
	std::cout << "============================|= Пользователь удален =|============================" << std::endl;
	cl = nxt;
}
*/
// ============================================== commands.cpp ==============================================================

enum type_json 
{
	JObject,
	JArray,
	JString,
	JInteger,
	JKeyValue,
	JUndefined
};

struct obj_val {
	const unsigned char* str_val;
	unsigned int str_len;
	unsigned int int_val;
};

class Object
{
	public:
		Object *next, *prev;
		bool is_openned;
		
		virtual	~Object() {}
		virtual type_json type() { return JUndefined; }
		virtual void* get() { return NULL; }
};

class JSONUndefined : public Object
{
	public:
		type_json type() { return JUndefined; }
};

class JSONKeyValue : public Object
{
	public:
		JSONKeyValue* right, *prev, *left;
		Object* pObj;
		unsigned char* key;
		
		JSONKeyValue()
		{
			right = prev = left = NULL;
			key = NULL;
			pObj = new JSONUndefined();
		}
		~JSONKeyValue()
		{
			if(key) delete[] key;
			if(pObj) delete pObj;
			
			if(right) delete right;
			if(left) delete left;
		}
		void set_key(const unsigned char* k, unsigned int len);
		void set_type(type_json tj, obj_val &ov);
		Object* find_key(char* kz, int lenz)
		{ 
			JSONKeyValue* finder = this;
			while(finder)
			{
				if(finder->key)
				{
					if(memcmp(finder->key, kz, lenz) == 0)
					{
						return finder->pObj;
					}
				}
				finder = finder->right;
			}
			return NULL;
		}
};

class Commands {
	public:
		Commands(void);
	   ~Commands(void);
		int parse();
		void set_user(User*, std::map<int,User*>*);
		void run(JSONKeyValue*);
		
		void get_pm_user_list();
		void restore_pm_message();
		void send_on_off(PMFriend*, bool, bool);
	private:
		char action[50];
		char to[100];
		User *firstU, *currentU;
		std::map<int,User*>* users;
		void find_friends(PMFriend*);
		void send_pm_users_list(PMFriend*);
		void send_templated(int, char*, char*, bool, User*);
		void send_pm_message(double*, char*);
		void template_pm_message(User*, int, char*, int, int *, char*);
		void store_pm_message(User*, int, char*, int, int *, char*);
};

class PMFriend {
	public:
		User* user;
		char* group_name;
		char* nick_name;
		//char* nick;
		int id;
		PMFriend* next;
		PMFriend()
		{
			this->next        = NULL;
			this->group_name  = NULL;
			this->nick_name   = NULL;
			this->user        = NULL;
			//this->nick          = NULL;
		}
	   ~PMFriend()
		{
			if(this->group_name) delete[] this->group_name;
			if(this->nick_name)  delete[] this->nick_name;
			//if(this->nick)       delete[] this->nick;
			if(this->next)       delete   this->next;
		}
		int get_id()
		{
			if(this->id) return this->id;
			else  return user->id;
		}
		char* get_nick() { return this->nick_name; }
		char* get_group() { return this->group_name; }
		void set_friend(char* nick, char* group, int uid)
		{
			this->group_name = new char[ strlen(group)+6 ];
			strcpy(this->group_name, group);
			this->nick_name  = new char[ strlen(nick)+6  ];
			strcpy(this->nick_name, nick);
			this->id = uid;
			//strcpy(this->nick, uid);
		}
};

Commands::Commands()
{

}

Commands::~Commands()
{
}

void
Commands::run(JSONKeyValue* jkv)
{
	Object* action;
	if(jkv->left)
	{
		action = jkv->left->find_key("action", 6);
		
		if(action)
		{
			if(action->type() != JString) return;
			
			if(strcmp((char*)action->get(), "send_pm_message") == 0)
			{
				double* to_id = NULL;
				if( action = jkv->left->find_key("to_id", 5) )
					if(action->type() == JInteger) to_id = (double*)action->get();
					
				if( action = jkv->left->find_key("message", 5) )
					this->send_pm_message(to_id, (char*)action->get());
			}
			else if(strcmp((char*)action->get(), "get_contact_list") == 0)
			{
				this->send_pm_users_list(currentU->my_friend_first);
			}
			else if(strcmp((char*)action->get(), "drop_caches_contact_list") == 0)
			{
				delete currentU->my_friend_first;
				currentU->my_friend_first = NULL;
				this->get_pm_user_list();
				
				//std::map<int,User*>::iterator currUit;
				//currUit = this->users->find( id );
				
				this->send_pm_users_list(currentU->my_friend_first);
			}
		}
	}
}

void
Commands::template_pm_message(User* sendU, int from_id, char* nick_name, int to_id, int* utime, char* message)
{
	size_t sz = 0;
	char dbuff [33];
	
	sz = 43;
	sendU->add_to_out("<script> pmesendger.add_message({'from_id':", sz);
	sprintf(dbuff,"%d", from_id);
	sz = strlen(dbuff);
	sendU->add_to_out(dbuff, sz);
	sz = 14;
	sendU->add_to_out(",'nick_name':'", sz);
	sz = strlen(nick_name);
	sendU->add_to_out(nick_name, sz);
	sz = 10;
	sendU->add_to_out("','to_id':", sz);
	sprintf(dbuff, "%d", to_id);
	sz = strlen(dbuff);
	sendU->add_to_out(dbuff, sz);
	
	sz = 9;
	sendU->add_to_out(",'utime':", sz);
	sprintf(dbuff,"%d", (*utime));
	sz = strlen(dbuff);
	sendU->add_to_out(dbuff, sz);
	
	sz = 12;
	sendU->add_to_out(",'message':'", sz);
	sz = strlen(message);
	sendU->add_to_out_escape(message, sz);
	sz = 13;
	sendU->add_to_out("'});</script>", sz);
	sendU->send();
}

void
Commands::restore_pm_message()
{
	dbping();
	char* sql_query = new char[316];
	strcpy(sql_query,"");
	int utime;
	
	snprintf(sql_query, 311, "SELECT * FROM " TABLE_PM_OM " WHERE from_id='%d' LIMIT 300", currentU->get_id());
	
	std::cout << "SQL: "<< sql_query << std::endl;
	
	if(mysql_real_query(&mysql, sql_query, strlen(sql_query)) != 0)
	{
		fprintf(stderr,"Bad sql query, Error: %s\n", mysql_error(&mysql));
	} else {
		mysql_result = mysql_store_result(&mysql);
		while(mysql_row = mysql_fetch_row(mysql_result))
		{
			utime = atoi(mysql_row[4]);
			this->template_pm_message(currentU, atoi(mysql_row[2]), mysql_row[3], atoi(mysql_row[1]), &utime, mysql_row[5]);
		}
		if(mysql_result)
		{
			mysql_free_result(mysql_result);
		
			snprintf(sql_query, 311, "DELETE FROM " TABLE_PM_OM " WHERE from_id='%d'", currentU->get_id());
			mysql_real_query(&mysql, sql_query, strlen(sql_query));
		}
	}
	delete[] sql_query;
}

void
Commands::store_pm_message(User* sendU, int from_id, char* nick_name, int to_id, int* utime, char* message)
{
	dbping();
	size_t sz = strlen(nick_name) + strlen(message) + 316;
	char* sql_query = new char[sz];
	strcpy(sql_query,"");
	
	snprintf(sql_query, sz, "INSERT INTO " TABLE_PM_OM "(nick,to_id,from_id,utime,message,read_state) VALUES ('%s','%d','%d','%d','%s',0)",
							 nick_name, to_id, from_id, *utime, message);
	
	std::cout << "SQL: "<< sql_query << std::endl;
	
	if(mysql_real_query(&mysql, sql_query, strlen(sql_query)) != 0)
	{
		fprintf(stderr,"Bad sql query, Error: %s\n", mysql_error(&mysql));
	}
	delete[] sql_query;
}

void
Commands::send_pm_message(double* to_id, char* msg)
{
	if(to_id && msg)
	{
		std::map<int,User*>::iterator currUit;
		int id = (int)*to_id;
		int currT = (int)time(NULL);
		std::cout << "msg_time: " << currT << std::endl;
		
		currUit = this->users->find( id );
		if(currUit != this->users->end())
		{
			//this->template_pm_message(currentU, currentU->get_id(), currentU->get_nickname(), id, &currT, msg);
			this->template_pm_message(currentU, 0, currentU->get_nickname(), id, &currT, msg);
			this->template_pm_message(currUit->second, id, currentU->get_nickname(), (currentU->get_id()), &currT, msg);
		}
		else {
			//this->template_pm_message(currentU, currentU->get_id(), currentU->get_nickname(), id, &currT, msg);
			this->template_pm_message(currentU, 0, currentU->get_nickname(), id, &currT, msg);
			this->store_pm_message(currUit->second, id, currentU->get_nickname(), (currentU->get_id()), &currT, msg);
		}
	}
	//"<script> pmesendger.add_message({'from_id':%d,'nick_name':'%s','to_id':%d, 'utime':%d,'message':'%s'});</script>"
}

void
Commands::find_friends(PMFriend* firstPMF)
{
	std::map<int,User*>::iterator it;
	PMFriend*  finderPMF = firstPMF;
	
	finderPMF = firstPMF;
	//std::cout << "finder for: " << currentU->get_nickname() << std::endl;
	while(finderPMF)
	{
		if(finderPMF->group_name)
		{
			it = users->find( finderPMF->get_id() );
			if( it != users->end() )
			{
				finderPMF->user = it->second;
				//std::cout << "--------*--------: " << finderPMF->get_nick() << std::endl;
			} else {
				finderPMF->user = NULL;
				//std::cout << "-----------------: " << finderPMF->get_nick() << std::endl;
			}
		}
		finderPMF = finderPMF->next;
	}
}

void
Commands::send_pm_users_list(PMFriend* firstPMF)
{
	PMFriend*  finderPMF = firstPMF;
	
	while(finderPMF)
	{
		if(finderPMF->group_name)
		{
			this->send_templated(finderPMF->get_id(), finderPMF->get_nick(), finderPMF->get_group(), (finderPMF->user ? true : false), this->currentU);
		}
		finderPMF = finderPMF->next;
	}
}

void
Commands::send_templated(int id, char* n, char* g, bool is_o, User* cU)
{
	size_t sz = 0;
	char buff[370];
	//char ubuff[370];
	
	sprintf(buff, "<script> pmesendger.add({nick_name:'%s', uid: %d, is_online: %s, group_name: '%s'});</script>", 
					n, id, (is_o ? "true" : "false"), g);
	sz = strlen(buff);
	cU->add_to_out(buff, sz);
	cU->send();
}

void
User::fix_pm_list_shm(User* fixU, bool is_my, bool is_void)
{
	PMFriend*  finderPMF;
	if(is_my) finderPMF = this->my_friend_first;
	else finderPMF = this->me_friend_first;
	
	while(finderPMF)
	{
		if(finderPMF->group_name)
		{
			if(finderPMF->get_id() == fixU->id)
			{
				std::cout << "## fix "<<(is_my?"my":"me")<<" list:" << this->get_nickname() << " chto: " << finderPMF->get_nick() << (is_void ? " is offline" : " is online") << std::endl;
				if(is_void) finderPMF->user = NULL;
				else finderPMF->user = fixU;
				break;
			}
		}
		finderPMF = finderPMF->next;
	}
}

void
Commands::send_on_off(PMFriend* firstPMF, bool is_my, bool is_online)
{
	PMFriend*  finderPMF = firstPMF;
	while(finderPMF)
	{
		if(finderPMF->user)
		{
			if(is_my) this->send_templated(this->currentU->get_id(), this->currentU->get_nickname(), finderPMF->get_group(), is_online, finderPMF->user);
			finderPMF->user->fix_pm_list_shm(this->currentU, is_my, (!is_online));
		}
		finderPMF = finderPMF->next;
	}
}

void
Commands::get_pm_user_list()
{
	dbping();

	char* sql_query = new char[336];
	PMFriend* pMyF;
	
	if(currentU->my_friend_first) ;// this->send_pm_users_list(currentU->my_friend_first);
	else 
	{
		strcpy(sql_query,"");
		snprintf(sql_query, 332, "SELECT u.login, u.id, u.id, pcl.group FROM " TABLE_PM_CL " pcl "
										 "LEFT JOIN " TABLE_USER " u ON pcl.contact_id = u.id "
										 //"LEFT JOIN " TABLE " acs ON pcl.contact_id = acs.id_user "
								 "WHERE pcl.user_id = '%d' and u.id is not null LIMIT 350", currentU->id);
		
		std::cout << sql_query << std::endl;
		 
		if(mysql_real_query(&mysql, sql_query, strlen(sql_query)) != 0)
		{
			fprintf(stderr,"Bad sql query, Error: %s\n", mysql_error(&mysql));
		}
		else {
			
			mysql_result = mysql_store_result(&mysql);
			currentU->my_friend_first = new PMFriend();
			pMyF = currentU->my_friend_first;
			
			while(mysql_row = mysql_fetch_row(mysql_result))
			{
				pMyF->set_friend(mysql_row[0], mysql_row[3], atoi(mysql_row[2]));
				pMyF->next = new PMFriend();
				pMyF = pMyF->next;
			}
			
			this->find_friends(currentU->my_friend_first);
			//this->send_pm_users_list(currentU->my_friend_first);
			//delete currentU->my_friend_first;
		}
		if(mysql_result) mysql_free_result(mysql_result);
	}
	
	if(currentU->me_friend_first) ;//this->send_pm_users_list(currentU->my_friend_first);
	else
	{
		strcpy(sql_query,"");
		snprintf(sql_query, 332, "SELECT u.login, u.id, pcl.group FROM " TABLE_PM_CL " pcl "
										 "LEFT JOIN " TABLE_USER " u ON pcl.user_id = u.id "
										 //"LEFT JOIN " TABLE " acs ON pcl.contact_id = acs.id_user "
								 "WHERE pcl.contact_id = '%d' and u.id is not null LIMIT 350", currentU->id);
		
		std::cout << sql_query << std::endl;
		
		if(mysql_real_query(&mysql, sql_query, strlen(sql_query)) != 0)
		{
			fprintf(stderr,"Bad sql query, Error: %s\n", mysql_error(&mysql));
		}
		else {
			mysql_result = mysql_store_result(&mysql);
			currentU->me_friend_first = new PMFriend();
			pMyF = currentU->me_friend_first;
			
			while(mysql_row = mysql_fetch_row(mysql_result))
			{
				pMyF->set_friend(mysql_row[0], mysql_row[2], atoi(mysql_row[1]));
				pMyF->next = new PMFriend();
				pMyF = pMyF->next;
			}
			
			this->find_friends(currentU->me_friend_first);
			this->send_on_off(currentU->me_friend_first, true,  true);
			this->send_on_off(currentU->my_friend_first, false, true);
			//delete currentU->me_friend_first;
		}
		if(mysql_result) mysql_free_result(mysql_result);
	}
	delete[] sql_query;
}

void
Commands::set_user(User* user, std::map<int, User*> *u)
{
	this->currentU = user;
	//this->firstU = first_user;
	this->users = u;
}


extern void json_parse(char*, JSONKeyValue*);

int
Commands::parse()
{
	JSONKeyValue* jkv = new JSONKeyValue();
	json_parse(this->currentU->cC->inc_buffer, jkv);
	this->run(jkv);
	delete jkv;
	
	this->currentU->cC->inc_buffer[0] = 0;
	this->currentU->cC->inc_buffer_cur = this->currentU->cC->inc_buffer;
}


// ============================================== main.cpp ==================================================================
class Server {
	public:
		int server_fd;
#ifdef SUPPORT_MOD_COMM	
		int mod_comm_serv, mod_comm_msgsocket;
		struct sockaddr_un mod_comm_client_addr;
		socklen_t sin_size;
#endif
		char query[256];
		Server(void);
		~Server(void);
		void binding(void);
		
		Connection *firstC, *lastC, *currentC;
		User *firstU, *lastU, *currentU;
		std::map<int,User*> users;
		std::map<int,User*>::iterator current_user_it;
		std::map<int,Connection*> conns;
		std::map<int,Connection*>::iterator current_conn_it;
		
		void epoll_loop(void);
		void dbconnect();
		static void user_clean_timer(int sign);
		static void killall_exit(int sign);
		Commands cmd;
	private:
		void set_nonblocking(int*);
		void header_pase(std::string&);
		void send(const char*, size_t);
};

#define BUF_LEN 2500
Server::Server()
{

}

Server::~Server()
{
	
}

void
Server::dbconnect()
{
	mysql_init(&mysql);
	if (&mysql == NULL) {
		fprintf(stderr, "Sorry, no enough memory to allocate mysql-connection\n");
	}
	
	if (!mysql_real_connect(&mysql, mysql_server, mysql_user, mysql_password, mysql_db, 0, NULL, 0)) {
		fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(&mysql));
		exit(1);
	}
	
	if (mysql_real_query(&mysql, "SET names 'utf8'", 17) != 0) {
		fprintf(stderr,"Bad sql query, Error: %s\n", mysql_error(&mysql));
	}
}

void
Server::binding()
{
	struct sockaddr_in serveraddr;
	
	memset(&serveraddr, 0, sizeof(struct sockaddr_in));
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
		fprintf(stderr, "socket failed\n");
		exit(1);
	}
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons((short)(PORT));
	serveraddr.sin_addr.s_addr = inet_addr(BIND_O);//INADDR_ANY;
	if (bind(server_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) != 0) {
		fprintf(stderr, "Port zanyat\n");
		exit(1);
	}
	if (listen(server_fd, STACK_LISTEN) != 0) {
		fprintf(stderr, "Listen failed\n");
		exit(1);
	}

#ifdef SUPPORT_MOD_COMM
	struct sockaddr_un mod_comm_unix_addr;
	bzero (&(mod_comm_unix_addr), sizeof (mod_comm_unix_addr));
	mod_comm_unix_addr.sun_family = AF_UNIX;
	strcpy (mod_comm_unix_addr.sun_path, modcomm_socket);
	unlink(mod_comm_unix_addr.sun_path);

	
	if ((mod_comm_serv = socket (AF_UNIX, SOCK_STREAM, 0)) == -1) {
	        printf ("problem: cannot create socket, EXITING. Other copy of daemon is started?\n");
	        (void) ::close(server_fd);
	        exit(1);
	}
	if (bind(mod_comm_serv, (struct sockaddr *) &mod_comm_unix_addr, sizeof (struct sockaddr)) == -1) {
	   printf("problem: Cannot bind to a given unix sock.\n");        
	        (void) ::close(server_fd);
	        (void) ::close (mod_comm_serv);
	        exit(1);
	}
	chmod(mod_comm_unix_addr.sun_path, 0777);
	if (listen(mod_comm_serv, STACK_LISTEN) == -1) {
	        printf("Problem: cannot listen. EXITING. Other copy of daemon is started?\n");
	        (void) ::close(server_fd);
	        (void) ::close(mod_comm_serv);
	        exit(1);
	}
#endif

}

void
Server::set_nonblocking(int* fd)
{
	int opts;
	opts=fcntl(*fd, F_GETFL);
	if (opts < 0) {
		perror("fcntl failed\n");
		return;
	}
	opts = opts | O_NONBLOCK;
	if(fcntl(*fd, F_SETFL, opts) < 0) {
		perror("fcntl failed\n");
		return;
	}
}

void
Server::header_pase(std::string& buffer)
{

}

void
Server::send(const char* addition, size_t len)
{
	for( this->current_user_it = this->users.begin() ; this->current_user_it != this->users.end(); this->current_user_it++ )
	{
		this->current_user_it->second->add_to_out(addition, len);
		this->current_user_it->second->send();
	}
}


void
Server::epoll_loop()
{
	int i, ret, client_fd, num_fd;
	struct epoll_event  events[MAXEVENTS];
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	//char* buf = new char[BUF_LEN];
	//unsigned char plstr[] = "<pooled>";
	//unsigned char plstr_e[] = "</pooled>";
	char* buf_pool;
	size_t len_buf_pool;
	size_t poolend_sz;
	Pool pool_msg;
	int iter = 0;
	
	//char* a, * b, * b_start;
	//bool del_itr = false;

	
	// create epoll
	epoll_fd = epoll_create(100);
	ev.events = EPOLLOUT | EPOLLIN | EPOLLET;
	ev.data.fd = server_fd;
	// add server socket to epoll
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

#ifdef SUPPORT_MOD_COMM	
	// add unix socket for mod_communicator
	ev.events = EPOLLIN | EPOLLET | EPOLLHUP;
	ev.data.fd = mod_comm_serv;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, mod_comm_serv, &ev);
#endif
	
	while (1)
	{
		num_fd = epoll_wait(epoll_fd, events, MAXEVENTS, -1);
		
		for (i=0; i<num_fd; ++i)
		{
//-----------------------------------------------------------------------------------------------------------------------------------------
//																MOD COMMUNICATOR
//-----------------------------------------------------------------------------------------------------------------------------------------
#ifdef SUPPORT_MOD_COMM	
			if(events[i].data.fd == mod_comm_serv)
			{
				sin_size = sizeof(struct sockaddr_un);
				mod_comm_msgsocket = accept(mod_comm_serv, (struct sockaddr *) &mod_comm_client_addr, &sin_size);
				if(mod_comm_msgsocket < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
				
				client_fd = recv_fd(mod_comm_msgsocket, query);
				::close(mod_comm_msgsocket);
				
				if (client_fd > 0)
				{
					set_nonblocking(&client_fd);
					
					ev.data.fd = client_fd;
					ev.events = EPOLLIN | EPOLLET;
					epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
					

					currentC = new Connection(client_fd);
////////////////////currentC->socket_buffer.append(query);
					strcpy(currentC->inc_buffer, query);
					if(zi_str4cmp(query, 'G', 'E', 'T', ' '))
					{
						currentC->method = METHOD_GET;
						//------------------------------------------------------------
						ev.data.fd = currentC->user_socket_fd;
						ev.events = EPOLLOUT | EPOLLET;
						epoll_ctl(epoll_fd, EPOLL_CTL_MOD, currentC->user_socket_fd, &ev);
						//------------------------------------------------------------
					}
					if(currentC->parse_header(currentC->inc_buffer + 256)) //////////////////////////////////
					{
						// --- Создаем список соединений --- //
						if (lastC == NULL) { 
							firstC = currentC;
							lastC = currentC;
							currentC->next = NULL;
							currentC->prev = NULL;
						} else { 
							lastC->next = currentC;
							currentC->next = NULL;
							currentC->prev = lastC;
							lastC = currentC;
						}
						
						
						if(currentC->method == METHOD_GET)
						{
							ev.data.fd = currentC->user_socket_fd;
							ev.events = EPOLLOUT | EPOLLET;
							epoll_ctl(epoll_fd, EPOLL_CTL_MOD, currentC->user_socket_fd, &ev);
						} 
						
						
						// -------------- Ищем пользователя в системе, с таким же ником, и индификатором сессии ----------- //
						currentU = firstU;
						while(currentU != NULL) {
							if( strcmp(currentU->session, currentC->session) == 0 ) if( currentU->id == currentC->id ) break;
							currentU = currentU->next;
						}
						
						// ------------------------- Если не найден, регистрируем нового -------------------------- //
						if(currentU == NULL)
						{
							currentU = new User(currentC->session, currentC->id);
							if( !currentU->is_valid() ) {
								delete currentU;
								remove_conn(currentC, firstC, lastC, true);
								continue;
							}
							if (lastU == NULL) { 
								firstU = currentU;
								lastU = currentU;
								currentU->next = NULL;
								currentU->prev = NULL;
							} else { 
								lastU->next = currentU;
								currentU->next = NULL;
								currentU->prev = lastU;
								lastU = currentU;
							}
						}
						
						currentU->add_connection(currentC);
						remove_conn(currentC, firstC, lastC, false); // Убрать соединение из не зарегестрированных в системе
					
					} else delete currentC;
				}
			}
			else
#endif 
//-----------------------------------------------------------------------------------------------------------------------------------------
			if(events[i].data.fd == server_fd)
			{
				client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &len);
				
				if(client_fd < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
				set_nonblocking(&client_fd);
				
				ev.data.fd = client_fd;
				ev.events = EPOLLIN | EPOLLET;
				epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
				
				conns[client_fd] = new Connection(client_fd);
			}
			else 
			{
				client_fd = events[i].data.fd;
				
//				currentU = firstU;
				currentC = NULL;
				for( this->current_user_it = this->users.begin() ; this->current_user_it != this->users.end(); this->current_user_it++ )
				{
					currentU = this->current_user_it->second;
					currentU->current_conn_it = currentU->conns.find(client_fd);

					if(currentU->current_conn_it != currentU->conns.end())
					{
						currentC = currentU->current_conn_it->second;
						currentU->cC = currentC;
						break;
					}
				}
			
				if( this->current_user_it == this->users.end() )
				{
					currentU = NULL;
					current_conn_it = conns.find(client_fd);
					if(current_conn_it != conns.end()) currentC = current_conn_it->second;
				}
				
				// --------------------------------------------------------------------------------------------
			  
				if (events[i].events & (EPOLLERR | EPOLLHUP))
				{
					if(currentU) currentU->remove_conn();
					else {
						conns.erase(current_conn_it);
						delete currentC;
					}
					std::cout << "ERROR" << std::endl;
				}
				else if(events[i].events & EPOLLIN)
				{
					ret = currentC->fill_buffer(); // Разбор входящего подключения
					
					if((currentC->method == CONNECTED) && currentU)
					{
						if( ((*(currentC->inc_buffer_cur-1)) == '>') && ((*(currentC->inc_buffer_cur-2)) == '>') )
						{
							*(currentC->inc_buffer_cur-2) = 0;
							cmd.set_user(currentU, &users);
							cmd.parse();
							continue;
						}
					}
					
					if( ret == 1 )                  //++ Закончить обработку соединения
					{
						if(currentC->method == METHOD_POST)
						{
							if(strlen(currentC->to) == 0)
							{
								len_buf_pool = strlen(currentC->inc_buffer);
								//len_buf_pool = (currentC->inc_buffer_cur - currentC->inc_buffer);
								if(currentC->post_parsed == true)
								{
									this->send(currentC->inc_buffer, len_buf_pool);
									//currentC->inc_buffer_cur = currentC->inc_buffer;
									//*currentC->inc_buffer = 0;
								}
								if(currentC->add_pool) pool_msg.set(currentC->inc_buffer, len_buf_pool);
							}
							else 
							{
								current_user_it = users.find( atoi(currentC->to) );
								if(current_user_it != users.end()) currentU = current_user_it->second;
								else currentU = NULL;
						
								if(currentU)
								{
									len_buf_pool = strlen(currentC->inc_buffer);
									//len_buf_pool = (currentC->inc_buffer_cur - currentC->inc_buffer);
									currentU->add_to_out(currentC->inc_buffer, len_buf_pool);
									currentU->send();
									//currentC->inc_buffer_cur = currentC->inc_buffer;
									//*currentC->inc_buffer = 0;
								}
							}
						}

						// Удалить соединение 
						if(currentU) currentU->remove_conn();
						else {
							conns.erase(current_conn_it);
							delete currentC;
						}
					}
					else if( ret == -1 ) continue;  //++ Обработка событий EPOLL
					else if( ret ==  0 )            //++ Продолжить обработку соединения
					{
						// ------------------------- Ищем пользователя в системе, с таким же ником, и индификатором сессии -------------------------- //
						currentU = NULL;
						for(this->current_user_it = this->users.begin() ; this->current_user_it != this->users.end(); this->current_user_it++)
						{
							/*if( strcmp(this->current_user_it->second->session, currentC->session) == 0 ) */
							if(this->current_user_it->second->get_id() == currentC->id )
							{
								currentU = this->current_user_it->second;
								break;
							}
						}

						// ------------------------- Если не найден, регистрируем нового -------------------------- //
						if(!currentU)
						{
							currentU = new User(currentC->session, currentC->id);
							
							if( !currentU->is_valid() )
							{
								delete currentU;
								conns.erase(current_conn_it);
								delete current_conn_it->second;
								continue;
							}
							else users[currentC->id] = currentU;
						}

						currentU->conns[client_fd] = currentC;
						conns.erase(current_conn_it);
						
						// Убрать соединение из не зарегестрированных в системе
					}
				}
				else if(events[i].events & EPOLLOUT)
				{

					if(currentC->method == METHOD_GET)
					{
						currentC->method = CONNECTED;
						
						poolend_sz = 8;
						currentC->out_buffer_end = currentC->out_buffer;
						currentC->add_to_out("<pooled>", poolend_sz);
						//currentC->add_to_out(plstr, poolend_sz);
						currentC->send();
						
						for(iter=0; iter<10; iter++)
						{
							buf_pool = pool_msg.get();
							//if(buf_pool == NULL) break;
							len_buf_pool = strlen(buf_pool);
							currentC->add_to_out(buf_pool, len_buf_pool); // TODO: FIx THIS
							currentC->send();
						}
						
						poolend_sz = 9;
						currentC->add_to_out("</pooled>", poolend_sz);
						//currentC->add_to_out(plstr_e, poolend_sz);
						currentC->send();
						
						if(currentU)
						{
							cmd.set_user(currentU, &users);
							if(!currentU->my_friend_first) cmd.restore_pm_message();
							cmd.get_pm_user_list();
						}
					} 
					else if(currentU) currentU->send();
					
					currentC->inc_buffer[0] = 0;
					currentC->inc_buffer_cur = currentC->inc_buffer;
					
				} else if(events[i].events & EPOLLPRI) {
					std::cout << "EPOLLPRI" << std::endl;
				} else if(events[i].events & EPOLLET) {
					std::cout << "EPOLLET" << std::endl;
				}
			
			}
		}
	}
}

bool
Connection::parse_post_command(char* Buf)
{
	char http_header[] = "HTTP/1.1 200 OK\r\nServer: ZioN StreamServer v0.1\r\n\r\n Ok";
	::send(this->user_socket_fd, &http_header, strlen(http_header), 0);
	
	char* startB, * endB, * currB;
	size_t len_buf_pool;
	startB = Buf;
	
	for(endB = startB + strlen(Buf); startB < endB; ++startB) 
	{
		if( strncmp(startB, "msg=", 4) == 0 )
		{
			startB = startB + 4;
			
			currB = startB;
			while((*startB != '&'))
			{
				if(*startB == '\0') break;
				startB++;
			}
			
			//Buf = new char[startB - currB]; ???????????????????
			url_ndecode(currB, this->inc_buffer, (startB - currB));
			this->inc_buffer[(int)(startB - currB)] = 0;
			this->post_parsed = true;
		}
		
		if( strncmp(startB, "pul=", 4) == 0 )
		{
			startB = startB + 4;
			if(*startB == '1') this->add_pool = true;
		}
		
		if( strncmp(startB, "to=", 3) == 0 )
		{
			startB = startB + 3;
			currB = startB;
			while((*startB != '&'))
			{
				if(*startB == '\0') break;
				startB++;
			}
			url_ndecode(currB, this->to, (startB - currB)+1);
		}
	}

}

bool
Connection::parse_header(char* a)
{
	char* buf_cur = this->inc_buffer;
	char* buf_cur_new_line = this->inc_buffer;
	char* len;
	char* pos;
	
	for(; buf_cur < a; buf_cur++)
		if(*buf_cur == CR)
		{
			if(this->method == METHOD_GET)
			{
				if(zi_str4cmp(buf_cur_new_line, 'G', 'E', 'T', ' '))
				{
					char dst[3 * NICK_LEN];
					if((buf_cur_new_line + 13) > buf_cur) break;
					pos = buf_cur_new_line + 10;
					for(; pos < buf_cur; pos++)
						if(*pos == '/')
							if((pos - buf_cur_new_line + 10) > (3 * NICK_LEN)) break;
							else
							{ 
								url_ndecode((buf_cur_new_line + 10), dst, (int)(pos - (buf_cur_new_line + 10)) + 1);
								this->id = atoi(dst);
								buf_cur_new_line = pos + 1;
								pos++;
								for(; pos < buf_cur; pos++)
									if(*pos == '/')
									{
										if((int)(pos - buf_cur_new_line) != SESSION_LEN) break;
										else
										{
											strcpy(this->nickname, dst);
											strncpy(this->session, buf_cur_new_line, 32);
											this->session[32] = 0;
											return true;
										}
									} else if(ISXDIGIT(*pos) == 0) return false;
							}
				}
			}
			else if(this->method == METHOD_POST)
			{
				if(strncmp(buf_cur_new_line, "Content-Length: ", 16) == 0)
				{
					len = new char[20];
					strcpy(len, "");
					strncat (len, (buf_cur_new_line + 16), (buf_cur - buf_cur_new_line - 16));
					this->content_len = atoi(len);
					delete[] len;
					return true;
				}
			}
			buf_cur_new_line = buf_cur + 2;
		}
	return false;
}

int
Connection::fill_buffer()
{
	//char* buf = new char[BUF_LEN];
	char* a = this->inc_buffer_cur;
	int ret;
	
	while(1)
	{
		ret = recv(user_socket_fd, bufz, BUF_LEN, 0);
		if(ret == 0)
		{
			return 1;
			break;
		}
		else if(ret == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				return -1;
			} else {
				return 1;
			}
		}
		
		if(this->inc_buffer_cur + ret > this->inc_buffer_end) { std::cout << std::endl << " == Oops == " << std::endl; return 1; }
		
		if(this->method == CONNECTION)
		{
			if(zi_str4cmp(bufz, 'G', 'E', 'T', ' '))
			{
				this->method = METHOD_GET;
				//------------------------------------------------------------//
				ev.data.fd = user_socket_fd;							      //
				ev.events = EPOLLOUT | EPOLLET | EPOLLIN ;					  //
				epoll_ctl(epoll_fd, EPOLL_CTL_MOD, this->user_socket_fd, &ev);//
				//------------------------------------------------------------//
			}
			else if(zi_str4cmp(bufz, 'P', 'O', 'S', 'T'))
			{
				this->method = METHOD_POST;
			}
			else if(strncmp("<policy-file-request/>\x00", bufz, 23) == 0)
			{
				this->method = CROSSDOMAIN_SEND;
				//strcpy(this->out_buffer,
				size_t sz = 351;
				this->add_to_out(
					   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
					   "\t<cross-domain-policy xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://www.adobe.com/xml/schemas/PolicyFileSocket.xsd\">\n"
					   "\t<allow-access-from domain=\"*\" to-ports=\"*\" secure=\"false\" />\n"
					   "\t<site-control permitted-cross-domain-policies=\"master-only\" />\n"
					   "</cross-domain-policy>\x00", sz);
				//this->out_buffer_end = this->out_buffer + 351;
				//------------------------------------------------------------//
				ev.data.fd = user_socket_fd;								  //
				ev.events = EPOLLOUT | EPOLLET ;							  //
				epoll_ctl(epoll_fd, EPOLL_CTL_MOD, this->user_socket_fd, &ev);//
				//------------------------------------------------------------//
				this->send();
				return 1;
			}
			else {
				return 1;
			}
		}
		
		strncat(this->inc_buffer, bufz, ret);
		this->inc_buffer_cur = (this->inc_buffer_cur + ret);
		
		if(this->method == METHOD_GET || this->method == METHOD_POST)
		{
			for(; a < this->inc_buffer_cur + ret; a++)
				if(zi_str4cmp(a, CR, LF, CR, LF))
				{
					a += 2;
					if(this->content_len == -1){
						this->parse_header(a);
						this->content = a;
						if(this->method == METHOD_GET)
						{
							return 0;
						}
						break;
					}
				}
				
			if(this->content_len > 0) this->content_len -= ((int)(this->inc_buffer_cur - a) >= 2) ? (int)(this->inc_buffer_cur - a) : ret;
			if(this->method == METHOD_POST && this->content_len == -2)
			{
				this->parse_post_command(this->content+2);
				return 1;
			}
		}
		
	}
	return -1;
}

void
Server::user_clean_timer(int sig)
{
	for( srv->current_user_it = srv->users.begin() ; srv->current_user_it != srv->users.end(); srv->current_user_it++ )
	{
		if(srv->current_user_it->second->conns.size() == 0)
		{
			srv->cmd.set_user(srv->current_user_it->second, &(srv->users));
			srv->cmd.send_on_off(srv->current_user_it->second->me_friend_first, true, false);
			srv->cmd.send_on_off(srv->current_user_it->second->my_friend_first, false, false);
			delete srv->current_user_it->second;
			srv->users.erase(srv->current_user_it);
		}
	}
}

//std::map<int,Connection*> conns;
//std::map<int,Connection*>::iterator current_conn_it;
void
killall_exit(int signo)
{
    for( srv->current_user_it = srv->users.begin() ; srv->current_user_it != srv->users.end(); srv->current_user_it++ )
    {
        delete srv->current_user_it->second;
        srv->users.erase(srv->current_user_it);
    }
    
    for( srv->current_conn_it = srv->conns.begin() ; srv->current_conn_it != srv->conns.end(); srv->current_conn_it++ )
    {
	delete srv->current_conn_it->second;
	srv->conns.erase(srv->current_conn_it);
    }
    ::close(srv->server_fd);
    std::cout << std::endl << "SIGM Faul" << std::endl;
    exit(1);
}


int main()
{
	//daemonize();
	bufz = new char[BUF_LEN+6];
	srv = new Server();
	srv->binding();
	srv->dbconnect();
	
	struct sigaction alarm_act;
	struct sigaction suka;
	struct itimerval alarm_timer;
	
	bzero(&alarm_act, sizeof(struct sigaction));
	alarm_act.sa_handler=&srv->user_clean_timer;
	sigaction(SIGALRM, &alarm_act, NULL);
	
	suka.sa_handler = killall_exit;
//	suka.sa_flags = SA_SIGSEV;
        if(sigaction(SIGSEGV, &suka, NULL)) std::cout << std::endl << "Ok" << std::endl;
	
	alarm_timer.it_interval.tv_sec=10;
	alarm_timer.it_interval.tv_usec=0;
	alarm_timer.it_value.tv_sec=10;
	alarm_timer.it_value.tv_usec=0;
	
	if (setitimer(ITIMER_REAL, &alarm_timer, NULL))
	{
		fprintf (stderr, "Cannot start alarm timer - %s\n", strerror(errno));
		return -1;
	} 
	
	
	srv->epoll_loop();
	delete[] bufz;
	return 0;
}

