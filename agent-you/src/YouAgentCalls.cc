/*************************************************************
 *
 *     YaST2      SuSE Labs                        -o)
 *     --------------------                        /\\
 *                                                _\_v
 *           www.suse.de / www.suse.com
 * ----------------------------------------------------------
 *
 * File:	  YouAgentCalls.cc
 *
 * Author: 	  Stefan Schubert <schubi@suse.de>
 *
 * Description:   You agent calls
 *
 * $Header$
 *
 *************************************************************/


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

/*-------------------------------------------------------------*/
/* Create all parent directories of @param name, as necessary  */
/*-------------------------------------------------------------*/
void YouAgent::create_directories(string name)
{
    if ( mainscragent )
    {
	YCPPath path = ".target.mkdir";
	YCPString value ( name );
	YCPValue ret = mainscragent->Execute( path, value );

	if ( ret->isBoolean() )	// success
	{
	    if (  ret->asBoolean()->value() )
	    {
		y2milestone( "Path %s created", name.c_str () );
	    }
	    else
	    {
		y2error( "Cannot create path %s", name.c_str() );
	    }
	}
	else
	{
	    y2error("<.target.mkdir> System agent returned nil.");
	}
    }
    else
    {
	y2error("No system agent installed");
    }
}


/***************************************************************************
 * Public Member-Functions						   *
 ***************************************************************************/

/*--------------------------------------------------------------------------*
 * Initialize the agentr for you-update with the environment
 * delivered by the setMap:
 * $[
 *   "language":"german",
 *	"architecture":"i386",
 *	"rootpath":"/",
 *   "yastpath":"/var/lib/YaST",
 *	"patchpath":"/var/lib/YaST/patches"]
 * Returns true or false
 *--------------------------------------------------------------------------*/

YCPValue YouAgent::setEnvironment ( YCPMap setMap )
{
   YCPBoolean ok ( true );
   YCPValue dummyValue = YCPVoid();

   y2debug( "CALLING setEnvironment");

   dummyValue = setMap->value(YCPString(ROOTPATH));
   if ( !dummyValue.isNull() && dummyValue->isString() )
   {
      rootPath = dummyValue->asString()->value();
      y2debug( "setEnvironment: rootpath = %s",rootPath.c_str() );

      if ( rootPath != "/"
	   && mainscragent )
      {
	  // targetpkg is already initialized wirth targetroot /
	   YCPPath path = ".targetpkg.dbTargetPath";
	   
	   YCPValue ret = mainscragent->Write( path,
					YCPString( rootPath ) );

	   if ( ret->isBoolean() )	// success
	   {
	       if (  ret->asBoolean()->value() )
	       {
		   y2milestone( "Set agent to %s",
				rootPath.c_str() );
	       }
	       else
	       {
		   y2error( "Cannot set rootpatch" );
		   ok = false;
	       }
	   }
	   else
	   {
	       y2error("<.targetpkg.dbTargetPath> System agent returned nil.");
	       ok = false;
	   }
      }
   }

   dummyValue = setMap->value(YCPString(YASTPATH));
   if ( !dummyValue.isNull() && dummyValue->isString() )
   {
      yastPath = dummyValue->asString()->value();
      y2debug( "SetEnvironment: yastpath = %s",yastPath.c_str() );
   }
   else
   {
      yastPath = "";
      y2error("YASTPATH not found" );
      ok = false;
   }

   dummyValue = setMap->value(YCPString(LANGUAGE));
   if ( !dummyValue.isNull() && dummyValue->isString() )
   {
      language = dummyValue->asString()->value();
      y2debug( "SetEnvironment: language = %s",language.c_str() );
   }
   else
   {
      language = "";
      y2error( "LANGUAGE not found" );
      ok = false;
   }

   dummyValue = setMap->value(YCPString(ARCHITECTURE));
   if ( !dummyValue.isNull() && dummyValue->isString() )
   {
      architecture = dummyValue->asString()->value();
      y2debug( "SetEnvironment: architecture = %s",architecture.c_str() );
   }
   else
   {
      architecture = "";
      y2error("ARCHITECTURE not found" );
      ok = false;
   }

   if ( ok->value() )
   {
      //evaluate current version
      ConfigFile updateInf ( yastPath + "/update.inf" );
      Entries entriesUpdateInf;
      Entries::iterator pos;
      string version;

      entriesUpdateInf.clear();
      updateInf.readFile ( entriesUpdateInf, " :" );
      pos = entriesUpdateInf.find ( DISTRIBUTION );
      if ( pos != entriesUpdateInf.end() )
      {
	 Values values = (pos->second).values;
	 Values::iterator posValues;
	 if ( values.size() >= 0 )
	 {
	    posValues = values.begin();
	    version = *posValues;
	 }
	 else
	 {
	    y2error( "Distribution_version in update.inf not found" );
	    version = "";
	 }
      }
      else
      {
	 y2error( "Distribution_version in update.inf not found" );
	 version = "";
      }

      // 7.0.0 ---> 7.0

      string::size_type last;
      last = version.find_last_of(".");
      if ( last != string::npos )
      {
	 distributionVersion = version.substr( 0, last );
      }
      else
      {
	 distributionVersion = version;
      }

      pos = entriesUpdateInf.find (PRODUCTNAME );
      if ( pos != entriesUpdateInf.end() )
      {
	 Values values = (pos->second).values;
	 Values::iterator posValues;
	 if ( values.size() >= 0 )
	 {
	    posValues = values.begin();
	    productName = *posValues;
	 }
	 else
	 {
	     y2warning( "Product_Nmae in update.inf not found" );
	     productName = "";
	 }
      }
      else
      {
	  y2warning( "Product_name in update.inf not found" );
	  productName = "";
      }

      pos = entriesUpdateInf.find ( PRODUCTVERSION );
      if ( pos != entriesUpdateInf.end() )
      {
	 Values values = (pos->second).values;
	 Values::iterator posValues;
	 if ( values.size() >= 0 )
	 {
	    posValues = values.begin();
	    productVersion = *posValues;
	 }
	 else
	 {
	    y2warning( "Product_Version in update.inf not found" );
	    productVersion = "";
	 }
      }
      else
      {
	  y2warning( "Product_Version in update.inf not found" );
	  productVersion = "";
      }

      if ( productVersion == "" ||
	   productName == "" )
      {
	  // try to read this values via  /var/lib/YaST2/infomap.ycp
	  int fd = open( "/var/lib/YaST2/infomap.ycp", O_RDONLY );

	  if ( fd >= 0 )
	  {
	      YCPParser parser( fd, "/var/lib/YaST2/infomap.ycp" );
    
	      parser.setBuffered();  
	      YCPValue contents = parser.parse();

	      close( fd );
	      
	      if ( ! contents.isNull() &&
		   contents->isMap() )
	      {
		  YCPMap dummymap = contents->asMap();
		  YCPValue dummyValue = YCPVoid();

		  dummyValue = dummymap->value(YCPString(PRODUCTNAME));
		  if ( !dummyValue.isNull() &&
		       dummyValue->isString() )
		  {
		      productName = dummyValue->asString()->value();
		  }
		  dummyValue = dummymap->value(YCPString( PRODUCTVERSION ));
		  if ( !dummyValue.isNull() &&
		       dummyValue->isString() )
		  {
		      productVersion = dummyValue->asString()->value();
		  }		  
	      }
	  }
	  if ( productVersion != "" 
	       && productName != "" )
	  {
	      y2warning( "Taking default entries from infomap.ycp %s / %s",
			 productName.c_str(), productVersion.c_str() );
	      
	      // saving into update.inf
	      ConfigFile updateInf( yastPath + "/update.inf"  );      
	      Entries entriesUpdate;
	      
	      // Reading old udpate.inf
	      Values values;
	      Element element;

	      // Creating tree
	      values.clear();
	      element.values = values;
	      element.multiLine = true;
	      entriesUpdate.insert(pair<const string, const Element>
				   ( NOTARX, element ) );
	      entriesUpdate.insert(pair<const string, const Element>
				   ( NOBACKUP, element ) ); 
	      entriesUpdate.insert(pair<const string, const Element>
				   ( DEFAULTINSTSRCFTP, element ) );
	      entriesUpdate.insert(pair<const string, const Element>
				   ( PROHIBITED, element ) );
	      element.multiLine = false;
	      entriesUpdate.insert(pair<const string, const Element>
				   ( DISTRIBUTIONVERSION, element ) );
	      entriesUpdate.insert(pair<const string, const Element>
				   ( DISTRIBUTIONNAME, element ) );
	      entriesUpdate.insert(pair<const string, const Element>
				   ( BASESYSTEM, element ) );
	      entriesUpdate.insert(pair<const string, const Element>
				   (DISTRIBUTIONRELEASE , element ) );
	      
	      updateInf.readFile ( entriesUpdate, ":" );
	      
	      // changing whitespaces with -
	      string::size_type productPos = productName.find_first_of ( " " );
	      while ( productPos != string::npos )
	      {
		  productName[productPos] = '-';
		  productPos = productName.find_first_of ( " ", productPos );
	      }

	      // PRODUCTNAME
	      values.clear();
	      values.push_back ( productName );
	      element.values = values;
	      element.multiLine = false;
	      entriesUpdate.erase( PRODUCTNAME );
	      entriesUpdate.insert(pair<const string, const Element>
				   ( PRODUCTNAME, element ) ); 

	      // PRODUCTVERSION
	      values.clear();
	      values.push_back ( productVersion );
	      element.values = values;
	      element.multiLine = false;
	      entriesUpdate.erase( PRODUCTVERSION );
	      entriesUpdate.insert(pair<const string, const Element>
				   ( PRODUCTVERSION, element ) );
	      if (  !updateInf.writeFile ( entriesUpdate,
					   "/var/lib/YaST/update.inf -- (c) 2001 SuSE GmbH",
					   ':' ) )
	      {
		  y2error ( "Error while writing the file update.inf" );
	      }
	  }
      }

      // changing whitespaces with -
      string::size_type productPos = productName.find_first_of ( " " );
      while ( productPos != string::npos )
      {
	  productName[productPos] = '-';
	  productPos = productName.find_first_of ( " ", productPos );
      }
   }

   y2debug ( "Produkt_Name : %s; Produkt_Version: %s; Distribution: %s",
	     productName.c_str(),
	     productVersion.c_str(),
	     distributionVersion.c_str() );
   
   if ( ok->value() )
   {
      dummyValue = setMap->value(YCPString(PATCHPATH));
      if ( !dummyValue.isNull() && dummyValue->isString() )
      {
	 if ( isBusiness() )
	 {
	      destPatchPath = dummyValue->asString()->value() + "/"+
		  architecture +
		  "/update/" +
		  productName + "/" +
		  productVersion;
	 }
	 else
	 {
	     destPatchPath = dummyValue->asString()->value() + "/"+
		 architecture +
		 "/update/" +
		 distributionVersion;
	 }

	 // creating destPatchPath
	 create_directories(destPatchPath + PATCH);

	 y2debug( "SetEnvironment: patchpath = %s",destPatchPath.c_str() );
      }
      else
      {
	 destPatchPath = "";
	 y2error( "PATCHPATH not found" );
	 ok = false;
      }
   }

   // evaluate Proxy entries
   if ( mainscragent )
   {
       YCPPath rcpath = ".rc.system.HTTP_PROXY_PASSWORD";
       YCPValue ret = mainscragent->Read( rcpath, YCPString("") );
       if ( ret->isString() )	// success ??
       {
	   httpProxyPassword = ret->asString()->value();
	   y2debug( "HTTP_PROXY_PASSWORD : %s",
		    httpProxyPassword.c_str() );
       }

       YCPPath rcuserpath = ".rc.system.HTTP_PROXY_USER";
       ret = mainscragent->Read( rcuserpath, YCPString("") );
       if ( ret->isString() )	// success ??
       {
	   httpProxyUser = ret->asString()->value();
	   y2debug( "HTTP_PROXY_USER : %s",
		    httpProxyUser.c_str() );	   
       }

       YCPPath path = ".http.setProxyUser";
       ret = mainscragent->Execute( path,
				    YCPString( httpProxyUser ),
				    YCPString( httpProxyPassword ));       
   }

   if ( ok->value() )
   {
      y2debug( "SetEnvironment: RETURN TRUE");
   }
   else
   {
      y2debug( "SetEnvironment: RETURN FALSE");
   }


   return ok;
}


