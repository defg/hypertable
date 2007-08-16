/**
 * Copyright (C) 2007 Doug Judd (Zvents, Inc.)
 * 
 * This file is part of Hypertable.
 * 
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or any later version.
 * 
 * Hypertable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef HYPERTABLE_MANAGER_H
#define HYPERTABLE_MANAGER_H

#include <string>

#include "Common/Properties.h"
#include "AsyncComm/Comm.h"

#include "MasterClient.h"

namespace hypertable {

  class Manager {

  public:

    static void Initialize(std::string configFile);
    static Manager *Instance() { return msInstance; }

    void CreateTable(std::string name, std::string schema);

  protected:

    /**
     *  Constructor.
     */
    Manager(PropertiesPtr &propsPtr);

  private:

    static Manager *msInstance;

    CommPtr mCommPtr;
    MasterClientPtr mMasterPtr;

    //public Table CreateTable(std::string name, std::string schema);
    //public std::string GetSchema(std::string tableName);
    //Table OpenTable();
    //void DeleteTable();
    // String [] ListTables();
  };

}

#endif // HYPERTABLE_MANAGER_H
