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

# Package:	online-update
# Summary:	Call YOU during installation
# Authors:	Arvin Schnell <arvin@suse.de>
module Yast
  class InstYouClient < Client
    def main
      Yast.import "Pkg"

      textdomain "online-update"

      Yast.import "Directory"
      Yast.import "FileUtils"
      Yast.import "GetInstArgs"
      Yast.import "Internet"
      Yast.import "Label"
      Yast.import "Mode"
      Yast.import "OnlineUpdate"
      Yast.import "OnlineUpdateCallbacks"
      Yast.import "PackageCallbacksInit"
      Yast.import "PackagesUI"
      Yast.import "Popup"
      Yast.import "ProductControl"
      Yast.import "ProductFeatures"
      Yast.import "Stage"
      Yast.import "Wizard"
      Yast.import "Installation"

      @saved_path = Ops.add(Directory.vardir, "/selected_patches.ycp")
      @restarted_path = Ops.add(Directory.vardir, "/continue_you")

      # Called backwards
      if GetInstArgs.going_back
        Builtins.y2milestone("going_back -> returning `auto")
        return :auto
      end

      @after_restart = false
      @after_reboot = false
      if FileUtils.Exists(@restarted_path)
        @action = Convert.to_string(
          SCR.Read(path(".target.ycp"), @restarted_path)
        )
        Builtins.y2milestone("installation restarted from YOU (%1)", @action)
        @after_restart = true
        @after_reboot = @action == "reboot"
        SCR.Execute(path(".target.remove"), @restarted_path)
      end

      if !Internet.do_you && !@after_restart # nothing to do
        Builtins.y2milestone("Internet::do_you is false -> `auto")
        return :auto
      end

      @already_up = false
      @already_up = Internet.Status if !Mode.test

      @i_set_demand = false

      if !@already_up
        Wizard.SetContents(_("Initializing ..."), Empty(), "", true, true)

        if !Internet.demand
          Internet.SetDemand(true)
          @i_set_demand = true
        end

        Internet.Start("")

        @i = 150
        while Ops.greater_than(@i, 0)
          break if !Internet.Status

          break if Internet.Connected

          # ping anything (www.suse.com) to trigger dod connections
          SCR.Execute(
            path(".target.bash_background"),
            "/bin/ping -c 1 -w 1 195.135.220.3"
          )

          Builtins.sleep(1000)
          @i = Ops.subtract(@i, 1)
        end
        if Ops.less_than(@i, 1)
          Builtins.y2warning(
            "Internet::Status timed out, no connection available?"
          )
        end
      end

      @ret = :auto

      # initalize package callbacks
      PackageCallbacksInit.InitPackageCallbacks

      if @after_restart || Hack("init-target-and-sources")
        Pkg.TargetInit(Installation.destdir, false) # reinitialize target after release notes were read (#232247)
      else
        Pkg.TargetFinish
        Pkg.TargetInitialize(Installation.destdir)
        Pkg.TargetLoad
      end
      # source data are cleared in registration client, and
      # inst_ask_online_update.ycp may not be called
      Pkg.SourceStartManager(true)

      @selected = 0
      @check_licenses = false
      @reboot_needed = false
      @normal_patches_selected = false

      # check if YaST was restarted
      if FileUtils.Exists(@saved_path)
        @saved = Convert.to_string(SCR.Read(path(".target.ycp"), @saved_path))
        # saved value actually is not needed during installation
        Builtins.y2milestone("previous action of YOU: %1", @saved)
        SCR.Execute(path(".target.remove"), @saved_path)
      end

      # save solver flags
      @solver_flags_backup = Pkg.GetSolverFlags
      # ignore reccomends when we weant only pkg management patches
      Pkg.SetSolverFlags(
        { "ignoreAlreadyRecommended" => true, "onlyRequires" => true }
      )
      # first solver run, so preselecting works well
      Pkg.PkgSolve(true)

      # select the patches affecting pkg management
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
      # solver must be run after preselction (bnc#474601)
      @solved = Pkg.PkgSolve(true)

      if Ops.less_than(@selected, 1) && @after_reboot
        Builtins.y2milestone(
          "no patch available after reboot, skiping inst_you"
        )
        return :auto
      end

      # run package selector to allow user interaction
      if !@solved ||
          ProductFeatures.GetBooleanFeature("globals", "manual_online_update") ||
          Hack("ui")
        Pkg.TargetInitDU([]) # init DiskUsage counter (#197497)
        @ret_sel = nil
        begin
          @ret_sel = PackagesUI.RunPackageSelector({ "mode" => :youMode })
          if @ret_sel == :cancel
            Builtins.y2milestone("package selector canceled -> `next")
            Pkg.SetSolverFlags(@solver_flags_backup)
            return :next
          end
          if @ret_sel == :accept
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
            if @restart_yast && @normal_patches_selected &&
                !Popup.ContinueCancel(OnlineUpdate.more_selected_message)
              # user wants to re-examine the selection
              @ret_sel = nil
            end
          end
        end until @ret_sel == :cancel || @ret_sel == :accept
      elsif Ops.greater_than(@selected, 0)
        @check_licenses = true
      end

      @more_patches_needed = false
      Builtins.foreach(Pkg.ResolvableProperties("", :patch, "")) do |patch|
        if Ops.get_symbol(patch, "status", :none) == :selected
          Builtins.y2milestone("selected patch: %1", patch)
          if Ops.get_boolean(patch, "affects_pkg_manager", false)
            if Ops.get_boolean(patch, "reboot_needed", false)
              @ret = :reboot
            elsif @ret != :reboot
              @ret = :restart_same_step
            end
          elsif Ops.get_boolean(patch, "reboot_needed", false)
            # patch requiring reboot should be installed in this run
            @ret = :reboot
          else
            @normal_patches_selected = true
          end
        # patch not selected: touch saved_path to force the restart
        elsif Ops.get_boolean(patch, "is_needed", false)
          Builtins.y2milestone("patch needed but not selected: %1", patch)
          @more_patches_needed = true
        end
      end

      # tell the caller (/sbin/yast2) to call online update again
      if @ret != :auto && @more_patches_needed
        @save_message = @ret == :reboot ? "reboot" : "restart"
        SCR.Write(
          path(".target.ycp"),
          @saved_path,
          Ops.add("inst_", @save_message)
        )
      end
      # no packagemanager/reboot patch selected ...
      if @ret == :auto
        # ... and nothing to install -> skip the installation at all
        if !@normal_patches_selected &&
            !Pkg.IsAnyResolvable(:package, :to_install) &&
            !Pkg.IsAnyResolvable(:package, :to_remove)
          Builtins.y2milestone("no patch selected after all -> `next")
          Pkg.SetSolverFlags(@solver_flags_backup)
          return :next
        end
      end

      # if the package selector was not opened, ask to confirm licenses
      if @check_licenses
        @rejected = false
        Builtins.foreach(Pkg.GetPackages(:selected, true)) do |p|
          license = Pkg.PkgGetLicenseToConfirm(p)
          if license != nil && license != ""
            rt_license = Builtins.sformat("<p><b>%1</b></p>\n%2", p, license)
            if !Popup.AnyQuestionRichText(
                # popup heading, with rich text widget and Yes/No buttons
                _("Do you accept this license agreement?"),
                rt_license,
                70,
                20,
                Label.YesButton,
                Label.NoButton,
                :focus_none
              )
              Builtins.y2milestone("License not accepted: %1", p)
              Pkg.PkgTaboo(p)
              @rejected = true
            else
              Pkg.PkgMarkLicenseConfirmed(p)
            end
          end
        end
        # we must run solver again and offer manual intervention if it fails
        if @rejected && !Pkg.PkgSolve(true)
          @ret_sel = PackagesUI.RunPackageSelector({ "mode" => :youMode })
          if @ret_sel == :cancel
            Builtins.y2milestone("package selector canceled -> `next")
            Pkg.SetSolverFlags(@solver_flags_backup)
            return :next
          end
        end
      end

      # install the patches
      OnlineUpdateCallbacks.RegisterOnlineUpdateCallbacks
      WFM.call("online_update_install")

      if @ret == :reboot
        # message popup
        Popup.Message(
          _(
            "Some application requiring restart has been updated. The system will\nreboot now then continue the installation.\n"
          )
        )
        if @more_patches_needed
          Builtins.y2milestone(
            "there are more patches to install after reboot..."
          )
        end
        # after reboot, run the online update again
        # (even if no patches remain, to init Pkg before going on (see bnc#672966)
        @ret = :reboot_same_step
        SCR.Write(path(".target.ycp"), @restarted_path, "reboot")
      elsif @ret == :restart_same_step
        Popup.Message(OnlineUpdate.restart_message)
        if @more_patches_needed
          Builtins.y2milestone(
            "restarting YaST, expecting another patches in the next YOU run"
          )
          SCR.Write(path(".target.ycp"), @restarted_path, "restart")
        else
          Builtins.y2milestone(
            "restarting YaST, but there are no patches for the next YOU run"
          )
          @ret = :restart_yast
        end
      end

      if !@already_up
        Internet.Stop("")

        Internet.SetDemand(false) if @i_set_demand
      end
      Builtins.y2milestone("result of inst_you: %1", @ret)
      Pkg.SetSolverFlags(@solver_flags_backup)
      @ret
    end

    # I have a feeling that we may need a lot of hacks here
    # to make things work
    def Hack(what)
      hack = SCR.Read(path(".target.size"), Ops.add("/tmp/hack-", what)) != -1
      Builtins.y2milestone("HACK: %1", what) if hack
      hack
    end
  end
end

Yast::InstYouClient.new.main