/*--------------------------------------------------------------------------*
 * Evaluate last update-status:
 * Returns a map like :
 * $[ "days":"3","status":"ok",
 *    "patches":[ <patch-ID1>, <patch-mode1>, <patch-description1>,
 *		     <update-date1>,<status1>
 *                <patch-ID2>, <patch-mode2>, <patch-description2>,
 *		     <update-date2>,<status2>...	]]
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::lastUpdateStatus ( void )
{
   YCPMap ret;

   y2debug( "CALLING LastUpdateStatus" );

   ConfigFile lastUpdate ( destPatchPath + PATCH+ "/lastUpdate" );
   Entries entries;
   Entries::iterator pos;
   string updateTime;
   string status;

   entries.clear();
   lastUpdate.readFile ( entries, " :" );
   pos = entries.find ( TIME );
   if ( pos != entries.end() )
   {
      Values values = (pos->second).values;
      Values::iterator posValues;

      if ( values.size() >= 0 )
      {
	    posValues = values.begin();
	    updateTime = *posValues;
      }
      else
      {
	 y2error( "Update time not found" );
	 updateTime = "";
      }
   }
   else
   {
      y2error( "Update time not found" );
      updateTime = "";
   }

   if ( updateTime.size() > 0 )
   {
      long update = atol ( updateTime.c_str() );
      long current = ( long ) time ( 0 );
      char days[30];

      sprintf ( days, "%d", (int)( ( current - update ) /86400 ));
      updateTime = days;
   }
   else
   {
      updateTime = "-";
   }

   ret->add ( YCPString ( DAYS ),
	      YCPString ( updateTime ) );

   pos = entries.find ( STATUS );
   if ( pos != entries.end() )
   {
      Values values = (pos->second).values;
      Values::iterator posValues;

      if ( values.size() >= 0 )
      {
	    posValues = values.begin();
	    status = *posValues;
      }
      else
      {
	 y2error( "STATUS not found" );
	 status = "-";
      }
   }
   else
   {
      y2error( "STATUS not found" );
      status = "-";
   }

   ret->add ( YCPString ( STAT ),
	      YCPString ( status ) );

   if ( currentPatchInfo != NULL )
   {
      delete currentPatchInfo;
      currentPatchInfo = NULL;
   }

   currentPatchInfo = new  PatchInfo ( destPatchPath + PATCH, language);

   PatchList oldPatchList = currentPatchInfo->getRawPatchList();
   PatchList::iterator patchPos;
   YCPList patchList;

   for ( patchPos = oldPatchList.begin() ; patchPos != oldPatchList.end();
	 patchPos++ )
   {
      PatchElement patchElement = patchPos->second;
      patchList->add ( YCPString ( patchElement.id ) );
      patchList->add ( YCPString ( patchElement.kind ) );
      patchList->add ( YCPString ( patchElement.shortDescription ) );
      patchList->add ( YCPString ( patchElement.date.substr(0,4) + "." +
				   patchElement.date.substr(4,2) + "." +
				   patchElement.date.substr(6,2) ) );
      patchList->add ( YCPString ( patchElement.state ) );
   }

   ret->add ( YCPString ( PATCHES ),
	      patchList );

   return ret;
}


/*--------------------------------------------------------------------------*
 * setting server
 * Returns true of false
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::setServer ( YCPMap setMap )
{
   YCPBoolean ret ( true );
   YCPValue dummyValue = YCPVoid();
   
   y2debug( "CALLING setServer" );

   dummyValue = setMap->value(YCPString(NAME));
   if ( !dummyValue.isNull() && dummyValue->isString() )
   {
      server = dummyValue->asString()->value();
      y2debug( "setServer: server = %s", server.c_str() );
   }
   else
   {
      server = "";
      y2error("NAME not found" );
      ret = false;
   }

   dummyValue = setMap->value(YCPString(PATH));
   if ( !dummyValue.isNull() && dummyValue->isString() )
   {
      sourcePath = dummyValue->asString()->value();
      y2debug( "setServer: PATH = %s", sourcePath.c_str() );
   }
   else
   {
      sourcePath = "";
      y2error("PATH not found" );
      ret = false;
   }

   dummyValue = setMap->value(YCPString(KIND));
   if ( !dummyValue.isNull() && dummyValue->isString() )
   {
      string kind = dummyValue->asString()->value();
      y2debug( "setServer: KIND = %s", kind.c_str() );

      string availString[] =
	  { "ftp", "FTP", "http", "HTTP", "cd", "CD", "harddisk", "Harddisk", "local", "nfs", "Net", "" };
      ServerKind availKind[] =
	  {  FTP,   FTP,  HTTP,   HTTP,   CD,   CD,   HD,         HD,          HD,      NFS,   NFS,  NO };

      serverKind = NO;
      for ( int i = 0; availString[i] != ""; i++ )
      {
	  if ( availString[i] == kind )
	  {
	      serverKind = availKind[i];
	  }
      }
   }
   else
   {
      serverKind = NO;
   }
   
   if ( serverKind == NO )
   {
       y2error("Serverkind  not recognized" );
       ret = false;
   }   
   
   return ret;
}


/*--------------------------------------------------------------------------*
 * Connection to selected server, harddisk, CD ......
 * Returns $[ "ok": false, "message","Connection refused" ]
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::connect ( )
{
   YCPMap ret;
   bool ok = true;
   string message = "";

   y2debug( "CALLING connect" );

   if ( serverKind == NO )
   {
      return YCPError( "missing call setServer()", YCPVoid());      
   }

   localSourcePath = "";
   
   if ( serverKind == CD
	|| serverKind == NFS )
   {
       // Mounting CD or NFS to /var/adm/mount
       if ( mainscragent )
       {
	   YCPPath path = ".target.mount";
	   YCPList param;
	   string source = "";

	   if ( serverKind == CD )
	   {
	       source = "/dev/";	   
	       if ( sourcePath != "" )
	       {
		   source = source + sourcePath;
	       }
	       else
	       {
		   //default
		   source = source + "cdrom";
	       }
	   }
	   else
	   {
	       //NFS
	       source = server+ ":"+ sourcePath;
	   }

	   param->add( YCPString( source ) );
	   param->add( YCPString( "/var/adm/mount" ) );
	   param->add( YCPString( "/var/log/y2mountlog") );
	   
	   YCPValue ret = mainscragent->Execute( path, param );

	   if ( ret->isBoolean() )	// success
	   {
	       if (  ret->asBoolean()->value() )
	       {
		   y2milestone( "%s to /var/adm/mount mounted",
				source.c_str () );
		   localSourcePath = "/var/adm/mount";		   
	       }
	       else
	       {
		   y2error( "Cannot mount %s to /var/adm/mount",
			    source.c_str() );
		   ok = false;
	       }
	   }
	   else
	   {
	       y2error("<.target.mount> System agent returned nil.");
	       ok = false;
	   }
       }
       else
       {
	   y2error("No system agent installed");
	   ok = false;
       }
   }
   else if ( serverKind == FTP )
   {
      // connecting via ftp
       if ( mainscragent )
       {
	   string proxy = "";
	   ok = false;
	   YCPPath rcpath = ".rc.system.FTP_PROXY";
	   YCPValue ret = mainscragent->Read( rcpath, YCPString("") );
	   if ( ret->isString() )	// success ??
	   {
	       proxy = ret->asString()->value();
	   }

	   YCPPath path = ".ftp.connect_to";
	   if ( proxy.length() > 0 )
	   {
	       // trying to connect with proxy
	       message = "Trying to connect via proxy : " + proxy + "\n";
	       y2debug ( "%s", message.c_str() );
	       YCPMap value;

	       value->add( YCPString ( SERVER ), YCPString( server ));
	       value->add( YCPString ( PROXY ), YCPString( proxy ));
	       value->add( YCPString ( TIMEOUT ), YCPInteger(1));
	   
	       ret = mainscragent->Execute( path, value );

	       if ( ret->isMap() )	// success ??
	       {
		   YCPMap retMap = ret->asMap();
		   YCPValue dummyValue = YCPVoid();

		   dummyValue = retMap->value(YCPString(OK));
		   if ( !dummyValue.isNull() && dummyValue->isBoolean() )
		   {
		       ok = dummyValue->asBoolean()->value();
		   }
		   dummyValue = retMap->value(YCPString(MESSAGE));
		   if ( !dummyValue.isNull() && dummyValue->isString() )
		   {
		       message += dummyValue->asString()->value() + "\n";
		   }
		   if ( ok )
		   {
		       y2debug( "%s connected", proxy.c_str() );
		   }
		   else
		   {
		       y2error( "proxy %s not connected (%s)",
				proxy.c_str(),
				message.c_str() );
		       message += "connect to " + server + " directly ";
		   }
	       }
	       else
	       {
		   y2error("<.ftp.connect_to> System agent returned nil.");
		   ok = false;
	       }
	   }
	   if ( !ok )
	   {
	       // not over proxy
	       YCPMap value;

	       value->add( YCPString ( SERVER ), YCPString( server ));
	       value->add( YCPString ( TIMEOUT ), YCPInteger(5));
	   
	       ret = mainscragent->Execute( path, value );

	       if ( ret->isMap() )	// success ??
	       {
		   YCPMap retMap = ret->asMap();
		   YCPValue dummyValue = YCPVoid();

		   dummyValue = retMap->value(YCPString(OK));
		   if ( !dummyValue.isNull() && dummyValue->isBoolean() )
		   {
		       ok = dummyValue->asBoolean()->value();
		   }
		   dummyValue = retMap->value(YCPString(MESSAGE));
		   if ( !dummyValue.isNull() && dummyValue->isString() )
		   {
		       message += dummyValue->asString()->value() + "\n";
		   }
		   if ( ok )
		   {
		       y2debug( "%s connected", server.c_str() );
		   }
		   else
		   {
		       y2error( "server %s not connected (%s)",
				server.c_str(),
				message.c_str() );
		   }
	       }
	       else
	       {
		   y2error("<.ftp.connect_to> System agent returned nil.");
		   ok = false;
	       }	       
	   }
       }
       else
       {
	   y2error("No system agent installed");
	   ok = false;
       }
   }
   else if ( serverKind == HD )
   {
      localSourcePath = sourcePath;       
   }   

   ret->add ( YCPString ( MESSAGE ), YCPString( message ));
   ret->add ( YCPString ( OK ), YCPBoolean ( ok ) );

   return ret;
}



/*--------------------------------------------------------------------------*
 * Disconnection from selected server
 * Returns $[ "ok": false, "message","Connection refused" ]
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::disconnect ( )
{
   YCPMap ret;
   bool ok = true;
   string message = "Disconnected";

   y2debug( "CALLING Disconnect" );
   
   if ( serverKind == NO )
   {
      return YCPError( "missing call setServer()", YCPVoid());             
   }

   if ( serverKind == CD
	|| serverKind == NFS )
   {
       // Unmounting  /var/adm/mount
       if ( mainscragent )
       {
	   YCPPath path = ".target.umount";
	   
	   YCPValue ret = mainscragent->Execute( path,
					YCPString( "/var/adm/mount" ) );

	   if ( ret->isBoolean() )	// success
	   {
	       if (  ret->asBoolean()->value() )
	       {
		   y2milestone( "/var/adm/mount unmounted");
	       }
	       else
	       {
		   y2error( "Cannot unmount /var/adm/mount" );
		   ok = false;
	       }
	   }
	   else
	   {
	       y2error("<.target.umount> System agent returned nil.");
	       ok = false;
	   }
       }
       else
       {
	   y2error("No system agent installed");
	   ok = false;
       }
   }
   else
   {
       // connected via ftp
       if ( mainscragent )
       {
	   YCPPath path = ".ftp.disconnect";
	   YCPValue ret = mainscragent->Execute( path, YCPString( "" ) );

	   if ( ret->isMap() )	// success ??
	   {
	       YCPMap retMap = ret->asMap();
	       YCPValue dummyValue = YCPVoid();

	       dummyValue = retMap->value(YCPString(OK));
	       if ( !dummyValue.isNull() && dummyValue->isBoolean() )
	       {
		   ok = dummyValue->asBoolean()->value();
	       }
	       dummyValue = retMap->value(YCPString(MESSAGE));
	       if ( !dummyValue.isNull() && dummyValue->isString() )
	       {
		   message = dummyValue->asString()->value();
	       }
	       if ( ok )
	       {
		   y2debug( "disconnected" );
	       }
	       else
	       {
		   y2error( "error while disconnecting" );
	       }
	   }
	   else
	   {
	       y2error("<.ftp.disconnect> System agent returned nil.");
	       ok = false;
	   }
       }
       else
       {
	   y2error("No system agent installed");
	   ok = false;
       }
   }

   ret->add ( YCPString ( OK ), YCPBoolean ( ok ) );
   ret->add ( YCPString ( MESSAGE ),
	      YCPString ( message ) );

   return ret;
}


/*--------------------------------------------------------------------------*
 * Read the patch-list from the server
 * Returns $[ "ok": false, "message","not connected",
 *            "continue": false, "progress", 10]
 *--------------------------------------------------------------------------*/

