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

#    Summary: YOU dialogs
#    Authors: Cornelius Schumacher <cschum@suse.de>
require "yast"

module Yast
  class OnlineUpdateDialogsClass < Module
    def main
      Yast.import "UI"

      textdomain "online-update"

      Yast.import "Label"
      Yast.import "Mode"
      Yast.import "Package"
      Yast.import "Popup"
      Yast.import "Wizard"
    end

    def ErrorPopupGeneric(message, details, type)
      details = _("No details available.") if Builtins.size(details) == 0

      detailsStringOn = _("&Details <<")
      detailsStringOff = _("&Details >>")

      detailsButton = PushButton(Id(:details), detailsStringOff)

      heading = _("Error")

      buttons = nil
      if type == :skip
        buttons = HBox(
          detailsButton,
          PushButton(Id(:tryagain), _("Try again")),
          PushButton(Id(:skip), _("Skip Patch")),
          PushButton(Id(:all), _("Skip All")),
          PushButton(Id(:abort), _("Abort Update"))
        )
      elsif type == :ignore
        buttons = HBox(
          detailsButton,
          PushButton(Id(:ok), Label.IgnoreButton),
          PushButton(Id(:abort), _("Abort Update"))
        )
      elsif type == :ignorewarning
        heading = _("Warning")
        buttons = HBox(
          detailsButton,
          PushButton(Id(:retry), Label.RetryButton),
          PushButton(Id(:ok), Label.IgnoreButton),
          PushButton(Id(:abort), _("Abort Update"))
        )
      else
        buttons = HBox(detailsButton, PushButton(Id(:ok), Label.OKButton))
      end

      UI.OpenDialog(
        Opt(:decorated),
        VBox(
          HBox(HSpacing(0.5), Left(Heading(heading))),
          VSpacing(0.2),
          Label(message),
          ReplacePoint(Id(:rp), HBox(HSpacing(0))),
          buttons
        )
      )

      ret = nil
      showDetails = false

      while ret != :ok && ret != :tryagain && ret != :all && ret != :abort &&
          ret != :skip &&
          ret != :retry
        ret = UI.UserInput

        if ret == :details
          if showDetails
            UI.ReplaceWidget(Id(:rp), VSpacing(0))
            UI.ChangeWidget(Id(:details), :Label, detailsStringOff)
          else
            UI.ReplaceWidget(
              Id(:rp),
              HBox(
                HSpacing(0.5),
                HWeight(1, RichText(Opt(:plainText), details)),
                HSpacing(0.5)
              )
            )
            UI.ChangeWidget(Id(:details), :Label, detailsStringOn)
          end
          showDetails = !showDetails
        end
      end

      UI.CloseDialog

      if type != :plain
        return deep_copy(ret)
      else
        if ret == :ok
          return true
        else
          return false
        end
      end
    end

    def IgnoreWarningPopup(message, details)
      Convert.to_symbol(ErrorPopupGeneric(message, details, :ignorewarning))
    end

    def IgnorePopup(message, details)
      Convert.to_symbol(ErrorPopupGeneric(message, details, :ignore))
    end

    def SkipPopup(message, details)
      Convert.to_symbol(ErrorPopupGeneric(message, details, :skip))
    end

    def ErrorPopup(message, details)
      Convert.to_symbol(ErrorPopupGeneric(message, details, :plain))
    end

    #  ConfirmAbortUpdate has same layout as/but different text than ConfirmAbort
    def ConfirmAbortUpdate(how_to)
      what_will_happen = ""

      if how_to == :painless
        # Warning text for aborting the update before a patch is installed
        what_will_happen = _(
          "If you abort the installation now, no patch will be installed.\nYour installation will remain untouched.\n"
        )
      elsif how_to == :incomplete
        # Warning text for aborting if some patches are installed, some not
        what_will_happen = _(
          "Patch download and installation in progress.\n" +
            "If you abort the installation now, the update is incomplete.\n" +
            "Repeat the update, including the download, if desired.\n"
        )
      elsif how_to == :unusable
        # Warning text for aborting an installation during the install process
        what_will_happen = _(
          "If you abort the installation now,\n" +
            "at least one patch is not installed correctly.\n" +
            "You will need to do the update again."
        )
      elsif how_to == :done
        Builtins.y2debug("Aborting after everything should be installed?")
      else
        Builtins.y2warning(
          "Unknown symbol for what will happen when aborting - please correct in calling module"
        )
      end

      UI.OpenDialog(
        Opt(:decorated),
        HBox(
          HSpacing(1),
          VBox(
            VSpacing(0.2),
            HCenter(
              HSquash(
                VBox(
                  # Confirm user request to abort installation
                  Left(Label(_("Really abort YaST Online Update?"))),
                  Left(Label(what_will_happen))
                )
              )
            ),
            HBox(
              # Button that will really abort the installation
              PushButton(Id(:really_abort), _("&Abort Update")),
              HStretch(),
              # Button that will continue with the installation
              PushButton(Id(:continue), Opt(:default), _("&Continue Update"))
            ),
            VSpacing(0.2)
          ),
          HSpacing(1)
        )
      )

      ret = UI.UserInput
      UI.CloseDialog

      ret == :really_abort
    end

    def DisplayMsgYou(message, header, yes_button, no_button)
      UI.OpenDialog(
        Opt(:decorated),
        HBox(
          HSpacing(1),
          VBox(
            Left(Heading(header)),
            VSpacing(0.2),
            Label(message),
            HBox(
              PushButton(Id(:yes), yes_button),
              PushButton(Id(:no), Opt(:default), no_button)
            ),
            VSpacing(0.2)
          ),
          HSpacing(1)
        )
      )
      ret = UI.UserInput
      UI.CloseDialog
      ret == :yes
    end


    def DisplayMsgYouOk(message, header, ok_button)
      UI.OpenDialog(
        Opt(:decorated),
        HBox(
          HSpacing(1),
          VBox(
            Left(Heading(header)),
            VSpacing(0.2),
            Label(message),
            HBox(PushButton(Id(:ok), Opt(:default), ok_button)),
            VSpacing(0.2)
          ),
          HSpacing(1)
        )
      )
      ret = UI.UserInput
      UI.CloseDialog
      ret == :ok
    end

    def MessagePopup(patches, pre)
      patches = deep_copy(patches)
      message = ""
      details = ""

      i = 0
      while Ops.less_than(i, Builtins.size(patches))
        patch = Ops.get(patches, i, {})

        name = Ops.get_string(patch, "name", "")
        summary = Ops.get_string(patch, "summary", "")

        info = ""
        if pre
          info = Ops.get_string(patch, "preinformation", "")
        else
          info = Ops.get_string(patch, "postinformation", "")
        end

        header = Builtins.sformat(_("<b>Patch:</b> %1<br>"), name)
        header = Ops.add(
          header,
          Builtins.sformat(_("<b>Summary:</b> %1<br>"), summary)
        )

        message = Ops.add(message, header)
        message = Ops.add(Ops.add(Ops.add(message, "<pre>"), info), "</pre>")

        packages = Ops.get_list(patch, "packages", [])

        if Ops.greater_than(Builtins.size(packages), 0)
          details = Ops.add(details, header)

          details = Ops.add(details, _("<b>Packages:</b>"))

          details = Ops.add(details, "<ul>")

          Builtins.foreach(packages) do |p|
            details = Ops.add(Ops.add(Ops.add(details, "<li>"), p), "</li>")
          end

          details = Ops.add(details, "</ul>")
        end

        i = Ops.add(i, 1)
      end

      detailsStringOn = _("Patch &Details <<")
      detailsStringOff = _("Patch &Details >>")

      detailsButton = PushButton(Id(:details), detailsStringOff)

      detailsTerm = HBox(
        HSpacing(0.5),
        HWeight(1, RichText(Opt(:plainText), details)),
        HSpacing(0.5)
      )

      buttons = nil
      if pre
        buttons = HBox(
          details == "" ? VSpacing(0) : detailsButton,
          PushButton(Id(:ok), _("Install Patch")),
          PushButton(Id(:skip), _("Skip Patch"))
        )
      else
        buttons = HBox(
          details == "" ? VSpacing(0) : detailsButton,
          PushButton(Id(:ok), Label.OKButton)
        )
      end

      w = 20
      h = 5

      if Ops.greater_than(Builtins.size(message), 100)
        w = 60
        h = 15
      end

      Builtins.y2milestone("Going to open the message dialog")

      UI.OpenDialog(
        Opt(:decorated),
        VBox(
          HSpacing(w),
          VSpacing(0.2),
          HBox(VSpacing(h), RichText(message)),
          ReplacePoint(Id(:rp), VSpacing(0)),
          buttons
        )
      )

      Builtins.y2milestone("Dialog opened")

      ret = nil
      showDetails = false

      while ret != :ok && ret != :skip
        ret = Convert.to_symbol(UI.UserInput)

        if ret == :details
          if showDetails
            UI.ReplaceWidget(Id(:rp), HSpacing(0))
            UI.ChangeWidget(Id(:details), :Label, detailsStringOff)
          else
            UI.ReplaceWidget(
              Id(:rp),
              HBox(HSpacing(0.5), HWeight(1, RichText(details)), HSpacing(0.5))
            )
            UI.ChangeWidget(Id(:details), :Label, detailsStringOn)
          end
          showDetails = !showDetails
        end
      end

      UI.CloseDialog

      if ret == :ok
        return true
      else
        return false
      end
    end

    publish :function => :IgnoreWarningPopup, :type => "symbol (string, string)"
    publish :function => :IgnorePopup, :type => "symbol (string, string)"
    publish :function => :SkipPopup, :type => "symbol (string, string)"
    publish :function => :ErrorPopup, :type => "symbol (string, string)"
    publish :function => :ConfirmAbortUpdate, :type => "boolean (symbol)"
    publish :function => :DisplayMsgYou, :type => "boolean (string, string, string, string)"
    publish :function => :DisplayMsgYouOk, :type => "boolean (string, string, string)"
    publish :function => :MessagePopup, :type => "boolean (list <map>, boolean)"
  end

  OnlineUpdateDialogs = OnlineUpdateDialogsClass.new
  OnlineUpdateDialogs.main
end
