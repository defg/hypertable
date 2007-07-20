/**
 * Copyright 2007 Doug Judd (Zvents, Inc.)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at 
 *
 * http://www.apache.org/licenses/LICENSE-2.0 
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <cstdio>
#include <iostream>
using namespace std;

extern "C" {
#include <errno.h>
#if defined(__APPLE__)
#include <sys/event.h>
#endif
}

#include "Common/Logger.h"

#include "IOHandler.h"
#include "Reactor.h"
using namespace hypertable;

#if defined(__linux__)

void IOHandler::AddPollInterest(int mode) {
  struct epoll_event event;

  mPollInterest |= mode;

  memset(&event, 0, sizeof(struct epoll_event));
  event.data.ptr = this;
  event.events = EPOLLERR | EPOLLHUP;

  if (mPollInterest & Reactor::READ_READY)
    event.events |= EPOLLIN;
  if (mPollInterest & Reactor::WRITE_READY)
    event.events |= EPOLLOUT;

  if (epoll_ctl(mReactor->pollFd, EPOLL_CTL_MOD, mSd, &event) < 0) {
    LOG_VA_ERROR("epoll_ctl(%d, EPOLL_CTL_MOD, sd=%d) (mode=%x) : %s", 
		 mReactor->pollFd, mSd, mode, strerror(errno));
    *((int *)0) = 1;
  }
}



void IOHandler::RemovePollInterest(int mode) {
  struct epoll_event event;

  mPollInterest &= ~mode;

  memset(&event, 0, sizeof(struct epoll_event));
  event.data.ptr = this;
  event.events = EPOLLERR | EPOLLHUP;

  if (mPollInterest & Reactor::READ_READY)
    event.events |= EPOLLIN;
  if (mPollInterest & Reactor::WRITE_READY)
    event.events |= EPOLLOUT;

  if (epoll_ctl(mReactor->pollFd, EPOLL_CTL_MOD, mSd, &event) < 0) {
    LOG_VA_ERROR("epoll_ctl(EPOLL_CTL_MOD, sd=%d) (mode=%x) : %s", mSd, mode, strerror(errno));
    exit(1);
  }
}



void IOHandler::DisplayEvent(struct epoll_event *event) {
  char buf[128];

  buf[0] = 0;
  if (event->events & EPOLLIN)
    strcat(buf, "EPOLLIN ");
  else if (event->events & EPOLLOUT)
    strcat(buf, "EPOLLOUT ");
  else if (event->events & EPOLLPRI)
    strcat(buf, "EPOLLPRI ");
  else if (event->events & EPOLLERR)
    strcat(buf, "EPOLLERR ");
  else if (event->events & EPOLLHUP)
    strcat(buf, "EPOLLHUP ");
  else if (event->events & EPOLLET)
    strcat(buf, "EPOLLET ");
  else if (event->events & EPOLLONESHOT)
    strcat(buf, "EPOLLONESHOT ");

  if (buf[0] == 0)
    sprintf(buf, "0x%x ", event->events);

  clog << "epoll events = " << buf << endl;

  return;
}



#elif defined(__APPLE__)

void IOHandler::AddPollInterest(int mode) {
  struct kevent events[2];
  int count=0;
  if (mode & Reactor::READ_READY) {
    EV_SET(&events[count], mSd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
    count++;
  }
  if (mode & Reactor::WRITE_READY) {
    EV_SET(&events[count], mSd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
    count++;
  }
  assert(count > 0);
    
  if (kevent(mReactor->kQueue, events, count, 0, 0, 0) == -1) {
    LOG_VA_ERROR("kevent(sd=%d) (mode=%x) : %s", mSd, mode, strerror(errno));
    exit(1);
  }
  mPollInterest |= mode;
}

void IOHandler::RemovePollInterest(int mode) {
  struct kevent devents[2];
  int count = 0;

  if (mode & Reactor::READ_READY) {
    EV_SET(&devents[count], mSd, EVFILT_READ, EV_DELETE, 0, 0, 0);
    count++;
  }

  if (mode & Reactor::WRITE_READY) {
    EV_SET(&devents[count], mSd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
    count++;
  }

  if (kevent(mReactor->kQueue, devents, count, 0, 0, 0) == -1 && errno != ENOENT) {
    LOG_VA_ERROR("kevent(sd=%d) (mode=%x) : %s", mSd, mode, strerror(errno));
    exit(1);
  }
}


/**
 *
 */