// don't move into function, g++ bug
static YCPList dirList;
static int posDirList;
static string patchDirectory;

YCPValue YouAgent::getPatchList (  )
{
   YCPMap ret;
   bool ok = true;
   bool cont = true;
   string message = "";
   string checkpatch = "";
   int progress = 0;
   static int counter;

   y2debug( "CALLING GetPatchList" );

   if ( serverKind == NO )
   {
      return YCPError( "missing call setServer()", YCPVoid());
   }
   
   if ( !mainscragent )
   {
      return YCPError( "No system agent installed", YCPVoid());       
   }
   
   if ( currentPatchInfo != NULL )
   {
      delete currentPatchInfo;
      currentPatchInfo = NULL;
   }

   currentPatchInfo = new  PatchInfo ( destPatchPath + PATCH, language );

   if ( serverKind == CD
	|| serverKind == HD
	|| serverKind == NFS )
   {
      // installation from local source
      message = "";
      checkpatch = "";
      cont = false;
      progress = 100;

      if ( localSourcePath.size() > 0 )
      {
	 string source = localSourcePath + PATCH;
	 bool found = false;

	 YCPPath path = ".target.size";
	 YCPString value ( source );
	 YCPValue ret = mainscragent->Execute( path, value );

	 if ( ret->isInteger() )	// success
	 {
	     if (  ret->asInteger()->value() >= 0 )
	     {
		 found = true;
	     }
	 }
	 else
	 {
	     y2error("<.target.size> System agent returned nil.");
	 }
	 
	 if ( !found )
	 {
	     // Checking other pathes
	     if ( isBusiness() )
	     {
		 source = localSourcePath + "/" +
		     architecture +
		     "/update/" +
		     productName + "/" +
		     productVersion + PATCH;
	     }
	     else
	     {
		 source = localSourcePath + "/" +
		     architecture +
		     "/update/" +
		     distributionVersion + PATCH;
	     }
	     
	     value = YCPString( source );
	     YCPValue ret = mainscragent->Execute( path, value );

	     if ( ret->isInteger() )	// success
	     {
		 if (  ret->asInteger()->value() >= 0 )
		 {
		     found = true;
		 }
	     }
	     else
	     {
		 y2error("<.target.size> System agent returned nil.");
	     }
	 }
	  
	 if ( found )
	 {
	    // path found
	    PatchInfo patchInfo ( source, language );

	    PatchList patchList = patchInfo.getRawPatchList();
	    PatchList::iterator patchPos;
	    time_t     currentTime  = time( 0 );
	    struct tm* currentLocalTime = localtime( &currentTime );
	    char date[12];
	    PatchList localPatchList = currentPatchInfo->getRawPatchList();

	    sprintf ( date, "%04d%02d%02d",
		      currentLocalTime->tm_year+1900,
		      currentLocalTime->tm_mon+1,
		      currentLocalTime->tm_mday );

	    for ( patchPos = patchList.begin() ;
		  patchPos != patchList.end();
		  patchPos++ )
	    {
	       PatchElement patchElement = patchPos->second;

	       PatchList::iterator pos = localPatchList.find(
							   patchElement.id );

	       if ( pos == localPatchList.end() )
	       {
		  // does not exist --> fetch it from the local-source
		  string command = "/bin/cp ";

		  command += localSourcePath + "/" +
		     architecture +
		     "/update/" +
		     distributionVersion + PATCH + "/" +
		     patchElement.id;
		  if ( patchElement.date.size() > 0 )
		  {
		     command+= "." + patchElement.date;
		  }
		  if ( patchElement.state.size() > 0 )
		  {
		     command+= "." + patchElement.state;
		  }

		  command += " " + destPatchPath + PATCH + "/" +
		     patchElement.id +
		     "." + date + ".new";

		  YCPPath path = ".target.bash";
		  YCPString value ( command );
		  YCPValue ret = mainscragent->Execute( path, value );

		  if ( ret->isInteger() )	// success
		  {
		      if (  ret->asInteger()->value() >= 0 )
		      {
			  y2debug( "%s ok", command.c_str() );
			  message += "Reading patch-description " +
			      patchElement.id + "\n";			  
			  found = true;
		      }
		      else
		      {
			  y2error( "%s", command.c_str() );
			  ok = false;
			  message = "Could not copy patch-information";	
		      }
		  }
		  else
		  {
		      y2error("<.target.bash> System agent returned nil.");
		      ok = false;
		      message = "Could not copy patch-information";	
		  }
	       }
	    }
	 }
	 else
	 {
	    y2error( "Could not find localSourcePath %s",
		     localSourcePath.c_str());
	    ok = false;
	    message = "No SuSE-patch-CD or SuSE path found.";
	    cont = false;
	 }
      }
      else
      {
	 y2error( "Could not find localSourcePath");
	 ok = false;
	 message = "internal error";
	 cont = false;
      }
   }
   else if ( serverKind == FTP
	     || serverKind == HTTP )
   {
      // installing via ftp or http

      if ( ok && dirList->size() <= 0 )
      {
	 counter = 1;
	 if ( serverKind == FTP )
	 {
	     // reading ftp-directory

	     string source = "";
	     bool found = false;

	     if ( isBusiness() )
	     {
		 source = sourcePath + "/" + architecture +
		     "/update/" + productName + "/" +
		     productVersion + PATCH;
	     }
	     else
	     {
		 source = sourcePath + "/" + architecture +
		     "/update/" +
		     distributionVersion +
		     PATCH;
	     }
	     
	     YCPPath path = ".ftp.cd";
	     YCPString value ( source );
	     YCPValue ret = mainscragent->Execute( path, value );

	     if ( ret->isMap() )	// success
	     {
		 YCPMap retMap = ret->asMap();
		 YCPValue dummyValue = YCPVoid();

		 dummyValue = retMap->value(YCPString(OK));
		 if ( !dummyValue.isNull()
		      && dummyValue->isBoolean()
		      && dummyValue->asBoolean()->value())
		 {
		     found = true;
		     patchDirectory = source;
		 }
		 else
		 {
		     y2warning( "Path %s not found",
				source.c_str() );
		 }
	     }
	     else
	     {
		 y2error("<.ftp.cd> System agent returned nil for %s.",
			 source.c_str() );
	     }

	     if ( !found )
	     {
		 // try direct path
		 value = YCPString ( PATCH );
		 
		 YCPValue ret = mainscragent->Execute( path, value );
		 if ( ret->isMap() )	// success
		 {
		     YCPMap retMap = ret->asMap();
		     YCPValue dummyValue = YCPVoid();

		     dummyValue = retMap->value(YCPString(OK));
		     if ( !dummyValue.isNull()
			  && dummyValue->isBoolean()
			  && dummyValue->asBoolean()->value())
		     {
			 found = true;
			 patchDirectory = source;
		     }
		     else
		     {
			 y2warning( "Path %s not found",
				    source.c_str() );
		     }
		 }
		 else
		 {
		     y2error("<.ftp.cd> System agent returned nil for %s.",
			     source.c_str() );
		 }
	     }

	     if ( !found )
	     {
		 ok = cont = false;
		 message = "Path " + source + " not found";
	     }
	     else
	     {
		 YCPPath path = ".ftp.getDirectory";
		 YCPString value ( "" );
		 YCPValue ret = mainscragent->Execute( path, value );
		 
		 if ( ret->isList() )	// success
		 {
		     dirList = ret->asList();
		     posDirList = 0;
		     progress = 10;		     
		 }
		 else
		 {
		     y2error("<.ftp.getDirectory> System agent returned nil.");
		     ok = cont = false;
		     message = "Cannot read directory";		     
		 }		 
	     }
	 }
	 else
	 {
	     // HTTP
	     string sourcefile;
	     string destfile;

	     if ( isBusiness() )
	     {
		 sourcefile = HTTPADRESS + server + "/" + sourcePath + "/"+
		     architecture + "/update/" + productName + "/" +
		     productVersion + PATCH + "/" + DIRECTORY;
		 patchDirectory = HTTPADRESS + server + "/" + sourcePath + "/"+
		     architecture + "/update/" + productName + "/" +
		     productVersion + PATCH;		 
	     }
	     else
	     {
		 sourcefile = HTTPADRESS + server + "/" + sourcePath + "/"+
		     architecture + "/update/" + 
		     distributionVersion + PATCH + "/" + DIRECTORY;
		 patchDirectory = HTTPADRESS + server + "/" + sourcePath + "/"+
		     architecture + "/update/" + 
		     distributionVersion + PATCH;
	     }

	     destfile = destPatchPath + PATCH + "/" + DIRECTORY;

	     YCPPath path = ".http.getFile";
	     YCPValue ret = mainscragent->Execute( path,
						   YCPString( sourcefile ),
						   YCPString( destfile ));

	     if ( ret->isMap() )	// success
	     {
		 YCPMap retMap = ret->asMap();
		 YCPValue dummyValue = YCPVoid();

		 dummyValue = retMap->value(YCPString(OK));
		 if ( !dummyValue.isNull()
		      && dummyValue->isBoolean()
		      && dummyValue->asBoolean()->value() == false )
		 {
		     ok = false;
		     message = "ERROR with wget";
		 }
	     }
	     else
	     {
		 y2error("<.http.getFile> System agent returned nil.");
		 ok = false;
		 message = "ERROR with wget";		 
	     }
	     
	     if ( !ok )
	     {
		 // trying not SuSE path
		 sourcefile = HTTPADRESS + server + "/" + sourcePath +
		     PATCH + "/" + DIRECTORY;
		 patchDirectory = HTTPADRESS + server + "/" + sourcePath +
		     PATCH;
		 YCPPath path = ".http.getFile";
		 YCPValue ret = mainscragent->Execute( path,
						       YCPString( sourcefile ),
						       YCPString( destfile ));

		 if ( ret->isMap() )	// success
		 {
		     YCPMap retMap = ret->asMap();
		     YCPValue dummyValue = YCPVoid();

		     dummyValue = retMap->value(YCPString(OK));
		     if ( !dummyValue.isNull()
			  && dummyValue->isBoolean()
			  && dummyValue->asBoolean()->value() == false )
		     {
			 ok = cont = false;
			 dummyValue = retMap->value(YCPString(MESSAGE));
			 if ( !dummyValue.isNull()
			      && dummyValue->isString()	)
			 {
			     message = dummyValue->asString()->value();
			 }
		     }
		     else
		     {
			 ok = true;
			 message = "";
		     }
		 }
		 else
		 {
		     y2error("<.http.getFile> System agent returned nil.");
		     ok = false;
		     message = "ERROR with wget";		 
		 }
	     }
	     else
	     {
		 message = "";
	     }

	     if ( ok )
	     {
		 dirList = YCPList();

		 // reading patches/directory in the dirList
		 ifstream file ( destfile.c_str() );
		 char buffer[100];

		 if ( !file )
		 {
		     ok = cont = false;
		     message = "Directory file not found";
		 }
		 else
		 {
		     do
		     {
			 file.getline( buffer, 99 );

			 if ( file.good() )
			 {
			     if ( buffer[ strlen( buffer )-1 ] == '\n' )
			     {
				 buffer[ strlen( buffer )-1 ] = 0;
			     }
			     
			     dirList->add ( YCPString(  buffer ) );
			 }
		     } while ( file.good() );
		 }
     
		 posDirList = 0;
		 progress = 10;
	     }	     	     
	 }
      }
      else if ( ok )
      {
	 // fetch every patch-description-file from ftp or http server

	 if ( posDirList == dirList->size() )
	 {
	    //last entry found
	    cont = false;
	    progress = 100;
	    dirList= YCPList();
	 }
	 else
	 {
	     string patchname = ".";
	     YCPValue dummyValue = dirList->value(posDirList++);
	     if ( !dummyValue.isNull() && dummyValue->isString() )
	     {
		 patchname = dummyValue->asString()->value();
	     }

	    if ( patchname != "." &&
		 patchname != ".." &&
		 patchname.find_first_of ("-") != string::npos )
	    {
	       // it is a patch-file. Check, if it does already exist on the
	       // client.
	       PatchList patchList = currentPatchInfo->getRawPatchList();
	       PatchList::iterator pos = patchList.find( patchname );

	       if ( pos == patchList.end() )
	       {
		  // does not exist --> fetch it from the ftp-server.
		  time_t     currentTime  = time( 0 );
		  struct tm* currentLocalTime = localtime( &currentTime );
		  char date[12];

		  sprintf ( date, "%04d%02d%02d",
			    currentLocalTime->tm_year+1900,
			    currentLocalTime->tm_mon+1,
			    currentLocalTime->tm_mday );

		  if ( serverKind == FTP )
		  {
 		      YCPPath path = ".ftp.getFile";
		      YCPValue ret = mainscragent->Execute( path,
					    YCPString( patchname ),
					    YCPString( destPatchPath +
						       PATCH +
						       "/" +
						       patchname +
						       "." + date + ".new"));

		      if ( ret->isBoolean() )	// success
		      {
			  if (  !(ret->asBoolean()->value()) )
			  {
			      cont = ok = false;
			      y2error ( "Cannot get %s",
					patchname.c_str() );
			      message = "Cannot get " + patchname;
			  }
			  else
			  {
			      message = patchname;
			      checkpatch =  destPatchPath +
				  PATCH +
				  "/" +
				  patchname +
				  "." + date + ".new";
			  }
		      }
		      else
		      {
			  y2error("<.ftp.getFile> System agent "
				  "returned nil for %s.",
				  patchname.c_str() );
			  cont = ok = false;
			  message = "Cannot get " + patchname;	
		      }
		  }
		  else
		  {
		      // HTTP
		      string sourcefile;
		      string destfile;

		      sourcefile = patchDirectory + "/" + patchname;

		      destfile = destPatchPath + PATCH + "/" + patchname +
			  "." + date + ".new";

		      YCPPath path = ".http.getFile";
		      YCPValue ret = mainscragent->Execute( path,
						YCPString( sourcefile ),
						YCPString( destfile ));

		      if ( ret->isMap() )	// success
		      {
			  YCPMap retMap = ret->asMap();
			  YCPValue dummyValue = YCPVoid();

			  dummyValue = retMap->value(YCPString(OK));
			  if ( !dummyValue.isNull()
			       && dummyValue->isBoolean()
			       && dummyValue->asBoolean()->value() == false )
			  {
			      dummyValue = retMap->value(YCPString(MESSAGE));
			      if ( !dummyValue.isNull()
				   && dummyValue->isString()	)
			      {
				  message = dummyValue->asString()->value();
			      }
			      ok = cont =false;			      
			  }
		      }
		      else
		      {
			  y2error("<.target.size> System agent returned nil.");
			  ok = cont =false;
			  message = "ERROR with wget";		 
		      }
		      
		      if ( ok )
		      {
			  message = sourcefile;
			  checkpatch =  destfile;
		      }
		  }
	       }
	    }

	    // setting progress-bar
	    float prog = counter;
	    progress = (int) (prog / dirList->size() * 100 +10);
	    if ( progress > 100 )
	       progress = 100;
	    counter++;
	 }
      }
   }

   ret->add ( YCPString ( OK ), YCPBoolean ( ok ) );
   ret->add ( YCPString ( MESSAGE ), YCPString( message ) );
   ret->add ( YCPString ( CHECKPATCH ), YCPString( checkpatch ) );   
   ret->add ( YCPString ( CONTINUE ), YCPBoolean( cont ) );
   ret->add ( YCPString ( PROGRESS ), YCPInteger ( progress ) );

   return ret;
}


