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

# File:	clients/online_update_install.ycp
# Package:	Configuration of online_update
# Summary: YOU installation page
# Authors: Cornelius Schumacher <cschum@suse.de>
#
# $Id$
#
# This is a client for installation.
# It displays the dialog with a progress of the actual installation and
# executes the installation.
module Yast
  class OnlineUpdateInstallClient < Client
    def main
      Yast.import "UI"
      Yast.import "Pkg"

      textdomain "online-update"

      Yast.import "Label"
      Yast.import "Stage"
      Yast.import "Wizard"
      Yast.import "OnlineUpdateDialogs"


      @contents = VBox(
        VSpacing(0.2),
        # progress window label
        LogView(Id(:log), _("Progress Log"), 5, 0),
        VSpacing(0.5),
        ReplacePoint(
          Id(:rppatch),
          # progress bar label
          ProgressBar(Id(:you_patch_progress), _("Package Progress"))
        ),
        VSpacing(0.2),
        ReplacePoint(
          Id(:rpprogress),
          # progress bar label
          ProgressBar(Id(:you_total_progress), _("Total Progress"))
        ),
        VSpacing(0.5),
        VSpacing(0.2)
      )
      # help text for online update
      @help_part1 = _(
        "<p>After connecting to the update server,\n" +
          "YaST will download all selected patches.\n" +
          "This could take some time. Download details are shown in the log window.</p>"
      )

      # help text for online update
      @help_part2 = _(
        "<p>If special messages associated with patches are available, they will be shown in an extra dialog when the patch is installed.</p>\n"
      )

      @help_text = Ops.add(@help_part1, @help_part2)

      # using SetContents (define in online_update.ycp)
      Wizard.SetContents(
        _("Patch Download and Installation"),
        @contents,
        @help_text,
        false,
        false
      )

      Wizard.SetNextButton(:next, Label.FinishButton) if !Stage.cont

      @total_progress = Ops.multiply(
        Builtins.size(Pkg.GetPackages(:selected, true)),
        2
      )
      @total_progress = 100 if @total_progress == 0

      UI.ReplaceWidget(
        Id(:rpprogress),
        # progress bar label
        ProgressBar(
          Id(:you_total_progress),
          _("Total Progress"),
          @total_progress
        )
      )

      # FIXME no chance to get this after Commit (bug 188556)
      @changed_packages = Pkg.IsAnyResolvable(:package, :to_remove)

      @commit = Pkg.PkgCommit(0)

      Yast.import "OnlineUpdateCallbacks"

      # progress information
      OnlineUpdateCallbacks.ProgressLog(_("Installation finished.\n"))

      if Stage.cont
        # one more finish message (#191400)
        UI.ReplaceWidget(
          Id(:rppatch),
          # label
          Left(Label(Opt(:boldFont), _("Patch installation finished.")))
        )
        UI.ReplaceWidget(Id(:rpprogress), Empty())
      end


      if Ops.get_list(@commit, 1, []) != []
        @details = Ops.add(Ops.add(Pkg.LastError, "\n"), Pkg.LastErrorDetails)
        # error message
        OnlineUpdateDialogs.ErrorPopup(_("Patch processing failed."), @details)
      end

      Wizard.EnableBackButton
      Wizard.EnableNextButton
      Wizard.SetFocusToNextButton

      @changed_packages = @changed_packages ||
        Ops.greater_than(Ops.get_integer(@commit, 0, 0), 0)

      UI.ChangeWidget(Id(:you_patch_progress), :Value, 100)
      UI.ChangeWidget(Id(:you_total_progress), :Value, @total_progress)

      @ret = :next
      if !Stage.cont
        @ret = :abort if !@changed_packages
        return @ret
      end
      begin
        @ret = Convert.to_symbol(UI.UserInput)

        @ret = :abort if @ret == :cancel && !@changed_packages

        if @ret == :abort && !OnlineUpdateDialogs.ConfirmAbortUpdate(:done)
          @ret = :this
        end

        if !@changed_packages && (@ret == :next || @ret == :cancel)
          @ret = :abort
        end
      end until @ret == :next || @ret == :back || @ret == :abort || @ret == :again ||
        @ret == :cancel

      @ret
    end
  end
end

Yast::OnlineUpdateInstallClient.new.main
