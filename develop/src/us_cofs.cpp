#include "../include/us_cofs.h"
#ifndef WIN32
#include <unistd.h>
#endif
#include <math.h>
#include <float.h>

US_CofS_W::US_CofS_W(QWidget *p, const char *name) : Data_Control_W(16, p, name)
{
   setCaption(tr("C of s Analysis:"));
   fn = "";
   pm = new US_Pixmap();
   fit_status = false;
   fit_ff0 = false;
   fit_posBaseline = false;
   fit_negBaseline = false;
   f_f0=1.5;
   iteration = 0;
   bottom = 1.0;
   top = 100.0;
   resolution = 50;
   cofs_GUI();
   plot2 = 1;
   connect(this, SIGNAL(dataLoaded()), this, SLOT(enableButtons()));
}

US_CofS_W::US_CofS_W(double bottom, double top, QString fn, 
                     unsigned int cell, unsigned int lambda, QWidget *p, const char *name) 
   : Data_Control_W(16, p, name)
{
   plot2 = 1;
   setCaption(tr("C of s Analysis:"));
   pm = new US_Pixmap();
   fit_status = false;
   fit_ff0 = false;
   fit_posBaseline = false;
   fit_negBaseline = false;
   f_f0=1.5;
   iteration = 0;
   this->bottom = bottom;
   this->top = top;
   this->fn = fn;
   resolution = 50;
   reset_flag = true;
   cofs_GUI();
   connect(this, SIGNAL(dataLoaded()), this, SLOT(enableButtons()));
   load_data(fn);
   this->selected_cell = cell;
   this->selected_lambda = lambda;
   this->selected_channel = 0;
   cell_select->setSelected(cell, true);
   lambda_select->setSelected(lambda, true);
   show_cell(cell);
   show_lambda(lambda);
   //   plot_edit();
}

US_CofS_W::~US_CofS_W()
{
   delete resplot;
}

void US_CofS_W::cofs_GUI()
{
   resplot = new US_ResidualPlot(0,0);
   int spacing = 2;
   pb_second_plot->setText(tr("C(s) distribution"));
   //   delete pb_reset;
   pb_reset->setGeometry(0,0,0,0);
   delete smoothing_lbl;
   delete smoothing_counter;
   smoothing_counter = NULL;
   delete range_lbl;
   delete range_counter;
   range_counter = NULL;
   delete position_lbl;
   delete position_counter;
   position_counter = NULL;
   
   lbl1_excluded->setText(tr("RMSD:"));

   xpos = border;
   ypos = 407;
   
   lbl_info1 = new QLabel(tr(" Status\n Info:"),this);
   lbl_info1->setPalette( QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label) );
   lbl_info1->setGeometry(xpos, ypos, buttonw/2, 2*buttonh+5);
   lbl_info1->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));

   xpos += buttonw/2;

   lbl_info2 = new QLabel(tr(""),this);
   lbl_info2->setFrameStyle(QFrame::WinPanel|Sunken);
   lbl_info2->setAlignment(AlignCenter|AlignVCenter);
   lbl_info2->setPalette( QPalette(USglobal->global_colors.cg_edit, USglobal->global_colors.cg_edit, USglobal->global_colors.cg_edit) );
   lbl_info2->setGeometry(xpos, ypos, (int) (1.5* buttonw), 2*buttonh+5);
   lbl_info2->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1));

   xpos = border;
   ypos += 2*buttonh + spacing + 5;

   lbl_bottom = new QLabel(tr(" Lower Limit (s):"), this);
   Q_CHECK_PTR(lbl_bottom);
   lbl_bottom->setAlignment(AlignLeft|AlignVCenter);
   lbl_bottom->setPalette( QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_bottom->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));
   lbl_bottom->setGeometry(xpos, ypos, buttonw, buttonh);

   xpos += buttonw + spacing;

   cnt_bottom= new QwtCounter(this);
   Q_CHECK_PTR(cnt_bottom);
   cnt_bottom->setRange(0.1, 9999.0, 0.1);
   cnt_bottom->setValue(bottom);
   cnt_bottom->setNumButtons(3);
   cnt_bottom->setPalette( QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
   cnt_bottom->setEnabled(false);
   cnt_bottom->setGeometry(xpos, ypos, buttonw, buttonh);
   connect(cnt_bottom, SIGNAL(valueChanged(double)), SLOT(update_bottom(double)));

   xpos = border;
   ypos += buttonh + spacing;

   lbl_top = new QLabel(tr(" Upper Limit (s):"), this);
   Q_CHECK_PTR(lbl_top);
   lbl_top->setAlignment(AlignLeft|AlignVCenter);
   lbl_top->setPalette( QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_top->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));
   lbl_top->setGeometry(xpos, ypos, buttonw, buttonh);

   xpos += buttonw + spacing;

   cnt_top= new QwtCounter(this);
   Q_CHECK_PTR(cnt_top);
   cnt_top->setRange(1.0, 100000, 1);
   cnt_top->setValue(top);
   cnt_top->setNumButtons(3);
   cnt_top->setEnabled(false);
   cnt_top->setPalette( QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
   cnt_top->setGeometry(xpos, ypos, buttonw, buttonh);
   connect(cnt_top, SIGNAL(valueChanged(double)), SLOT(update_top(double)));

   xpos = border;
   ypos += buttonh + spacing;

   lbl_resolution = new QLabel(tr(" Resolution (s):"), this);
   Q_CHECK_PTR(lbl_resolution);
   lbl_resolution->setAlignment(AlignLeft|AlignVCenter);
   lbl_resolution->setPalette( QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_resolution->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));
   lbl_resolution->setGeometry(xpos, ypos, buttonw, buttonh);

   xpos += buttonw + spacing;

   cnt_resolution= new QwtCounter(this);
   Q_CHECK_PTR(cnt_resolution);
   cnt_resolution->setRange(1, 500, 1);
   cnt_resolution->setValue(resolution);
   cnt_resolution->setEnabled(false);
   cnt_resolution->setNumButtons(3);
   cnt_resolution->setPalette( QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
   cnt_resolution->setGeometry(xpos, ypos, buttonw, buttonh);
   connect(cnt_resolution, SIGNAL(valueChanged(double)), SLOT(update_resolution(double)));

   xpos = border;
   ypos += buttonh + spacing;

   lbl_ff0 = new QLabel(tr(" Fit f/f0?"), this);
   Q_CHECK_PTR(lbl_ff0);
   lbl_ff0->setAlignment(AlignLeft|AlignVCenter);
   lbl_ff0->setPalette( QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_ff0->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));
   lbl_ff0->setGeometry(xpos, ypos, buttonw, buttonh);

   xpos += 2 * buttonw/3 + spacing;

   cb_ff0 = new QCheckBox(this);
   cb_ff0->setGeometry(xpos+10, ypos+5, 14, 14);
   cb_ff0->setChecked(false);
   cb_ff0->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize, QFont::Bold));
   cb_ff0->setPalette( QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
   connect(cb_ff0, SIGNAL(clicked()), SLOT(set_ff0()));

   xpos = border;
   xpos += buttonw + spacing;

   cnt_ff0= new QwtCounter(this);
   Q_CHECK_PTR(cnt_ff0);
   cnt_ff0->setRange(1, 40, 0.01);
   cnt_ff0->setValue(f_f0);
   cnt_ff0->setEnabled(true);
   cnt_ff0->setNumButtons(3);
   cnt_ff0->setPalette( QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
   cnt_ff0->setGeometry(xpos, ypos, buttonw, buttonh);
   connect(cnt_ff0, SIGNAL(valueChanged(double)), SLOT(update_ff0(double)));

   xpos = border;
   ypos += buttonh + spacing;

   lbl_posBaseline = new QLabel(tr(" Fit +Baseline?"), this);
   Q_CHECK_PTR(lbl_posBaseline);
   lbl_posBaseline->setAlignment(AlignLeft|AlignVCenter);
   lbl_posBaseline->setPalette( QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_posBaseline->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));
   lbl_posBaseline->setGeometry(xpos, ypos, buttonw, buttonh);

   xpos += 2 * buttonw/3 + spacing;

   cb_posBaseline = new QCheckBox(this);
   cb_posBaseline->setGeometry(xpos+10, ypos+5, 14, 14);
   cb_posBaseline->setChecked(false);
   cb_posBaseline->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize, QFont::Bold));
   cb_posBaseline->setPalette( QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
   connect(cb_posBaseline, SIGNAL(clicked()), SLOT(set_posBaseline()));

   xpos = border;
   xpos += buttonw + spacing;

   lbl_negBaseline = new QLabel(tr(" Fit -Baseline?"), this);
   Q_CHECK_PTR(lbl_negBaseline);
   lbl_negBaseline->setAlignment(AlignLeft|AlignVCenter);
   lbl_negBaseline->setPalette( QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_negBaseline->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));
   lbl_negBaseline->setGeometry(xpos, ypos, buttonw, buttonh);

   xpos += 2 * buttonw/3 + spacing;

   cb_negBaseline = new QCheckBox(this);
   cb_negBaseline->setGeometry(xpos+10, ypos+5, 14, 14);
   cb_negBaseline->setChecked(false);
   cb_negBaseline->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize, QFont::Bold));
   cb_negBaseline->setPalette( QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
   connect(cb_negBaseline, SIGNAL(clicked()), SLOT(set_negBaseline()));


   xpos = border;
   ypos += buttonh + spacing;

   pb_fit = new QPushButton(tr("Start Fit"), this);
   Q_CHECK_PTR(pb_fit);
   pb_fit->setAutoDefault(false);
   pb_fit->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize));
   pb_fit->setPalette( QPalette(USglobal->global_colors.cg_pushb, USglobal->global_colors.cg_pushb_disabled, USglobal->global_colors.cg_pushb_active));
   pb_fit->setGeometry(xpos, ypos, buttonw, buttonh);
   pb_fit->setEnabled(false);
   connect(pb_fit, SIGNAL(clicked()), SLOT(fit()));

   xpos += buttonw + spacing;

   lbl_iteration = new QLabel(tr(" "),this);
   lbl_iteration->setFrameStyle(QFrame::WinPanel|Sunken);
   lbl_iteration->setAlignment(AlignCenter|AlignVCenter);
   lbl_iteration->setPalette( QPalette(USglobal->global_colors.cg_edit, USglobal->global_colors.cg_edit, USglobal->global_colors.cg_edit) );
   lbl_iteration->setGeometry(xpos, ypos, buttonw, buttonh);
   lbl_iteration->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1));
   
   setup_GUI();
}