/*--------------------------------------------------------------------------*
 * Evaluate patch-list. Returns a map like
 * $[ <patch-ID1>:[<patch-mode1>,<patch-description1>,<size1>,<install>],
 *    <patch-ID2>:[<patch-mode2>,<patch-description2>,<size2>,<install>],...
 *  ]
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::newPatchList ( void )
{
   YCPMap ret;

   y2debug( "CALLING NewPatchList" );
   
   if ( serverKind == NO )
   {
      return YCPError( "missing call setServer()", YCPVoid());       
   }

   if ( !mainscragent )
   {
      return YCPError( "mainsrcagent not initialized", YCPVoid());        
   }

   if ( currentPatchInfo != NULL )
   {
      delete currentPatchInfo;
      currentPatchInfo = NULL;
   }

   currentPatchInfo = new  PatchInfo ( destPatchPath + PATCH, language );
   PatchList newPatchList = currentPatchInfo->getRawPatchList();
   PatchList::iterator patchPos;

   for ( patchPos = newPatchList.begin() ; patchPos != newPatchList.end();
	 patchPos++ )
   {
      PatchElement patchElement = patchPos->second;

      if ( patchElement.state == NEW ||
	   patchElement.state == ERROR )
      {
	 YCPList patchList;

	 patchList->add ( YCPString ( patchElement.kind ) );
	 patchList->add ( YCPString ( patchElement.shortDescription ) );
	 patchList->add ( YCPString ( patchElement.size ) );

	 bool packageInstalled = false;

	 if ( patchElement.rawPackageInfo != NULL &&
	      patchElement.kind != OPTIONAL )
	 {
	     if ( patchElement.installTrigger != ""
		  && mainscragent )
	     {
		 // Running trigger script in order to check if the
		 // packages have to be installed
		 YCPPath path = ".target.bash";
		 YCPString value ( patchElement.installTrigger );
		 YCPValue ret = mainscragent->Execute( path, value );

		 if ( ret->isInteger() )	// success ???
		 {
		     if (  ret->asInteger()->value() >= 0 )
		     {
			 y2debug( "%s ok", patchElement.installTrigger.c_str() );
			 packageInstalled = true;
		     }
		     else
		     {
			 y2debug( "%s returned ERROR",
				  patchElement.installTrigger.c_str() );
		     }
		 }
		 else
		 {
		     y2error("<.target.bash> System agent returned nil.");
		 }		 
	     }
	     else
	     {
		 // Evaluate, if a package is of the patch is already installed
		 // on the current system. --> It will be selected to install.
		 // But not patches with kind "optional"
	    
		 PackVersList packageList =
		     patchElement.rawPackageInfo->getRawPackageList( false );
		 PackVersList::iterator pos;

		 for ( pos = packageList.begin();
		       pos != packageList.end();
		       pos++ )
		 {
		     PackageKey packageKey = *pos;
		     string rpmVersion = "";
		     YCPPath path = ".targetpkg.info.version";
	   
		     YCPValue ret = mainscragent->Read( path,
					YCPString( packageKey.name() ) );

		     if ( ret->isString() )	// success
		     {
			 rpmVersion = ret->asString()->value();
		     }
		     else
		     {
			 y2error("<.targetpkg.info.version> System agent returned nil.");
		     }
		     
		     if (  rpmVersion != "" )
		     {
			 string packageName = packageKey.name();
			 bool basePackage;
			 int installationPosition;
			 int cdNr;
			 string instPath;
			 string version;
			 long buildTime;
			 int rpmSize;

			 patchElement.rawPackageInfo->getRawPackageInstallationInfo(
					        packageName,
						basePackage,
						installationPosition,
						cdNr,
						instPath,
						version,
						buildTime, rpmSize );
			 if ( CompVersion ( version, rpmVersion ) == V_NEWER )
			 {
			     packageInstalled = true;
			 }
		     }
		     else
		     {
			 // Checking if a provides is installed
			 PackList packageList = patchElement.rawPackageInfo->getProvides(
									  packageKey.name()
									  );
			 PackList::iterator posList;
			 for ( posList = packageList.begin();
			       posList != packageList.end();
			       ++posList )
			 {
			     YCPPath path = ".targetpkg.installed";
	   
			     YCPValue ret = mainscragent->Read( path,
					YCPString( *posList) );

			     if ( ret->isBoolean() 	// success
				  && ret->asBoolean()->value() )
			     {
				 packageInstalled = true;
			     }
			 }
		     }
		 }
	     }
	 }

	 if ( packageInstalled )
	 {
	    // patch will be selected to install.
	    patchList->add ( YCPString("X") );
	 }
	 else
	 {
	    patchList->add ( YCPString(" ") );
	 }

	 ret->add ( YCPString ( patchElement.id ),
		    patchList );
      }
   }

   return ret;
}


/*--------------------------------------------------------------------------*
 * Evaluate all information about the patch
 * Returns a map like:
 * $[ "description":"text","packages":
 *     $[<package-name1>:[<description1>,<installed-version1>,<new-version1>],
 *       <package-name2>:[<description2>,<installed-version2>,<new-version2>],
 *	..
 *     ]]
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::getPatchInformation ( const YCPString patchName )
{
   YCPMap ret;

   y2debug( "CALLING getPatchInformation" );

   if ( serverKind == NO )
   {
      return YCPError( "missing call setServer()", YCPVoid());              
   }

   if ( !mainscragent )
   {
      return YCPError( "No system agent installed", YCPVoid());       
   }
   
   if ( currentPatchInfo == NULL )
   {
       currentPatchInfo = new  PatchInfo ( destPatchPath + PATCH, language );       
   }

   PatchList patchList = currentPatchInfo->getRawPatchList();
   PatchList::iterator patchPos;

   patchPos = patchList.find( patchName->value());

   if ( patchPos != patchList.end() )
   {
       PatchElement patchElement = patchPos->second;
       string description = "Bug-ID: ";

       description += patchElement.id + "\n";
       description += "Kind: " + patchElement.kind + "\n";
       description += patchElement.longDescription + "\n" +
	   patchElement.preInformation + "\n" +
	   patchElement.postInformation;

       ret->add ( YCPString ( DESCRIPTION ),
		  YCPString ( description ) );

       YCPMap packageMap;

       if ( patchElement.rawPackageInfo != NULL )
       {
	   PackVersList packageList =
	       patchElement.rawPackageInfo->getRawPackageList( false );
	   PackVersList::iterator pos;

	   for ( pos = packageList.begin();
		 pos != packageList.end();
		 pos++ )
	   {
	       PackageKey packageKey = *pos;
	       string packageName = packageKey.name();
	       bool basePackage;
	       int installationPosition;
	       int cdNr;
	       string instPath;
	       string version;
	       long buildTime;
	       int rpmSize;	       
	       string shortDescription =
		   patchElement.rawPackageInfo->getLabel( packageName );

	       patchElement.rawPackageInfo->getRawPackageInstallationInfo(
						packageName,
						basePackage,
						installationPosition,
						cdNr,
						instPath,
						version,
						buildTime, rpmSize );

	       YCPList packageList;
	       packageList->add ( YCPString ( shortDescription ) );

	       string rpmVersion = "";
	       YCPPath path = ".targetpkg.info.version";
	   
	       YCPValue retpkg = mainscragent->Read( path,
					YCPString( packageName ) );

	       if ( retpkg->isString() )	// success
	       {
		   rpmVersion = retpkg->asString()->value();
	       }
	       else
	       {
		   y2error("<.targetpkg.info.version> System agent returned nil.");
	       }
	       
	       packageList->add ( YCPString ( rpmVersion ) );
	       packageList->add ( YCPString ( version ) );

	       packageMap->add ( YCPString ( packageName ),
				 packageList );
	   }
       }

       ret->add ( YCPString ( PACKAGES ),
		  packageMap );
   }
   else
   {
       y2error( "Patch %s not found", patchName->value().c_str() );
   }

   return ret;
}


/*--------------------------------------------------------------------------*
 * Reading patch with all RPMs from server.
 * Returns $[ "ok": false, "message","not connected", "nextPackage",
 * <package-path>, "nextPackageSize", <byte>]
 *--------------------------------------------------------------------------*/
