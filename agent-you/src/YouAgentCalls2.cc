/*************************************************************
 *
 *     YaST2      SuSE Labs                        -o)
 *     --------------------                        /\\
 *                                                _\_v
 *           www.suse.de / www.suse.com
 * ----------------------------------------------------------
 *
 * File:	  YouAgentCalls2.cc
 *
 * Author: 	  Stefan Schubert <schubi@suse.de>
 *
 * Description:   You agent calls
 *
 * $Header$
 *
 *************************************************************/

#include "YouAgentCalls.h"

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
			 // Checking if category is a installed package
			 string packageName = packageKey.name();
			 string shortDescription;
			 string longDescription;
			 string notify;
			 string delDescription;
			 string category;
			 int size;

			 patchElement.rawPackageInfo->getRawPackageDescritption(
						packageName,
						shortDescription,
						longDescription,
						notify,
						delDescription,
						category,
						size);
			 if ( category.length() > 0 )
			 {
			     string rpmVersion = "";
			     YCPPath path = ".targetpkg.info.version";
	   
			     YCPValue retpkg = mainscragent->Read( path,
								   YCPString( category ) );
			     if ( retpkg->isString()
				  && retpkg->asString()->value() != "" )	// success
			     {
				 packageInstalled = true;				 
			     }
			     else
			     {
				 y2error("<.targetpkg.info.version> System agent returned nil.");
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
static bool updateOnlyInstalled = false;
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
	    updateOnlyInstalled = patchElement.updateOnlyInstalled;    
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

   if ( updateOnlyInstalled
	&& ok
	&& posPackageList != packageList.end() )
   {
       // installing only installed packages or packages
       // which have an alias to a installed package
       y2milestone ( "Updateing only installed packages" );
       bool packageInstall = false;
       PatchList patchList = currentPatchInfo->getRawPatchList();
       PatchList::iterator patchPos;

       patchPos = patchList.find( patchName->value());

       if ( patchPos != patchList.end() )
       {
	   PatchElement patchElement = patchPos->second;

	   while ( !packageInstall
		   && posPackageList != packageList.end() )
	   {
	       PackageKey packageKey = *posPackageList;
	       
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
		   y2milestone ( "Package %s; %s<->%s",
				 packageName.c_str(),
				 version.c_str(),
				 rpmVersion.c_str() );
		   if ( CompVersion ( version, rpmVersion ) == V_NEWER )
		   {
		       packageInstall = true;
		       y2milestone ( "--->installing" );
		   }
	       }
	       else
	       {
		  // Checking if category is a installed package
		  string packageName = packageKey.name();		   
		  string shortDescription;
		  string longDescription;
		  string notify;
		  string delDescription;
		  string category;
		  int size;

		  patchElement.rawPackageInfo->getRawPackageDescritption(
						packageName,
						shortDescription,
						longDescription,
						notify,
						delDescription,
						category,
						size);

		  if ( category.length() > 0 )
		  {
		      y2milestone ( "Checking category : %s", category.c_str() );
		      YCPPath path = ".targetpkg.info.version";
		      string rpmVersion = "";
	       
		      YCPValue ret = mainscragent->Read( path,
							 YCPString( category ) );

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
			  packageInstall = true;
			  y2milestone ( "--->installing" );			   
		      }
		  }
	       }
	       if ( !packageInstall )
	       {
		   posPackageList++;
	       }
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
	       int size;
	       string shortDescription;
	       string longDescription;
	       string notify;
	       string delDescription;
	       string category;
	       YCPList packageList;
	       bool packageInstall = true;	       	       

	       patchElement.rawPackageInfo->getRawPackageInstallationInfo(
					 packageName,
					 basePackage,
					 installationPosition,
					 cdNr,
					 instPath,
					 version,
					 buildTime,
					 rpmSize );
	       patchElement.rawPackageInfo->getRawPackageDescritption(
						packageName,
						shortDescription,
						longDescription,
						notify,
						delDescription,
						category,
						size);
	       shortDescription =
		   patchElement.rawPackageInfo->getLabel( packageName );
	       
	       serie = patchElement.rawPackageInfo->getSerieOfPackage(
								 packageName );
	       if ( patchElement.updateOnlyInstalled )
	       {
		   // installing only installed packages or packages
		   // which have an alias to a installed package
		   packageInstall = false;
		   string rpmVersion = "";
		   YCPPath path = ".targetpkg.info.version";
	       
		   YCPValue ret = mainscragent->Read( path,
						      YCPString( packageName ) );

		   if ( ret->isString() )	// success
		   {
		       rpmVersion = ret->asString()->value();
		   }
		   else
		   {
		       y2error("<.targetpkg.info.version> System agent returned nil.");
		   }
		   
		   if (  rpmVersion != ""
			 && CompVersion ( version, rpmVersion ) == V_NEWER )
		   {
		       packageInstall = true;
		   }
		   else
		   {
		       // Checking if the category is a installed package
		       if ( category.length() > 0 )
		       {
			   y2milestone ( "Checking category : %s", category.c_str() );
			   YCPValue ret = mainscragent->Read( path,
							      YCPString( category ) );

			   if ( ret->isString()
				&& ret->asString()->value() != "" )	// success
			   {
			       packageInstall = true;			       
			   }
			   else
			   {
			       y2error("<.targetpkg.info.version> System agent returned nil.");
			   }
		       }
		   }
	       }
	       
	       if ( packageInstall
		    && serie != SCRIPT )
	       {
		   // only packages will be returned ( no scripts )
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
		       packageList->add ( YCPString (
						 destPatchPath + "/" + instPath ));
		   }
		   packageList->add ( YCPString ( shortDescription ));

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