void US_CofS_W::setup_GUI()
{
   int j=0;
   int rows = 15, columns = 4, spacing = 2;
   
   QGridLayout * background = new QGridLayout(this,2,2,spacing);   
   QGridLayout * subGrid1 = new QGridLayout(rows, columns, spacing);
   for (int i=0; i<rows; i++)
   {
      subGrid1->setRowSpacing(i, 26);
   }
   subGrid1->addMultiCellWidget(pb_load,j,j,0,1);
   subGrid1->addMultiCellWidget(pb_details,j,j,2,3);
   j++;
   subGrid1->addMultiCellWidget(pb_second_plot,j,j,0,1);
   subGrid1->addMultiCellWidget(pb_save,j,j,2,3);
   j++;
   subGrid1->addMultiCellWidget(pb_print,j,j,0,1);
   subGrid1->addMultiCellWidget(pb_view,j,j,2,3);
   j++;
   subGrid1->addMultiCellWidget(pb_help,j,j,0,1);
   subGrid1->addMultiCellWidget(pb_close,j,j,2,3);
   j++;
   subGrid1->addMultiCellWidget(banner1,j,j,0,3);
   j++;
   subGrid1->addMultiCellWidget(lbl_run_id1,j,j,0,1);
   subGrid1->addMultiCellWidget(lbl_run_id2,j,j,2,3);
   j++;
   subGrid1->addMultiCellWidget(lbl_temperature1,j,j,0,1);
   subGrid1->addMultiCellWidget(lbl_temperature2,j,j,2,3);
   j++;
   subGrid1->addMultiCellWidget(lbl_cell_info1,j,j,0,1);
   subGrid1->addMultiCellWidget(lbl_cell_info2,j,j,2,3);
   j++;
   subGrid1->addMultiCellWidget(lbl_cell_descr,j,j,0,3);
   j++;
   subGrid1->addMultiCellWidget(cell_select,j,j+2,0,1);
   subGrid1->addMultiCellWidget(lambda_select,j,j+2,2,3);
   j=j+3;
   subGrid1->addMultiCellWidget(banner2,j,j,0,3);
   j++;
   subGrid1->addWidget(pb_density,j,0);
   subGrid1->addWidget(density_le,j,1);
   subGrid1->addWidget(pb_viscosity,j,2);
   subGrid1->addWidget(viscosity_le,j,3);
   j++;
   subGrid1->addWidget(pb_vbar,j,0);
   subGrid1->addWidget(vbar_le,j,1);
   subGrid1->addWidget(lbl1_excluded,j,2);
   subGrid1->addWidget(lbl2_excluded,j,3);
      
   rows = 11, columns = 4, spacing = 2, j=0;
   QGridLayout * subGrid2 = new QGridLayout(rows, columns, spacing);
   for (int i=0; i<rows; i++)
   {
      subGrid2->setRowSpacing(i, 26);
   }

   subGrid2->addMultiCellWidget(lbl_info1,j,j+1,0,0);
   subGrid2->addMultiCellWidget(lbl_info2,j,j+1,1,3);
   j=j+2;
   subGrid2->addMultiCellWidget(lbl_bottom,j,j,0,1);
   subGrid2->addMultiCellWidget(cnt_bottom,j,j,2,3);
   j++;
   subGrid2->addMultiCellWidget(lbl_top,j,j,0,1);
   subGrid2->addMultiCellWidget(cnt_top,j,j,2,3);
   j++;
   subGrid2->addMultiCellWidget(lbl_resolution,j,j,0,1);
   subGrid2->addMultiCellWidget(cnt_resolution,j,j,2,3);
   j++;
   subGrid2->addWidget(lbl_ff0,j,0);
   subGrid2->addWidget(cb_ff0,j,1);
   subGrid2->addMultiCellWidget(cnt_ff0,j,j,2,3);
   j++;
   subGrid2->addWidget(lbl_posBaseline,j,0);
   subGrid2->addWidget(cb_posBaseline,j,1);
   subGrid2->addWidget(lbl_negBaseline,j,2);
   subGrid2->addWidget(cb_negBaseline,j,3);
   j++;
   subGrid2->addMultiCellWidget(pb_fit,j,j,0,1);
   subGrid2->addMultiCellWidget(lbl_iteration,j,j,2,3);
   j++;
   subGrid2->addMultiCellWidget(pb_exsingle,j,j,0,1);
   subGrid2->addMultiCellWidget(cnt_exsingle,j,j,2,3);
   j++;
   subGrid2->addMultiCellWidget(pb_exrange,j,j,0,1);
   subGrid2->addMultiCellWidget(cnt_exrange,j,j,2,3);
   j++;
   subGrid2->addWidget(lbl_status,j,0);
   subGrid2->addMultiCellWidget(progress,j,j,1,3);
      
   background->addLayout(subGrid1,0,0);
   background->addWidget(analysis_plot,0,1);
   background->addLayout(subGrid2,1,0);
   background->addWidget(edit_plot,1,1);
   background->setColStretch(0,1);
   background->setColStretch(1,4);
   background->setColSpacing(0,350);
   background->setColSpacing(1,550);

   qApp->processEvents();
   QRect r = background->cellGeometry(0, 0);

   global_Xpos += 30;
   global_Ypos += 30;
   
   this->setGeometry(global_Xpos, global_Ypos, r.width()+550+spacing*3, this->height());



}