static string lastPatch = "";
static PackVersList packageList;
static PackVersList::iterator posPackageList;

YCPValue YouAgent::getPatch ( const YCPString patchName )
{
   YCPMap ret;
   bool ok = true;
   string message = "";

   y2debug( "CALLING GetPatch" );

   if ( serverKind == NO )
   {
      return YCPError( "missing call setServer()", YCPVoid());
   }

   if ( !mainscragent )
   {
      return YCPError( "No system agent installed", YCPVoid());       
   }

   if ( currentPatchInfo == NULL )
   {
       currentPatchInfo = new  PatchInfo ( destPatchPath + PATCH, language );
   }

   if ( ok )
   {
      if ( patchName->value() != lastPatch )
      {
	 // New patch
	 PatchList patchList = currentPatchInfo->getRawPatchList();
	 PatchList::iterator patchPos;

	 patchPos = patchList.find( patchName->value());

	 if ( patchPos != patchList.end() )
	 {
	    PatchElement patchElement = patchPos->second;

	    if ( patchElement.rawPackageInfo != NULL )
	    {
	       packageList =
		  patchElement.rawPackageInfo->getRawPackageList( false );
	       posPackageList = packageList.begin();
	    }
	    else
	    {
	       // Error
	       ok = false;
	    }
	 }
	 lastPatch = patchName->value();
      }
      else
      {
	 PatchList patchList = currentPatchInfo->getRawPatchList();
	 PatchList::iterator patchPos;

	 patchPos = patchList.find( patchName->value());

	 if ( patchPos != patchList.end()
	      && posPackageList != packageList.end() )
	 {
	    PatchElement patchElement = patchPos->second;

	    // reading next package
	    PackageKey packageKey = *posPackageList;
	    string packageName = packageKey.name();
	    bool basePackage;
	    int installationPosition;
	    int cdNr;
	    string instPath;
	    string version;
	    long buildTime;
	    int rpmSize;
	    bool otherInternetServer = false;

	    patchElement.rawPackageInfo->getRawPackageInstallationInfo(
					 packageName,
					 basePackage,
					 installationPosition,
					 cdNr,
					 instPath,
					 version,
					 buildTime,
					 rpmSize );

	    // create directory on the client-machine

	    if ( instPath.substr( 0, strlen( FTPADRESS ) ) == FTPADRESS
		 || instPath.substr( 0, strlen( HTTPADRESS ) ) == HTTPADRESS )
	    {
	       otherInternetServer = true;
	       // rpm is on another fpt server. So no specific serie
	       // can be taken.
	       string clientPath = destPatchPath;
	       clientPath = clientPath + "/" + OTHERS;
	       create_directories	( clientPath );
	    }
	    else
	    {
	       // Server is a SuSE server
	       // create directory on the client-machine under the correct
	       // serie
	       string::size_type posDir = instPath.find_last_of("/");
	       if ( posDir != string::npos )
	       {
		  string clientPath = destPatchPath;
		  clientPath += "/" + instPath.substr(0,posDir);
		  create_directories       ( clientPath );
	       }
	    }

	    if ( serverKind == CD 
		 || serverKind == NFS
		 || serverKind == HD )
	    {

	       if ( otherInternetServer )
	       {
		  // rpm has been saved under others/<rmp>
		  instPath = OTHERS;
		  instPath += "/";
		  instPath += packageName + ".rpm";
	       }

	       // reading rpm from local source
	       string command = "/bin/cp ";

	       command += localSourcePath + "/" +
		  architecture +
		  "/update/" +
		  distributionVersion + "/" +
		  instPath;

	       command += " " + destPatchPath + "/" +
		  instPath;

	       YCPPath path = ".target.bash";
	       YCPString value ( command );
	       YCPValue ret = mainscragent->Execute( path, value );

	       if ( ret->isInteger() )	// success
	       {
		   if (  ret->asInteger()->value() >= 0 )
		   {
		       y2debug( "%s ok", command.c_str() );
		       message += "Copied  " + instPath;		       
		   }
		   else
		   {
		       y2error( "%s", command.c_str() );
		       ok = false;
		       message = "Could not copy package " + instPath;
		   }
	       }
	       else
	       {
		   y2error("<.target.bash> System agent returned nil.");
		   ok = false;
		   message = "Could not copy package " + instPath;
	       }	       
	    }
	    else
	    {
	       // reading rpm from ftp or http

	       if ( otherInternetServer )
	       {
		   if ( instPath.substr ( 0, strlen( HTTPADRESS ) ) == HTTPADRESS )
		   {
		       // fetching via http
		       YCPPath path = ".http.setUser";
		       YCPValue ret = mainscragent->Execute( path,
						   YCPString( "" ),
						   YCPString( "" ));
		       path = ".http.setProxyUser";
		       ret = mainscragent->Execute( path,
						   YCPString( "" ),
						   YCPString( "" ));
		       

		       path = ".http.getFile";
		       ret = mainscragent->Execute( path,
						    YCPString( instPath ),
						    YCPString( destPatchPath + "/" +
							       OTHERS + "/" + 
							       packageName + ".rpm" ));
		       if ( ret->isMap() )	// success
		       {
			   YCPMap retMap = ret->asMap();
			   YCPValue dummyValue = YCPVoid();

			   dummyValue = retMap->value(YCPString(OK));
			   if ( !dummyValue.isNull()
				&& dummyValue->isBoolean()
				&& dummyValue->asBoolean()->value() == false )
			   {
			       ok = false;
			       dummyValue = retMap->value(YCPString(MESSAGE));
			       if ( !dummyValue.isNull()
				    && dummyValue->isString()	)
			       {
				   message = dummyValue->asString()->value();
			       }
			   }
			   else
			   {
			       ok = true;
			       message = "";
			   }
		       }
		       else
		       {
			   y2error("<.http.getFile> System agent returned nil.");
			   ok = false;
			   message = "ERROR with wget";		 
		       }
		       
		       path = ".http.setUser";
		       ret = mainscragent->Execute( path,
						    YCPString( httpUser ),
						    YCPString( httpPassword ));
		       path = ".http.setProxyUser";
		       ret = mainscragent->Execute( path,
						    YCPString( httpProxyUser ),
						    YCPString( httpProxyPassword ));
		   }
		   else
		   {
		       // fetching via ftp
		       // extract serverpath from ftp://<ftp-server>/<path>/<rpm-package>

		       instPath = instPath.substr( strlen( FTPADRESS ) ); // extract ftp:://

		       string::size_type posDir = instPath.find_first_of("/");
		       if ( posDir != string::npos )
		       {
			   instPath = instPath.substr ( posDir ); // extract servername
		       }

		       y2debug( "  ftp: %s -> %s/others/%s.rpm",
				instPath.c_str(),
				destPatchPath.c_str(),
				packageName.c_str() );
		       
		       YCPPath path = ".ftp.getFile";
		       YCPValue ret = mainscragent->Execute( path,
					    YCPString( instPath ),
					    YCPString( destPatchPath + "/" +
						       OTHERS + "/" +
						       packageName + ".rpm" ));

		       if ( ret->isBoolean() )	// success
		       {
			   if (  !(ret->asBoolean()->value()) )
			   {
			      ok = false;
			      y2error ( "Cannot get %s",
					instPath.c_str() );
			      message = "Cannot get " + instPath;
			   }
			   else
			   {
			      message = instPath;
			   }
		       }
		       else
		       {
			  y2error("<.ftp.getFile> System agent "
				  "returned nil for %s.",
				  instPath.c_str() );
			  ok = false;
			  message = "Cannot get " + instPath;	
		       }		       
		   }
	       }
	       else
	       {
		  // NO other ftp or http server
		  YCPPath path = ".targetpkg.checkSourcePackage";
	   
		  YCPValue ret = mainscragent->Execute( path,
				YCPString( destPatchPath + "/" + instPath ),
				YCPString( version ));
		   
		  if ( ret->isBoolean()
		       && !ret->asBoolean()->value() )
		  {
		      if ( serverKind == FTP )
		      {
			  // changing to fpt-update-directory

			  string source = "";
			  bool found = false;

			  if ( isBusiness() )
			  {
			      source = sourcePath + "/" + architecture +
				  "/update/" + productName + "/" +
				  productVersion;
			  }
			  else
			  {
			      source = sourcePath + "/" + architecture +
				  "/update/" +
				  distributionVersion;
			  }
			  
			  YCPPath path = ".ftp.cd";
			  YCPString value ( source );
			  YCPValue ret = mainscragent->Execute( path, value );
			  
			  if ( ret->isMap() )	// success
			  {
			      YCPMap retMap = ret->asMap();
			      YCPValue dummyValue = YCPVoid();

			      dummyValue = retMap->value(YCPString(OK));
			      if ( !dummyValue.isNull()
				   && dummyValue->isBoolean()
				   && dummyValue->asBoolean()->value())
			      {
				  found = true;
				  patchDirectory = source;
			      }
			      else
			      {
				  y2warning( "Path %s not found",
					     source.c_str() );
			      }
			  }
			  else
			  {
			      y2error("<.ftp.cd> System agent returned nil for %s.",
				      source.c_str() );
			  }

			  if ( !found )
			  {
			      // try direct path
			      value = YCPString ( PATCH );
		 
			      YCPValue ret = mainscragent->Execute( path, value );
			      if ( ret->isMap() )	// success
			      {
				  YCPMap retMap = ret->asMap();
				  YCPValue dummyValue = YCPVoid();

				  dummyValue = retMap->value(YCPString(OK));
				  if ( !dummyValue.isNull()
				       && dummyValue->isBoolean()
				       && dummyValue->asBoolean()->value())
				  {
				      found = true;
				      patchDirectory = source;
				  }
				  else
				  {
				      y2warning( "Path %s not found",
						 source.c_str() );
				  }
			      }
			      else
			      {
				  y2error("<.ftp.cd> System agent returned nil for %s.",
					  source.c_str() );
			      }
			  }

			  if ( !found )
			  {
			      ok = false;
			      message = "Path " + source + " not found";
			  }
			  else
			  {

			      y2debug( "  ftp: %s -> %s/%s",
				       instPath.c_str(),
				       destPatchPath.c_str(),
				       instPath.c_str() );
			      
			      path = ".ftp.getFile";
			      ret = mainscragent->Execute( path,
							   YCPString( instPath ),
							   YCPString( destPatchPath + "/" +
								      instPath));

			      if ( ret->isBoolean() )	// success
			      {
				  if (  !(ret->asBoolean()->value()) )
				  {
				      ok = false;
				      y2error ( "Cannot get %s",
						instPath.c_str() );
				      message = "Cannot get " + instPath;
				  }
				  else
				  {
				      message = instPath;
				  }
			      }
			      else
			      {
				  y2error("<.ftp.getFile> System agent "
					  "returned nil for %s.",
					  instPath.c_str() );
				  ok = false;
				  message = "Cannot get " + instPath;	
			      }
			  }
		      }
		      else
		      {
			  //getting via http
			  string sourcefile;
			  string destfile;

			  if ( isBusiness() )
			  {
			      sourcefile = HTTPADRESS + server + "/" + sourcePath + "/"+
				  architecture + "/update/" + productName + "/" +
				  productVersion + "/" + instPath;
			  }
			  else
			  {
			      sourcefile = HTTPADRESS + server + "/" + sourcePath + "/"+
				  architecture + "/update/" + 
				  distributionVersion + "/" + instPath;
			  }
			  destfile = destPatchPath +  "/" + instPath;


			  YCPPath path = ".http.getFile";
			  YCPValue ret = mainscragent->Execute( path,
								YCPString( sourcefile ),
								YCPString( destfile ));

			  if ( ret->isMap() )	// success
			  {
			      YCPMap retMap = ret->asMap();
			      YCPValue dummyValue = YCPVoid();

			      dummyValue = retMap->value(YCPString(OK));
			      if ( !dummyValue.isNull()
				   && dummyValue->isBoolean()
				   && dummyValue->asBoolean()->value()
				   == false )
			      {
				  ok = false;
				  message = "ERROR with wget";
			      }
			  }
			  else
			  {
			      y2error("<.http.getFile> System agent returned nil.");
			      ok = false;
			      message = "ERROR with wget";		 
			  }
	     
			  if ( !ok )
			  {
			      // trying not SuSE path
			      sourcefile = HTTPADRESS + server + "/" + sourcePath + "/"+
				  instPath;

			      path = ".http.getFile";
			      ret = mainscragent->Execute( path,
							   YCPString( sourcePath ),
							   YCPString( destfile ));

			      if ( ret->isMap() )	// success
			      {
				  YCPMap retMap = ret->asMap();
				  YCPValue dummyValue = YCPVoid();

				  dummyValue = retMap->value(YCPString(OK));
				  if ( !dummyValue.isNull()
				       && dummyValue->isBoolean()
				       && dummyValue->asBoolean()->value() == false )
				  {
				      ok = false;
				      dummyValue = retMap->value(YCPString(MESSAGE));
				      if ( !dummyValue.isNull()
					   && dummyValue->isString()	)
				      {
					  message = dummyValue->asString()->value();
				      }
				  }
				  else
				  {
				      ok = true;
				      message = "";
				  }
			      }
			      else
			      {
				  y2error("<.http.getFile> System agent returned nil.");
				  ok = false;
				  message = "ERROR with wget";		 
			      }
			  }
			  else
			  {
			      message = "";
			  }
		      }
		  }
		  else
		  {
			y2debug( "ftp: %s/%s is already on the client",
				 destPatchPath.c_str(),
				 instPath.c_str() );
		  }
	       }
	    }

	    // next package
	    posPackageList++;
	 }
	 else
	 {
	     ok = false;
	     message = "No more package available.";
	 }
      }
   }

   if ( ok && posPackageList != packageList.end() )
   {
      PatchList patchList = currentPatchInfo->getRawPatchList();
      PatchList::iterator patchPos;

      patchPos = patchList.find( patchName->value());

      if ( patchPos != patchList.end() )
      {
	 PatchElement patchElement = patchPos->second;

	 // evaluate next package
	 PackageKey packageKey = *posPackageList;
	 string packageName = packageKey.name();
	 bool basePackage;
	 int installationPosition;
	 int cdNr;
	 string instPath;
	 string version;
	 long buildTime;
	 int rpmSize;

	 patchElement.rawPackageInfo->getRawPackageInstallationInfo(
				         packageName,
					 basePackage,
					 installationPosition,
					 cdNr,
					 instPath,
					 version,
					 buildTime,
					 rpmSize );

	 string serie = patchElement.rawPackageInfo->getSerieOfPackage(
								packageName );

	 if ( instPath.substr ( 0, strlen ( FTPADRESS ) ) == FTPADRESS   )
	 {
	    // package is on another server. So we have to
	    // change to it.
	    string server;
	    server = instPath.substr( strlen( FTPADRESS ) ); // extract ftp:://

	    string::size_type posServer = server.find_first_of("/");
	    if ( posServer != string::npos )
	    {
	       server = server.substr ( 0,posServer ); // extract servername
	    }
	    ret->add ( YCPString ( NEXTSERVER ),
		       YCPString ( server ));
	    ret->add ( YCPString ( NEXTPACKAGE ),
		       YCPString ( destPatchPath + "/" + OTHERS + "/" + packageName + ".rpm" ));
	    ret->add ( YCPString ( NEXTSERVERKIND ),
		       YCPString ( "ftp" ));

	 }
	 else if ( instPath.substr ( 0, strlen ( HTTPADRESS ) ) == HTTPADRESS )
	 {
	    // package is on another server. So we have to
	    // change to it.
	    string httpServer;
	    httpServer = instPath.substr( strlen( HTTPADRESS ) ); // extract http:://

	    string::size_type posServer = httpServer.find_first_of("/");
	    if ( posServer != string::npos )
	    {
	       httpServer = httpServer.substr ( 0,posServer ); // extract servername
	    }
	    ret->add ( YCPString ( NEXTSERVER ),
		       YCPString ( httpServer ));
	    ret->add ( YCPString ( NEXTPACKAGE ),
		       YCPString ( destPatchPath + "/" + OTHERS + "/" + packageName + ".rpm" ));
	    ret->add ( YCPString ( NEXTSERVERKIND ),
		       YCPString ( "http" ));

	 }
	 else
	 {
	    // package is on current server
	    ret->add ( YCPString ( NEXTPACKAGE ),
		       YCPString ( destPatchPath + "/" + instPath ));
	 }
	 ret->add ( YCPString ( NEXTPACKAGESIZE ),
		    YCPInteger ( rpmSize ));
	 ret->add ( YCPString ( NEXTSERIE ),
		    YCPString ( serie ));
      }
   }

   ret->add ( YCPString ( OK ), YCPBoolean ( ok ) );
   ret->add ( YCPString ( MESSAGE ), YCPString( message ) );

   return ret;
}


