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

# Package:	Online update
# Summary:	Selection dialog
# Authors:	Cornelius Schumacher <cschum@suse.de>
#
# Call the PackageSelector in YOU mode.
module Yast
  class OnlineUpdateSelectClient < Client
    def main
      Yast.import "Pkg"
      Yast.import "UI"

      textdomain "online-update"

      Yast.import "Directory"
      Yast.import "FileUtils"
      Yast.import "Label"
      Yast.import "OnlineUpdate"
      Yast.import "Popup"
      Yast.import "ProductFeatures"
      Yast.import "Wizard"
      Yast.import "PackageSystem"
      Yast.import "Report"
      Yast.import "OnlineUpdateDialogs"

      if OnlineUpdate.cd_update
        @canceled = false
        begin
          @initMessage = _("Initializing for CD update...")
          UI.OpenDialog(Opt(:decorated), Label(@initMessage))
          OnlineUpdate.cd_source = Pkg.SourceCreate(
            OnlineUpdate.cd_url,
            OnlineUpdate.cd_directory
          )
          UI.CloseDialog

          if OnlineUpdate.cd_source == -1
            @canceled = !Popup.AnyQuestion(
              "",
              # error popup: cancel/retry buttons follow
              _(
                "Initialization failed. Check that\nyou have inserted the correct CD.\n"
              ),
              Label.RetryButton,
              Label.CancelButton,
              :focus_yes
            )
          end
        end while OnlineUpdate.cd_source == -1 && !@canceled

        return :abort if @canceled
      end

      @restart_yast = false
      @reboot_needed = false
      @relogin_needed = false
      @normal_patches_selected = false
      @selected = 0
      @saved_path = Ops.add(Directory.vardir, "/selected_patches.ycp")
      @reboot_packages = []

      # check if YaST was restarted
      if FileUtils.Exists(@saved_path)
        @saved = Convert.to_string(SCR.Read(path(".target.ycp"), @saved_path))
        # before restart, patch requiring reboot was installed:
        # show the reboot popup now
        if @saved != nil && Ops.is_string?(@saved) && @saved == "reboot"
          @reboot_needed = true
        end
        SCR.Execute(path(".target.remove"), @saved_path)
      end

      @solver_flags_backup = Pkg.GetSolverFlags
      # ignore reccomends when we weant only pkg management patches
      Pkg.SetSolverFlags(
        { "ignoreAlreadyRecommended" => true, "onlyRequires" => true }
      )
      Pkg.PkgSolve(true)

      # 1st, select the patches affecting pkg management
      @selected = Pkg.ResolvablePreselectPatches(:affects_pkg_manager)
      Builtins.y2milestone(
        "Preselected patches for pkg management: %1",
        @selected
      )

      # if no patch is selected, pre-select all security and recommended
      if Ops.less_than(@selected, 1)
        # restore the original solver settings - enable recommends
        Pkg.SetSolverFlags(@solver_flags_backup)
        # new solver run required to include recommends
        Pkg.PkgSolve(true)

        @selected = Pkg.ResolvablePreselectPatches(:all)
        Builtins.y2milestone("All preselected patches: %1", @selected)
      end

      Wizard.ClearContents # do not show the remnant of initial progress

      @display_support_status = ProductFeatures.GetBooleanFeature(
        "software",
        "display_support_status"
      )
      @widget_options = Opt(:youMode)
      if @display_support_status == true
        @widget_options = Builtins.add(@widget_options, :confirmUnsupported)
      end
      @widget_options = Builtins.add(@widget_options, :repoMgr)

      @simple_mode = OnlineUpdate.simple_mode &&
        UI.HasSpecialWidget(:SimplePatchSelector) == true
      if @simple_mode
        UI.OpenDialog(
          Opt(:defaultsize),
          term(:SimplePatchSelector, Id(:selector))
        )
      else
        UI.OpenDialog(
          Opt(:defaultsize),
          PackageSelector(Id(:selector), @widget_options)
        )
      end

      @ret = nil
      @current = "simple"

      begin
        @ret = Convert.to_symbol(UI.RunPkgSelection(Id(:selector)))
        Builtins.y2milestone("RunPkgSelection returned %1", @ret)

        # FATE#312509: Show if patch needs a reboot and offer
        # to delay the patch installation
        if @ret == :accept
          if ! OnlineUpdateDialogs.validate_selected_patches
            @ret = nil
            next
          end
        end

        UI.CloseDialog

        if @ret == :details
          UI.OpenDialog(
            Opt(:defaultsize),
            PackageSelector(Id(:selector), @widget_options)
          )
          @current = "rich"
        end
        if @ret == :cancel && @simple_mode && @current == "rich"
          UI.OpenDialog(
            Opt(:defaultsize),
            term(:SimplePatchSelector, Id(:selector))
          )
          @current = "simple"
          @ret = nil
        end
        if @ret == :online_update_configuration || @ret == :repo_mgr
          @result = nil
          if @ret == :online_update_configuration
            @required_package = "yast2-online-update-configuration"

            if !PackageSystem.Installed(@required_package) &&
                !PackageSystem.CheckAndInstallPackages([@required_package])
              Report.Error(
                Builtins.sformat(
                  _(
                    "Cannot configure online update repository \nwithout having package %1 installed"
                  ),
                  @required_package
                )
              )
            else
              Builtins.y2milestone(
                "starting online_update_configuration client"
              )
              @cfg_result = Convert.to_symbol(
                WFM.CallFunction("online_update_configuration", [])
              )
              Builtins.y2milestone(
                "online_update_configuration result: %1",
                @cfg_result
              )
            end
          else
            @result = WFM.CallFunction("repositories", [:sw_single_mode])
          end

          if @result == :next || @result == :finish
            Pkg.SetSolverFlags(
              { "ignoreAlreadyRecommended" => true, "onlyRequires" => true }
            )
            Pkg.PkgSolve(true)
            # select the patches affecting pkg management
            @selected = Pkg.ResolvablePreselectPatches(:affects_pkg_manager)
            Builtins.y2milestone("patches for pkg management: %1", @selected)
            if Ops.less_than(@selected, 1)
              Pkg.SetSolverFlags(@solver_flags_backup)
              Pkg.PkgSolve(true)
              @selected = Pkg.ResolvablePreselectPatches(:all)
              Builtins.y2milestone("preselected patches: %1", @selected)
            end
          end
          # open the dialog with patch view again
          UI.OpenDialog(
            Opt(:defaultsize),
            PackageSelector(Id(:selector), @widget_options)
          )
        end

        if @ret == :accept
          @restart_yast = false
          @normal_patches_selected = false
          Builtins.foreach(Pkg.ResolvableProperties("", :patch, "")) do |patch|
            if Ops.get_symbol(patch, "status", :none) == :selected
              if Ops.get_boolean(patch, "affects_pkg_manager", false)
                @restart_yast = true
              else
                @normal_patches_selected = true
              end
            end
          end
          if @restart_yast && @normal_patches_selected
            if !Popup.ContinueCancel(OnlineUpdate.more_selected_message)
              @ret = nil
              UI.OpenDialog(
                Opt(:defaultsize),
                PackageSelector(Id(:selector), @widget_options)
              )
            end
          end
        end
      end until @ret == :cancel || @ret == :accept

      Wizard.ClearContents

      Builtins.y2milestone("RunPkgSelection finally returned '%1'", @ret)

      # restore the original solver settings, just to be sure...
      Pkg.SetSolverFlags(@solver_flags_backup)

      return :abort if @ret == :cancel

      @more_patches_needed = false
      Builtins.foreach(Pkg.ResolvableProperties("", :patch, "")) do |patch|
        if Ops.get_symbol(patch, "status", :none) == :selected
          Builtins.y2milestone("selected patch: %1", patch)

          # check if release notes package was selected for update
          # if so, user should see new release notes (fate#314072)
          Builtins.foreach(Ops.get_map(patch, "contents", {})) do |name, version|
            if !OnlineUpdate.show_release_notes &&
                Builtins.issubstring(name, "release-notes")
              Builtins.foreach(Pkg.ResolvableProperties(name, :package, "")) do |package|
                if Ops.get_symbol(package, "status", :none) == :selected
                  OnlineUpdate.show_release_notes = true
                  Builtins.y2milestone(
                    "release notes package '%1' (%2) selected for installation",
                    name,
                    version
                  )
                end
              end
            end
          end

          if Ops.get_boolean(patch, "reboot_needed", false)
            @reboot_needed = true
            @reboot_packages = Convert.convert(
              Builtins.union(
                @reboot_packages,
                [Ops.get_string(patch, "name", "")]
              ),
              :from => "list",
              :to   => "list <string>"
            )
          end
          if Ops.get_boolean(patch, "relogin_needed", false)
            @relogin_needed = true
          end
          if Ops.get_boolean(patch, "affects_pkg_manager", false)
            @restart_yast = true
          else
            @normal_patches_selected = true
          end
        # patch not selected: bug #188541 - touch saved_path to force the restart
        elsif Ops.get_boolean(patch, "is_needed", false)
          Builtins.y2milestone("patch needed but not selected: %1", patch)
          @more_patches_needed = true
        end
      end

      # tell the caller (/sbin/yast2) to call online update again
      if @restart_yast && @more_patches_needed
        # what if kernel (=reboot) and zypp (=restart) are installed together?
        # => restart YaST and show reboot message after the second run
        @save_message = "restart"
        @save_message = "reboot" if @reboot_needed
        SCR.Write(path(".target.ycp"), @saved_path, @save_message)
        # show reboot popup only when yast is not going to restart now
        @reboot_needed = false
      elsif @restart_yast
        Builtins.y2debug("nothing left for the second run, no restart needed")
      end

      # no patch selected
      if !@restart_yast && !@normal_patches_selected &&
          !Pkg.IsAnyResolvable(:package, :to_install) &&
          !Pkg.IsAnyResolvable(:package, :to_remove)
        # run commit to save changes others than selecting package (bnc#811237)
        Pkg.PkgCommit(0)
        @ret = :cancel
      end
      OnlineUpdate.restart_yast = @restart_yast
      OnlineUpdate.reboot_needed = @reboot_needed
      OnlineUpdate.reboot_packages = deep_copy(@reboot_packages)
      OnlineUpdate.relogin_needed = @relogin_needed

      if @ret == :cancel
        return :abort
      else
        return :next
      end
    end
  end
end

Yast::OnlineUpdateSelectClient.new.main
