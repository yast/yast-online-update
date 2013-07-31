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
# Summary:	Data used for Online Update
# Authors:	Gabriele Strattner <gs@suse.de>
#		Stefan Schubert <schubi@suse.de>
require "yast"

module Yast
  class OnlineUpdateClass < Module
    def main
      textdomain "online-update"

      # flag: do we do an update from CD?
      @cd_update = false

      # no. of temporary source
      @cd_source = -1

      # if yast should be restarted after installation of patches
      @restart_yast = false

      # if patch requiring relogin was installed
      @relogin_needed = false

      # popup message
      @relogin_message = _(
        "At least one of the updates installed requires restart of the session.\nLog out and in again as soon as possible.\n"
      )

      # popup message
      @restart_message = _(
        "Packages for package management were updated.\nFinishing and restarting now."
      )

      # if patch with reboot flag was installed
      @reboot_needed = false

      # packages that are requiring reboot
      @reboot_packages = []

      # popup message
      @reboot_message = _(
        "At least one of the updates installed requires a system reboot to function\nproperly. Reboot the system as soon as possible."
      )

      # popup message
      @reboot_message_list = _(
        "These updates require a system reboot to function properly:\n" +
          "\n" +
          "%1.\n" +
          "\n" +
          "Reboot the system as soon as possible."
      )

      # continue/cancel popup text
      @more_selected_message = _(
        "There are patches for package management available that require a restart of YaST.\n" +
          "They should be installed first and all other patches after the restart.\n" +
          "\n" +
          "You selected some other patches to be installed now.\n" +
          "\n" +
          "Continue with installing your selection?"
      )

      # If simple Package selector should be opened
      @simple_mode = false

      # Default URL for Patch CD source
      @cd_url = "cd:///"

      # Default Patch CD source directory
      @cd_directory = "patches"

      # If release notes should be shown after update
      @show_release_notes = false 

      # EOF
    end

    publish :variable => :cd_update, :type => "boolean"
    publish :variable => :cd_source, :type => "integer"
    publish :variable => :restart_yast, :type => "boolean"
    publish :variable => :relogin_needed, :type => "boolean"
    publish :variable => :relogin_message, :type => "string"
    publish :variable => :restart_message, :type => "string"
    publish :variable => :reboot_needed, :type => "boolean"
    publish :variable => :reboot_packages, :type => "list <string>"
    publish :variable => :reboot_message, :type => "string"
    publish :variable => :reboot_message_list, :type => "string"
    publish :variable => :more_selected_message, :type => "string"
    publish :variable => :simple_mode, :type => "boolean"
    publish :variable => :cd_url, :type => "string"
    publish :variable => :cd_directory, :type => "string"
    publish :variable => :show_release_notes, :type => "boolean"
  end

  OnlineUpdate = OnlineUpdateClass.new
  OnlineUpdate.main
end