/*--------------------------------------------------------------------------*
 * Returns the pre-install-information of a patch
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::getPreInstallInformation ( const YCPString patchName )
{
   YCPString ret("");

   y2debug( "CALLING GetPreInstallInformation for %s",
	    patchName->value().c_str() );


   if ( currentPatchInfo == NULL )
   {
       currentPatchInfo = new  PatchInfo ( destPatchPath + PATCH, language );                     
   }

   PatchList patchList = currentPatchInfo->getRawPatchList();
   PatchList::iterator patchPos;

   patchPos = patchList.find( patchName->value());

   if ( patchPos != patchList.end() )
   {
       PatchElement patchElement = patchPos->second;
       ret = YCPString ( patchElement.preInformation );
   }
   else
   {
       y2error( "patch %s not found", patchName->value().c_str() );
   }

   return ret;
}


/*--------------------------------------------------------------------------*
 * Returns the post-install-information of a patch
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::getPostInstallInformation ( const YCPString patchName )
{
   YCPString ret("");

   y2debug( "CALLING GetPostInstallInformation for %s",
	    patchName->value().c_str());

   if ( currentPatchInfo == NULL )
   {
       currentPatchInfo = new  PatchInfo ( destPatchPath + PATCH, language );                            
   }
   
   PatchList patchList = currentPatchInfo->getRawPatchList();
   PatchList::iterator patchPos;
   
   patchPos = patchList.find( patchName->value());
      
   if ( patchPos != patchList.end() )
   {
       PatchElement patchElement = patchPos->second;
       ret = YCPString ( patchElement.postInformation );
   }
   else
   {
       y2error( "patch %s not found", patchName->value().c_str() );
   }

   return ret;
}


/*--------------------------------------------------------------------------*
 * Returns a list of packages ( with path ) which belong to the patch
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::getPackages ( const YCPString patchName )
{
   YCPList ret;

   y2debug( "CALLING GetPackages for %s",
	    patchName->value().c_str());

   if ( currentPatchInfo == NULL )
   {
       currentPatchInfo = new  PatchInfo ( destPatchPath + PATCH, language );                                   
   }

   PatchList patchList = currentPatchInfo->getRawPatchList();
   PatchList::iterator patchPos;

   patchPos = patchList.find( patchName->value());

   if ( patchPos != patchList.end() )
   {
       PatchElement patchElement = patchPos->second;
       if ( patchElement.rawPackageInfo != NULL )
       {
	   PackVersList packageList =
	       patchElement.rawPackageInfo->getRawPackageList( false );
	   PackVersList::iterator pos;

	   for ( pos = packageList.begin();
		 pos != packageList.end();
		 pos++ )
	   {
	       PackageKey  packageKey = *pos;
	       string packageName = packageKey.name();
	       bool basePackage;
	       int installationPosition;
	       int cdNr;
	       string instPath;
	       string version;
	       long buildTime;
	       string serie;
	       int rpmSize;
	       YCPList packageList;

	       patchElement.rawPackageInfo->getRawPackageInstallationInfo(
					 packageName,
					 basePackage,
					 installationPosition,
					 cdNr,
					 instPath,
					 version,
					 buildTime,
					 rpmSize );
	       
	       string shortDescription =
		   patchElement.rawPackageInfo->getLabel( packageName );
	       
	       serie = patchElement.rawPackageInfo->getSerieOfPackage(
								 packageName );

	       string compString = "ftp://";
	       if ( instPath.substr ( 0, 6 ) == compString   )
	       {
		  // rpm has been token from another server and has been stored
		  // under others/<rpm>
		  packageList->add ( YCPString ( destPatchPath + "/" + OTHERS +
					 "/" + packageName + ".rpm" ));
	       }
	       else
	       {
		  packageList->add ( YCPString ( destPatchPath + "/" + instPath ));
	       }
	       packageList->add ( YCPString ( shortDescription ));

	       if ( serie != SCRIPT )
	       {
		   // only packages will be returned
		   ret->add ( YCPList ( packageList ) );
		   y2debug ( "%s added (%s)",
			     packageName.c_str(),
			     serie.c_str());
	       }
	    }
	 }
   }
   else
   {
       y2error( "patch %s not found", patchName->value().c_str() );
   }

   return ret;
}


/*--------------------------------------------------------------------------*
 * Writes the installation-status of a patch.
 * Valid stati: installed, error, new
 * Returns true or false
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::changePatchUpdateStatus (
				const YCPString patchName,
				const YCPString status )
{
   YCPBoolean ret ( true );

   y2debug( "CALLING changePatchUpdateStatus for patch %s",
	    patchName->value().c_str() );

   if ( currentPatchInfo == NULL )
   {
       currentPatchInfo = new  PatchInfo ( destPatchPath + PATCH, language );                                          
   }

   PatchList patchList = currentPatchInfo->getRawPatchList();
   PatchList::iterator patchPos;

   patchPos = patchList.find( patchName->value() );

   if ( patchPos != patchList.end() )
   {
       PatchElement patchElement = patchPos->second;
       time_t     currentTime  = time( 0 );
       struct tm* currentLocalTime = localtime( &currentTime );
       char date[12];
       
       sprintf ( date, "%04d%02d%02d",
		 currentLocalTime->tm_year+1900,
		 currentLocalTime->tm_mon+1,
		 currentLocalTime->tm_mday );

       string sourceFile = destPatchPath;
       sourceFile += PATCH;
       sourceFile += "/" + patchPos->first + "." +
	   patchElement.date + "." + patchElement.state;

       string destFile = destPatchPath;
       destFile += PATCH;
       destFile += "/" + patchPos->first + "." +
	   date + "." + status->value();

       if ( sourceFile != destFile )
       {
	   string command = "mv ";
	   command += sourceFile + " " + destFile;

	   YCPPath path = ".target.bash";
	   YCPString value ( command );
	   YCPValue ret = mainscragent->Execute( path, value );

	   if ( ret->isInteger() )	// success
	   {
	       if (  ret->asInteger()->value() >= 0 )
	       {
		   y2debug( "%s ok", command.c_str() );
	       }
	       else
	       {
		   y2error( "%s", command.c_str() );
		   ret = YCPBoolean ( false );		    
	       }
	   }
	   else
	   {
	       y2error("<.target.bash> System agent returned nil.");
	       ret = YCPBoolean ( false );		    		
	   }
       }
       else
       {
	   y2warning( "status %s still remains", status->value().c_str() );
       }
   }

   return ret;
}


/*--------------------------------------------------------------------------*
 * Writes the installation-status of the current installation.
 * Returns true or false
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::closeUpdate ( const YCPBoolean success )
{
   YCPBoolean ret ( true );

   y2debug( "CALLING closeUpdate status : %s",
	    success->value() ? "ok": "false" );

   ConfigFile lastUpdate ( destPatchPath + PATCH + "/lastUpdate" );
   Entries entries;
   string updateTime;
   string status;
   Values valuesTime, valuesStatus;
   Element elementTime, elementStatus;
   char timeString[30];

   sprintf ( timeString, "%ld", (long) time( 0 ) );;

   entries.clear();

   valuesTime.push_back ( timeString );
   elementTime.values = valuesTime;
   elementTime.multiLine = false;
   entries.insert(pair<const string, const Element>
		  ( TIME, elementTime ) );

   valuesStatus.push_back ( success->value() ? "ok" : "error" );
   elementStatus.values = valuesStatus;
   elementStatus.multiLine = false;
   entries.insert(pair<const string, const Element>
		  ( STATUS, elementStatus ) );

   if (  !lastUpdate.writeFile ( entries,
			"lastupdate -- (c) 1998 SuSE GmbH",
				 ':' ) )
   {
      y2error( "Error while writing the file %s/patches/lastUpdate",
	       destPatchPath.c_str());
      ret = false;
   }

   return ret;
}


/*--------------------------------------------------------------------------*
 * Check, if the patch works with the currrent installed YaST2-Version
 * Returns $[ "ok": false, "minYaST2Version,"1.0.0" ]
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::checkYaST2Version (
				const YCPString installedYaST2Version,
				const YCPString patchName )
{
   YCPMap ret;
   bool ok = true;
   string minVersion = "";

   y2debug( "CALLING checkYaST2Version for %s",
	    patchName->value().c_str());

   if ( currentPatchInfo == NULL )
   {
       currentPatchInfo = new  PatchInfo ( destPatchPath + PATCH, language );                                                 
   }

   PatchList patchList = currentPatchInfo->getRawPatchList();
   PatchList::iterator patchPos;

   patchPos = patchList.find( patchName->value());

   if ( patchPos != patchList.end() )
   {
       PatchElement patchElement = patchPos->second;

       if ( patchElement.minYaST2Version.size() > 0 )
       {
	   CompareVersion compareVersion;
	   compareVersion = CompVersion (
					 patchElement.minYaST2Version.c_str(),
					 installedYaST2Version->value().c_str() );

	   if ( compareVersion == V_NEWER )
	   {
	       ok = false;
	       minVersion = patchElement.minYaST2Version;
	       y2error( "patch %s does not work with current YaST2-version",
			patchName->value().c_str() );
	       y2error( "%s <--> %s",
			patchElement.minYaST2Version.c_str(),
			installedYaST2Version->value().c_str() );
	   }
       }
   }
   else
   {
       y2error( "patch %s not found", patchName->value().c_str() );
       ok = false;
   }

   ret->add ( YCPString ( OK ), YCPBoolean ( ok ) );
   ret->add ( YCPString ( MINYAST2VERSION ), YCPString( minVersion ) );

   return ret;
}


/*--------------------------------------------------------------------------*
 * Deletes all packages of a patch
 * Returns true or false
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::deletePackages ( const YCPString patchName )
{
   bool ok = true;

   y2debug( "CALLING deletePackages of patch %s ",
	    patchName->value().c_str());

   if ( currentPatchInfo == NULL )
   {
       currentPatchInfo = new  PatchInfo ( destPatchPath + PATCH, language );                                                        
   }

   if ( ok )
   {
      // Reading all packages
      PatchList patchList = currentPatchInfo->getRawPatchList();
      PatchList::iterator patchPos;

      patchPos = patchList.find( patchName->value());

      if ( patchPos != patchList.end() )
      {
	 PatchElement patchElement = patchPos->second;

	 if ( patchElement.rawPackageInfo != NULL )
	 {
	    PackVersList packageList =
	       patchElement.rawPackageInfo->getRawPackageList( false );
	    PackVersList::iterator pos;

	    for ( pos = packageList.begin();
		  pos != packageList.end();
		  pos++ )
	    {
	       PackageKey packageKey = *pos;
	       string packageName = packageKey.name();
	       bool basePackage;
	       int installationPosition;
	       int cdNr;
	       string instPath;
	       string version;
	       long buildTime;
	       int rpmSize;

	       patchElement.rawPackageInfo->getRawPackageInstallationInfo(
					 packageName,
					 basePackage,
					 installationPosition,
					 cdNr,
					 instPath,
					 version,
					 buildTime,
					 rpmSize );


	       // deleting rpm s
	       string filename = destPatchPath + "/" + instPath;
	       YCPPath path = ".target.remove";
	       YCPString value ( filename );
	       YCPValue ret = mainscragent->Execute( path, value );

	       y2debug( "  remove %s",
			filename.c_str() );
	    }
	 }
      }
   }

   return YCPBoolean ( ok );
}


/*--------------------------------------------------------------------------*
 * Is the installed system a business product ?
 * Returns true or false
 *--------------------------------------------------------------------------*/
