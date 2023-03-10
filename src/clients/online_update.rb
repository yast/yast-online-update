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

# Summary:	Main file
# Authors:	Gabriele Strattner <gs@suse.de>
#		Stefan Schubert <schubi@suse.de>
#              Cornelius Schumacher <cschum@suse.de>

# bsc#1205913
# 1. We may update a rubygem
# 2. inst_release_notes may be called, which (indirectly) requires dbus
# Then rubygems would try loading the gemspec of the uninstalled older gem
# and crash. Prevent it by requiring dbus early.
begin
      require "dbus"
rescue LoadError
      # Call site will check if it's there
end

require "y2packager/resolvable"
require "ui/ui_extension_checker"

module Yast
  class OnlineUpdateClient < Client
    include Yast::Logger

    def main
      Yast.import "Pkg"

      textdomain "online-update"

      Yast.import "CommandLine"
      Yast.import "Confirm"
      Yast.import "Directory"
      Yast.import "FileUtils"
      Yast.import "Kernel"
      Yast.import "Label"
      Yast.import "Mode"
      Yast.import "OnlineUpdate"
      Yast.import "OnlineUpdateCallbacks"
      Yast.import "PackageLock"
      Yast.import "Popup"
      Yast.import "Product"
      Yast.import "Progress"
      Yast.import "SourceManager"
      Yast.import "URL"
      Yast.import "Wizard"
      Yast.import "GetInstArgs"
      Yast.import "Installation"

      # the command line description map
      @cmdline = {
        "id"         => "online_update",
        # command line help text
        "help"       => _("Online Update module"),
        "guihandler" => fun_ref(method(:OnlineUpdateSequence), "symbol ()"),
        "actions"    => {
          "cd_update"   => {
            "handler" => fun_ref(method(:CDUpdateHandler), "boolean (map)"),
            # command line help text for cd_update action
            "help"    => _(
              "Start Patch CD Update"
            )
          },
          "simple_mode" => {
            "handler" => fun_ref(method(:SimpleModeHandler), "boolean (map)"),
            # command line help text for simple_mode action
            "help"    => _(
              "Use simple package selector"
            )
          }
        },
        "options"    => {
          "cd_url"       => {
            # command line help text for cd_url option
            "help" => Builtins.sformat(
              _("URL for Patch CD (default value is '%1')"),
              OnlineUpdate.cd_url
            ),
            "type" => "string"
          },
          "cd_directory" => {
            # command line help text for cd_directory option
            "help" => Builtins.sformat(
              _("Directory for patch data on Patch CD (default value is '%1')"),
              OnlineUpdate.cd_directory
            ),
            "type" => "string"
          }
        },
        "mappings"   => {
          "cd_update"   => ["cd_url", "cd_directory"],
          "simple_mode" => []
        }
      }

      # first check for obsoleted arguments
      @arg_n = 0
      while Ops.less_than(@arg_n, Builtins.size(WFM.Args))
        @arg = WFM.Args(@arg_n)
        if @arg == path(".cd_default") || @arg == ".cd_default"
          Builtins.y2warning(
            ".cd_default parameter is OBSOLETE, use cd_update instead."
          )
          OnlineUpdate.cd_update = true
        elsif @arg == path(".simple_mode") || @arg == ".simple_mode"
          OnlineUpdate.simple_mode = true
        elsif @arg == path(".auto.get") || @arg == ".auto.get"
          Builtins.y2warning(
            ".auto.get parameter for online_update is OBSOLETE, use zypper instead."
          )
        end
        @arg_n = Ops.add(@arg_n, 1)
      end
      @ret = :next
      if OnlineUpdate.cd_update || OnlineUpdate.simple_mode # obsolete .cd_default argument was used
        @ret = OnlineUpdateSequence()
      else
        @ret = CommandLine.Run(@cmdline)
      end
      deep_copy(@ret)
    end

    # Main sequence for Online Update
    def OnlineUpdateSequence
      ui_extension_checker = UIExtensionChecker.new("pkg")
      return :cancel unless ui_extension_checker.ok?

      Wizard.CreateDialog
      Wizard.SetDesktopTitleAndIcon("org.opensuse.yast.OnlineUpdate")
      # help text for online-update initialization
      Wizard.RestoreHelp(
        _(
          "<p>The system is initializing the installation and update repositories. Software repositories can be altered in the <b>Installation Source</b> module.</p>"
        )
      )

      stages = [
        # progress stage label
        _("Initialize the target system"),
        # progress stage label
        _("Refresh software repositories"),
        # progress stage label
        _("Check for available updates")
      ]
      steps = [
        # progress step label
        _("Initializing the target system..."),
        # progress step label
        _("Refreshing software repositories..."),
        # progress step label
        _("Checking for available updates..."),
        # final progress step label
        _("Finished")
      ]

      caption = OnlineUpdate.cd_update ?
        # dialog caption
        _("Initializing CD Update") :
        # dialog caption
        _("Initializing Online Update")
      Progress.New(caption, " ", 2, stages, steps, "")

      # check whether running as root
      # and having the packager for ourselves
      if !Confirm.MustBeRoot ||
          !Ops.get_boolean(PackageLock.Connect(false), "connected", false)
        Wizard.CloseDialog
        return :cancel
      end

      Progress.NextStage

      # initialize target to import all trusted keys (#165849)
      Pkg.TargetInit(Installation.destdir, false)

      Progress.NextStage

      enabled_only = true
      mgr_ok = Pkg.SourceStartManager(enabled_only)
      if !mgr_ok
        Report.LongWarning(
          # TRANSLATORS: error popup (detailed info follows)
          _("There was an error in the repository initialization.") + "\n" +
          Pkg.LastError
        )
      end

      Progress.NextStage

      if !OnlineUpdate.cd_update # CD for cd update was not initialized yet
        # an update repository is available?
        is_available = Pkg.SourceGetCurrent(true).any? do |repo|
          source_data = Pkg.SourceGeneralData(repo)
          update_repo = source_data["is_update_repo"]
          log.info("Update repository found: #{source_data["url"]}") if update_repo
          update_repo
        end

        # that repository flag might not be reliable, check if a patch is available
        if !is_available
          is_available = Y2Packager::Resolvable.any?(kind: :patch, status: :available)
          log.info("Patch is available: #{is_available}")
        end

        if !is_available
          # inst_scc is able to register the system and add update repos for SLE,
          # for openSUSE, let's use repository manager
          client = Product.short_name.match?(/openSUSE/i) ? "repositories" : "inst_scc"

          if WFM.ClientExists(client)
            if Popup.YesNo(
              # yes/no question
              _(
                "No update repository\nconfigured yet. Run configuration workflow now?"
              )
            )

              res = WFM.CallFunction(client, [])
              if res != :next && res != :finish
                Wizard.CloseDialog
                return :abort
              end
            end
          else
            # error message
            Report.Error(_("No update repository configured yet."))
          end
        end
      end

      OnlineUpdateCallbacks.RegisterOnlineUpdateCallbacks

      Progress.Finish

      # Main dialog cycle
      #

      dialog = [
        ["online_update_select", [GetInstArgs.Buttons(false, true)]],
        ["online_update_install", [GetInstArgs.Buttons(false, true)]]
      ]

      id = 0
      result = :next

      while id >= 0 && id < dialog.size
        page = dialog[id]
        module_name = page.fetch(0, "")
        module_args = page.fetch(1, [])

        Builtins.y2debug(
          "ONLINE: Module: %1 Args: %2",
          module_name,
          module_args
        )

        if id == Ops.subtract(Builtins.size(dialog), 1)
          Wizard.SetNextButton(:next, Label.FinishButton)
        end

        result = WFM.CallFunction(module_name, module_args)

        result = :cancel if result == nil

        if result == :again
          next
        elsif result == :cancel || result == :abort
          break
        elsif result == :next || result == :auto
          id += 1
        elsif result == :back
          id -= 1
        elsif result == :finish
          if !Mode.installation && !Mode.update
            id = dialog.size - 1 # call last module
          else
            result = :next
            break
          end
        end
      end

      if result == :next
        Kernel.InformAboutKernelChange
        if OnlineUpdate.show_release_notes
          WFM.CallFunction(
            "inst_release_notes",
            [GetInstArgs.Buttons(false, true)]
          )
        end
      end

      Wizard.CloseDialog

      # `back is strange, user can go back after installation...
      return :abort if result == :abort || result == :cancel

      if OnlineUpdate.cd_update && Ops.greater_than(OnlineUpdate.cd_source, -1)
        # map of the new source added on start
        new_source = {}
        sources = []
        Builtins.foreach(Pkg.SourceEditGet) do |one_source|
          srcid = Ops.get_integer(one_source, "SrcId", -1)
          source_data = Builtins.union(
            Pkg.SourceGeneralData(srcid),
            Pkg.SourceProductData(srcid)
          )
          if srcid == OnlineUpdate.cd_source
            new_source = deep_copy(source_data)
          else
            sources = Builtins.add(sources, source_data)
          end
        end
        add_new_source = true
        Builtins.foreach(sources) do |source|
          Builtins.y2debug("source to check %1", source)
          # checking URL is not enough, typical url is cd:///
          if Ops.get_string(source, "url", "") ==
              Ops.get_string(new_source, "url", "") &&
              Ops.get_string(source, "product_dir", "") ==
                Ops.get_string(new_source, "product_dir", "") &&
              Ops.get_string(source, "productname", "") ==
                Ops.get_string(new_source, "productname", "") &&
              Ops.get_string(source, "productversion", "") ==
                Ops.get_string(new_source, "productversion", "")
            Builtins.y2milestone(
              "Patch CD source already present, not adding again"
            )
            add_new_source = false
            raise Break
          end
        end
        if add_new_source
          # now, ensure there's first CD in the drive (bnc#381594)
          parsed = URL.Parse(Ops.get_string(new_source, "url", ""))
          if Ops.get_string(parsed, "scheme", "") == "cd"
            # SourceProvideFile should use a callback to ask user for 1st media
            if Pkg.SourceProvideFile(
                OnlineUpdate.cd_source,
                1,
                "/media.1/media"
              ) == nil
              add_new_source = false
            end
          end
        end

        Pkg.SourceSaveAll if add_new_source
      end

      if OnlineUpdate.restart_yast
        if FileUtils.Exists(Ops.add(Directory.vardir, "/selected_patches.ycp"))
          Popup.Message(OnlineUpdate.restart_message)
        end
        OnlineUpdate.restart_yast = false
      end
      if OnlineUpdate.reboot_needed
        if Ops.greater_than(Builtins.size(OnlineUpdate.reboot_packages), 0)
          Popup.Message(
            Builtins.sformat(
              OnlineUpdate.reboot_message_list,
              Builtins.mergestring(OnlineUpdate.reboot_packages, "\n")
            )
          )
        else
          Popup.Message(OnlineUpdate.reboot_message)
        end
      elsif OnlineUpdate.relogin_needed
        Popup.Message(OnlineUpdate.relogin_message)
      end

      :next
    end

    # command line handler for CD update (=> add CD as installation source)
    def CDUpdateHandler(options)
      options = deep_copy(options)
      OnlineUpdate.cd_update = true
      if Ops.get_string(options, "cd_url", "") != ""
        OnlineUpdate.cd_url = Ops.get_string(options, "cd_url", "")
      end
      if Ops.get_string(options, "cd_directory", "") != ""
        OnlineUpdate.cd_directory = Ops.get_string(options, "cd_directory", "")
      end
      OnlineUpdateSequence()
      true
    end

    # command line handler for "Simple Mode": simple package selector
    def SimpleModeHandler(options)
      options = deep_copy(options)
      OnlineUpdate.simple_mode = true
      OnlineUpdateSequence()
      true
    end
  end
end

Yast::OnlineUpdateClient.new.main
