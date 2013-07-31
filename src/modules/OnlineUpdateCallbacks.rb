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

# Module:		OnlineUpdateCallbacks.ycp
#
# Authors:		Cornelius Schumacher <cschum@suse.de>
#
# Purpose:		provides the Callbacks for the online update
require "yast"

module Yast
  class OnlineUpdateCallbacksClass < Module
    def main
      Yast.import "Pkg"
      Yast.import "UI"

      textdomain "online-update"

      Yast.import "OnlineUpdateDialogs"
      Yast.import "PackageCallbacks"
      Yast.import "Popup"
      Yast.import "Report"

      @total_progress = 0

      # if user aborted the installation
      @aborted = false

      # last callback called
      @last_callback = ""

      # indentation of subtasks progress reports
      @indent = "  "
    end

    # Callback for patch (resp. delta rpm) progress widget.
    # @param [Fixnum] num position of progress widget (0 to 100)
    def PatchProgressCallback(num)
      # do not save this one - we need to store Start/Done pairs
      # last_callback	= "PatchProgressCallback";

      Builtins.y2debug("PatchProgressCallback %1", num)
      if UI.WidgetExists(Id(:you_patch_progress))
        UI.ChangeWidget(Id(:you_patch_progress), :Value, num)
      end

      return false if @aborted

      ret = Convert.to_symbol(UI.PollInput)

      if (ret == :abort || ret == :cancel) &&
          OnlineUpdateDialogs.ConfirmAbortUpdate(:incomplete)
        @aborted = true
        return false
      end
      true
    end

    def ProgressDownloadCallback(percent, bps_avg, bps_current)
      Builtins.y2debug("ProgressDownloadCallback %1%%", percent)
      PatchProgressCallback(percent)
    end

    def MessageCallback(patchname, patchsummary, message)
      @last_callback = "MessageCallback"

      # handle all messages as post (OK only)
      patches = [
        {
          "name"            => patchname,
          "summary"         => patchsummary,
          "postinformation" => message
        }
      ]

      OnlineUpdateDialogs.MessagePopup(patches, false)

      nil
    end

    # add a text to the installation progress log
    def ProgressLog(text)
      Builtins.y2debug("ProgressLog %1", text)

      UI.ChangeWidget(Id(:log), :LastLine, text) if UI.WidgetExists(Id(:log))

      nil
    end

    #   Callback for starting download of a package.
    def StartProvide(name, archivesize, remote)
      # progress log item (%1 is name of package)
      ProgressLog(Builtins.sformat(_("Retrieving %1..."), name))
      if UI.WidgetExists(Id(:you_patch_progress))
        UI.ChangeWidget(
          Id(:you_patch_progress),
          :Label,
          # progress bar label
          _("Package Download Progress")
        )
        UI.ChangeWidget(Id(:you_patch_progress), :Value, 0)
      end
      @last_callback = "StartProvide"

      nil
    end

    def StartDownload(url, localfile)
      # // reformat the URL
      # string url_report = URL::FormatURL(URL::Parse(url), max_size);
      #
      # FIXME new text to ProgressLog?
      # // progress log item (%1 is name of file)
      # string message = sformat (_("Downloading: %1"), url_report);
      # ProgressLog (message);
      # // + progress to 0?
      @last_callback = "StartDownload"

      nil
    end



    #  Callback for starting installation of a package.
    def StartPackage(pkg_name, name, summary, installsize, is_delete)
      p_name = name != "" ? name : pkg_name
      # progress log action (what is being done with the package)
      action = is_delete ? _("Removing") : _("Installing")
      # progress log item: %1 is action ("Removing" ot "Installing"),
      #  %2 is name of package, %3 is summary
      text = Builtins.sformat("%1 %2: \"%3\"", action, p_name, summary)
      if summary == ""
        # alternative progress log item: only action and package name
        text = Builtins.sformat("%1 %2", action, p_name)
      end

      ProgressLog(text)
      if UI.WidgetExists(Id(:you_patch_progress))
        UI.ChangeWidget(
          Id(:you_patch_progress),
          :Label,
          # progress bar label
          _("Package Installation Progress")
        )
        UI.ChangeWidget(Id(:you_patch_progress), :Value, 0)
      end
      @last_callback = "StartPackage"

      nil
    end

    # Callback for finishing an action in the log
    # @param [Boolean] line true if we are adding to the end of line
    def FinishLine(line)
      # progress log item (=previous action finished correctly)
      ProgressLog(Ops.add(Ops.add(line ? @indent : "", _("OK")), "\n"))
      true
    end


    # callback for 'package installed' action
    #
    #  return "" for ignore
    #  return "R" for retry
    #  return "C" for abort (not implemented !)
    def DonePackage(error, reason)
      ret = PackageCallbacks.DonePackage(error, reason)
      if ret == "I"
        FinishLine(true)
        @total_progress = Ops.add(@total_progress, 1)
        if UI.WidgetExists(Id(:you_total_progress))
          UI.ChangeWidget(Id(:you_total_progress), :Value, @total_progress)
        end
      end
      @last_callback = "DonePackage"
      ret
    end

    # callback for 'package provided' action
    #
    #  return "" for ignore
    #  return "R" for retry
    #  return "C" for abort (not implemented !)
    def DoneProvide(error, reason, name)
      ret = PackageCallbacks.DoneProvide(error, reason, name)
      if ret == "I"
        FinishLine(false) if @last_callback != "FinishPatchDeltaProvide"
        @total_progress = Ops.add(@total_progress, 1)
        if UI.WidgetExists(Id(:you_total_progress))
          UI.ChangeWidget(Id(:you_total_progress), :Value, @total_progress)
        end
      end
      @last_callback = "DoneProvide"
      ret
    end

    def DoneDownload(error_value, error_text)
      Builtins.y2debug("DoneDownload %1, %2", error_value, error_text)
      PackageCallbacks.DoneDownload(error_value, error_text) 
      # if (last_callback != "FinishPatchDeltaProvide")
      #     FinishLine (false);
      # last_callback   = "DoneDownload";

      nil
    end

    # callback for start of delta download
    def StartDeltaDownload(name, download_size)
      # progress log item (%1 is name of delta RPM
      if @last_callback == "StartProvide" || @last_callback == "StartDownload"
        ProgressLog(Ops.add("\n", @indent))
      end

      # Progress log. Leave the space at the end, some other text may follow
      ProgressLog(Builtins.sformat(_("Downloading delta RPM %1 "), name))
      if UI.WidgetExists(Id(:you_patch_progress))
        UI.ChangeWidget(
          Id(:you_patch_progress),
          :Label,
          # progress bar label
          _("Delta RPM Download Progress")
        )
        UI.ChangeWidget(Id(:you_patch_progress), :Value, 0)
      end
      @last_callback = "StartDeltaDownload"

      nil
    end

    # callback for delta download progress
    # @return [Boolean] abort the download?
    def ProgressDeltaDownload(num)
      Builtins.y2debug("ProgressDeltaDownload %1", num)
      ret = PatchProgressCallback(num)
      @last_callback = "ProgressDeltaDownload"
      ret
    end

    # callback for problem during downloading delta
    def ProblemDeltaDownload(description)
      Builtins.y2debug("ProblemDeltaDownload: %1", description)
      ProgressLog(
        Ops.add(
          Ops.add(
            Ops.add("\n", @indent),
            # progress log item (previous action failed(%1 is reason)
            Builtins.sformat(_("Failed to download delta RPM: %1"), description)
          ),
          "\n"
        )
      )
      @last_callback = "ProblemDeltaDownload"

      nil
    end

    # callback for start of applying delta rpm
    def StartDeltaApply(name)
      # Progress log item (%1 is name of delta RPM).
      # Leave the space at the end, some other text may follow.
      ProgressLog(
        Ops.add(@indent, Builtins.sformat(_("Applying delta RPM: %1 "), name))
      )
      if UI.WidgetExists(Id(:you_patch_progress))
        UI.ChangeWidget(
          Id(:you_patch_progress),
          :Label,
          # progress bar label
          _("Delta RPM Application Progress")
        )
        UI.ChangeWidget(Id(:you_patch_progress), :Value, 0)
      end
      @last_callback = "StartDeltaApply"

      nil
    end

    # progress of applying delta
    # (cannot be aborted)
    def ProgressDeltaApply(num)
      Builtins.y2debug("ProgressDeltaApply: %1", num)
      if UI.WidgetExists(Id(:you_patch_progress))
        UI.ChangeWidget(Id(:you_patch_progress), :Value, num)
      end
      @last_callback = "ProgressDeltaApply"

      nil
    end

    # callback for problem during aplying delta
    def ProblemDeltaApply(description)
      Builtins.y2debug("ProblemDeltaAply: %1", description)
      ProgressLog(
        Ops.add(
          Ops.add(
            Ops.add("\n", @indent),
            # progress log item (previous action failed(%1 is reason)
            Builtins.sformat(_("Failed to apply delta RPM: %1"), description)
          ),
          "\n"
        )
      )
      @last_callback = "ProblemDeltaApply"

      nil
    end

    # callback for start of downloading patch
    def StartPatchDownload(name, download_size)
      # progress log item (%1 is name of delta RPM)
      ProgressLog(Ops.add("\n", @indent)) if @last_callback == "StartProvide"
      # Progress log; lave the space at the end, some other text may follow.
      ProgressLog(Builtins.sformat(_("Downloading patch RPM %1 "), name))
      if UI.WidgetExists(Id(:you_patch_progress))
        UI.ChangeWidget(
          Id(:you_patch_progress),
          :Label,
          # progress bar label
          _("Patch RPM Download Progress")
        )
        UI.ChangeWidget(Id(:you_patch_progress), :Value, 0)
      end
      @last_callback = "StartPatchDownload"

      nil
    end

    # callback for path download progress
    # @return [Boolean] abort the download?
    def ProgressPatchDownload(num)
      Builtins.y2debug("ProgressPatchDownload %1", num)
      ret = PatchProgressCallback(num)
      @last_callback = "ProgressPatchDownload"
      ret
    end


    # callback for problem during aplying delta
    def ProblemPatchDownload(description)
      Builtins.y2debug("ProblemPatchDownload: %1", description)
      ProgressLog(
        Ops.add(
          Ops.add(
            Ops.add("\n", @indent),
            # progress log item (previous action failed(%1 is reason)
            Builtins.sformat(_("Failed to download patch RPM: %1"), description)
          ),
          "\n"
        )
      )
      @last_callback = "ProblemPatchDownload"

      nil
    end

    # finish of download/application of delta or patch download
    def FinishPatchDeltaProvide
      FinishLine(false) if @last_callback != "DoneDownload"
      @last_callback = "FinishPatchDeltaProvide"

      nil
    end


    # Script callbacks

    # a script has been started
    def ScriptStart(patch_name, patch_version, patch_arch, script_path)
      Builtins.y2milestone(
        "ScriptStart: patch_name:%1, patch_version:%2, patch_arch:%3, script:%4",
        patch_name,
        patch_version,
        patch_arch,
        script_path
      )
      patch_full_name = PackageCallbacks.FormatPatchName(
        patch_name,
        patch_version,
        patch_arch
      )

      if UI.WidgetExists(Id(:you_patch_progress))
        UI.ChangeWidget(
          Id(:you_patch_progress),
          :Label,
          # progress bar label
          _("Script Execution Progress")
        )
        UI.ChangeWidget(Id(:you_patch_progress), :Value, 0)
      end

      # log entry, %1 is name of the patch which contains the script
      log_line = Builtins.sformat(_("Starting script %1"), patch_full_name)

      ProgressLog(Ops.add("\n", log_line))
      @last_callback = "ScriptStart"

      nil
    end

    # print output of the script
    def ScriptProgress(ping, output)
      Builtins.y2milestone("ScriptProgress: ping:%1, output: %2", ping, output)

      if output != nil && output != ""
        # add the output to the log widget
        ProgressLog(output)
      end

      @last_callback = "ScriptProgress"

      input = UI.PollInput

      if input == :abort || input == :close
        Builtins.y2milestone("Aborting the script (input: %1)", input)
        return false
      else
        return true
      end
    end

    # an error has occurred
    def ScriptProblem(description)
      Builtins.y2milestone("ScriptProblem: %1", description)

      # FIXME: use rather LongError here?
      Popup.Error(description)
      @last_callback = "ScriptProblem"

      "A" # = Abort, TODO: support also Ignore and Retry
    end

    # the script has finished
    def ScriptFinish
      Builtins.y2milestone("ScriptFinish")

      if UI.WidgetExists(Id(:you_patch_progress))
        UI.ChangeWidget(Id(:you_patch_progress), :Value, 100)
      end

      FinishLine(true)
      @total_progress = Ops.add(@total_progress, 1)
      if UI.WidgetExists(Id(:you_total_progress))
        UI.ChangeWidget(Id(:you_total_progress), :Value, @total_progress)
      end

      @last_callback = "ScriptFinish"

      nil
    end

    # display a message
    def Message(patch_name, patch_version, patch_arch, message)
      patch_full_name = PackageCallbacks.FormatPatchName(
        patch_name,
        patch_version,
        patch_arch
      )
      Builtins.y2milestone("Message (%1): %2", patch_full_name, message)

      if patch_full_name != ""
        # label, %1 is patch name with version and architecture
        patch_full_name = Builtins.sformat(_("Patch %1\n\n"), patch_full_name)
      end

      # use richtext, the message might be too long for standard popup
      Popup.LongMessage(Ops.add(patch_full_name, message))
      @last_callback = "Message"

      true # = continue, TODO: use Continue/Cancel dialog
    end



    #   Constructor
    def RegisterOnlineUpdateCallbacks
      Builtins.y2milestone("OnlineUpdateCallbacks constructor")

      Pkg.CallbackStartProvide(
        fun_ref(method(:StartProvide), "void (string, integer, boolean)")
      )
      Pkg.CallbackProgressProvide(
        fun_ref(method(:PatchProgressCallback), "boolean (integer)")
      )
      Pkg.CallbackDoneProvide(
        fun_ref(method(:DoneProvide), "string (integer, string, string)")
      )

      Pkg.CallbackStartPackage(
        fun_ref(
          method(:StartPackage),
          "void (string, string, string, integer, boolean)"
        )
      )
      Pkg.CallbackProgressPackage(
        fun_ref(method(:PatchProgressCallback), "boolean (integer)")
      )
      Pkg.CallbackDonePackage(
        fun_ref(method(:DonePackage), "string (integer, string)")
      )

      Pkg.CallbackResolvableReport(
        fun_ref(method(:MessageCallback), "void (string, string, string)")
      )

      Pkg.CallbackStartDownload(
        fun_ref(method(:StartDownload), "void (string, string)")
      )
      Pkg.CallbackProgressDownload(
        fun_ref(
          method(:ProgressDownloadCallback),
          "boolean (integer, integer, integer)"
        )
      )
      Pkg.CallbackDoneDownload(
        fun_ref(method(:DoneDownload), "void (integer, string)")
      )

      Pkg.CallbackMediaChange(
        fun_ref(
          PackageCallbacks.method(:MediaChange),
          "string (string, string, string, string, integer, string, integer, string, boolean, list <string>, integer)"
        )
      )

      # delta download
      Pkg.CallbackStartDeltaDownload(
        fun_ref(method(:StartDeltaDownload), "void (string, integer)")
      )
      Pkg.CallbackProgressDeltaDownload(
        fun_ref(method(:ProgressDeltaDownload), "boolean (integer)")
      )
      Pkg.CallbackProblemDeltaDownload(
        fun_ref(method(:ProblemDeltaDownload), "void (string)")
      )
      Pkg.CallbackFinishDeltaDownload(
        fun_ref(method(:FinishPatchDeltaProvide), "void ()")
      )

      # delta application
      Pkg.CallbackStartDeltaApply(
        fun_ref(method(:StartDeltaApply), "void (string)")
      )
      Pkg.CallbackProgressDeltaApply(
        fun_ref(method(:ProgressDeltaApply), "void (integer)")
      )
      Pkg.CallbackProblemDeltaApply(
        fun_ref(method(:ProblemDeltaApply), "void (string)")
      )
      Pkg.CallbackFinishDeltaApply(
        fun_ref(method(:FinishPatchDeltaProvide), "void ()")
      )

      # patch download
      Pkg.CallbackStartPatchDownload(
        fun_ref(method(:StartPatchDownload), "void (string, integer)")
      )
      Pkg.CallbackProgressPatchDownload(
        fun_ref(method(:ProgressPatchDownload), "boolean (integer)")
      )
      Pkg.CallbackProblemPatchDownload(
        fun_ref(method(:ProblemPatchDownload), "void (string)")
      )
      Pkg.CallbackFinishPatchDownload(
        fun_ref(method(:FinishPatchDeltaProvide), "void ()")
      )

      # script callbacks
      Pkg.CallbackScriptStart(
        fun_ref(method(:ScriptStart), "void (string, string, string, string)")
      )
      Pkg.CallbackScriptProgress(
        fun_ref(method(:ScriptProgress), "boolean (boolean, string)")
      )
      Pkg.CallbackScriptProblem(
        fun_ref(method(:ScriptProblem), "string (string)")
      )
      Pkg.CallbackScriptFinish(fun_ref(method(:ScriptFinish), "void ()"))

      Pkg.CallbackMessage(
        fun_ref(method(:Message), "boolean (string, string, string, string)")
      )

      nil
    end

    # Refresh all sources with autorefresh enabled.
    # This function is a temporary solution for bug #154990
    def RefreshAllSources
      Builtins.y2milestone("Refreshing all sources...")
      mgr_ok = Pkg.SourceStartManager(true)
      if !mgr_ok
        # error popoup (detailed info follows)
        Report.LongWarning(
          Ops.add(
            _("There was an error in the repository initialization.") + "\n",
            Pkg.LastError
          )
        )
      end

      all_sources = Pkg.SourceEditGet

      # There are no sources, nothing to refresh
      if all_sources == nil || Ops.less_than(Builtins.size(all_sources), 1)
        Builtins.y2warning("No sources defined, nothing to refresh...")
        return
      end
      Builtins.foreach(all_sources) do |one_source|
        source_id = Ops.get_integer(one_source, "SrcId")
        source_autorefresh = Ops.get_boolean(one_source, "autorefresh", true)
        source_enabled = Ops.get_boolean(one_source, "enabled", true)
        if source_id != nil && source_autorefresh == true &&
            source_enabled == true
          Builtins.y2milestone("Refreshing source: %1", source_id)
          Pkg.SourceRefreshNow(source_id)
        end
      end
      Builtins.y2milestone("... refreshing done")

      nil
    end

    # Refresh sources given by argument
    def RefreshSources(sources)
      sources = deep_copy(sources)
      Builtins.y2milestone("Refreshing sources...")
      Builtins.foreach(sources) do |one_source|
        source_id = Ops.get_integer(one_source, "SrcId")
        source_autorefresh = Ops.get_boolean(one_source, "autorefresh", true)
        source_enabled = Ops.get_boolean(one_source, "enabled", true)
        if source_id != nil && source_autorefresh == true &&
            source_enabled == true
          Builtins.y2milestone("Refreshing source: %1", source_id)
          Pkg.SourceRefreshNow(source_id)
        end
      end
      Builtins.y2milestone("... refreshing done")

      nil
    end

    publish :function => :PatchProgressCallback, :type => "boolean (integer)"
    publish :function => :ProgressDownloadCallback, :type => "boolean (integer, integer, integer)"
    publish :function => :MessageCallback, :type => "void (string, string, string)"
    publish :function => :ProgressLog, :type => "void (string)"
    publish :function => :StartProvide, :type => "void (string, integer, boolean)"
    publish :function => :StartDownload, :type => "void (string, string)"
    publish :function => :StartPackage, :type => "void (string, string, string, integer, boolean)"
    publish :function => :FinishLine, :type => "boolean (boolean)"
    publish :function => :DonePackage, :type => "string (integer, string)"
    publish :function => :DoneProvide, :type => "string (integer, string, string)"
    publish :function => :DoneDownload, :type => "void (integer, string)"
    publish :function => :StartDeltaDownload, :type => "void (string, integer)"
    publish :function => :ProgressDeltaDownload, :type => "boolean (integer)"
    publish :function => :ProblemDeltaDownload, :type => "void (string)"
    publish :function => :StartDeltaApply, :type => "void (string)"
    publish :function => :ProgressDeltaApply, :type => "void (integer)"
    publish :function => :ProblemDeltaApply, :type => "void (string)"
    publish :function => :StartPatchDownload, :type => "void (string, integer)"
    publish :function => :ProgressPatchDownload, :type => "boolean (integer)"
    publish :function => :ProblemPatchDownload, :type => "void (string)"
    publish :function => :FinishPatchDeltaProvide, :type => "void ()"
    publish :function => :ScriptStart, :type => "void (string, string, string, string)"
    publish :function => :ScriptProgress, :type => "boolean (boolean, string)"
    publish :function => :ScriptProblem, :type => "string (string)"
    publish :function => :ScriptFinish, :type => "void ()"
    publish :function => :Message, :type => "boolean (string, string, string, string)"
    publish :function => :RegisterOnlineUpdateCallbacks, :type => "void ()"
    publish :function => :RefreshAllSources, :type => "void ()"
    publish :function => :RefreshSources, :type => "void (list <map>)"
  end

  OnlineUpdateCallbacks = OnlineUpdateCallbacksClass.new
  OnlineUpdateCallbacks.main
end
