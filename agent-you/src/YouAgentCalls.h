/*************************************************************
 *
 *     YaST2      SuSE Labs                        -o)
 *     --------------------                        /\\
 *                                                _\_v
 *           www.suse.de / www.suse.com
 * ----------------------------------------------------------
 *
 * File:	  YouAgentCalls.h
 *
 * Author: 	  Stefan Schubert <schubi@suse.de>
 *
 * Description:   You agent calls
 *
 * $Header$
 *
 *************************************************************/

#ifndef YouAgentCalls_h
#define YouAgentCalls_h

#include <locale.h>
#include <libintl.h>
#include <stdlib.h>
#include <config.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <fstream.h>
#include <sys/types.h>
#include <utime.h>
#include <iostream.h>
#include <fcntl.h>


#include "YouAgent.h"
#include <scr/SCRAgent.h>
#include <scr/SCRInterpreter.h>
#include <Y2.h>
#include <YCP.h>
#include <ycp/y2log.h>
#include <pkg/ConfigFile.h>
#include "PatchInfo.h"

#define PATHLEN		256
#define ROOTPATH 	"rootpath"
#define YASTPATH 	"yastpath"
#define LANGUAGE 	"language"
#define FTPSERVER 	"ftpserver"
#define PATCHPATH 	"patchpath"
#define DISTRIBUTION 	"Distribution_Version"
#define TIME		"Time"
#define STATUS		"Status"
#define STAT		"status"
#define DAYS		"days"
#define PATCHES 	"patches"
#define OK		"ok"
#define MESSAGE 	"message"
#define CONTINUE 	"continue"
#define CHECKPATCH	"checkpatch"
#define PROGRESS 	"progress"
#define NEW		"new"
#define DELETED		"deleted"
#define LOAD		"load"
#define ERROR		"error"
#define INSTALLED	"installed"
#define DESCRIPTION	"description"
#define PACKAGES	"packages"
#define ARCHITECTURE	"architecture"
#define MINYAST2VERSION	"minYaST2Version"
#define PATCH		"/patches"
#define NEXTPACKAGE	"nextPackage"
#define NEXTSERVER	"nextServer"
#define NEXTSERVERKIND	"nextServerKind"
#define NEXTPACKAGESIZE "nextPackageSize"
#define NEXTSERIE	"nextSerie"
#define OTHERS		"others"
#define OPTIONAL	"optional"
#define NAME		"name"
#define PATH		"path"
#define KIND		"kind"
#define FTPADRESS		"ftp://"
#define HTTPADRESS		"http://"
#define PRODUCTNAME	"Product_Name"
#define PRODUCTVERSION	"Product_Version"
#define PRODUKTNAME     "produktname"
#define PRODUKTVERSION  "produktversion"
#define DIRECTORY	"directory"
#define NOTARX 		"Notarx"
#define NOBACKUP 	"Nobackup"
#define DEFAULTINSTSRCFTP "DefaultInstsrcFTP"
#define PROHIBITED 	"Prohibited"
#define DISTRIBUTIONVERSION "Distribution_Version"
#define DISTRIBUTIONNAME "Distribution_Name"
#define BASESYSTEM "Basesystem"
#define DISTRIBUTIONRELEASE "Distribution_Release"
#define TRUE		"true"
#define FALSE		"false"
#define SCRIPT		"script"
#define SERVER		"server"
#define PROXY		"proxy"
#define TIMEOUT		"timeout"
#endif // YouAgentCalls_h
/*--------------------------- EOF -------------------------------------------*/
