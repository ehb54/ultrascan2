#include "../include/us_font.h"

US_Font::US_Font(QString *temp_fontFamily, int *temp_fontSize, QWidget *p, const char *name) : QDialog(p, name, true)
{
   USglobal=new US_Config();
      
   int buttonh   = 26;
   int buttonw   = 102;
   int column1   = 208;
   int column2   = 100;
   int border   = 4;
   int spacing   = 2;
   int width   = 310;
   int xpos      = border;
   int ypos      = border;
   QString str;
   
   fontSize = temp_fontSize;
   fontFamily = temp_fontFamily;
   oldFont.setPointSize(*fontSize);
   oldFont.setFamily(*fontFamily);
   
   setPalette(QPalette(USglobal->global_colors.cg_frame, USglobal->global_colors.cg_frame, USglobal->global_colors.cg_frame));
   setCaption(tr("Font Selection Dialog"));

   lbl_info = new QLabel(tr("Please select a Base Font:"), this);
   Q_CHECK_PTR(lbl_info);
   lbl_info->setFrameStyle(QFrame::WinPanel|QFrame::Raised);
   lbl_info->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);
   lbl_info->setPalette(QPalette(USglobal->global_colors.cg_frame, USglobal->global_colors.cg_frame, USglobal->global_colors.cg_frame));
   lbl_info->setGeometry(xpos, ypos, width, buttonh);
   lbl_info->setFont(QFont(*fontFamily, *fontSize, QFont::Bold));
   
   ypos += 2 + buttonh + spacing;
   xpos = border;

   pb_font = new QPushButton(tr("Select Font"), this);
   Q_CHECK_PTR(pb_font);
   pb_font->setAutoDefault(false);
   pb_font->setFont(QFont(*fontFamily, *fontSize));
   pb_font->setPalette( QPalette(USglobal->global_colors.cg_pushb, USglobal->global_colors.cg_pushb_disabled, USglobal->global_colors.cg_pushb_active));
   pb_font->setGeometry(xpos, ypos, buttonw, buttonh);
   connect(pb_font, SIGNAL(clicked()), SLOT(selectFont()));

   ypos += buttonh + spacing;
   
   lbl_family1 = new QLabel(tr("Current Family:"),this);
   lbl_family1->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
   lbl_family1->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_family1->setGeometry(xpos, ypos, column2, buttonh);
   lbl_family1->setFont(QFont(*fontFamily, *fontSize));
   
   xpos += column2 + spacing;
   
   lbl_family2 = new QLabel(" " + *fontFamily, this);
   lbl_family2->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
   lbl_family2->setPalette(QPalette(USglobal->global_colors.cg_edit, USglobal->global_colors.cg_edit, USglobal->global_colors.cg_edit));
   lbl_family2->setGeometry(xpos, ypos, column1, buttonh);
   lbl_family2->setFont(QFont(*fontFamily, *fontSize));
   
   ypos += buttonh + spacing;
   xpos = border;

   lbl_point1 = new QLabel(tr("Point Size:"),this);
   lbl_point1->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
   lbl_point1->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_point1->setGeometry(xpos, ypos, column2, buttonh);
   lbl_point1->setFont(QFont(*fontFamily, *fontSize));
   
   xpos += column2 + spacing;
   
   str.sprintf(tr(" %d points"), *fontSize);
   lbl_point2 = new QLabel(str, this);
   lbl_point2->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
   lbl_point2->setPalette(QPalette(USglobal->global_colors.cg_edit, USglobal->global_colors.cg_edit, USglobal->global_colors.cg_edit));
   lbl_point2->setGeometry(xpos, ypos, column1, buttonh);
   lbl_point2->setFont(QFont(*fontFamily, *fontSize));
   
   ypos += buttonh + spacing;
   xpos = border;
   
   lbl_sample = new QLabel(tr("Selected Font Samples:"), this);
   Q_CHECK_PTR(lbl_sample);
   lbl_sample->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);
   lbl_sample->setPalette(QPalette(USglobal->global_colors.cg_frame, USglobal->global_colors.cg_frame, USglobal->global_colors.cg_frame));
   lbl_sample->setGeometry(xpos, ypos, width, buttonh);
   lbl_sample->setFont(QFont(*fontFamily, *fontSize-1, QFont::Bold));


   ypos += buttonh + spacing;
   xpos = border;

   lbl_font1 = new QLabel(tr("Small Font Sample"),this);
   lbl_font1->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
   lbl_font1->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_font1->setGeometry(xpos, ypos, column1, buttonh);
   lbl_font1->setFont(QFont(*fontFamily, *fontSize - 1));
   
   ypos += buttonh + spacing;

   lbl_font2 = new QLabel(tr("Regular Font Sample"),this);
   lbl_font2->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
   lbl_font2->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_font2->setGeometry(xpos, ypos, column1, buttonh);
   lbl_font2->setFont(QFont(*fontFamily, *fontSize));

   ypos += buttonh + spacing;

   lbl_font3 = new QLabel(tr("Regular Font Sample, Bold"),this);
   lbl_font3->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
   lbl_font3->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_font3->setGeometry(xpos, ypos, column1, buttonh);
   lbl_font3->setFont(QFont(*fontFamily, *fontSize, QFont::Bold));
   
   ypos += buttonh + spacing;

   lbl_font4 = new QLabel(tr("Large Font Sample"),this);
   lbl_font4->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
   lbl_font4->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_font4->setGeometry(xpos, ypos, column1, buttonh);
   lbl_font4->setFont(QFont(*fontFamily, *fontSize + 1));

   ypos += buttonh + spacing;

   lbl_font5 = new QLabel(tr("Large Font Sample, Bold"),this);
   lbl_font5->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
   lbl_font5->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_font5->setGeometry(xpos, ypos, column1, buttonh);
   lbl_font5->setFont(QFont(*fontFamily, *fontSize + 1, QFont::Bold));
   
   ypos += buttonh + spacing;

   lbl_font6 = new QLabel(tr("Title Font Sample"),this);
   lbl_font6->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
   lbl_font6->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
   lbl_font6->setGeometry(xpos, ypos, column1, buttonh);
   lbl_font6->setFont(QFont(*fontFamily, *fontSize + 2, QFont::Bold));

   ypos += 2 + buttonh + spacing;
   xpos = border;

   pb_help = new QPushButton(tr("Help"), this);
   Q_CHECK_PTR(pb_help);
   pb_help->setAutoDefault(false);
   pb_help->setFont(QFont(*fontFamily, *fontSize + 1));
   pb_help->setPalette( QPalette(USglobal->global_colors.cg_pushb, USglobal->global_colors.cg_pushb_disabled, USglobal->global_colors.cg_pushb_active));
   pb_help->setGeometry(xpos, ypos, buttonw, buttonh);
   connect(pb_help, SIGNAL(clicked()), SLOT(help()));

   xpos += buttonw + spacing;
   
   pb_ok = new QPushButton(tr("OK"), this);
   Q_CHECK_PTR(pb_ok);
   pb_ok->setAutoDefault(false);
   pb_ok->setFont(QFont(*fontFamily, *fontSize + 1));
   pb_ok->setPalette( QPalette(USglobal->global_colors.cg_pushb, USglobal->global_colors.cg_pushb_disabled, USglobal->global_colors.cg_pushb_active));
   pb_ok->setGeometry(xpos, ypos, buttonw, buttonh);
   connect(pb_ok, SIGNAL(clicked()), SLOT(check()));

   xpos += buttonw + spacing;

   pb_cancel = new QPushButton(tr("Cancel"), this);
   Q_CHECK_PTR(pb_cancel);
   pb_cancel->setAutoDefault(false);
   pb_cancel->setFont(QFont(*fontFamily, *fontSize + 1));
   pb_cancel->setPalette( QPalette(USglobal->global_colors.cg_pushb, USglobal->global_colors.cg_pushb_disabled, USglobal->global_colors.cg_pushb_active));
   pb_cancel->setGeometry(xpos, ypos, buttonw, buttonh);
   connect(pb_cancel, SIGNAL(clicked()), SLOT(cancel()));

   ypos += buttonh + border;

   global_Xpos += 30;
   global_Ypos += 30;
   
   setMinimumSize(width+8, ypos);
   setGeometry(global_Xpos, global_Ypos, width+8, ypos);
   setup_GUI();
}

