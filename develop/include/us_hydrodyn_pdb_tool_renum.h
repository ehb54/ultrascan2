// ---------------------------------------------------------------------------------------------
// --------------- WARNING: this code is generated by an automatic code generator --------------
// ---------------------------------------------------------------------------------------------
// -------------- WARNING: any modifications made to this code will be overwritten -------------
// ---------------------------------------------------------------------------------------------

#ifndef US_HYDRODYN_PDB_TOOL_RENUM_H
#define US_HYDRODYN_PDB_TOOL_RENUM_H

#include "us_hydrodyn_pdb_tool.h"
#include "qlabel.h"
#include "qstring.h"
#include "qlayout.h"
#include "qlineedit.h"
#include "qfontmetrics.h"
#include "qfile.h"
#include "qfiledialog.h"

using namespace std;

class US_EXTERN US_Hydrodyn_Pdb_Tool_Renum : public QDialog
{
   Q_OBJECT

   public:
      US_Hydrodyn_Pdb_Tool_Renum(
                                 void                     *              us_hydrodyn,
                                 map < QString, QString > *              parameters,
                                 QWidget *                               p = 0,
                                 const char *                            name = 0
                                 );

      ~US_Hydrodyn_Pdb_Tool_Renum();

   private:

      US_Config *                             USglobal;

      QLabel *                                lbl_title;
      QLabel *                                lbl_credits_1;
      QLabel *                                lbl_inputfile;
      QLineEdit *                             le_inputfile;
      QLabel *                                lbl_usechainlist;
      QLineEdit *                             le_usechainlist;
      QLabel *                                lbl_reseqatom;
      QLineEdit *                             le_reseqatom;
      QLabel *                                lbl_startatom;
      QLineEdit *                             le_startatom;
      QLabel *                                lbl_chainrestartatom;
      QLineEdit *                             le_chainrestartatom;
      QLabel *                                lbl_modelrestartatom;
      QLineEdit *                             le_modelrestartatom;
      QLabel *                                lbl_reseqresidue;
      QLineEdit *                             le_reseqresidue;
      QLabel *                                lbl_startresidue;
      QLineEdit *                             le_startresidue;
      QLabel *                                lbl_chainrestartresidue;
      QLineEdit *                             le_chainrestartresidue;
      QLabel *                                lbl_modelrestartresidue;
      QLineEdit *                             le_modelrestartresidue;
      QLabel *                                lbl_striphydrogens;
      QLineEdit *                             le_striphydrogens;
      QLabel *                                lbl_itassertemplate;
      QLineEdit *                             le_itassertemplate;

      QPushButton *                           pb_help;
      QPushButton *                           pb_close;
      void                     *              us_hydrodyn;
      map < QString, QString > *              parameters;


      void                                    setupGUI();

   private slots:

      void                                    update_inputfile( const QString & );
      void                                    update_usechainlist( const QString & );
      void                                    update_reseqatom( const QString & );
      void                                    update_startatom( const QString & );
      void                                    update_chainrestartatom( const QString & );
      void                                    update_modelrestartatom( const QString & );
      void                                    update_reseqresidue( const QString & );
      void                                    update_startresidue( const QString & );
      void                                    update_chainrestartresidue( const QString & );
      void                                    update_modelrestartresidue( const QString & );
      void                                    update_striphydrogens( const QString & );
      void                                    update_itassertemplate( const QString & );

      void                                    help();
      void                                    cancel();

   protected slots:

      void                                    closeEvent( QCloseEvent * );
};

#endif
