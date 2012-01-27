/*
*     timer_node.h
*
* 2006-2007 Copyright (c) 
* Robert Iakobashvili, <coroberti@gmail.com>
* All rights reserved.
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

#ifndef TIMER_NODE_H
#define TIMER_NODE_H

/* forward declaration */
struct timer_node;

/* Prototype of the function to be called on timer expiration.*/
typedef int (*handle_timer) (struct timer_node*, void*, unsigned long);

/*
  struct timer_node - 
  to be use in timer_queue as the user data structure.
 */
typedef struct timer_node
{
  /* The next timer shot in msec since the epoch (Jan 1 1970) */
  unsigned long next_timer;

  /* Interval in msec between periodic timer shots. Zero for non-periodic timer. */
  unsigned long period;
  
  /* Function to be called on timer expiration. Trying to be Object Oriented ...*/
  handle_timer func_timer;

  long timer_id;
} timer_node;


#endif /*  TIMER_NODE_H */