void IOHandler::DisplayEvent(struct kevent *event) {

  clog << "kevent: ident=" << hex << (long)event->ident;

  switch (event->filter) {
  case EVFILT_READ:
    clog << ", EVFILT_READ, fflags=";
    if (event->fflags & NOTE_LOWAT)
      clog << "NOTE_LOWAT";
    else
      clog << event->fflags;
    break;
  case EVFILT_WRITE:
    clog << ", EVFILT_WRITE, fflags=";
    if (event->fflags & NOTE_LOWAT)
      clog << "NOTE_LOWAT";
    else
      clog << event->fflags;
    break;
  case EVFILT_AIO:
    clog << ", EVFILT_AIO, fflags=" << event->fflags;
    break;
  case EVFILT_VNODE:
    clog << ", EVFILT_VNODE, fflags={";
    if (event->fflags & NOTE_DELETE)
      clog << " NOTE_DELETE";
    if (event->fflags & NOTE_WRITE)
      clog << " NOTE_WRITE";
    if (event->fflags & NOTE_EXTEND)
      clog << " NOTE_EXTEND";
    if (event->fflags & NOTE_ATTRIB)
      clog << " NOTE_ATTRIB";
    if (event->fflags & NOTE_LINK)
      clog << " NOTE_LINK";
    if (event->fflags & NOTE_RENAME)
      clog << " NOTE_RENAME";
    if (event->fflags & NOTE_REVOKE)
      clog << " NOTE_REVOKE";
    clog << " }";
    break;
  case EVFILT_PROC:
    clog << ", EVFILT_VNODE, fflags={";
    if (event->fflags & NOTE_EXIT)
      clog << " NOTE_EXIT";
    if (event->fflags & NOTE_FORK)
      clog << " NOTE_FORK";
    if (event->fflags & NOTE_EXEC)
      clog << " NOTE_EXEC";
    if (event->fflags & NOTE_TRACK)
      clog << " NOTE_TRACK";
    if (event->fflags & NOTE_TRACKERR)
      clog << " NOTE_TRACKERR";
    if (event->fflags & NOTE_CHILD)
      clog << " NOTE_CHILD";
    clog << " pid=" << (event->flags & NOTE_PDATAMASK);
    break;
  case EVFILT_SIGNAL:
    clog << ", EVFILT_SIGNAL, fflags=" << event->fflags;
    break;
  case EVFILT_TIMER:
    clog << ", EVFILT_TIMER, fflags={";
    if (event->fflags & NOTE_SECONDS)
      clog << " NOTE_SECONDS";
    if (event->fflags & NOTE_USECONDS)
      clog << " NOTE_USECONDS";
    if (event->fflags & NOTE_NSECONDS)
      clog << " NOTE_NSECONDS";
    if (event->fflags & NOTE_ABSOLUTE)
      clog << " NOTE_ABSOLUTE";
    clog << " }";
    break;
  }

  if(event->flags != 0) {
    clog << ", flags=";
    if ((event->flags & EV_EOF) || (event->flags & EV_ERROR)) {
      clog << "{";
      if (event->flags & EV_EOF)
	clog << " EV_EOF";
      if (event->flags & EV_ERROR)
	clog << " EV_ERROR";
      clog << "}";
    }
    else
      clog << hex << event->flags;
  }
  clog << ", data=" << dec << (long)event->data << endl;
}

#endif