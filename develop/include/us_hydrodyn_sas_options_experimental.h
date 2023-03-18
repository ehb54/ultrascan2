#ifndef US_HYDRODYN_SAS_OPTIONS_EXPERIMENTAL_H
#define US_HYDRODYN_SAS_OPTIONS_EXPERIMENTAL_H

// QT defs:

#include <qlabel.h>
#include <qstring.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qframe.h>
#include <qcheckbox.h>
#include <qwt_counter.h>
#include <qbuttongroup.h>

#include "us_util.h"

//standard C and C++ defs:

#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

using namespace std;

class US_EXTERN US_Hydrodyn_SasOptionsExperimental : public QFrame
{
   Q_OBJECT

   public:
      friend class US_Hydrodyn;

      US_Hydrodyn_SasOptionsExperimental(struct saxs_options *, bool *, void *, QWidget *p = 0, const char *name = 0);
      ~US_Hydrodyn_SasOptionsExperimental();

   public:
      struct saxs_options *saxs_options;
      bool        *sas_options_experimental_widget;
      void        *us_hydrodyn;
      US_Config   *USglobal;

      QLabel      *lbl_info;

      QCheckBox   *cb_iqq_scale_linear_offset;
      QCheckBox   *cb_autocorrelate;
      QCheckBox   *cb_hybrid_radius_excl_vol;

      QCheckBox   *cb_subtract_radius;
      QCheckBox   *cb_iqq_use_atomic_ff;
      QCheckBox   *cb_iqq_use_saxs_excl_vol;

      QCheckBox   *cb_iqq_scale_nnls;
      QCheckBox   *cb_iqq_log_fitting;
      QCheckBox   *cb_iqq_scaled_fitting;
      QCheckBox   *cb_iqq_scale_play;

      QPushButton *pb_help;
      QPushButton *pb_cancel;

   private slots:
      
      void setupGUI();

      void set_iqq_scale_linear_offset();
      void set_autocorrelate();
      void set_hybrid_radius_excl_vol();
      void set_subtract_radius();
      void set_iqq_use_atomic_ff();
      void set_iqq_use_saxs_excl_vol();
      void set_iqq_scale_nnls();
      void set_iqq_log_fitting();
      void set_iqq_scaled_fitting();
      void set_iqq_scale_play();

      void cancel();
      void help();
   
   protected slots:

      void closeEvent(QCloseEvent *);
};

#endif