void US_CofS_W::second_plot()
{
   plot2++;
   if (plot2 == 5)
   {
      plot2 = 1;
   }
   second_plot(plot2);
   if (plot2 == 1)
   {
      pb_second_plot->setText(tr("C(s) Distribution"));
   }
   if (plot2 == 2)
   {
      pb_second_plot->setText(tr("C(MW) Distribution"));
   }
   if (plot2 == 3)
   {
      pb_second_plot->setText(tr("C(D) Distribution"));
   }
   if (plot2 == 4)
   {
      pb_second_plot->setText(tr("Residuals"));
   }
}

void US_CofS_W::second_plot(int val)
{
   resplot->setData(&residuals, global_Xpos+30, global_Ypos+30);
   resplot->show();
   resplot->repaint();
   unsigned int i, j;
   analysis_plot->setAxisAutoScale(QwtPlot::yLeft);
   if (val == 1)
   {
      QString s1, s2;
      analysis_plot->clear();
      s1.sprintf(tr(": Cell %d (%d nm)\nResiduals"), selected_cell+1, run_inf.wavelength[selected_cell][selected_lambda]);
      s2 = tr("Run ");
      s2.append(run_inf.run_id);
      s2.append(s1);
      analysis_plot->setTitle(s2);
      double *x, line_x[2];
      double *y, line_y[2];
      unsigned int *curve, zeroline;
      x = new double [residuals.scan[0].conc.size()];
      y = new double [residuals.scan[0].conc.size()];
      curve = new unsigned int [residuals.scan.size()]; // one extra for the zero line
      QwtSymbol symbol;
      QPen p_raw, p_zero;
      p_raw.setWidth(1);
      p_zero.setWidth(2);
      symbol.setStyle(QwtSymbol::Ellipse);
      p_raw.setColor(Qt::yellow);
      p_zero.setColor(Qt::red);
      symbol.setPen(Qt::yellow);
      symbol.setBrush(Qt::yellow);
      analysis_plot->setAxisTitle(QwtPlot::xBottom, tr("Radius in cm"));
      analysis_plot->setAxisTitle(QwtPlot::yLeft, "OD Difference");
      line_x[0] = residuals.radius[0] - (residuals.radius[residuals.radius.size()-1] - residuals.radius[0])/30.0;
      line_x[1] = residuals.radius[residuals.radius.size()-1] + (residuals.radius[residuals.radius.size()-1] - residuals.radius[0])/30;
      analysis_plot->setAxisScale(QwtPlot::xBottom, line_x[0], line_x[1]);
      line_y[0] = 0.0;
      line_y[1] = 0.0;
      symbol.setSize(1);
      zeroline = analysis_plot->insertCurve("Zero Line");
      analysis_plot->setCurveStyle(zeroline, QwtCurve::Lines);
      analysis_plot->setCurveData(zeroline, line_x, line_y, 2);
      analysis_plot->setCurvePen(zeroline, p_zero);

      for (i=0; i<residuals.radius.size(); i++)
      {
         x[i] = residuals.radius[i];
      }
      for (i=0; i<residuals.scan.size(); i++)
      {
         for (j=0; j<residuals.scan[0].conc.size(); j++)
         {
            y[j] = residuals.scan[i].conc[j];
         }
         curve[i] = analysis_plot->insertCurve("Residual Scan");
         analysis_plot->setCurveSymbol(curve[i], symbol);
         analysis_plot->setCurveStyle(curve[i], QwtCurve::NoCurve);
         analysis_plot->setCurveData(curve[i], x, y, residuals.radius.size());
         analysis_plot->setCurvePen(curve[i], p_raw);
      }
      analysis_plot->replot();
      delete [] curve;
      delete [] x;
      delete [] y;
   }
   if (val == 2)
   {
      QString s1, s2;
      double *x, *y;
      x = new double [resolution];
      y = new double [resolution];
      for (i=0; i<resolution; i++)
      {
         x[i] = s20w[i];
         y[i] = frequency[i];
      }
      analysis_plot->clear();
      s1.sprintf(tr(": Cell %d (%d nm)\nC(s) Analysis"), selected_cell+1, run_inf.wavelength[selected_cell][selected_lambda]);
      s2 = tr("Run ");
      s2.append(run_inf.run_id);
      s2.append(s1);
      analysis_plot->setTitle(s2);
      analysis_plot->setAxisTitle(QwtPlot::xBottom, tr("Corrected Sed. Coeff. (1e-13 s)"));
      analysis_plot->setAxisTitle(QwtPlot::yLeft, tr("Frequency"));
      analysis_plot->setAxisAutoScale(QwtPlot::xBottom);
      long distro = analysis_plot->insertCurve("C(s) Distribution");
      analysis_plot->setCurvePen(distro, QPen(Qt::yellow, 2, SolidLine));
      analysis_plot->setCurveData(distro, x, y, resolution);
      analysis_plot->replot();
      delete [] x;
      delete [] y;
   }
   if (val == 3)
   {
      QString s1, s2;
      double *x, *y;
      x = new double [resolution];
      y = new double [resolution];
      for (i=0; i<resolution; i++)
      {
         x[i] = mw[i];
         y[i] = frequency[i];
      }
      analysis_plot->clear();
      s1.sprintf(tr(": Cell %d (%d nm)\nC(MW) Analysis"), selected_cell+1, run_inf.wavelength[selected_cell][selected_lambda]);
      s2 = tr("Run ");
      s2.append(run_inf.run_id);
      s2.append(s1);
      analysis_plot->setTitle(s2);
      analysis_plot->setAxisTitle(QwtPlot::xBottom, tr("Molecular Weight (Dalton)"));
      analysis_plot->setAxisTitle(QwtPlot::yLeft, tr("Frequency"));
      analysis_plot->setAxisAutoScale(QwtPlot::xBottom);
      long distro = analysis_plot->insertCurve("Molecular Weight Distribution");
      analysis_plot->setCurvePen(distro, QPen(Qt::yellow, 2, SolidLine));
      analysis_plot->setCurveData(distro, x, y, resolution);
      analysis_plot->replot();
      delete [] x;
      delete [] y;
   }
   if (val == 4)
   {
      QString s1, s2;
      double *x, *y;
      x = new double [resolution];
      y = new double [resolution];
      for (i=0; i<resolution; i++)
      {
         //         x[i] = 1.0/pow(D20w[i], 0.5);
         x[i] = D20w[i];
         y[i] = frequency[i];
      }
      analysis_plot->clear();
      s1.sprintf(tr(": Cell %d (%d nm)\nC(D) Analysis"), selected_cell+1, run_inf.wavelength[selected_cell][selected_lambda]);
      s2 = tr("Run ");
      s2.append(run_inf.run_id);
      s2.append(s1);
      analysis_plot->setTitle(s2);
      analysis_plot->setAxisTitle(QwtPlot::xBottom, tr("D20,W (cm^2/sec)"));
      analysis_plot->setAxisTitle(QwtPlot::yLeft, tr("Frequency"));
      //      analysis_plot->setAxisOptions(QwtPlot::xBottom, QwtAutoScale::Logarithmic);
      analysis_plot->setAxisAutoScale(QwtPlot::xBottom);
      long distro = analysis_plot->insertCurve("Diffusion Coefficient Distribution");
      analysis_plot->setCurvePen(distro, QPen(Qt::yellow, 2, SolidLine));
      analysis_plot->setCurveData(distro, x, y, resolution);
      analysis_plot->replot();
      delete [] x;
      delete [] y;
   }
}

