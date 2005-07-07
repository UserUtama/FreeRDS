/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   xrdp: A Remote Desktop Protocol server.
   Copyright (C) Jay Sorg 2004-2005

   generic operating system calls

*/

#if !defined(OS_CALLS_H)
#define OS_CALLS_H

int g_init_system(void);
int g_exit_system(void);
void
g_printf(char *format, ...);
void
g_sprintf(char* dest, char* format, ...);
void
g_hexdump(char* p, int len);
void*
g_malloc(int size, int zero);
void
g_free(void* ptr);
void
g_memset(void* ptr, int val, int size);
void
g_memcpy(void* d_ptr, const void* s_ptr, int size);
int
g_getchar(void);
int
g_tcp_set_no_delay(int sck);
int
g_tcp_socket(void);
int
g_tcp_local_socket(void);
void
g_tcp_close(int sck);
int
g_tcp_connect(int sck, char* address, char* port);
int g_tcp_force_send(int sck, char* data, int len);
int g_tcp_force_recv(int sck, char* data, int len);
int g_tcp_set_non_blocking(int sck);
int g_tcp_bind(int sck, char* port);
int g_tcp_local_bind(int sck, char* port);
int g_tcp_listen(int sck);
int g_tcp_accept(int sck);
int g_tcp_recv(int sck, void* ptr, int len, int flags);
int g_tcp_send(int sck, void* ptr, int len, int flags);
int g_tcp_last_error_would_block(int sck);
int g_tcp_select(int sck1, int sck2);
void g_sleep(int msecs);
void g_random(char* data, int len);
int g_abs(int i);
int g_memcmp(void* s1, void* s2, int len);
int g_file_open(char* file_name);
int g_file_close(int fd);
int g_file_read(int fd, char* ptr, int len);
int g_file_write(int fd, char* ptr, int len);
int g_file_seek(int fd, int offset);
int g_file_lock(int fd, int start, int len);
int g_strlen(char* text);
char* g_strcpy(char* dest, char* src);
char* g_strncpy(char* dest, char* src, int len);
char* g_strcat(char* dest, char* src);
char* g_strdup(char* in);
int g_strcmp(char* c1, char* c2);
int g_strncmp(char* c1, char* c2, int len);
long g_load_library(char* in);
int g_free_library(long lib);
void* g_get_proc_address(long lib, char* name);
int g_system(char* aexec);
void g_signal(int sig_num, void (*func)(int));

#endif