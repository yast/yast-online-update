// -*- c++ -*-

#ifndef YouAgent_h
#define YouAgent_h

#include <scr/SCRAgent.h>
#include <scr/SCRInterpreter.h>
#include <Y2.h>
#include "PatchInfo.h"

/**
 * @short SCR Agent for YOU transfer
 */

typedef enum _serverKind {
      NO,
      FTP,
      HTTP,
      CD,
      HD,
      NFS
} ServerKind;

typedef enum _CompareVersion {
      V_OLDER,
      V_EQUAL,
      V_NEWER,
      V_UNCOMP
} CompareVersion;


class YouAgent : public SCRAgent 
{
public:
    YouAgent();
    ~YouAgent();

    /**
     * Reads data. Destroy the result after use.
     * @param path Specifies what part of the subtree should
     * be read. The path is specified _relatively_ to Root()!
     */
    YCPValue Read(const YCPPath& path, const YCPValue& arg = YCPNull());

   YCPValue Execute (const YCPPath& path,
		     const YCPValue& value = YCPNull(),
		     const YCPValue& arg = YCPNull());
   
    /**
     * Writes data. 
     */
    YCPValue Write(const YCPPath& path, const YCPValue& value,
		   const YCPValue& arg = YCPNull());

    /**
     * Get a list of all subtrees.
     */
    YCPValue Dir(const YCPPath& path);
   
protected:

   /**
    * Initialize the server for ftp-update with the environment
    * delivered by the setMap:
    * $[ 
    *   "language":"german",
    *	"architecture":"i386",
    *	"rootpath":"/",
    *   "yastpath":"/var/lib/YaST",
    *	"patchpath":"/var/lib/YaST/patches"]
    * Returns true or false
    **/
   YCPValue	setEnvironment ( YCPMap setMap );

   /**
    * Evaluate last update-status:
    * Returns a map like :
    * $[ "days":"3","status":"ok",
    *    "patches":[ <patch-ID1>, <patch-mode1>, <patch-description1>,
    *		     <update-date1>,<status1>, 	
    *                <patch-ID2>, <patch-mode2>, <patch-description2>,
    *		     <update-date2>,<status2>, ...	]]
    **/
   YCPValue 	lastUpdateStatus ( void );

   /**
    * setting server
    * Parameter: $[ "name": "ftp.suse.com", "path": "/pub/suse",
    *		    "kind": "ftp" ]
    * Returns true of false
    **/
   YCPValue	setServer ( YCPMap server );

   /**
    * Connecting to server
    * 
    * Returns $[ "ok": false, "message","Connection refused" ]
    **/
   YCPValue	connect ( );

   /**
    * Disconnecting from server
    * 
    * Returns $[ "ok": false, "message","not connected" ]
    **/
   YCPValue	disconnect ( );
   
   /**
    * Read the patch-list from the server
    * Returns $[ "ok": false, "message","not connected",
    *            "continue": false, "progress", 10]
    **/
   YCPValue	getPatchList (  );

   /**
    * Evaluate a list of patches, which are new. Returns a map like
    * $[ <patch-ID1>:[<patch-mode1>,<patch-description1>,<size1>,<to-install1>],
    *    <patch-ID2>:[<patch-mode2>,<patch-description2>,<size2>,<to_install2>],...
    *  ]
    **/
   YCPValue	newPatchList ( void );

   /**
    * Evaluate all informations about the patch
    * Returns a map like:
    * $[ "description":"text","packages":
    *  $[<package-name1>:[<description1>,<installed-version1>,<new-version1>],
    *   <package-name2>:[<description2>,<installed-version2>,<new-version2>],..
    *   ]]
    **/
   YCPValue 	getPatchInformation ( const YCPString patchName );

   /**
    * Reading patch with all RPMs from server.
    * Returns $[ "ok": false, "message","not connected","nextPackage",
    * <package-path>, "nextPackageSize", <byte>]
    **/
   YCPValue	getPatch ( const YCPString patcheName );

   /**
    * Returns the pre-install-information of a patch
    **/
   YCPValue	getPreInstallInformation ( const YCPString patchName );

   /**
    * Returns the post-install-information of a patch
    **/
   YCPValue	getPostInstallInformation ( const YCPString patchName );
   
   /**
    * Returns a list of packages ( with path ) which belong to the patch.
    **/
   YCPValue	getPackages ( const YCPString patchName );

   /**
    * Check, if the patch works with the currrent installed YaST2-Version
    * Returns $[ "ok": false, "minYaST2Version,"1.0.0" ]
    **/
   YCPValue	checkYaST2Version ( const YCPString installedYaST2Version,
				       const YCPString patchName );

   /**
    * Deletes all packages of a patch
    * Returns true or false
    **/
   YCPValue	deletePackages ( const YCPString patchName );
   
   /**
    * Writes the installation-status of a patch.
    * Valid stati: installed, error, new
    * Returns true or false
    **/
   YCPValue	changePatchUpdateStatus (const YCPString patchName,
					    const YCPString status );

   /**
    * Writes the installation-status of the current installation.
    * Returns true or false
    **/
   YCPValue	closeUpdate ( const YCPBoolean success );

   /** 
    * Is the installed system a business product ?
    * Returns true or false
    **/
    bool isBusiness ( void );

   /**
    * Checking authorization of the user	
    * Returns stati: "ok", "error_login", "error_path"
    **/
    YCPValue checkAuthorization ( YCPString password,
					    YCPString registrationCode );

   /** 
    * Getting SuSE product informations.
    * Returns a map like [ "Distribution_Verison":"7.2",
    *			   "Product_Version":"3.0",
    *			   "Product_Name":"Email Server" ]
    **/
    YCPValue getProductInfo ( void );

    /**
     * Create all parent directories of @param name, as necessary
     **/
    void create_directories(string name);
    
    /**
     * compare two versions of a package    
     **/
    CompareVersion CompVersion( string left,
				string right );


   string server; // like ftp.suse.com
   ServerKind serverKind;
   string architecture; // like i386
   string distributionVersion; // like 7.0
   string productVersion;
   string productName; 
   string destPatchPath; // like /var/lib/YaST/patches
   string ftpLogFilename;
   string sourcePath; // of the server
   string localSourcePath;
   string httpPassword;
   string httpUser;
   string rootPath; // Root-Path ( update )
   string yastPath; // Path of YaST-directory
   string language; // Selected language
   string httpProxyUser;
   string httpProxyPassword; 
   PatchInfo *currentPatchInfo; //Patch-info about all patches on the client   
};


#endif // YouAgent_h