US_Font::~US_Font()
{
}
void US_Font::setup_GUI()
{
   QBoxLayout *topbox=new QVBoxLayout(this,2);
   topbox->addWidget(lbl_info);
   topbox->addWidget(pb_font);
   QGridLayout * lineGrid = new QGridLayout(topbox,2,2);
   lineGrid->addWidget(lbl_family1,0,0);
   lineGrid->addWidget(lbl_family2,0,1);
   lineGrid->addWidget(lbl_point1,1,0);
   lineGrid->addWidget(lbl_point2,1,1);
   topbox->addWidget(lbl_sample);
   topbox->addWidget(lbl_font1);
   topbox->addWidget(lbl_font2);
   topbox->addWidget(lbl_font3);
   topbox->addWidget(lbl_font4);
   topbox->addWidget(lbl_font5);
   topbox->addWidget(lbl_font6);
   QBoxLayout *buttonbox=new QHBoxLayout(topbox);
   buttonbox->addWidget(pb_help);
   buttonbox->addWidget(pb_ok);
   buttonbox->addWidget(pb_cancel);

}
void US_Font::closeEvent(QCloseEvent *e)
{
   e->accept();
   global_Xpos -= 30;
   global_Ypos -= 30;
}

void US_Font::cancel()
{
   reject();
}