bool YouAgent::isBusiness ( void )
{
    bool ret = true;
   
    y2debug( "CALLING isBusiness; productname: %s",
	     productName.c_str() );
   
   
   if ( productName != "" &&
	productName != "SuSE-Linux" )
   {
       ret = true;
       y2debug( "return true" );
   }
   else
   {
       ret = false;
       y2debug( "return false" );       
   }
   
   return ret;
}


/*--------------------------------------------------------------------------*
 * Checking authorization of the user					    
 * Returns stati: "ok", "error_login", "error_path"
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::checkAuthorization ( YCPString password,
					YCPString registrationCode )
{
   string ret ( "ok" );

   y2debug( "CALLING checkAuthorization" );
   
   if ( serverKind == NO )
   {
      return YCPError( "missing call setServer()",
		       YCPString ( "error_path" ));                     
   }

   if ( !mainscragent )
   {
       return YCPError( "No system agent installed",
		       YCPString ( "error_internal" ));                     
   }
   
   if ( !password.isNull() &&  !password->isVoid() &&
	!registrationCode.isNull() && !registrationCode->isVoid())
   {
       httpUser = registrationCode->value();
       httpPassword = password->value();              
       YCPPath path = ".http.setUser";
       YCPValue ret = mainscragent->Execute( path,
					     YCPString( httpUser ),
					     YCPString( httpPassword ));
   }
   else
   {
       ret = "error_login";
   }

   if ( ret == "ok" )
   {
       // Checking, if file ...patches/directory does exists 
       string sourcefile;
       string destfile;

       if ( isBusiness() )
       {
	   sourcefile = HTTPADRESS + server + "/" + sourcePath + "/"+
	       architecture + "/update/" + productName + "/" +
	       productVersion + PATCH + "/" + DIRECTORY;
       }
       else
       {
	   sourcefile = HTTPADRESS + server + "/" + sourcePath + "/"+
	       architecture + "/update/" + 
	       distributionVersion + PATCH + "/" + DIRECTORY;
       }

       destfile = destPatchPath + PATCH + "/" + DIRECTORY;
       
       YCPPath path = ".http.getFile";
       YCPValue retEx = mainscragent->Execute( path,
					     YCPString( sourcefile ),
					     YCPString( destfile ));

       if ( retEx->isMap() )	// success
       {
	   YCPMap retMap = retEx->asMap();
	   YCPValue dummyValue = YCPVoid();

	   dummyValue = retMap->value(YCPString(OK));
	   if ( !dummyValue.isNull()
		&& dummyValue->isBoolean()
		&& dummyValue->asBoolean()->value() == false )
	   {
	       dummyValue = retMap->value(YCPString(MESSAGE));
	       if ( !dummyValue.isNull()
		    && dummyValue->isString()	)
	       {
		   ret = dummyValue->asString()->value();
	       }
	   }
       }
       else
       {
	   y2error("<.http.getFile> System agent returned nil.");
	   ret = "error_path";
       }
	     
       if ( ret != "ok" )
       {
	   // trying not SuSE path
	   sourcefile = HTTPADRESS + server + "/" + sourcePath +
	       PATCH + "/" + DIRECTORY;

	   YCPPath path = ".http.getFile";
	   YCPValue retEx = mainscragent->Execute( path,
						 YCPString( sourcefile ),
						 YCPString( destfile ));

	   if ( retEx->isMap() )	// success
	   {
	       YCPMap retMap = retEx->asMap();
	       YCPValue dummyValue = YCPVoid();
	       
	       dummyValue = retMap->value(YCPString(OK));
	       if ( !dummyValue.isNull()
		    && dummyValue->isBoolean()
		    && dummyValue->asBoolean()->value() == false )
	       {
		   dummyValue = retMap->value(YCPString(MESSAGE));
		   if ( !dummyValue.isNull()
			&& dummyValue->isString()	)
		   {
		       ret = dummyValue->asString()->value();
		   }
	       }
	       else
	       {
		   ret = OK;
	       }
	   }
	   else
	   {
	       y2error("<.http.getFile> System agent returned nil.");
	       ret = "error_path";
	   }
       }
   }
   
   return YCPString( ret );
}


/*--------------------------------------------------------------------------*
 * Getting SuSE product informations.
 * Returns a map like [ "Distribution_Verison":"7.2",
 *			   "Product_Version":"3.0",
 *			   "Product_Name":"Email Server" ]
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::getProductInfo ( void )
{
   YCPMap ret;
   
   y2debug( "CALLING GetProductInfo" );
   
   ret->add ( YCPString ( DISTRIBUTION ),
	      YCPString ( distributionVersion ) );
   ret->add ( YCPString ( PRODUCTNAME ),
	      YCPString ( productName ) );
   ret->add ( YCPString ( PRODUCTVERSION ),
	      YCPString ( productVersion ) );
   
   return ret;
}


/*--------------------------------------------------------------------------*
 * compare two versions of a package
 *--------------------------------------------------------------------------*/
