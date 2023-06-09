// ---------------------------------------------------------------------------------------------
// --------------- WARNING: this code is generated by an automatic code generator --------------
// ---------------------------------------------------------------------------------------------
// -------------- WARNING: any modifications made to this code will be overwritten -------------
// ---------------------------------------------------------------------------------------------

#include "../include/us_saxs_util.h"
#include "../include/us_cmdline_app.h"

bool US_Saxs_Util::run_dammin()
{
   errormsg = "";
   QStringList args;
   QStringList app_text;
   QStringList response;

   if ( control_parameters.count( "dammingnomfile" ) )
   {
      QFileInfo fi( control_parameters[ "dammingnomfile" ] );
      QString baseName = fi.baseName ( true  );
      if ( baseName.length() > 8 )
      {
          QString ext      = fi.extension( false );
          int     use_len  = 8;
          QString destname = baseName.left( use_len );
          int     copy     = 0;
          while ( QFile::exists( destname ) )
          {
             QString copy_str = QString( "%1" ).arg( copy );
             if ( copy_str.length() > 7 )
             {
                errormsg = 
                    QString( "Error: dammin: too many attempts to create unique 8 character name for %1" )
                    .arg( control_parameters[ "dammingnomfile" ] );
                return false;
             }
             use_len = 8 - copy_str.length();
             destname = baseName.left( use_len ) + QString( "%1" ).arg( ++copy );
          }
          control_parameters[ "dammingnom8file" ] = 
              baseName.left( 8 ) + "." + ext;
      } else {
          control_parameters[ "dammingnom8file" ] = 
             control_parameters[ "dammingnomfile" ];
      }
   } else {
      errormsg = "Error: dammin: parameter dammingnomfile must be defined";
      return false;
   }
   if ( control_parameters.count( "dammingnom8file" ) )
   {
      QFileInfo fi( control_parameters[ "dammingnom8file" ] );
      control_parameters[ "damminname" ] = fi.baseName( true );
   } else {
      errormsg = "Error: dammin: parameter dammingnom8file must be defined";
      return false;
   }
   if ( !control_parameters.count( "dammingnomfile" ) )
   {
      errormsg = 
         "Error: dammin: required parameter dammingnomfile not defined";
      return false;
   }
   if ( !control_parameters.count( "damminmode" ) )
   {
      errormsg = 
         "Error: dammin: required parameter damminmode not defined";
      return false;
   }
   if ( control_parameters.count( "damminmode" ) )
   {
      app_text << "[E]xpert";
      response << control_parameters[ "damminmode" ];
   }
   if ( control_parameters.count( "damminname" ) )
   {
      app_text << "Log file name";
      response << control_parameters[ "damminname" ];
   }
   if ( control_parameters.count( "damminname" ) )
   {
      app_text << "Input data, GNOM output file name";
      response << control_parameters[ "damminname" ];
   }
   if ( control_parameters.count( "dammindescription" ) )
   {
      app_text << "Enter project description";
      response << control_parameters[ "dammindescription" ];
   }
   if ( control_parameters.count( "damminangularunits" ) )
   {
      app_text << "Angular units in the input file:";
      response << control_parameters[ "damminangularunits" ];
   }
   if ( control_parameters.count( "damminfitcurvelimit" ) )
   {
      app_text << "Portion of the curve to be fitted ...... <";
      response << control_parameters[ "damminfitcurvelimit" ];
   }
   if ( control_parameters.count( "damminknotstofit" ) )
   {
      app_text << "Number of knots in the curve to fit .... <";
      response << control_parameters[ "damminknotstofit" ];
   }
   if ( control_parameters.count( "damminconstantsubtractionprocedure" ) )
   {
      app_text << "Zero for automatic subtraction ......... <";
      response << control_parameters[ "damminconstantsubtractionprocedure" ];
   }
   if ( control_parameters.count( "damminmaxharmonics" ) )
   {
      app_text << "Maximum order of harmonics ............. <";
      response << control_parameters[ "damminmaxharmonics" ];
   }
   if ( control_parameters.count( "dammininitialdamtype" ) )
   {
      app_text << "Initial DAM: type S for sphere [default]";
      response << control_parameters[ "dammininitialdamtype" ];
   }
   if ( control_parameters.count( "damminsymmetry" ) )
   {
      app_text << "Symmetry: P1...19";
      response << control_parameters[ "damminsymmetry" ];
   }
   if ( control_parameters.count( "damminspherediameter" ) )
   {
      app_text << "Sphere  diameter [Angstrom] ............ <";
      response << control_parameters[ "damminspherediameter" ];
   }
   if ( control_parameters.count( "damminpackingradius" ) )
   {
      app_text << "Packing radius of dummy atoms .......... <";
      response << control_parameters[ "damminpackingradius" ];
   }
   if ( control_parameters.count( "damminexpectedshape" ) )
   {
      app_text << "Expected particle shape: <P>rolate, <O>blate";
      response << control_parameters[ "damminexpectedshape" ];
   }
   if ( control_parameters.count( "damminradius1stcoordinationsphere" ) )
   {
      app_text << "Radius of 1st coordination sphere ...... <";
      response << control_parameters[ "damminradius1stcoordinationsphere" ];
   }
   if ( control_parameters.count( "damminloosenesspenaltyweight" ) )
   {
      app_text << "Looseness penalty weight ............... <";
      response << control_parameters[ "damminloosenesspenaltyweight" ];
   }
   if ( control_parameters.count( "dammindisconnectivitypenaltyweight" ) )
   {
      app_text << "Disconnectivity penalty weight ......... <";
      response << control_parameters[ "dammindisconnectivitypenaltyweight" ];
   }
   if ( control_parameters.count( "damminperipheralpenaltyweight" ) )
   {
      app_text << "Peripheral penalty weight .............. <";
      response << control_parameters[ "damminperipheralpenaltyweight" ];
   }
   if ( control_parameters.count( "damminfixingthersholdslosandrf" ) )
   {
      app_text << "Fixing thresholds Los and Rf <";
      response << control_parameters[ "damminfixingthersholdslosandrf" ];
   }
   if ( control_parameters.count( "damminrandomizestructure" ) )
   {
      app_text << "Randomize the structure [ Y / N ] ...... <";
      response << control_parameters[ "damminrandomizestructure" ];
   }
   if ( control_parameters.count( "damminweight" ) )
   {
      app_text << "Weight: 0=s^2, 1=Emphas.s->0, 2=Log .... <";
      response << control_parameters[ "damminweight" ];
   }
   if ( control_parameters.count( "dammininitialscalefactor" ) )
   {
      app_text << "Initial scale factor ................... <";
      response << control_parameters[ "dammininitialscalefactor" ];
   }
   if ( control_parameters.count( "damminfixscalefactor" ) )
   {
      app_text << "Fix the scale factor [ Y / N ] ......... <";
      response << control_parameters[ "damminfixscalefactor" ];
   }
   if ( control_parameters.count( "dammininitialannealingtemperature" ) )
   {
      app_text << "Initial annealing temperature .......... <";
      response << control_parameters[ "dammininitialannealingtemperature" ];
   }
   if ( control_parameters.count( "damminannealingschedulefactor" ) )
   {
      app_text << "Annealing schedule factor .............. <";
      response << control_parameters[ "damminannealingschedulefactor" ];
   }
   if ( control_parameters.count( "damminnumberofindependentatomstomodify" ) )
   {
      app_text << "# of independent atoms to modify ....... <";
      response << control_parameters[ "damminnumberofindependentatomstomodify" ];
   }
   if ( control_parameters.count( "damminmaxnumberiterationseacht" ) )
   {
      app_text << "Max # of iterations at each T .......... <";
      response << control_parameters[ "damminmaxnumberiterationseacht" ];
   }
   if ( control_parameters.count( "damminmaxnumbersuccesseseacht" ) )
   {
      app_text << "Max # of successes at each T ........... <";
      response << control_parameters[ "damminmaxnumbersuccesseseacht" ];
   }
   if ( control_parameters.count( "damminminnumbersuccessestocontinue" ) )
   {
      app_text << "Min # of successes to continue ......... <";
      response << control_parameters[ "damminminnumbersuccessestocontinue" ];
   }
   if ( control_parameters.count( "damminmaxnumberannealingsteps" ) )
   {
      app_text << "Max # of annealing steps ............... <";
      response << control_parameters[ "damminmaxnumberannealingsteps" ];
   }
   if ( control_parameters.count( "damminexpectedshape" ) )
   {
      app_text << "Expected particle shape: <P>rolate, <O>blate";
      response << control_parameters[ "damminexpectedshape" ];
   }
   app_text << "====  Simulated annealing procedure started  ====";
   response << "___run___";
   args     << "dammin";


   // check requireed parameters defined ?

   int argc = 0;
   // const char *argv0 = "us_saxs_util";
   // const char **argv = &argv0;
   char **argv = (char **)0;
   QApplication app( argc, argv, false );

   QString cla_error_msg;
   US_Cmdline_App cla(
                      &app,
                      "", // dir
                      args,
                      app_text,
                      response,
                      &cla_error_msg,
                      240000
                      );

   if ( !cla_error_msg.isEmpty() )
   {
      errormsg = cla_error_msg;
      return false;
   }

   app.exec();

   if ( !cla_error_msg.isEmpty() )
   {
      errormsg = "Error: run_dammin():" + cla_error_msg;
      return false;
   }
   if ( control_parameters.count( "damminname" ) )
   {
      control_parameters[ "dammin1pdb" ] = 
          control_parameters[ "damminname" ] + "-1.pdb";
   }
   if ( control_parameters.count( "damminname" ) )
   {
      control_parameters[ "damminfit" ] = 
          control_parameters[ "damminname" ] + ".fit";
   }
   if ( control_parameters.count( "damminname" ) )
   {
      control_parameters[ "damminfir" ] = 
          control_parameters[ "damminname" ] + ".fir";
   }
   if ( control_parameters.count( "damminname" ) )
   {
      control_parameters[ "damminlog" ] = 
          control_parameters[ "damminname" ] + ".log";
   }
   if ( control_parameters.count( "dammingnomfile" ) )
   {
      QFileInfo fi( control_parameters[ "dammingnomfile" ] );
      control_parameters[ "damminfullname" ] = fi.baseName( true );
   } else {
      errormsg = "Error: dammin: parameter dammingnomfile must be defined";
      return false;
   }
   if ( control_parameters.count( "dammin1pdb" ) &&
        control_parameters.count( "damminfullname" ) )
   {
      QFileInfo fi( control_parameters[ "dammin1pdb" ] );
      QString baseName = fi.baseName ( true  );
      QString ext      = fi.extension( false );

      if ( !fi.exists() )
      {
          errormsg = QString( "Error: dammin: the expected output file %1 does not exist" )
              .arg( control_parameters[ "dammin1pdb" ] );
          return false;
      }

      int copy = 0;
      QString destfile = control_parameters[ "damminfullname" ] + "." + ext;
      while ( QFile::exists( destfile ) )
      {
          destfile = control_parameters[ "damminfullname" ] + QString( "_%1" ).arg( ++copy ) + "." + ext;
      }
      QDir qd;
      if ( !qd.rename( control_parameters[ "dammin1pdb" ], destfile ) )
      {
          errormsg = QString( "Error renaming %1 to %2" )
              .arg( control_parameters[ "dammin1pdb" ] )
              .arg( destfile );
          return false;
      }
      output_files << destfile;
   }
   if ( control_parameters.count( "damminfit" ) &&
        control_parameters.count( "damminfullname" ) )
   {
      QFileInfo fi( control_parameters[ "damminfit" ] );
      QString baseName = fi.baseName ( true  );
      QString ext      = fi.extension( false );

      if ( !fi.exists() )
      {
          errormsg = QString( "Error: dammin: the expected output file %1 does not exist" )
              .arg( control_parameters[ "damminfit" ] );
          return false;
      }

      int copy = 0;
      QString destfile = control_parameters[ "damminfullname" ] + "." + ext;
      while ( QFile::exists( destfile ) )
      {
          destfile = control_parameters[ "damminfullname" ] + QString( "_%1" ).arg( ++copy ) + "." + ext;
      }
      QDir qd;
      if ( !qd.rename( control_parameters[ "damminfit" ], destfile ) )
      {
          errormsg = QString( "Error renaming %1 to %2" )
              .arg( control_parameters[ "damminfit" ] )
              .arg( destfile );
          return false;
      }
      output_files << destfile;
   }
   if ( control_parameters.count( "damminfir" ) &&
        control_parameters.count( "damminfullname" ) )
   {
      QFileInfo fi( control_parameters[ "damminfir" ] );
      QString baseName = fi.baseName ( true  );
      QString ext      = fi.extension( false );

      if ( !fi.exists() )
      {
          errormsg = QString( "Error: dammin: the expected output file %1 does not exist" )
              .arg( control_parameters[ "damminfir" ] );
          return false;
      }

      int copy = 0;
      QString destfile = control_parameters[ "damminfullname" ] + "." + ext;
      while ( QFile::exists( destfile ) )
      {
          destfile = control_parameters[ "damminfullname" ] + QString( "_%1" ).arg( ++copy ) + "." + ext;
      }
      QDir qd;
      if ( !qd.rename( control_parameters[ "damminfir" ], destfile ) )
      {
          errormsg = QString( "Error renaming %1 to %2" )
              .arg( control_parameters[ "damminfir" ] )
              .arg( destfile );
          return false;
      }
      output_files << destfile;
   }
   if ( control_parameters.count( "damminlog" ) &&
        control_parameters.count( "damminfullname" ) )
   {
      QFileInfo fi( control_parameters[ "damminlog" ] );
      QString baseName = fi.baseName ( true  );
      QString ext      = fi.extension( false );

      if ( !fi.exists() )
      {
          errormsg = QString( "Error: dammin: the expected output file %1 does not exist" )
              .arg( control_parameters[ "damminlog" ] );
          return false;
      }

      int copy = 0;
      QString destfile = control_parameters[ "damminfullname" ] + "." + ext;
      while ( QFile::exists( destfile ) )
      {
          destfile = control_parameters[ "damminfullname" ] + QString( "_%1" ).arg( ++copy ) + "." + ext;
      }
      QDir qd;
      if ( !qd.rename( control_parameters[ "damminlog" ], destfile ) )
      {
          errormsg = QString( "Error renaming %1 to %2" )
              .arg( control_parameters[ "damminlog" ] )
              .arg( destfile );
          return false;
      }
      output_files << destfile;
   }

   return true;
}
