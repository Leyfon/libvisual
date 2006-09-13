// Libvisual-c++ - C++ bindings for Libvisual
//
// Copyright (C) 2006 Chong Kai Xiong <descender@phreaker.net>
//
// Author: Chong Kai Xiong <descender@phreaker.net>
//
// $Id: lv_os.hpp,v 1.1 2006-09-13 21:06:30 descender Exp $
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

#ifndef LV_OS_HPP
#define LV_OS_HPP

#include <libvisual/lv_os.h>

namespace Lv
{

  inline void scheduler_realtime_start ()
  {
      // FIXME: May return VISUAL_ERROR_OS_SCHED_NOT_SUPPORTED and others
      visual_os_scheduler_realtime_start ();
  }

  inline void scheduler_realtime_stop ()
  {
      // FIXME: May return VISUAL_ERROR_OS_SCHED_NOT_SUPPORTED and others
      visual_os_scheduler_realtime_stop ();
  }

  inline void scheduler_realtime_yield ()
  {
      // FIXME: May return VISUAL_ERROR_OS_SCHED_NOT_SUPPORTED and others
      visual_os_scheduler_yield ();
  }

} // namespace Lv

#endif // #ifndef LV_OS_HPP