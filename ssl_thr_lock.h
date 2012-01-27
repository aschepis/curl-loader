/* 
*   ssl_thr_lock.h
*
*   Author: Jeremy Brown <jbrown_at_novell.com> 
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
  Style changes by Robert Iakobashvili <coroberti@gmail.com>.
*/

#ifndef SSL_THR_LOCK_H
#define SSL_THR_LOCK_H

#include <openssl/crypto.h>

void locking_function (int mode, int n, const char * file, int line);
unsigned long id_function(void);

int thread_openssl_setup(void);
int thread_openssl_cleanup(void);


#endif /* SSL_THR_LOCK_H */