CompareVersion YouAgent::CompVersion( string left,
				      string right )
{
   string::size_type beginLeft,beginRight;

   if (  left == right )
   {
      y2debug( "---> is equal" );
      return V_EQUAL;
   }

   string leftString = "";
   string rightString = "" ;
   bool compareNumber = true;
   long leftNumber = -1;
   long rightNumber = -1;

   beginLeft = left.find_first_of ( ". -/" );
   beginRight = right.find_first_of ( ". -/" );

   if ( beginLeft != string::npos )
   {
      leftString = left.substr ( 0, beginLeft );
   }
   else
   {
      // last entry
      leftString = left;
   }
   if ( beginRight != string::npos )
   {
      rightString = right.substr ( 0, beginRight );
   }
   else
   {
      // last entry
      rightString = right;
   }

   if (  rightString.empty() && leftString.empty() )
   {
      y2debug( "---> is equal" );
      return V_EQUAL;
   }

   char *rest = NULL;

   leftNumber = strtol( leftString.c_str(), &rest, 10 );
   if ( rest != NULL && *rest != 0 )
   {
      compareNumber = false;
   }

   rightNumber = strtol( rightString.c_str(), &rest, 10 );
   if ( rest != NULL && *rest != 0)
   {
      compareNumber = false;
   }

   y2debug( "comparing #%s# <-> #%s#",
	    leftString.c_str(), rightString.c_str());

   if ( ( rightString.empty()  &&
	  leftNumber == 0 &&
	  beginLeft == string::npos) ||
	( leftString.empty() &&
	  rightNumber == 0 &&
	  beginRight == string::npos ) )
   {
      // evaluate like 7.0 and 7.0.0 --> equal
      y2debug( "---> is equal" );
      return V_EQUAL;
   }

   if ( ( rightString.empty()  && !leftString.empty() ) ||
	( compareNumber && leftNumber>rightNumber ) ||
	( !compareNumber && leftString > rightString ) )
   {
      y2debug( "---> is newer" );
      return V_NEWER;
   }

   if (( !rightString.empty() && leftString.empty() ) ||
	( compareNumber && leftNumber<rightNumber ) ||
	( !compareNumber && leftString < rightString ) )
   {
      y2debug( "---> is older" );
      return V_OLDER;
   }

   if ( rightString == leftString )
   {
      string dummy1,dummy2;

      if ( beginLeft != string::npos )
      {
	 dummy1 = left.substr( beginLeft +1 );
      }
      else
      {
	 dummy1 = "";
      }

      if ( beginRight != string::npos )
      {
	 dummy2 = right.substr ( beginRight +1 );
      }
      else
      {
	 dummy2 = "";
      }

      if ( !dummy1.empty() && !dummy2.empty() )
      {
	 string sepLeft = left.substr( beginLeft,1 );
	 string sepRight = right.substr( beginRight,1 );

	 // checking if dummy2 is revision-number and dummy1
	 // is not a revistion-number
	 if ( sepLeft == "." &&
	      sepRight == "-" )
	 {
	    y2debug( "left is no revision---> is newer" );
	    return V_NEWER;
	 }

	 if ( sepLeft == "-" &&
	      sepRight == "." )
	 {
	    y2debug( "right is no revision---> is older" );
	    return V_OLDER;
	 }
      }

      return CompVersion( dummy1, dummy2);
   }

  y2debug( "---> do not know" );

  return V_UNCOMP;
}


/*--------------------------- EOF -------------------------------------------*/
