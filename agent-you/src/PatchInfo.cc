/*************************************************************
 *
 *     YaST2      SuSE Labs                        -o)
 *     --------------------                        /\\
 *                                                _\_v
 *           www.suse.de / www.suse.com
 * ----------------------------------------------------------
 *
 * File:	  PatchInfo.cc
 *
 * Author: 	  Stefan Schubert <schubi@suse.de>
 *
 * Description:   Parse the patch-directory which gives information
 *                about the patches
 *
 * $Header$
 *
 *************************************************************/

/*
 * $Log$
 * Revision 1.1  2001/11/12 16:57:25  schubi
 * agent for handling you
 *
 * Revision 1.7  2001/09/27 08:55:00  gs
 * Bugfix: close patch description file correctly
 * PatchInfo(): patchElement.rawPackageInfo->closeMedium(); added
 *
 * Revision 1.6  2001/09/06 16:04:41  schubi
 * memory optimization
 *
 * Revision 1.5  2001/07/04 16:50:47  arvin
 * - adapt for new automake/autoconf
 * - partly fix for gcc 3.0
 *
 * Revision 1.4  2001/07/04 14:25:05  schubi
 * new selection groups works besides the old
 *
 * Revision 1.3  2001/07/03 13:40:39  msvec
 * Fixed all y2log calls.
 *
 * Revision 1.2  2001/01/15 18:11:29  schubi
 * patch descriptions depends on language
 *
 * Revision 1.1  2000/11/27 09:26:49  schubi
 * ftp-update first working version
 *
 *
 */

#define _GNU_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream.h>
#include <errno.h>
#include <iconv.h>
#include <string.h>

#include <ycp/y2log.h>
#include "PatchInfo.h"

#define KIND			"Kind"
#define SHORTDESCRIPTION 	"Shortdescription."
#define LONGDESCRIPTION		"Longdescription."
#define PREINFORMATION		"Preinformation."
#define POSTINFORMATION		"Postinformation."
#define LANGUAGE		"Language"
#define DISTRIBUTION		"Distribution"
#define SIZE			"Size"
#define MINYAST1VERSION		"MinYaST1Version"
#define MINYAST2VERSION		"MinYaST2Version"
#define NEW			"new"
#define DELETED			"deleted"
#define LOAD			"load"
#define ERROR			"error"
#define INSTALLED		"installed"
#define BUILDTIME		"buildtime"
#define PACKAGES		"Packages"
#define DEFAULT			"english"



#define BUFFERLEN	1100
#define PATHLEN		256

/****************************************************************/
/* public member-functions					*/
/****************************************************************/

/*-------------------------------------------------------------*/
/* creates a PatchInfo				       	       */
/*-------------------------------------------------------------*/


