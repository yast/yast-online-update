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

#include "YouAgentCalls.h"

/***************************************************************************
 * Public Member-Functions						   *
 ***************************************************************************/

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

		  dummyValue = dummymap->value(YCPString(PRODUKTNAME));
		  if ( !dummyValue.isNull() &&
		       dummyValue->isString() )
		  {
		      productName = dummyValue->asString()->value();
		  }
		  dummyValue = dummymap->value(YCPString( PRODUKTVERSION ));
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

   if ( serverKind == CD
	|| serverKind == HD
	|| serverKind == NFS )
   {
      // installation from local source

      if ( ok && dirList->size() <= 0 )
      {
	  // Reading Dirlist
	  counter = 1;
	  if ( localSourcePath.size() > 0 )
	  {
	      string source = localSourcePath + PATCH;
	      bool found = false;

	      // check directory
	      YCPPath path = ".target.dir";
	      YCPString value ( source );
	      YCPValue ret = mainscragent->Read( path, value );

	      if ( !ret.isNull() && !ret->isVoid() )	// success
	      {
		  y2debug( "*** Path %s found",  source.c_str() );
		  found = true;
	      }
	      else
	      {
		  y2error("<.target.dir> System agent returned nil.");
	      }

	      if ( !found )
	      {
		  // Checking other paths
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
		  ret = mainscragent->Read( path, value );

		  if ( !ret.isNull() && !ret->isVoid() )	// success
		  {
		      y2debug( "*** Path %s found",  source.c_str() );
		      found = true;
		  }
		  else
		  {
		      y2error("<.target.dir> System agent returned nil.");
		  }
	      }

	      if ( found
		   && ret->isList() )
	      {
		  // path found
		  dirList = ret->asList();
		  posDirList = 0;
		  progress = 10;
	      }
	      else
	      {
		  ok = cont = false;
		  message = "No SuSE-patch-CD or SuSE path found.";
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
      else
      {
	  // Reading each patch description
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

	      string command = "ls " + destPatchPath +
		  PATCH + "/" + patchname + ".* >/dev/null 2>>/dev/null";
	      YCPPath path = ".target.bash";
	      YCPString value ( command );
	      YCPValue ret = mainscragent->Execute( path, value );

	      if ( ret->isInteger()
		   && ret->asInteger()->value() != 0 )
	      {
		  // does not exist --> fetch it from the local-source
		  time_t     currentTime  = time( 0 );
		  struct tm* currentLocalTime = localtime( &currentTime );
		  char date[12];

		  sprintf ( date, "%04d%02d%02d",
			    currentLocalTime->tm_year+1900,
			    currentLocalTime->tm_mon+1,
			    currentLocalTime->tm_mday );

		  string command = "/bin/cp ";

		  command += localSourcePath + "/" +
		      architecture +
		      "/update/" +
		      distributionVersion + PATCH + "/" +
		      patchname;

		  command += " " + destPatchPath + PATCH + "/" +
		      patchname +
		      "." + date + ".new";

		  YCPPath path = ".target.bash";
		  YCPString value ( command );
		  YCPValue ret = mainscragent->Execute( path, value );

		  if ( ret->isInteger() )	// success
		  {
		      if (  ret->asInteger()->value() == 0 )
		      {
			  y2debug( "%s ok", command.c_str() );
			  message += "Reading patch-description " +
			      patchname;
			  checkpatch = destPatchPath + PATCH + "/" +
			      patchname +
			      "." + date + ".new";
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

	  // setting progress-bar
	  float prog = counter;
	  progress = (int) (prog / dirList->size() * 100 +10);
	  if ( progress > 100 )
	      progress = 100;
	  counter++;
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

	      string command = "ls " + destPatchPath +
		  PATCH + "/" + patchname + ".* >/dev/null 2>>/dev/null";
	      YCPPath path = ".target.bash";
	      YCPString value ( command );
	      YCPValue ret = mainscragent->Execute( path, value );

	      if ( ret->isInteger() )	// success
	      {
		  if (  ret->asInteger()->value() != 0 ) // does not exist
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
	  }
	  // setting progress-bar
	  float prog = counter;
	  progress = (int) (prog / dirList->size() * 100 +10);
	  if ( progress > 100 )
	      progress = 100;
	  counter++;
      }
   }

   if ( posDirList == dirList->size() )
   {
       //last entry found
       cont = false;
       progress = 100;
       dirList= YCPList();

       // rereading patchinfo
       if ( currentPatchInfo != NULL )
       {
	   delete currentPatchInfo;
	   currentPatchInfo = NULL;
       }

       currentPatchInfo = new  PatchInfo ( destPatchPath + PATCH, language);
   }

   ret->add ( YCPString ( OK ), YCPBoolean ( ok ) );
   ret->add ( YCPString ( MESSAGE ), YCPString( message ) );
   ret->add ( YCPString ( CHECKPATCH ), YCPString( checkpatch ) );
   ret->add ( YCPString ( CONTINUE ), YCPBoolean( cont ) );
   ret->add ( YCPString ( PROGRESS ), YCPInteger ( progress ) );

   return ret;
}

/*--------------------------- EOF -------------------------------------------*/
