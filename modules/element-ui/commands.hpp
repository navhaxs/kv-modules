/*
    commands.hpp - This file is part of Element
    Copyright (C) 2013  Michael Fisher <mfisher31@gmail.com>

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
*/

#ifndef ELEMENT_GUI_COMMANDS_HPP
#define ELEMENT_GUI_COMMANDS_HPP


namespace element {
namespace gui {

   namespace Commands {
       static const int showAbout              = 1;
       static const int showPluginManager      = 2;
       static const int showPreferences        = 3;
   }

   namespace CommandCategories
   {
       static const char* const general       = "General";
       static const char* const editing       = "Editing";
       static const char* const view          = "View";
       static const char* const windows       = "Windows";
   }

}}


#endif /* ELEMENT_GUI_COMMANDS_HPP */
