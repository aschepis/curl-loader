/*
*     screen.c
*
* 2007 Copyright (c) 
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

// must be first include
#include "fdsetsize.h"

#include <errno.h>
#include <unistd.h>
#include <sys/select.h>

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "screen.h"
#include "batch.h"
#include "client.h"
#include "loader.h"

//static int test_fd_readable (int fd);
static int on_keybord_input (int key, batch_context* bctx);


static struct termios start_tty, run_tty;

void screen_init ()
{
    /* Initializes curses library */
  //initscr();
  /* Improves performances */
  //noecho();
  //fprintf(stderr, "\033[2J");

  /* disable echoing keyboard input */
  tcgetattr(STDIN_FILENO, &start_tty);
  run_tty = start_tty;
  run_tty.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &run_tty);
}

void screen_release ()
{
  /* re-enable echoing */
  tcsetattr(STDIN_FILENO, TCSANOW, &start_tty);
}

/*
 *  getch()  --  a non-blocking single character input from stdin
 *
 *  Returns a character, or -1 if an input error occurs.
 *
 *  Conditionals allow compiling with or without echoing of
 *  the input characters, and with or without flushing
 *  pre-existing existing  buffered input before blocking.
 *
 */
int
getch(void)
{
  struct termios otty, ntty;
  int ch = -1;

  tcgetattr(STDIN_FILENO, &otty);
  ntty = otty;

  ntty.c_lflag &= ~ICANON;   /* line settings    */

#if 1
  /* disable echoing the char as it is typed */
  ntty.c_lflag &= ~ECHO;     /* disable echo         */
#else
  /* enable echoing the char as it is typed */
  ntty.c_lflag |=  ECHO;     /* enable echo      */
#endif

  ntty.c_cc[VMIN]  = 0;      /* non-block for input  */
  ntty.c_cc[VTIME] = 1;      /* with timer*/

#if 0
/*
 * use this to flush the input buffer before
 * blocking for new input
 */
#define FLAG TCSAFLUSH
#else
/*
 * use this to return a char from the current input
 *  buffer, or block if no input is waiting.
 */
#define FLAG TCSANOW

#endif

  if (!tcsetattr(STDIN_FILENO, FLAG, &ntty)) 
   {
     /* get a single character from stdin */
     if (read (STDIN_FILENO, &ch, 1 ) == -1)
       {
         if (!stop_loading)
           {
             fprintf(stderr, "%s - read() failed with errno %d.\n", 
                     __func__, errno);
           }
         return -1;
       }

     /* restore old settings */
     tcsetattr(STDIN_FILENO, FLAG, &otty);
  }

  return ch;
} 

int screen_test_keyboard_input (batch_context* bctx)
{
  /*
  int rval_input = -1;
  if ((rval_input = test_fd_readable (STDIN_FILENO)) == -1)
    {
      fprintf(stderr, "%s - select () failed with errno %d.\n", 
              __func__, errno);
      return -1;
    }
  if (!rval_input)
    return 0;
  */

  int the_key;

  if ((the_key = getch ())== -1)
    {
      //fprintf(stderr, "%s - getch () failed with errno %d.\n", 
      //        __func__, errno);
      return 0;
    }
  
  return on_keybord_input (the_key, bctx);
}

int on_keybord_input (int key, batch_context* bctx)
{
  char ch = key;

     switch (ch) 
       {
       case 'm':
       case 'M':
         bctx->stop_client_num_gradual_increase = 1;
         //fprintf(stderr, "%s - stop_inc %d\n", 
         //        __func__, bctx->stop_client_num_gradual_increase);
         break;

       case 'a':
       case 'A':
         bctx->stop_client_num_gradual_increase = 0;
         break;

       case '+':
         add_loading_clients_num (bctx, 1);
         break;
         
         //case '-':
         //break;
         
       case '*':
         add_loading_clients_num (bctx, 10);
         break;
         
         //case '/':
         //break;
       }

     fprintf(stderr, "%s - got %c\n", __func__, key);
     return 0;
}

/****************************************************************************************
* Function name - test_fd_readable
*
* Description - Tests, whether the descriptor is readable
* Input -       fd -  file descriptor
*
* Return Code - When readable (1), when not readable (0), on error (-1)
****************************************************************************************/
/*
static int test_fd_readable (int fd)
{
  fd_set fdset;
  struct timeval timeout = {0,0};

  FD_ZERO(&fdset);
  FD_SET(fd, &fdset);

  return select (fd + 1, &fdset, NULL, NULL, &timeout);
}
*/