void US_CofS_W::enableButtons()
{
   pb_second_plot->setEnabled(false);
   pb_save->setEnabled(false);
   pb_view->setEnabled(false);
   pb_print->setEnabled(false);
   cnt_resolution->setEnabled(true);
   cnt_bottom->setEnabled(true);
   cnt_top->setEnabled(true);
   pb_fit->setEnabled(true);
   calc_correction(run_inf.avg_temperature);
   update_distribution();
}

void US_CofS_W::save()
{
   if (step == 0)
   {
      QMessageBox::message(tr("Attention:\n"),tr("You need to load some data first!\n\n      Click on \"Load Data\"\n"));
      return;
   }
   write_res();
   write_cofs();
   QPixmap p;
   QString fileName;
   second_plot(1);
   fileName.sprintf(htmlDir + "/cofs_resid_%d%d.", selected_cell + 1, selected_lambda +1);
   p = QPixmap::grabWidget(analysis_plot, 2, 2, analysis_plot->width() - 4, analysis_plot->height() - 4);
   pm->save_file(fileName, p);
   second_plot(2);
   fileName.sprintf(htmlDir + "/cofs_dis_%d%d.", selected_cell + 1, selected_lambda +1);
   p = QPixmap::grabWidget(analysis_plot, 2, 2, analysis_plot->width() - 4, analysis_plot->height() - 4);
   pm->save_file(fileName, p);
   second_plot(3);
   fileName.sprintf(htmlDir + "/cofs_mw_%d%d.", selected_cell + 1, selected_lambda +1);
   p = QPixmap::grabWidget(analysis_plot, 2, 2, analysis_plot->width() - 4, analysis_plot->height() - 4);
   pm->save_file(fileName, p);
   second_plot(4);
   fileName.sprintf(htmlDir + "/cofs_d_%d%d.", selected_cell + 1, selected_lambda +1);
   p = QPixmap::grabWidget(analysis_plot, 2, 2, analysis_plot->width() - 4, analysis_plot->height() - 4);
   pm->save_file(fileName, p);
   fileName.sprintf(htmlDir + "/cofs_edited_%d%d.", selected_cell + 1, selected_lambda +1);
   p = QPixmap::grabWidget(edit_plot, 2, 2, edit_plot->width() - 4, edit_plot->height() - 4);
   pm->save_file(fileName, p);
   fileName.sprintf(htmlDir + "/cofs_pixmap_%d%d.", selected_cell + 1, selected_lambda +1);
   p = QPixmap::grabWidget(resplot, 1, 1, resplot->width() - 2, resplot->height() - 2);
   pm->save_file(fileName, p);
}

void US_CofS_W::update_distribution()
{
   s_distribution.clear();
   D_distribution.clear();
   unsigned int i;
   double tmp1, tmp2, s;
   tmp1 = ((double)top - (double) bottom)/(double) (resolution - 1);
   tmp2 = bottom;
   for (i=0; i<resolution; i++)
   {
      s = tmp2 * 1.0e-13 / correction; // this distro is s20,W corrected from vhw and needs to be 
      s_distribution.push_back(s);     // adjusted for temperature and buffer to match expt. data
      D_distribution.push_back((R * (run_inf.avg_temperature + K0))/(AVOGADRO * f_f0 * 6.0 * viscosity * 0.01 * M_PI
                                                                     * pow((9.0 * s * f_f0 * vbar * viscosity * 0.01)/(2.0 * (1.0 - vbar * density)), 0.5)));
      tmp2 += tmp1;
   }
}

void US_CofS_W::help()
{
   US_Help *online_help;
   online_help = new US_Help(this);
   online_help->show_help("manual/cofs.html");
}

void US_CofS_W::update_ff0(double val)
{
   f_f0 = (float) val;
   update_distribution();
}

void US_CofS_W::set_ff0()
{
   if (fit_ff0)
   {
      fit_ff0 = false;
   }
   else
   {
      fit_ff0 = true;
   }
}

void US_CofS_W::set_posBaseline()
{
   if (fit_posBaseline)
   {
      fit_posBaseline = false;
   }
   else
   {
      fit_posBaseline = true;
      fit_negBaseline = false;
      cb_negBaseline->setChecked(false);
   }
}

void US_CofS_W::set_negBaseline()
{
   if (fit_negBaseline)
   {
      fit_negBaseline = false;
   }
   else
   {
      fit_negBaseline = true;
      fit_posBaseline = false;
      cb_posBaseline->setChecked(false);
   }
}

void US_CofS_W::update_bottom(double val)
{
   bottom = val;
   if (bottom >= top)
   {
      top = bottom + 1.0;
      cnt_top->setValue((double) top);
   }
   update_distribution();
}

void US_CofS_W::update_top(double val)
{
   top = val;
   if (top <= bottom)
   {
      bottom = top - 1.0;
      cnt_bottom->setValue((double) bottom);
   }
   update_distribution();
}

void US_CofS_W::update_resolution(double val)
{
   resolution = (unsigned int) val;
   update_distribution();
}