void US_Font::check()
{
   accept();
}

void US_Font::help()
{
   US_Help *online_help; online_help = new US_Help(this);
   online_help->show_help("manual/usfont.html");
}

void US_Font::selectFont()
{
   bool ok;
   QString str;
   currentFont = QFontDialog::getFont( &ok, oldFont, this );
   if ( !ok )
   {
      currentFont = oldFont;
   }
   *fontFamily = currentFont.family();
   *fontSize = currentFont.pointSize();
   //   lbl_info->setFont(QFont(currentFont.family(), currentFont.pointSize(), QFont::Bold));
   //   pb_font->setFont(QFont(currentFont.family(), currentFont.pointSize()));
   //   lbl_family1->setFont(QFont(currentFont.family(), currentFont.pointSize()));
   lbl_family2->setFont(QFont(currentFont.family(), currentFont.pointSize()));
   //   lbl_point1->setFont(QFont(currentFont.family(), currentFont.pointSize()));
   lbl_point2->setFont(QFont(currentFont.family(), currentFont.pointSize()));
   lbl_font1->setFont(QFont(currentFont.family(), currentFont.pointSize() - 1));
   lbl_font2->setFont(QFont(currentFont.family(), currentFont.pointSize()));
   lbl_font3->setFont(QFont(currentFont.family(), currentFont.pointSize(), QFont::Bold));
   lbl_font4->setFont(QFont(currentFont.family(), currentFont.pointSize() + 1));
   lbl_font5->setFont(QFont(currentFont.family(), currentFont.pointSize() + 1, QFont::Bold));
   lbl_font6->setFont(QFont(currentFont.family(), currentFont.pointSize() + 2, QFont::Bold));
   pb_ok->setFont(QFont(currentFont.family(), currentFont.pointSize() + 1));
   pb_cancel->setFont(QFont(currentFont.family(), currentFont.pointSize() + 1));
   pb_help->setFont(QFont(currentFont.family(), currentFont.pointSize() + 1));

   lbl_family2->setText(currentFont.family());
   lbl_point2->setNum(currentFont.pointSize());

}
