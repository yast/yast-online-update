/*************************************************************
 *    
 *     YaST2      SuSE Labs                        -o)
 *     --------------------                        /\\
 *                                                _\_v
 *           www.suse.de / www.suse.com
 * ----------------------------------------------------------
 *
 * Author: 	  Stefan Schubert  <schubi@suse.de>
 *
 * File:	  PatchInfo.h
 * Description:   header file for class PatchInfo
 *
 *
 *************************************************************/

#ifndef PatchInfo_h
#define PatchInfo_h

#include <map>
#include <set>
#include <string>
#include <iconv.h>

#include <pkg/RawPackageInfo.h>
#include <pkg/ConfigFile.h>

#define DEFAULTPATCHPATH	"/var/lib/YaST2/patches"

/**
 * @short Parse <patch-id>-directory which gives information about the patches
 *        
 */

typedef map<string,string> FtpPackageList;
    
struct PatchElement {
       
   PatchElement ( const string patchPath, const string patchFile );

   string id;
   string kind;  // [security|recommended|hotfix]
   string shortDescription;
   string longDescription;
   string preInformation;
   string postInformation;
   string installTrigger;
   string language;
   string distribution;
   string size;
   string buildtime;
   string minYaST1Version;
   string minYaST2Version;
   string state;
   string date;
   RawPackageInfo *rawPackageInfo;
   bool updateOnlyInstalled;    
};

typedef map<string,struct PatchElement> PatchList;



class PatchInfo
{
public:
   
   PatchInfo( const string path,const string desc_language );
   // Konstruktor; "path" of the patch-directory
    
   ~PatchInfo();

   PatchList getRawPatchList(){return patchlist;}
   // Return the list of patches
protected:
   PatchList patchlist;
};


#endif // PatchInfo_h
