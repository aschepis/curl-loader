/*
*     batch.c
*
* 2007 Copyright (C) 
* Robert Iakobashvili, <coroberti@gmail.com>
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

// must be the first include
#include "fdsetsize.h"

#include "batch.h"

int is_batch_group_leader (batch_context* bctx)
{
  return !bctx->batch_id;
}

size_t next_ipv4_shared_index (batch_context* bctx)
{
  size_t to_return = bctx->ipv4_shared_index;

  if (++bctx->ipv4_shared_index == (size_t)bctx->ip_shared_num)
    bctx->ipv4_shared_index = 0;

  return to_return;
}
size_t next_ipv6_shared_index (batch_context* bctx)
{
  size_t to_return = bctx->ipv6_shared_index;

  if (++bctx->ipv6_shared_index == (size_t)bctx->ip_shared_num)
    bctx->ipv6_shared_index = 0;

  return to_return;
}