void US_CofS_W::view()
{
   if (step == 0)
   {
      QMessageBox::message(tr("Attention:\n"),tr("You need to load some data first!\n\n      Click on \"Load Data\"\n"));
      return;
   }
   QString filestr, filename, temp;
   filename = USglobal->config_list.result_dir.copy();
   filename.append("/");
   filename.append(run_inf.run_id);
   temp.sprintf(".%d%d", selected_cell+1, selected_lambda+1);
   filestr = filename.copy();
   filestr.append(".cofs_res");
   filestr.append(temp);
   write_res();
   TextEdit *e;
   e = new TextEdit();
   e->setPalette(QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
   e->setGeometry(global_Xpos + 30, global_Ypos + 30, 685, 600);
   e->load(filestr);
   e->show();
   
}

void US_CofS_W::write_res()
{
   QString filestr, filename, temp, str;
   filename = USglobal->config_list.result_dir.copy();
   filename.append("/");
   filename.append(run_inf.run_id);
   temp.sprintf(".%d%d", selected_cell+1, selected_lambda+1);
   filestr = filename.copy();
   filestr.append(".cofs_res");
   filestr.append(temp);
   QFile res_f(filestr);
   if (res_f.open(IO_WriteOnly | IO_Translate))
   {
      QTextStream ts(&res_f);
      ts << "***************************************************\n";
      ts << tr("*              C(s) Analysis                      *\n");
      ts << "***************************************************\n\n\n";
      ts << tr("Data Report for Run \"") << run_inf.run_id << tr("\", Cell ") << (selected_cell + 1) 
         << tr(", Wavelength ") << (selected_lambda + 1) << "\n\n";
      ts << tr("Detailed Run Information:\n\n");
      ts << tr("Cell Description:        ") << run_inf.cell_id[selected_cell] << "\n";
      ts << tr("Raw Data Directory:      ") << run_inf.data_dir << "\n";
      ts << tr("Rotor Speed:             ") << run_inf.rpm[first_cell][0][0]  << " rpm\n";
      ts << tr("Average Temperature:     ") << run_inf.avg_temperature << " �C\n" ;
      if (run_inf.temperature_check == 1)
      {
         ts << tr("Temperature Variation:   Within Tolerance\n");
      }
      if (run_inf.temperature_check == -1)
      {
         ts << tr("Temperature Variation: (!) OUTSIDE TOLERANCE (!)\n");
      }
      i = (unsigned int) (run_inf.time_correction / 60);
      j = (unsigned int) (0.5 + (run_inf.time_correction - (i * 60)));
      ts << tr("Time Correction:         ") << i << tr(" minute(s) ") << j << tr(" second(s)\n");
      i = (unsigned int) (run_inf.duration / 3600);
      j = (unsigned int) (0.5 + (run_inf.duration - i * 3600) / 60);
      ts << tr("Run Duration:            ") << i << tr(" hour(s) ") << j << tr(" minute(s)\n");
      ts << tr("Wavelength:              ") << run_inf.wavelength[selected_cell][selected_lambda] << " nm\n";
      ts << tr("Baseline Absorbance:     ") << run_inf.baseline[selected_cell][selected_lambda] << " OD\n";
      ts << tr("Meniscus Position:       ") << run_inf.meniscus[selected_cell] << " cm\n";
      ts << tr("Edited Data starts at:   ") << run_inf.range_left[selected_cell][selected_lambda][0] << " cm\n";
      ts << tr("Edited Data stops at:    ") << run_inf.range_right[selected_cell][selected_lambda][0] << " cm\n\n\n";
      ts << tr("Hydrodynamic Settings:\n\n");
      ts << tr("Viscosity correction:    ") << viscosity << "\n" ;
      ts << tr("Viscosity (absolute):    ") << viscosity_tb << "\n" ;
      ts << tr("Density correction:      ") << density << " g/ccm\n" ;
      ts << tr("Density (absolute):      ") << density_tb << " g/ccm\n" ;
      ts << tr("Vbar:                    ") << vbar << " ccm/g\n" ;
      ts << tr("Vbar corrected for 20�C: ") << vbar20 << " ccm/g\n" ;
      ts << tr("Buoyancy (Water, 20�C) : ") << buoyancyw << "\n" ;
      ts << tr("Buoyancy (absolute)      ") << buoyancyb << "\n" ;
      ts << tr("Correction Factor:       ") << correction << "\n\n\n";
      ts << tr("Data Analysis Settings:\n\n");
      ts << tr("Resolution:              ") << resolution << "\n";
      ts << tr("Residual Mean Square Deviation: ") << rmsd << "\n";
      if (fit_ff0)
      {
         ts << tr("f/f0:                    ") << f_f0 << " (fitted)\n";
      }
      else
      {
         ts << tr("f/f0:                    ") << f_f0 << " (fixed)\n";
      }
      if (fit_posBaseline)
      {
         ts << tr("(+) Baseline Component:  ") << frequency[resolution] << "\n";
      }
      if (fit_negBaseline)
      {
         if (fit_posBaseline)
         {
            ts << tr("(-) Baseline Component:  ") << frequency[resolution+1] << "\n";
         }
         else
         {
            ts << tr("(-) Baseline Component:  ") << frequency[resolution] << "\n";
         }
      }

      // take the first s/MW pair and calculate an f/f0 corrected for 20,W:
      float f0, vol, rad_sphere, frict;
      vol = vbar20 * mw[0] / AVOGADRO;
      rad_sphere = pow((double) (vol * (3.0/4.0))/M_PI, (double) (1.0/3.0));
      f0 = rad_sphere * 6.0 * M_PI * VISC_20W;
      frict = (mw[0] * (1.0 - vbar20 * DENS_20W)) / (s20w[0] * AVOGADRO);
      f_f0 = frict/f0;      
      ts << "f/f0 corrected for Water at 20�C: " << f_f0 << "\n\n\n";

      ts << tr("Weight-Average sedimentation coefficient:\n\n");
      float sum_mw=0.0;
      float sum_s=0.0;
      float sum_freq=0.0;
      float sum_D=0.0;
      for (i=0; i<resolution; i++)
      {
         if (frequency[i] !=0.0)
         {
            sum_mw += frequency[i] * mw[i];
            sum_s += frequency[i] * s20w[i];
            sum_D += frequency[i] * D20w[i];
            sum_freq += frequency[i];
         }
      }
      ts << str.sprintf("Weight Average S20,W: %6.4e\n", sum_s/sum_freq);
      ts << str.sprintf("Weight Average D20,W: %6.4e\n", sum_D/sum_freq);
      ts << str.sprintf("Weight Average Molecular Weight: %6.4e\n", sum_mw/sum_freq);
      ts << "\n\n";
      ts << tr("Distribution Information:\n\n");
      ts << tr("Molecular Weight:    S 20,W:       D 20,W:\n\n");
      for (i=0; i<resolution; i++)
      {
         if (frequency[i] !=0.0)
         {
            ts << str.sprintf("%6.4e           %6.4e    %6.4e   (%7.3f", 
                              mw[i], s20w[i], D20w[i], 100.0 * frequency[i]/sum_freq);
            ts << " %)\n";
         }
      }
      res_f.close();
   }
}

void US_CofS_W::write_cofs()
{
   QString filestr, filename, temp1, temp2;
   unsigned int count=0;
   filename = USglobal->config_list.result_dir.copy();
   filename.append("/");
   filename.append(run_inf.run_id);
   temp1.sprintf(".%d%d", selected_cell+1, selected_lambda+1);
   filestr = filename.copy();
   filestr.append(".cofs_dis");
   filestr.append(temp1);
   QFile f(filestr);
   unsigned int i;   
   if (f.open(IO_WriteOnly | IO_Translate))
   {
      QTextStream ts(&f);
      //
      // Arrange the file as follows: First, write to the file all apparent sedimentation
      // values, for each scan time one row which contains all divisions, starting at the 
      // lowest concentration. The first column has the times of each scan, then follow
      // the sedimentation coefficients for each division for that time:
      //
      ts << "S_apparent\tS_20,W    \tD_apparent\tD_20,W    \tMW        \tFrequency\tf/f0(20,W)\n"; 
      for (i=0; i<resolution; i++)
      {
         if (frequency[i] > 0)
         {
            ts << temp2.sprintf("%6.4e\t%6.4e\t%6.4e\t%6.4e\t%6.4e\t%6.4e\t%6.4e\n", s_distribution[i], s20w[i], D_distribution[i], 
                                D20w[i], mw[i], frequency[i], f_f0);
         }
      }
      f.close();
   }
   filestr = USglobal->config_list.result_dir + "/" + run_inf.run_id + "-COFS.model" + temp1;
   f.setName(filestr);
   if (f.open(IO_WriteOnly | IO_Translate))
   {
      QTextStream ts(&f);
      QString str;
      ts << "COFS" << endl;
      ts << run_inf.run_id << endl;
      ts << "0" << endl; // noninteracting model string identifier
      count = 0;
      for (i=0; i<resolution; i++)
      {   
         if (frequency[i] > 0.0)
         {
            count++;
         }
      }
      ts << count << endl;      
      for (i=0; i<resolution; i++)
      {   
         if (frequency[i] > 0.0)
         {
            ts << frequency[i] << endl;
            ts << s20w[i] << endl;
            ts << D20w[i] << endl;
            ts << "0" << endl;
            ts << "0" << endl;
         }
      }
      f.close();
   }
}

void US_CofS_W::fit()
{
   iteration = 0;
   if (fit_status)
   {
      fit_status = false;
      pb_fit->setText(tr("Start Fit"));
   }
   else
   {
      fit_status = true;
      pb_fit->setText(tr("Stop Fit"));
      if (fit_ff0)
      {
         linesearch();
      }
      else
      {
         calc_residuals();
      }
      fit_status = false;
      pb_fit->setText(tr("Start Fit"));
   }
}

float US_CofS_W::calc_residuals()
{
   QString str;
   if (fit_status == false)
   {
      return(-1.0);
   }
   
   iteration ++;
   str.sprintf(tr("Iteration: %d"), iteration);
   lbl_iteration->setText(str);
   cnt_ff0->setValue((double) f_f0);
   double *nnls_a, *nnls_b, *nnls_x, nnls_rnorm, *nnls_wp, *nnls_zzp, initial_concentration = 1.0;
   int *nnls_indexp, result;
   struct mfem_scan single_scan;
   struct mfem_data experiment, fem_data; 
   struct mfem_initial initCVector; 
   unsigned int i, j, k, count;
   i = points * run_inf.scans[selected_cell][selected_lambda];
   k=0;
   if (fit_posBaseline)
   {
      k++;
   }
   if (fit_negBaseline)
   {
      k++;
   }
   j = resolution + k;
   nnls_a = new double [i * j]; // contains the model functions, end-to-end
   nnls_b = new double [i]; // contains the experimental data
   nnls_zzp = new double [i]; // pre-allocated working space for nnls
   nnls_x = new double [j]; // the solution vector, pre-allocated for nnls
   nnls_wp = new double [j]; // pre-allocated working space for nnls, On exit, wp[] will contain the dual solution vector, wp[i]=0.0 for all i in set p and wp[i]<=0.0 for all i in set z. */
   nnls_indexp = new int [j];
   US_MovingFEM *mfem;
   mfem = new US_MovingFEM(&fem_data, false);
   

   // initialize experimental data array sizes and radius positions:

   clear_data(&experiment);
   clear_data(&residuals);
   progress->setTotalSteps(resolution);
   progress->reset();
   initCVector.concentration.clear();
   initCVector.radius.clear();
   frequency.clear();
   mw.clear();
   s20w.clear();
   D20w.clear();
   double bottom = calc_bottom(rotor_list, cp_list, run_inf.rotor, 
                               run_inf.centerpiece[selected_cell], 0, run_inf.rpm[selected_cell][selected_lambda][0]);
   
   for (i=0; i<points; i++)
   {
      experiment.radius.push_back(radius[i]);
      residuals.radius.push_back(radius[i]);
      single_scan.conc.push_back(0.0); // this is the baseline vector to be added first
   }
   for (i=0; i<run_inf.scans[selected_cell][selected_lambda]; i++)
   {
      single_scan.time = (double) run_inf.time[selected_cell][selected_lambda][i];
      experiment.scan.push_back(single_scan);
      residuals.scan.push_back(single_scan);
   }
   count = 0;
   for (i=0; i<experiment.scan.size(); i++)
   {
      for (j=0; j<points; j++)
      {
         // populate the A matrix for the NNLS routine with the model function:
         nnls_b[count] = absorbance[i][j];
         count ++;
      }
   }

   count = 0;
   for (i=0; i<resolution; i++)
   {
      qApp->processEvents();
      if(!fit_status)
      {
         delete [] nnls_a;
         delete [] nnls_b;
         delete [] nnls_zzp;
         delete [] nnls_x;
         delete [] nnls_wp;
         delete [] nnls_indexp;
         return (-1.0);
      }
      // for each term in the linear combination we need to reset the 
      // simulation vectors, the experimental vector simply keeps getting overwritten:
      str.sprintf(tr("Calculating Lamm Eqation\nWorking on Term %d of %d\nwith f/f0: %4.2f"), i+1, resolution, f_f0); 
      lbl_info2->setText(str);
      clear_data(&fem_data);
      for (j=0; j<experiment.scan.size(); j++)
      {
         for (k=0; k<points; k++)
         {
            // reset concentration to zero:
            experiment.scan[j].conc[k] = 0.0;
         }
      }
      /*
        cout << "s: " << s_distribution[i] << ", D: "  << D_distribution[i] << ", rpm: " <<
        (double) run_inf.rpm[selected_cell][selected_lambda][0] << ", time: " <<  
        experiment.scan[experiment.scan.size()-1].time << ", meniscus: " <<
        (double) run_inf.meniscus[selected_cell] << ", bottom: " <<  bottom << ", c0: " << 
        initial_concentration << endl;
      */
      mfem->set_params(100, s_distribution[i], D_distribution[i],
                       (double) run_inf.rpm[selected_cell][selected_lambda][0], 
                       experiment.scan[experiment.scan.size()-1].time, 
                       (double) run_inf.meniscus[selected_cell], bottom, initial_concentration, &initCVector);

      // generate the next term of the linear combination:
      mfem->run();

      // interpolate model function to the experimental data so dimension 1 in A matches dimension of B:
      mfem->interpolate(&experiment, &fem_data);
      
      for (j=0; j<experiment.scan.size(); j++)
      {
         for (k=0; k<points; k++)
         {
            // populate the A matrix for the NNLS routine with the model function:
            nnls_a[count] = experiment.scan[j].conc[k];
            count ++;
         }
      }
      
      progress->setProgress(i+1);
   }
   if (fit_posBaseline)
   {
      // add positive baseline contributions:
      for (j=0; j<experiment.scan.size(); j++)
      {
         for (k=0; k<points; k++)
         {
            nnls_a[count] = 1.0;
            count ++;
         }
      }
   }
   if (fit_negBaseline)
   {
      // add negative baseline contributions:
      for (j=0; j<experiment.scan.size(); j++)
      {
         for (k=0; k<points; k++)
         {
            nnls_a[count] = -1.0;
            count ++;
         }
      }
   }
   i = points * run_inf.scans[selected_cell][selected_lambda];
   lbl_info2->setText(tr("Calculating NNLS solution..."));
   qApp->processEvents();
   k=0;
   if (fit_posBaseline)
   {
      k++;
   }
   if (fit_negBaseline)
   {
      k++;
   }
   result = nnls(nnls_a, i, i, resolution + k,
                 nnls_b, nnls_x, &nnls_rnorm, nnls_wp, nnls_zzp, nnls_indexp);
   lbl_info2->setText(tr("Calculating Residuals..."));
   for (i=0; i<resolution; i++)
   {
      frequency.push_back(nnls_x[i]);
      //      cout << i << ": " << frequency[i] << endl;
      if (nnls_x[i] != 0.0)
      {
         clear_data(&fem_data);
         for (j=0; j<experiment.scan.size(); j++)
         {
            for (k=0; k<points; k++)
            {
               // reset concentration to zero:
               experiment.scan[j].conc[k] = 0.0;
            }
         }
         mfem->set_params(100, s_distribution[i], D_distribution[i],
                          (double) run_inf.rpm[selected_cell][selected_lambda][0], 
                          experiment.scan[experiment.scan.size()-1].time, 
                          (double) run_inf.meniscus[selected_cell], bottom, initial_concentration, &initCVector);
         // generate the next term of the linear combination:
         mfem->run();

         // interpolate model function to the experimental data so dimension 1 in A matches dimension of B:
         mfem->interpolate(&experiment, &fem_data);
         for (j=0; j<experiment.scan.size(); j++)
         {
            for (k=0; k<points; k++)
            {
               residuals.scan[j].conc[k] += nnls_x[i] * experiment.scan[j].conc[k];;
            }
         }
         qApp->processEvents();
      }
   }
   if (fit_posBaseline)
   {
      // add positive baseline contribution:
      frequency.push_back(nnls_x[resolution]);
      //      cout << "Dimension when adding positive baseline: " << resolution << ", frequency: " << frequency[frequency.size()-1] << "(" << nnls_x[resolution] << ")"  << endl; 
      if (nnls_x[resolution] != 0.0)
      {
         for (j=0; j<experiment.scan.size(); j++)
         {
            for (k=0; k<points; k++)
            {
               residuals.scan[j].conc[k] += nnls_x[resolution];
            }
         }
      }
   }
   if (fit_negBaseline)
   {
      int dimension;
      // add negative baseline contribution:
      if (fit_posBaseline) // if we are also fitting the positive component, we need to increment one additional dimension
      {
         dimension = resolution+1;
      }
      else
      {
         dimension = resolution;
      }
      frequency.push_back(nnls_x[dimension]);
      //      cout << "Dimension when adding negative baseline: " << dimension << ", frequency: " << frequency[frequency.size()-1] << "(" << nnls_x[dimension] << ")"  << endl; 
      if (nnls_x[dimension] != 0.0)
      {
         for (j=0; j<experiment.scan.size(); j++)
         {
            for (k=0; k<points; k++)
            {
               residuals.scan[j].conc[k] -= nnls_x[dimension];
            }
         }
      }
   }
   plot_edit();
   long *resids;
   double **res;
   res = new double *[experiment.scan.size()];
   resids = new long [experiment.scan.size()];
   for (i=0; i<experiment.scan.size(); i++)
   {
      res[i] = new double [points];
   }
   for (j=0; j<experiment.scan.size(); j++)
   {
      for (k=0; k<points; k++)
      {
         res[j][k] = residuals.scan[j].conc[k];
      }
      resids[j] = edit_plot->insertCurve("C(s) model");
      edit_plot->setCurvePen(resids[j], QPen(Qt::red, 1, SolidLine));
      edit_plot->setCurveData(resids[j], radius, res[j], points);
      qApp->processEvents();
   }
   edit_plot->replot();
   
   rmsd = 0.0;
   for (j=0; j<experiment.scan.size(); j++)
   {
      for (k=0; k<points; k++)
      {
         residuals.scan[j].conc[k] = absorbance[j][k] - residuals.scan[j].conc[k]; 
         //         cout << "C[" << j << "][" << k << "]: " << residuals.scan[j].conc[k] << endl;
         rmsd += residuals.scan[j].conc[k] * residuals.scan[j].conc[k];
      }
   }
   rmsd /= points * experiment.scan.size();
   rmsd = pow((double)rmsd, 0.5);
   str.sprintf("%6.4f", rmsd);
   lbl2_excluded->setText(str);
   lbl_info2->setText(str.sprintf(tr("solution converged...\nrmsd: %5.3e, f/f0: %5.3f\n iterations: %d"), rmsd, f_f0, iteration));
   calc_distros();
   second_plot(plot2);
   for (i=0; i<experiment.scan.size(); i++)
   {
      delete [] res[i];
   }
   delete [] res;
   delete [] resids;
   delete [] nnls_a;
   delete [] nnls_b;
   delete [] nnls_zzp;
   delete [] nnls_x;
   delete [] nnls_wp;
   delete [] nnls_indexp;
   pb_second_plot->setEnabled(true);
   pb_save->setEnabled(true);
   pb_view->setEnabled(true);
   pb_print->setEnabled(true);
   return rmsd;
}

void US_CofS_W::calc_distros()
{
   mw.clear();
   s20w.clear();
   D20w.clear();
   for (unsigned int i=0; i<resolution; i++)
   {
      s20w.push_back(s_distribution[i] * correction);
      D20w.push_back(D_distribution[i] * K20 * viscosity_tb/((100.0 * VISC_20W) * (run_inf.avg_temperature + K0)));
      //      cout << "Correction: " << correction << ", d_uncorrected: " << D_distribution[i] << ", viscos.: " 
      //      << viscosity_tb << ", D20,W: " << D20w[i] <<endl;
      mw.push_back((s20w[i]/D20w[i])*(R * K20)/(1.0 - vbar20 * DENS_20W));
   }
}

void US_CofS_W::clear_data(mfem_data *d)
{
   unsigned int i;
   for (i=0; i<(*d).scan.size(); i++)
   {
      (*d).scan[i].conc.clear();
   }
   (*d).radius.clear();
   (*d).scan.clear();
}

float US_CofS_W::calc_testParameter(float val)
{
   f_f0 = val;
   update_distribution();
   return(calc_residuals());
}

float US_CofS_W::linesearch()
{
   // look for the minimum residual. Residual values are f0, f1, f2, evaluated at x0, x1, x2.
   // x0, x1, x2 are multipliers for incremental change of the parameter
   // calculate bracket: Assume the minimum is between x0=0 and some stepsize x2 away from x0.
   // then find an x1 in the middle between x0 and x2 and calculate f1(x1) and f2(x2), where
   // shift to the right
   // f is the residual of the function.
   bool check_flag=true;
   double old_f0=0.0, old_f1=0.0, old_f2=0.0;
   float x0 = 1.0, x1 = 2.0, x2 = 4.0, h = (float) 0.01, xmin, fmin, 
      tolerance= (float) 0.01, maxerror;
   unsigned int iter = 1;
   errno = 0;
   float f0 = calc_testParameter(x0);
   if (f0 == -1.0) return (-1.0);
   float f1 = calc_testParameter(x1);
   if (f1 == -1.0) return (-1.0);
   float f2 = calc_testParameter(x2);
   if (f2 == -1.0) return (-1.0);
   if(run_inf.exp_type.absorbance)
   {
      maxerror = 1.0e4;
   }
   else
   {
      maxerror = (float) 1.0e12; // allow a larger error for interference data
   }
   // make the initial step size smaller if we have very large residuals
   while (errno != 0 || (f0 >= maxerror || f0 < 0 || f1 >= maxerror || f1 < 0 || f2 >= maxerror || f2 < 0))
   {
      x1 /= 2;
      x2 /= 2;
      //cout << "Calling from 1. loop\n";
      f1 = calc_testParameter(x1);
      if (f1 == -1.0) return (-1.0);
      f2 = calc_testParameter(x2);
      if (f2 == -1.0) return (-1.0);
      if (x1 < FLT_MIN)
      {
         //cout << "abandoned with: " << "x0: " << x0 << ", x1: " << x1 << ", x2: " << x2 << ", f0: " << f0 << ", f1: " << f1 << ", f2: " << f2 << endl;
         //cout << FLT_MIN << ", " << FLT_MAX << ", errno: " << errno << endl;
         errno = 0;
         return (-1); //couldn't do anything for this search direction - fit didn't converge
      }
   }   check_flag=true;
   while(check_flag)
   {
      //cout << "mathlib: "  << errno << ", x0: " << x0 << ", x1: " << x1 << ", x2: " << x2 << ", f0: " << f0 << ", f1: " << f1 << ", f2: " << f2 << ", d1: " << f1-f0 << ", d2: " << f2-f1 << ", d3: " << f2-f0 << endl;
      if((isnotanumber(f0) && isnotanumber(f1)) 
         || (isnotanumber(f1) && isnotanumber(f2))
         || (isnotanumber(f0) || isnotanumber(f2))) // at least two values are screwed up, exit.
      {
         //cout << "error1 " << "x0: " << x0 << ", x1: " << x1 << ", x2: " << x2 << ", f0: " << f0 << ", f1: " << f1 << ", f2: " << f2 << endl;
         errno = 0;
         return(-1);
      } 
      // Check to see if the solution is really converging:

      if ((fabs(f2 - old_f2) < FLT_MIN) && (fabs(f1 - old_f1) < FLT_MIN) && (fabs(f0 - old_f0) < FLT_MIN))   // is the solution horizontal?
      {
         return(0);   // we found the minimum, return alpha=0
      }
      old_f0 = f0;
      old_f1 = f1;
      old_f2 = f2;


      if ((fabs(f2 - f0) < FLT_MIN) && (fabs(f1 - f0) < FLT_MIN) || (f0 > f1 && fabs(f2 - f1) < FLT_MIN))   // is the solution horizontal?
      {
         //cout << "error2 "  << "x0: " << x0 << ", x1: " << x1 << ", x2: " << x2 << ", f0: " << f0 << ", f1: " << f1 << ", f2: " << f2 << endl;
         return(0);   // we found the minimum, return alpha=0
      }
      if ((fabs(x0) < FLT_MIN) && (fabs(x1) < FLT_MIN) && (fabs(x2) < FLT_MIN))   // is the solution horizontal?
      {
         //cout << "error3 "  << "x0: " << x0 << ", x1: " << x1 << ", x2: " << x2 << ", f0: " << f0 << ", f1: " << f1 << ", f2: " << f2 << endl;
         return(0);   // we found the minimum, return alpha=0
      }
      //      if ((f0 < f1) && (f1 == f2) || (f0 < f1) && (f1 > f2) || (fabs(f1 - f0) < FLT_MIN) && f2 > f1)   // some weird cases can happen near the minimum
      if ((fabs(f0 - f1) < FLT_MIN) && fabs(f1 - f2) < FLT_MIN
          || (fabs(f0 - f1) < FLT_MIN) && f2 > f1)   // some weird cases can happen near the minimum
      {
         //cout << "error4 "  << "x0: " << x0 << ", x1: " << x1 << ", x2: " << x2 << ", f0: " << f0 << ", f1: " << f1 << ", f2: " << f2 << endl;
         return(0);   // we found the minimum, return alpha=0
      }
      if (f0 > f1 && f2 > f1) // we have a bracket
      {
         //cout << "bracket " << "x0: " << x0 << ", x1: " << x1 << ", x2: " << x2 << ", f0: " << f0 << ", f1: " << f1 << ", f2: " << f2 << endl;
         check_flag = false;
         break;
      }
      else if ((f2 > f1 && f1 > f0) || (f1 > f0 && f1 > f2) || (f1 == f2 && f1 > f0)) // shift to the left
      {
         //cout << "leftshift " << "x0: " << x0 << ", x1: " << x1 << ", x2: " << x2 << ", f0: " << f0 << ", f1: " << f1 << ", f2: " << f2 << endl;
         x2 = x1;
         f2 = f1;
         x1 = (x2 + x0)/2.0;
         f1 = calc_testParameter(x1);
         if (f1 == -1.0) return (-1.0);
      }
      else if (f0 > f1 && f1 > f2) // shift to the right
      {
         //cout << "rightshift " << "x0: " << x0 << ", x1: " << x1 << ", x2: " << x2 << ", f0: " << f0 << ", f1: " << f1 << ", f2: " << f2 << endl;
         x0 = x1;
         f0 = f1;
         x1 = x2;
         f1 = f2;
         x2 = x2 + (pow((double) 2, (double) (iter+2))) * h;
         f2 = calc_testParameter(x2);
         if (f2 == -1.0) return (-1.0);
         //cout << "rightshift #2" << "x0: " << x0 << ", x1: " << x1 << ", x2: " << x2 << ", f0: " << f0 << ", f1: " << f1 << ", f2: " << f2 << endl;
      }
      iter++; 
      //cout << iter << endl;
   } // get a bracket
   // search inside the bracket for the minimum and do a 2nd order polynomial fit
   x1 = (x0 + x2)/2.0;
   h = x1 - x0;
   //cout << "Calling from 2. loop\n";
   f1 = calc_testParameter(x1);
   if (f1 == -1.0) return (-1.0);
   while(true)
   {
      if (f0 < f1) // shift left
      {
         x2 = x1;
         f2 = f1;
         x1 = x0;
         f1 = f0;
         x0 = x1 - h;
         //cout << "Calling from 3. loop\n";
         f0 = calc_testParameter(x0);
         if (f0 == -1.0) return (-1.0);
      }
      if (f2 < f1) // shift right
      {
         x0 = x1;
         f0 = f1;
         x1 = x2;
         f1 = f2;
         x2 = x1 + h;
         //cout << "Calling from 4. loop\n";
         f2 = calc_testParameter(x2);
         if (f2 == -1.0) return (-1.0);
      }
      errno = 0;
      if (fabs(f0 - 2 * f1 + f2) <  FLT_MIN)
      {
         return(0); //division by zero above
      }
      xmin = x1 + (h * (f0 - f2))/(2 * (f0 - 2 * f1 + f2));
      //cout << "Calling from 5. loop\n";
      //cout << "xmin: " << xmin << ", diff: " << (2 * (f0 - 2 * f1 + f2)) << ", h: " << h << ", f0: " << f0 << ", f1: " << f1 << ", f2: " << f2 << endl;
      fmin = calc_testParameter(xmin);
      if (fmin == -1.0) return (-1.0);
      if (fmin < f1) 
      {
         x1 = xmin;
         f1 = fmin;
      }
      h = h / 2.0;
      if (h < tolerance)
      {
         return(x1);
      }
      x0 = x1 - h;
      x2 = x1 + h;
      //cout << "Calling from 6. loop\n";
      f0 = calc_testParameter(x0);
      if (f0 == -1.0) return (-1.0);
      f2 = calc_testParameter(x2);
      if (f2 == -1.0) return (-1.0);
   }
}