PatchInfo::PatchInfo( const string path, const string desc_language )
{
  y2debug( "reading PatchInfo on the client:" );

  DIR *dir = opendir( path.c_str() );

  patchlist.clear();

  if (!dir)
  {
     y2error( "Can't open directory %s for reading",
	      path.c_str());
     return;
  }
  else
  {
     struct dirent *entry;
     while ((entry = readdir(dir)))
     {
	if (!strcmp(entry->d_name, ".") ||
	    !(strcmp(entry->d_name, ".."))) continue;

	// check, if it is a patch-file
	string filename ( entry->d_name );
	string::size_type pos = filename.find ( "-" );

	if ( pos != string::npos )
	{
	   // patch-file found
	   ConfigFile patchEntries ( path + "/" + filename );
	   Entries entries;

	   PatchElement patchElement ( path, filename );  // Reading package-
	   // Information in the cunstructor of PatchElement

	   entries.clear();

	   Values kindValues;
	   Element kindElement;
	   kindElement.values = kindValues;
	   kindElement.multiLine = false;

	   Values shortDescriptionValues;
	   Element shortDescriptionElement;
	   shortDescriptionElement.values = shortDescriptionValues;
	   shortDescriptionElement.multiLine = false;

	   Values defaultShortDescriptionValues;
	   Element defaultShortDescriptionElement;
	   defaultShortDescriptionElement.values =
	      defaultShortDescriptionValues;
	   defaultShortDescriptionElement.multiLine = false;

	   Values longDescriptionValues;
	   Element longDescriptionElement;
	   longDescriptionElement.values = longDescriptionValues;
	   longDescriptionElement.multiLine = true;

	   Values defaultLongDescriptionValues;
	   Element defaultLongDescriptionElement;
	   defaultLongDescriptionElement.values =
	      defaultLongDescriptionValues;
	   defaultLongDescriptionElement.multiLine = true;

	   Values preInformationValues;
	   Element preInformationElement;
	   preInformationElement.values = preInformationValues;
	   preInformationElement.multiLine = true;

	   Values defaultPreInformationValues;
	   Element defaultPreInformationElement;
	   defaultPreInformationElement.values = defaultPreInformationValues;
	   defaultPreInformationElement.multiLine = true;

	   Values postInformationValues;
	   Element postInformationElement;
	   postInformationElement.values = postInformationValues;
	   postInformationElement.multiLine = true;

	   Values defaultPostInformationValues;
	   Element defaultPostInformationElement;
	   defaultPostInformationElement.values =
	      defaultPostInformationValues;
	   defaultPostInformationElement.multiLine = true;

	   Values languageValues;
	   Element languageElement;
	   languageElement.values = languageValues;
	   languageElement.multiLine = false;

	   Values packagesValues;
	   Element packagesElement;
	   packagesElement.values = packagesValues;
	   packagesElement.multiLine = true;

	   Values distributionValues;
	   Element distributionElement;
	   distributionElement.values = distributionValues;
	   distributionElement.multiLine = false;

	   Values sizeValues;
	   Element sizeElement;
	   sizeElement.values = sizeValues;
	   sizeElement.multiLine = false;

	   Values minYaST1VersionValues;
	   Element minYaST1VersionElement;
	   minYaST1VersionElement.values = minYaST1VersionValues;
	   minYaST1VersionElement.multiLine = false;

	   Values minYaST2VersionValues;
	   Element minYaST2VersionElement;
	   minYaST2VersionElement.values = minYaST2VersionValues;
	   minYaST2VersionElement.multiLine = false;

	   Values buildtimeValues;
	   Element buildtimeElement;
	   buildtimeElement.values = buildtimeValues;
	   buildtimeElement.multiLine = false;


	   entries.insert(pair<const string, const Element>
			  ( KIND, kindElement ) );

	   entries.insert(pair<const string, const Element>
			  ( SHORTDESCRIPTION+desc_language,
			    shortDescriptionElement ) );
	   string default_str = SHORTDESCRIPTION;
	   default_str += DEFAULT;
	   entries.insert(pair<const string, const Element>
			  ( default_str,
			    defaultShortDescriptionElement ) );

	   entries.insert(pair<const string, const Element>
			  ( LONGDESCRIPTION+desc_language,
			    longDescriptionElement ) );
	   default_str = LONGDESCRIPTION;
	   default_str += DEFAULT;
	   entries.insert(pair<const string, const Element>
			  ( default_str,
			    defaultLongDescriptionElement ) );

	   entries.insert(pair<const string, const Element>
			  ( PREINFORMATION+desc_language,
			    preInformationElement ) );
	   default_str = PREINFORMATION;
	   default_str += DEFAULT;
	   entries.insert(pair<const string, const Element>
			  ( default_str,
			    defaultPreInformationElement ) );

	   entries.insert(pair<const string, const Element>
			  ( POSTINFORMATION+desc_language,
			    postInformationElement ) );
	   default_str = POSTINFORMATION;
	   default_str += DEFAULT;
	   entries.insert(pair<const string, const Element>
			  ( default_str,
			    defaultPostInformationElement ) );

	   entries.insert(pair<const string, const Element>
			  ( LANGUAGE, languageElement ) );
	   entries.insert(pair<const string, const Element>
			  ( DISTRIBUTION, distributionElement ) );
	   entries.insert(pair<const string, const Element>
			  ( SIZE, sizeElement ) );
	   entries.insert(pair<const string, const Element>
			  ( MINYAST1VERSION, minYaST1VersionElement ) );
	   entries.insert(pair<const string, const Element>
			  ( MINYAST2VERSION, minYaST2VersionElement ) );
	   entries.insert(pair<const string, const Element>
			  ( PACKAGES, packagesElement ) );
	   entries.insert(pair<const string, const Element>
			  ( BUILDTIME, buildtimeElement ) );


	   patchEntries.readFile ( entries, " :" );
	   Entries::iterator posEntries;

	   posEntries = entries.find ( KIND );
	   if ( posEntries != entries.end() );
	   {
	      Values values = (posEntries->second).values;
	      Values::iterator posValues;
	      for ( posValues = values.begin(); posValues != values.end() ;
		    ++posValues )
	      {
		 patchElement.kind += *posValues;
	      }
	   }

	   posEntries = entries.find ( SHORTDESCRIPTION+desc_language );
	   if ( posEntries != entries.end() );
	   {
	      Values values = (posEntries->second).values;
	      Values::iterator posValues;
	      for ( posValues = values.begin(); posValues != values.end() ;
		    ++posValues )
	      {
		 patchElement.shortDescription += *posValues + " ";
	      }
	   }
	   if ( patchElement.shortDescription.length() <= 0 )
	   {
	      // default-entry
	      string default_str = SHORTDESCRIPTION;
	      default_str += DEFAULT;
	      posEntries = entries.find ( default_str );
	      if ( posEntries != entries.end() );
	      {
		 Values values = (posEntries->second).values;
		 Values::iterator posValues;
		 for ( posValues = values.begin(); posValues != values.end() ;
		       ++posValues )
		 {
		    patchElement.shortDescription += *posValues + " ";
		 }
	      }
	   }

	   posEntries = entries.find ( LONGDESCRIPTION+desc_language );
	   if ( posEntries != entries.end() );
	   {
	      Values values = (posEntries->second).values;
	      Values::iterator posValues;
	      for ( posValues = values.begin(); posValues != values.end() ;
		    ++posValues )
	      {
		 patchElement.longDescription += *posValues + "\n";
	      }
	   }
	   if ( patchElement.longDescription.length() <= 0 )
	   {
	      //default-entry
	      string default_str = LONGDESCRIPTION;
	      default_str += DEFAULT;
	      posEntries = entries.find ( default_str );
	      if ( posEntries != entries.end() );
	      {
		 Values values = (posEntries->second).values;
		 Values::iterator posValues;
		 for ( posValues = values.begin(); posValues != values.end() ;
		       ++posValues )
		 {
		    patchElement.longDescription += *posValues + "\n";
		 }
	      }
	   }

	   posEntries = entries.find ( PREINFORMATION + desc_language );
	   if ( posEntries != entries.end() );
	   {
	      Values values = (posEntries->second).values;
	      Values::iterator posValues;
	      for ( posValues = values.begin(); posValues != values.end() ;
		    ++posValues )
	      {
		 patchElement.preInformation += *posValues + "\n";
	      }
	   }
	   if ( patchElement.preInformation.length() <= 0 )
	   {
	      // default entry
	      string default_str = PREINFORMATION;
	      default_str += DEFAULT;
	      posEntries = entries.find ( default_str );
	      if ( posEntries != entries.end() );
	      {
		 Values values = (posEntries->second).values;
		 Values::iterator posValues;
		 for ( posValues = values.begin(); posValues != values.end() ;
		       ++posValues )
		 {
		    patchElement.preInformation += *posValues + "\n";
		 }
	      }
	   }

	   posEntries = entries.find ( POSTINFORMATION+desc_language );
	   if ( posEntries != entries.end() );
	   {
	      Values values = (posEntries->second).values;
	      Values::iterator posValues;
	      for ( posValues = values.begin(); posValues != values.end() ;
		    ++posValues )
	      {
		 patchElement.postInformation += *posValues + "\n";
	      }
	   }
	   if ( patchElement.postInformation.length() <= 0 )
	   {
	      // default entry
	      string default_str = POSTINFORMATION;
	      default_str += DEFAULT;
	      posEntries = entries.find ( default_str );
	      if ( posEntries != entries.end() );
	      {
		 Values values = (posEntries->second).values;
		 Values::iterator posValues;
		 for ( posValues = values.begin(); posValues != values.end() ;
		       ++posValues )
		 {
		    patchElement.postInformation += *posValues + "\n";
		 }
	      }
	   }

	   posEntries = entries.find ( LANGUAGE );
	   if ( posEntries != entries.end() );
	   {
	      Values values = (posEntries->second).values;
	      Values::iterator posValues;
	      for ( posValues = values.begin(); posValues != values.end() ;
		    ++posValues )
	      {
		 patchElement.language += *posValues;
	      }
	   }

	   posEntries = entries.find ( DISTRIBUTION );
	   if ( posEntries != entries.end() );
	   {
	      Values values = (posEntries->second).values;
	      Values::iterator posValues;
	      for ( posValues = values.begin(); posValues != values.end() ;
		    ++posValues )
	      {
		 patchElement.distribution += *posValues + " ";
	      }
	   }

	   posEntries = entries.find ( SIZE );
	   if ( posEntries != entries.end() );
	   {
	      Values values = (posEntries->second).values;
	      Values::iterator posValues;
	      for ( posValues = values.begin(); posValues != values.end() ;
		    ++posValues )
	      {
		 patchElement.size += *posValues;
	      }
	   }

	   posEntries = entries.find ( BUILDTIME );
	   if ( posEntries != entries.end() );
	   {
	      Values values = (posEntries->second).values;
	      Values::iterator posValues;
	      for ( posValues = values.begin(); posValues != values.end() ;
		    ++posValues )
	      {
		 patchElement.buildtime += *posValues;
	      }
	   }

	   posEntries = entries.find ( MINYAST1VERSION );
	   if ( posEntries != entries.end() );
	   {
	      Values values = (posEntries->second).values;
	      Values::iterator posValues;
	      for ( posValues = values.begin(); posValues != values.end() ;
		    ++posValues )
	      {
		 patchElement.minYaST1Version += *posValues;
	      }
	   }

	   posEntries = entries.find ( MINYAST2VERSION );
	   if ( posEntries != entries.end() );
	   {
	      Values values = (posEntries->second).values;
	      Values::iterator posValues;
	      for ( posValues = values.begin(); posValues != values.end() ;
		    ++posValues )
	      {
		 patchElement.minYaST2Version += *posValues;
	      }
	   }

	   pos = filename.find_last_of(".");
	   if ( pos != string::npos )
	   {
	      // Extract patch-state
	      string name = filename.substr(0,pos);
	      patchElement.state = filename.substr(pos+1);

	      // Extract date
	      pos = name.find_last_of (".");
	      if ( pos != string::npos )
	      {
		 patchElement.id = name.substr(0,pos);
		 string dummyDate = name.substr(pos+1);
		 patchElement.date = dummyDate;
	      }
	      else
	      {
		 patchElement.id = name;
	      }
	   }
	   else
	   {
	      patchElement.id = filename;
	   }

	   patchlist.insert(pair<const string, const PatchElement>
			    (patchElement.id, patchElement));
	   y2debug("Patch: ID :%s",   patchElement.id.c_str());
	   y2debug("      kind :%s", patchElement.kind.c_str());
	   y2debug("      shortdesc. :%s", patchElement.shortDescription.c_str());
	   y2debug("      longdesc. :%s", patchElement.longDescription.c_str());
	   y2debug("      preInformation :%s", patchElement.preInformation.c_str());
	   y2debug("      postInformation :%s", patchElement.postInformation.c_str());
	   y2debug("      language :%s", patchElement.language.c_str());
	   y2debug("      distribution :%s", patchElement.distribution.c_str());
	   y2debug("      size :%s", patchElement.size.c_str());
	   y2debug("      buildtime :%s", patchElement.buildtime.c_str());
	   y2debug("      minYaST1Version :%s", patchElement.minYaST1Version.c_str());
	   y2debug("      minYaST2Version :%s", patchElement.minYaST2Version.c_str());
	   y2debug("      state :%s", patchElement.state.c_str());
	   y2debug("      date :%s", patchElement.date.c_str());

	   patchElement.rawPackageInfo->closeMedium();
	}
     }
     if ( dir )
     {
	closedir(dir);
     }
  }
}

/*--------------------------------------------------------------*/
/* Cleans up						       	*/
/*--------------------------------------------------------------*/
PatchInfo::~PatchInfo()
{
}

/**
 *  Clears the PackageElement.
 */

PatchElement::PatchElement ( const string patchPath,
			     const string patchFile  )
{
   id = "";
   kind = "";
   shortDescription = "";
   longDescription = "";
   preInformation = "";
   postInformation = "";
   language = "";
   distribution = "";
   size = "";
   buildtime = "";
   minYaST1Version = "";
   minYaST2Version = "";
   state = "";
   date = "";
   rawPackageInfo = new RawPackageInfo ( patchPath, "",
					 patchFile );
}
