//
// YouAgent.cc
//
// Agent to communicate via YOU
//
// Maintainer: Stefan Schubert ( schubi@suse.de )
//

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <regex.h>       
#include <string>

#include <YCP.h>
#include <ycp/YCPParser.h>
#include <ycp/y2log.h>
#include "YouAgent.h"

#define MESSAGE "message"
#define OK "ok"
#define SETENVIRONMENT "setEnvironment"	// Execute
#define LASTUPDATESTATUS "lastUpdateStatus"	// Read
#define SETSERVER "setServer"	//Execute
#define CONNECT "connect"	//Execute
#define DISCONNECT "disconnect"	//Execute
#define GETPATCHLIST "getPatchList"	//Execute
#define NEWPATCHLIST "newPatchList"	//Read
#define PATCHINFORMATION "patchInformation"	//Read
#define GETPATCH "getPatch"	//Execute
#define PREINSTALLINFORMATION "preInstallInformation" //Read
#define POSTINSTALLINFORMATION "postInstallInformation" //Read
#define PACKAGES "packages"	//Read
#define PATCHUPDATESTATUS "patchUpdateStatus"	//Write
#define CLOSEUPDATE "closeUpdate"	//Execute
#define CHECKYAST2VERSION "checkYaST2Version"	//Execute
#define DELETEPACKAGES "deletePackages"	//Execute
#define ISBUSINESS "isBusiness" //READ
#define CHECKAUTHORIZATION "checkAuthorization" //Execute

/*==========================================================================
 * Public member functions
 *=========================================================================*/

/*--------------------------------------------------------------------------*
 * Constructor of the you agent
 *--------------------------------------------------------------------------*/
YouAgent::YouAgent()
{
    rpmDb = NULL;
    currentPatchInfo = NULL;
}


/*--------------------------------------------------------------------------*
 * Destructor of the you agent
 *--------------------------------------------------------------------------*/
YouAgent::~YouAgent()
{
}


/*--------------------------------------------------------------------------*
 * Execute path of the you agent
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::Execute (const YCPPath& path,
			    const YCPValue& value = YCPNull(),
			    const YCPValue& arg = YCPNull())
{
   string path_name = path->component_str (0);
   YCPValue ret = YCPVoid();

   if (  path_name == SETENVIRONMENT )
   {
      // setting user and password for the you-server
      
      if ( !value.isNull()
	   && value->isMap() )
      {
	  ret = setEnvironment( value->asMap() );
      }
   }
   else if (  path_name  == SETSERVER )
   {
      // setting server

      if ( !value.isNull()
	   && value->isMap() )
      {
	  ret = setServer ( value->asMap() );
      }
   }
   else if (  path_name  == CONNECT )
   {
      // Connecting to server

       ret = connect ( );
   }
   else if (  path_name  == DISCONNECT )
   {
      // Disconnecting from server

       ret = disconnect ( );
   }
   else if (  path_name  == GETPATCHLIST )
   {
      //Read the patch-list from the server 

       ret = getPatchList ( );
   }   
   else if (  path_name  == GETPATCH )
   {
      //  Reading patch with all RPMs from server.

       if ( !value.isNull()
	    && value->isString() )
       {
	   ret = getPatch ( value->asString() );
       }
   }
   else if (  path_name  == CLOSEUPDATE )
   {
      //  Writes the installation-status of the current installation.

       if ( !value.isNull()
	    && value->isBoolean() )
       {
	   ret = closeUpdate ( value->asBoolean() );
       }
   }
   else if (  path_name  == CHECKYAST2VERSION )
   {
      //  Check, if the patch works with the currrent installed YaST2-Version

       if ( !value.isNull()
	    && value->isString()
	    && !arg.isNull()
	    && arg->isString() )
       {
	   ret = checkYaST2Version ( value->asString(),
				     arg->asString() );
       }
   }
   else if (  path_name  == DELETEPACKAGES )
   {
      //  Deletes all packages of a patch

       if ( !value.isNull()
	    && value->isString() )
       {
	   ret = deletePackages ( value->asString() );
       }
   }
   else if ( path_name == CHECKAUTHORIZATION )
   {
       if ( !value.isNull()
	    && value->isString()
	    && !arg.isNull()
	    && arg->isString() )
       {
	   ret = checkAuthorization( value->asString(),
				     arg->asString() );
       }
   }      
   else
   {
      y2error ( "Path %s not found", path_name.c_str() );
      ret = YCPError ("Agentpath not found", YCPVoid());      
   }

   return ret;
}


/*--------------------------------------------------------------------------*
 * Read path of the you agent
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::Read(const YCPPath& path, const YCPValue& arg = YCPNull())
{
   string path_name = path->component_str (0);
   YCPValue ret = YCPVoid();

   if (  path_name == LASTUPDATESTATUS )
   {
      // Evaluate last update status
       ret = lastUpdateStatus();
   }
   else if (  path_name == NEWPATCHLIST )
   {
      // Evaluate a list of patches, which are new.
       ret = newPatchList();
   }
   else if (  path_name  == PATCHINFORMATION )
   {
       //Evaluate all informations about the patch
       if ( !arg.isNull()
	    && arg->isString() )
       {
	   ret = getPatchInformation( arg->asString() );
       }
   }
   else if (  path_name  == PREINSTALLINFORMATION )
   {
       // Returns the pre-install-information of a patch       
       if ( !arg.isNull()
	    && arg->isString() )
       {
	   ret = getPreInstallInformation( arg->asString() );
       }
   }
   else if (  path_name  == POSTINSTALLINFORMATION )
   {
       // Returns the post-install-information of a patch       
       if ( !arg.isNull()
	    && arg->isString() )
       {
	   ret = getPostInstallInformation( arg->asString() );
       }
   }
   else if (  path_name  == PACKAGES )
   {
       // Returns a list of packages ( with path ) which belong to the patch.
       if ( !arg.isNull()
	    && arg->isString() )
       {
	   ret = getPackages( arg->asString() );
       }
   }
   else if (  path_name  == ISBUSINESS )
   {
       // Returns true if the installed system is a business product.
       ret = YCPBoolean ( isBusiness() ); 
   }               
   else
   {
      y2error ( "Path %s not found", path_name.c_str() );
      ret = YCPError ("Agentpath not found", YCPVoid());      
   }

   return ret;
}


/*--------------------------------------------------------------------------*
 * Write path of the you agent ( dummy )
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::Write(const YCPPath& path, const YCPValue& value,
			 const YCPValue& arg = YCPNull())
{
   string path_name = path->component_str (0);
   YCPValue ret = YCPVoid();

   if (  path_name == PATCHUPDATESTATUS )
   {
      // Writes the installation status of a patch.
       if ( !value.isNull()
	    && value->isString()
	    && !arg.isNull()
	    && arg->isString() )
       {
	   ret = changePatchUpdateStatus ( value->asString(),
					   arg->asString() );
       }
   }
   else
   {
       y2error ( "Path %s not found", path_name.c_str() );
       ret = YCPError ("Agentpath not found", YCPVoid());      
   }

   return ret;
}


/*--------------------------------------------------------------------------*
 * Dir path of the you agent ( dummy )
 *--------------------------------------------------------------------------*/
YCPValue YouAgent::Dir(const YCPPath& path)
{
   return YCPVoid ();
}

