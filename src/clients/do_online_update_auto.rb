# encoding: utf-8

# ------------------------------------------------------------------------------
# Copyright (c) 2006-2012 Novell, Inc. All Rights Reserved.
#
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of version 2 of the GNU General Public License as published by the
# Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, contact Novell, Inc.
#
# To contact Novell about this file by physical or electronic mail, you may find
# current contact information at www.novell.com.
# ------------------------------------------------------------------------------

# File:	clients/do_online_update_auto.ycp
# Package:	Configuration of online_update
# Summary:	Run online update during autoinstallation
# Authors:	Jiri Suchomel <jsuchome@suse.cz>
#
# $Id$
module Yast
  class DoOnlineUpdateAutoClient < Client
    def main
      Yast.import "Pkg"

      Builtins.y2milestone("----------------------------------------")
      Builtins.y2milestone("do_online_update auto started")


      @ret = nil
      @func = ""
      @param = {}

      # Check arguments
      if Ops.greater_than(Builtins.size(WFM.Args), 0) &&
          Ops.is_string?(WFM.Args(0))
        @func = Convert.to_string(WFM.Args(0))
        if Ops.greater_than(Builtins.size(WFM.Args), 1) &&
            Ops.is_map?(WFM.Args(1))
          @param = Convert.to_map(WFM.Args(1))
        end
      end
      Builtins.y2debug("func=%1", @func)
      Builtins.y2debug("param=%1", @param)

      # Run the online update now: non-interactive version of
      # inst_you + online_update_install
      if @func == "Write"
        @ret = :auto

        Pkg.TargetInit("/", false)
        Pkg.SourceStartManager(true)
        Pkg.PkgSolve(true)

        @selected = Pkg.ResolvablePreselectPatches(:all)
        Builtins.y2milestone("All preselected patches: %1", @selected)

        if Ops.greater_than(@selected, 0)
          Builtins.foreach(Pkg.ResolvableProperties("", :patch, "")) do |patch|
            if Ops.get_symbol(patch, "status", :none) == :selected &&
                Ops.get_boolean(patch, "reboot_needed", false)
              # patch requiring reboot should be installed in this run
              @ret = :reboot
            end
          end
          Builtins.y2milestone("PkgSolve result: %1", Pkg.PkgSolve(false))
          @commit = Pkg.PkgCommit(0)
          Builtins.y2milestone("PkgCommit result: %1", @commit)
          if Ops.get_list(@commit, 1, []) != []
            Builtins.y2error(
              "Commit failed with %1 (%2).",
              Pkg.LastError,
              Pkg.LastErrorDetails
            )
          end
        end
      else
        Builtins.y2error("Unknown function: %1", @func)
        @ret = false
      end

      Builtins.y2debug("ret=%1", @ret)
      Builtins.y2milestone("do_online_update auto finished")
      Builtins.y2milestone("----------------------------------------")

      deep_copy(@ret) 

      # EOF
    end
  end
end

Yast::DoOnlineUpdateAutoClient.new.main
