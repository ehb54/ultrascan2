#include "../include/us_database.h"
#include <qinputdialog.h>

#ifdef UNIX
#include <sys/stat.h>
#endif

//! Constructor
/*! Constractor a new <var>US_Database</var> interface, with <var>p</var> as a parent and <var>us_database</var> as object name. */ 
US_Database::US_Database(QWidget *p, const char *name) : QFrame( p, name)
{
  int xpos, ypos, buttonw = 160, buttonh = 26;
  count = 0;
  QString str;
  selected_item =-1;

  USglobal = new US_Config();

  data_file = QDir::convertSeparators(
      USglobal->config_list.system_dir + "/etc/database.dat" );
  
  read_file();
  
  setPalette(QPalette(USglobal->global_colors.cg_frame, 
                      USglobal->global_colors.cg_frame, 
                      USglobal->global_colors.cg_frame));
  export_format = true;
  
  login_list.driver = "QMYSQL3";
  login_list.username = "";
  login_list.password = "";
  login_list.dbname = "";
  login_list.host = "";
  login_list.description = "";
  
  xpos = border;
  ypos = border;  
  
  lbl_title = new QLabel(tr("Database List:"), this);
  lbl_title->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);
  lbl_title->setFrameStyle(QFrame::WinPanel|Raised);
  lbl_title->setPalette(QPalette(USglobal->global_colors.cg_frame, USglobal->global_colors.cg_frame, USglobal->global_colors.cg_frame));
  lbl_title->setGeometry(xpos, ypos, buttonw*2 + spacing, buttonh);
  lbl_title->setFont(QFont(USglobal->config_list.fontFamily, USglobal->config_list.fontSize, QFont::Bold));
  
  xpos = border;
  ypos += buttonh + spacing;
  
  lb_dbname = new QListBox(this, "Names");
  lb_dbname->setPalette(QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
  lb_dbname->setGeometry(xpos, ypos, buttonw*2 + spacing, buttonh*3);
  lb_dbname->setSelected(0, true);
  connect(lb_dbname, SIGNAL(selected(int)), SLOT(select_db(int)));
  
  xpos = border;
  ypos += buttonh*3 + spacing;
  
  lbl_blank = new QLabel(tr("User Information:"), this);
  lbl_blank->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);
  lbl_blank->setFrameStyle(QFrame::WinPanel|Raised);
  lbl_blank->setPalette(QPalette(USglobal->global_colors.cg_frame, USglobal->global_colors.cg_frame, USglobal->global_colors.cg_frame));
  lbl_blank->setGeometry(xpos, ypos, buttonw*2 + spacing, buttonh);
  lbl_blank->setFont(QFont(USglobal->config_list.fontFamily, USglobal->config_list.fontSize, QFont::Bold));

  xpos = border;
  ypos += buttonh + spacing;

  lbl_description = new QLabel(tr(" Database Description:"),this);
  lbl_description->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  lbl_description->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
  lbl_description->setGeometry(xpos, ypos, buttonw, buttonh);
  lbl_description->setFont(QFont(USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));
  
  xpos += buttonw + spacing;
  
  le_description = new QLineEdit(this, "description");
  le_description->setGeometry(xpos, ypos, buttonw, buttonh);
  le_description->setPalette(QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
  connect (le_description, SIGNAL(textChanged(const QString &)), SLOT(update_description(const QString &)));

  xpos = border;
  ypos += buttonh + spacing;

  lbl_username = new QLabel(tr(" User Name:"),this);
  lbl_username->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  lbl_username->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
  lbl_username->setGeometry(xpos, ypos, buttonw, buttonh);
  lbl_username->setFont(QFont(USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));
  
  xpos += buttonw + spacing;
  
  le_username = new QLineEdit(this, "username");
  le_username->setGeometry(xpos, ypos, buttonw, buttonh);
  le_username->setPalette(QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
  connect (le_username, SIGNAL(textChanged(const QString &)), SLOT(update_username(const QString &)));

  xpos = border;
  ypos += buttonh + spacing;

  lbl_password = new QLabel(tr(" Password:"),this);
  lbl_password->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  lbl_password->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
  lbl_password->setGeometry(xpos, ypos, buttonw, buttonh);
  lbl_password->setFont(QFont(USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));
  
  xpos += buttonw + spacing;
  
  le_password = new QLineEdit(this, "password");
  le_password->setGeometry(xpos, ypos, buttonw, buttonh);
  le_password->setPalette(QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
  le_password->setEchoMode(QLineEdit::Password);
  connect (le_password, SIGNAL(textChanged(const QString &)), SLOT(update_password(const QString &)));

  xpos = border;
  ypos += buttonh + spacing;

  lbl_dbname = new QLabel(tr(" Database Name:"),this);
  lbl_dbname->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  lbl_dbname->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
  lbl_dbname->setGeometry(xpos, ypos, buttonw, buttonh);
  lbl_dbname->setFont(QFont(USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));
  
  xpos += buttonw + spacing;
  
  le_dbname = new QLineEdit(this, "dbname");
  le_dbname->setGeometry(xpos, ypos, buttonw, buttonh);
  le_dbname->setPalette(QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
  connect (le_dbname, SIGNAL(textChanged(const QString &)), SLOT(update_dbname(const QString &)));

  xpos = border;
  ypos += buttonh + spacing;

  lbl_host = new QLabel(tr(" Host Address:"),this);
  lbl_host->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  lbl_host->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
  lbl_host->setGeometry(xpos, ypos, buttonw, buttonh);
  lbl_host->setFont(QFont(USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));
  
  xpos += buttonw + spacing;
  
  le_host = new QLineEdit(this, "host");
  le_host->setGeometry(xpos, ypos, buttonw, buttonh);
  le_host->setPalette(QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
  connect (le_host, SIGNAL(textChanged(const QString &)), SLOT(update_host(const QString &)));

  xpos = border;
  ypos += buttonh + spacing;

  lbl_mysql = new QLabel(tr(" MYSQL"),this);
  lbl_mysql->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  lbl_mysql->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
  lbl_mysql->setGeometry(xpos, ypos, buttonw + spacing, buttonh);
  lbl_mysql->setFont(QFont(USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));

  cb_mysql = new QCheckBox(this);
  cb_mysql->setGeometry(xpos+(int) 2 * buttonw/3, ypos+5, 14, 14);
  cb_mysql->setPalette( QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
  cb_mysql->setChecked(export_format);
  connect(cb_mysql, SIGNAL(clicked()), SLOT(select_mysql()));

  xpos += buttonw + spacing;

  lbl_oracle = new QLabel(tr(" ORACLE"),this);
  lbl_oracle->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  lbl_oracle->setPalette(QPalette(USglobal->global_colors.cg_label, USglobal->global_colors.cg_label, USglobal->global_colors.cg_label));
  lbl_oracle->setGeometry(xpos, ypos, buttonw, buttonh);
  lbl_oracle->setFont(QFont(USglobal->config_list.fontFamily, USglobal->config_list.fontSize - 1, QFont::Bold));

  cb_oracle = new QCheckBox(this);
  cb_oracle->setGeometry(xpos+(int) 2 * buttonw/3, ypos+5, 14, 14);
  cb_oracle->setPalette( QPalette(USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal, USglobal->global_colors.cg_normal));
  cb_oracle->setChecked(!export_format);
  connect(cb_oracle, SIGNAL(clicked()), SLOT(select_oracle()));
    
  ypos += buttonh + 5 * spacing;
  xpos = border;

  pb_help = new QPushButton(tr("Help"), this);
  pb_help->setAutoDefault(false);
  pb_help->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize));
  pb_help->setPalette(QPalette(USglobal->global_colors.cg_pushb, USglobal->global_colors.cg_pushb_disabled, USglobal->global_colors.cg_pushb_active));
  pb_help->setGeometry(xpos, ypos, buttonw, buttonh);
  pb_help->setEnabled(true);
  connect(pb_help, SIGNAL(clicked()), SLOT(help()));

  xpos +=buttonw + spacing;

  pb_reset = new QPushButton(tr("Reset"), this);
  pb_reset->setAutoDefault(false);
  pb_reset->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize));
  pb_reset->setPalette(QPalette(USglobal->global_colors.cg_pushb, USglobal->global_colors.cg_pushb_disabled, USglobal->global_colors.cg_pushb_active));
  pb_reset->setGeometry(xpos, ypos, buttonw, buttonh);
  pb_reset->setEnabled(true);
  connect(pb_reset, SIGNAL(clicked()), SLOT(reset()));
  
  xpos = border;
  ypos += buttonh + spacing;
  
  pb_add = new QPushButton(tr("Add New Entry"), this);
  pb_add->setAutoDefault(false);
  pb_add->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize));
  pb_add->setPalette(QPalette(USglobal->global_colors.cg_pushb, USglobal->global_colors.cg_pushb_disabled, USglobal->global_colors.cg_pushb_active));
  pb_add->setGeometry(xpos, ypos, buttonw, buttonh);
  pb_add->setEnabled(true);
  connect(pb_add, SIGNAL(clicked()), SLOT(check_add()));
  
  xpos += buttonw + spacing;
  
  pb_delete = new QPushButton(tr("Delete Old Entry"), this);
  pb_delete->setAutoDefault(false);
  pb_delete->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize));
  pb_delete->setPalette(QPalette(USglobal->global_colors.cg_pushb, USglobal->global_colors.cg_pushb_disabled, USglobal->global_colors.cg_pushb_active));
  pb_delete->setGeometry(xpos, ypos, buttonw, buttonh);
  pb_delete->setEnabled(true);
  connect(pb_delete, SIGNAL(clicked()), SLOT(check_del()));
  
  xpos = border;
  ypos += buttonh + spacing;
  
  pb_submit = new QPushButton(tr("Save as Default"), this);
  pb_submit->setAutoDefault(false);
  pb_submit->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize));
  pb_submit->setPalette(QPalette(USglobal->global_colors.cg_pushb, USglobal->global_colors.cg_pushb_disabled, USglobal->global_colors.cg_pushb_active));
  pb_submit->setGeometry(xpos, ypos, buttonw, buttonh);
  pb_submit->setEnabled(false);
  connect(pb_submit, SIGNAL(clicked()), SLOT(check_save()));

  xpos += buttonw + spacing;
  
  pb_close = new QPushButton(tr("Cancel and Exit"), this);
  pb_close->setAutoDefault(false);
  pb_close->setFont(QFont( USglobal->config_list.fontFamily, USglobal->config_list.fontSize));
  pb_close->setPalette(QPalette(USglobal->global_colors.cg_pushb, USglobal->global_colors.cg_pushb_disabled, USglobal->global_colors.cg_pushb_active));
  pb_close->setGeometry(xpos, ypos, buttonw, buttonh);
  pb_close->setEnabled(true);
  connect(pb_close, SIGNAL(clicked()), SLOT(quit()));
  
  ypos += buttonh + border;
  xpos = 2 * border + buttonw*2 + spacing;

  global_Xpos += 30;
  global_Ypos += 30;

  setMinimumSize(xpos, ypos); 
  setGeometry(global_Xpos, global_Ypos, xpos, ypos);
  setup_GUI();
  update_screen();
}

//! Destructor
/*! destroy the US_Database. */
US_Database::~US_Database()
{
}

void US_Database::setup_GUI()
{
  QBoxLayout *topbox = new QVBoxLayout(this,4);
  topbox->addWidget(lbl_title);
  topbox->addWidget(lb_dbname);
  topbox->addWidget(lbl_blank);
  QGridLayout * lineGrid = new QGridLayout(topbox,5,2);
  lineGrid->addWidget(lbl_description,0,0);
  lineGrid->addWidget(le_description,0,1);
  lineGrid->addWidget(lbl_username,1,0);
  lineGrid->addWidget(le_username,1,1);
  lineGrid->addWidget(lbl_password,2,0);
  lineGrid->addWidget(le_password,2,1);
  lineGrid->addWidget(lbl_dbname,3,0);
  lineGrid->addWidget(le_dbname,3,1);
  lineGrid->addWidget(lbl_host,4,0);
  lineGrid->addWidget(le_host,4,1);
  QBoxLayout *bar=new QHBoxLayout(topbox);
  bar->addWidget(lbl_mysql);
  bar->addWidget(cb_mysql);
  bar->addWidget(lbl_oracle);
  bar->addWidget(cb_oracle);
  QGridLayout * buttonGrid = new QGridLayout(topbox,3,2);
  buttonGrid->addWidget(pb_help,0,0);
  buttonGrid->addWidget(pb_reset,0,1);
  buttonGrid->addWidget(pb_add,1,0);
  buttonGrid->addWidget(pb_delete,1,1);
  buttonGrid->addWidget(pb_submit,2,0);
  buttonGrid->addWidget(pb_close,2,1);
  topbox->activate();
}
void US_Database::read_file()
{
  QFile f( data_file );   // etc/database.dat

  if ( f.exists() )
  {
    QString dbfile = US_Config::get_home_dir(  ) + USDB;

    QFile chk( dbfile );

    if ( ! chk.exists() )   // usdbconf.bin
    {
      QMessageBox::message(
          tr("Attention:"), 
          tr("Do not find your default database USglobal file. \n"
             "Click 'Continue' to select one Database and save as your default DB. "),
          tr("Continue"));
    }
    else
    {
      chk.open( IO_ReadOnly );
      QDataStream ds( &chk );

      double  num[64];
      int     code[64];
      int     i;
      int     length;

      ds >> length;

      for ( i=0; i<length; i++ )
      {
        ds >> num[i];
        code[i]    = (int)(num[i]+0.5);
        db_name[i] = QChar(code[i]);
      }

      ds >> length;

      for ( i=0; i<length; i++ )
      {
        ds >> num[0];  // Throw away.
      }

      ds >> length;
      
      for ( i=0; i<length; i++ )
      {
        ds >> num[0]; // Throw away
      }

      ds >> length;
      
      for ( i=0; i<length; i++ )
      {
        ds >> num[i];
        code[i]      = (int)(num[i]+0.5);
        host_name[i] = QChar(code[i]);
      }
      
      chk.close();
    }

    f.open( IO_ReadOnly );
    QTextStream ts( &f );  // etc/database.dat

    struct US_LoginData temp_list;
    dblist.clear();

    while( ! ts.atEnd() )
    {
      temp_list.description = ts.readLine();
      temp_list.username    = ts.readLine();
      temp_list.password    = ts.readLine();
      temp_list.dbname      = ts.readLine();
      temp_list.host        = ts.readLine();
      temp_list.driver      = ts.readLine();
      dblist.push_back( temp_list );
    }

    count = dblist.size();
  }
  else
  {
    QMessageBox::message(
        tr("Attention:"), 
        tr("The database.dat could not be found.\n"
           "Click 'Continue' to create a new database.dat file."),
        tr("Continue"));
  }
}

void US_Database::select_db( int i )
{
  
  le_description->setText(dblist[i].description);
  le_description->setEnabled(false);
  le_username   ->setText(dblist[i].username);
  le_username   ->setEnabled(false);
  le_password   ->setText(dblist[i].password);
  le_password   ->setEnabled(false);
  le_dbname     ->setText(dblist[i].dbname);
  le_dbname     ->setEnabled(false);
  le_host       ->setText(dblist[i].host);
  le_host       ->setEnabled(false);
  
  if ( dblist[i].driver == "QMYSQL3" )
  {
    select_mysql();
  }
  
  if ( dblist[i].driver == "QOCI8" )
  {
    select_oracle();
  }

  cb_mysql ->setEnabled(false);
  cb_oracle->setEnabled(false);
  pb_submit->setEnabled(true);
  pb_add   ->setEnabled(false);
  selected_item = i;
}

/*!
  Replace struct US_LoginData <var>description</var> value with newText.
*/
void US_Database::update_description(const QString &newText)
{
  login_list.description = newText;
}

/*!
  Replace struct US_LoginData <var>username</var> value with newText.
*/
void US_Database::update_username(const QString &newText)
{
  login_list.username = newText;
}
/*!
  Replace struct US_LoginData <var>password</var> value with newText. 
*/

void US_Database::update_password(const QString &newText)
{
  login_list.password = newText;
}
/*!
  Replace struct US_LoginData <var>dbname</var> value with newText. 
*/

void US_Database::update_dbname(const QString &newText)
{
  login_list.dbname = newText;
}
/*!
  Replace struct US_LoginData <var>host</var> value with newText.
*/

void US_Database::update_host(const QString &newText)
{
  login_list.host = newText;
}
/*!
  Choose MySQL as database mode.
*/
void US_Database::select_mysql()
{
  export_format = true;
  cb_mysql->setChecked(true);
  cb_oracle->setChecked(false);
  login_list.driver = "QMYSQL3";
}
/*!
  Choose Oracle as database mode.
*/
void US_Database::select_oracle()
{
  export_format = false;
  cb_mysql->setChecked(false);
  cb_oracle->setChecked(true);
  login_list.driver = "QOCI8";
}

void US_Database::check_add()
{
  if ( login_list.description.isEmpty() )
  {
    QMessageBox::message(
        tr("Attention:"), 
        tr("Please enter a Description\n"
           "before saving..."));
    return;
  }

  if ( login_list.username.isEmpty() )
  {
    QMessageBox::message(
        tr("Attention:"), 
        tr("Please enter a User Name\n"
           "before saving..."));
    return;
  }

  if ( login_list.password.isEmpty() )
  {
    QMessageBox::message(
        tr("Attention:"), 
        tr("Please enter a Password\n"
           "before saving..."));
    return;
  }

  if ( login_list.dbname.isEmpty() )
  {
    QMessageBox::message(
        tr("Attention:"), 
        tr("Please enter a Database Name\n"
           "before saving..."));
    return;
  }

  if ( login_list.host.isEmpty() )
  {
    QMessageBox::message(
        tr("Attention:"), 
        tr("Please enter a Host Address\n"
           "(maybe \"localhost\"?)\n"
           "before saving..."));
    return;
  }

  US_DB_Admin* db_admin = new US_DB_Admin("");
  db_admin->show();
  connect(db_admin, SIGNAL(issue_permission(bool)), SLOT(add(bool)));
}

void US_Database::add( bool permission )
{
  if ( ! permission )
  {
    QMessageBox::message(
        tr("Attention:"), 
        tr("Permission denied"));
    return;
  }

  switch ( QMessageBox::information( this, 
        tr("Save this profile?"), 
        tr("Clicking 'OK' will save this profile to database.dat\n"
           "and set it as your default database login information."),
        tr("OK"), tr("CANCEL"), 0, 1))
  {
    case 0:
      Encode(""); // For usdbconf.bin
      
      if ( ! data_file.isEmpty() )
      {       
        // Get password md5 hash

        QString pw_hash = US_Encryption::md5( login_list.password );

        struct US_LoginData temp_list;
        temp_list.description = login_list.description;
        temp_list.username    = login_list.username;
        temp_list.password    = pw_hash;
        temp_list.dbname      = login_list.dbname;
        temp_list.host        = login_list.host;
        temp_list.driver      = login_list.driver;
        dblist.push_back( temp_list );



        QFile fw( data_file );

        fw.open( IO_WriteOnly | IO_Translate );
        QTextStream ts( &fw );
        count = dblist.size();
        
        for ( int i=0; i<count; i++ )
        {
          ts << dblist[i].description << "\n";
          ts << dblist[i].username << "\n";
          ts << dblist[i].password << "\n";
          ts << dblist[i].dbname << "\n";
          ts << dblist[i].host << "\n";
          ts << dblist[i].driver << "\n";
        }

        fw.close();
        quit();
        break;
      } 

    case 1:
      break;
  }
}

void US_Database::check_del()
{
  if(selected_item < 0)
  {
    QMessageBox::message(
        tr( "Attention:" ), 
        tr( "First select the login DB information\n"
            "which you want to delete from the list."));
    return;
  }

  US_DB_Admin* db_admin = new US_DB_Admin("");
  db_admin->show();
  connect(db_admin, SIGNAL( issue_permission(bool) ), 
                    SLOT  ( del(bool) ) );
}

void US_Database::del( bool permission )
{
  if ( ! permission )
  {
    QMessageBox::message(
        tr("Attention:"), 
        tr("Permission denied"));
    return;
  }

  QString selected_db_name = dblist[selected_item].dbname;

  vector <struct US_LoginData>::iterator item = dblist.begin() + selected_item;
  dblist.erase(item);

  if ( db_name == selected_db_name )   //remove usdbconf.bin
  {
    QString filename = US_Config::get_home_dir(  ) + USDB;
    QFile f( filename );
    
    if ( f.exists() )
    {
      f.remove();
    }
  }

  QFile fw( data_file );   // etc/database.dat
  fw.open ( IO_WriteOnly | IO_Translate );
  
  count = dblist.size();
  lb_dbname->clear();

  QTextStream ts( &fw );

  for ( int i=0; i<count; i++ )
  {
    ts << dblist[i].description << "\n";
    ts << dblist[i].username    << "\n";
    ts << dblist[i].password    << "\n";
    ts << dblist[i].dbname      << "\n";
    ts << dblist[i].host        << "\n";
    ts << dblist[i].driver      << "\n";
    
    lb_dbname->insertItem( dblist[i].description );
  }

  fw.close();
  clear();
}

void US_Database::check_save()
{
  bool ok;

  QString pw2 = QInputDialog::getText(
      tr( "Password Verification" ),
      tr( "Retype your password" ),
      QLineEdit::Password,
      QString::null,
      &ok,
      this );

  if ( ! ok ) return;

  QString md5string = US_Encryption::md5( pw2 );

  if ( QString::compare( md5string, le_password->text() ) != 0 )
  {
    QMessageBox::message(
        tr( "Attention:" ),
        tr( "Your input password is incorrect.\n" ) );
    return;
  }

  save( pw2 );

  //US_DB_Admin *db_admin;
  //db_admin = new US_DB_Admin(str);
  //db_admin->show();
  //connect(db_admin, SIGNAL(issue_pass(QString)), SLOT(save(QString)));
}

/*!
  Save all new values to DB file and exit.
*/
void US_Database::save( QString password )
{
  if ( password.isEmpty() )
  {
    QMessageBox::message(
        tr("Attention:"), 
        tr("Permission denied"));
    return;
  }

  switch( QMessageBox::information( this, 
        tr("Save this profile?"), 
        tr("Clicking 'OK' will save this profile\n"
           "as your default database login information."),
        tr("OK"), tr("CANCEL"), 0, 1) )
  {
    case 0:
      Encode( password );
      quit();
      break;

    case 1:
      break;
  }
}

void US_Database:: Encode( QString real_passwd )
{
  filename = US_Config::get_home_dir(  ) + USDB;

  QFile f( filename );
  f.open( IO_WriteOnly );
  QDataStream ds (&f);

  QChar c;
  int i, length;
  
  length = login_list.dbname.length();
  ds << length;
  
  for ( i=0; i<length; i++ )
  {
    c = login_list.dbname[i];
    ds << (double)c.unicode();
  }
  
  length = login_list.username.length();  
  ds << length;
  
  for ( i=0; i<length; i++ )
  {
    c = login_list.username[i];
    ds << (double)c.unicode();
  }

  // To change the database
  if ( ! real_passwd.isEmpty() )
  {
    length = real_passwd.length();
    ds << length;
    
    for ( i=0; i<length; i++ )
    {
      c = real_passwd[i];
      ds << (double)c.unicode();
    }
  }

  // For new entry
  else
  {
    length = login_list.password.length();
    ds << length;
    
    for ( i=0; i<length; i++ )
    {
      c = login_list.password[i];
      ds << (double)c.unicode();
    }
  }

  length = login_list.host.length();
  ds << length;
  
  for ( i=0; i<length; i++ )
  {
    c = login_list.host[i];
    ds << (double)c.unicode();
  }
  
  length = login_list.driver.length();
  ds << length;
  
  for ( i=0; i<length; i++ )
  {
    c = login_list.driver[i];
    ds << (double)c.unicode();
  }

  f.close();

#ifdef UNIX
  chmod( filename, 0600);
#endif
}

/*!
  Reset all values to empty.
*/
void US_Database::reset()
{
  clear();
  pb_add->setEnabled( true );
}

void US_Database::clear()
{
  if ( export_format == false )
  {
    select_mysql();
  }

  //lb_dbname->setEnabled(false);
  le_description->setEnabled(true);
  le_description->setText("");
  
  le_username   ->setEnabled(true);
  le_username   ->setText("");
  
  le_password   ->setEnabled(true);
  le_password   ->setText("");
  
  le_dbname     ->setEnabled(true);
  le_dbname     ->setText("");
  
  le_host       ->setEnabled(true);
  le_host       ->setText("");
  
  cb_mysql      ->setEnabled(true);
  cb_oracle     ->setEnabled(true);
  pb_submit     ->setEnabled(false);
  
  selected_item = -1;

}
/*!
  Open a netscape browser to load help page.
*/
void US_Database::help()
{
  US_Help* online_help; online_help = new US_Help(this);
  online_help->show_help("manual/database_config.html");
}

/*!
  Clean all values and exit.
*/
void US_Database::quit()
{
  login_list.driver      = "QMYSQL3";
  login_list.username    = "";
  login_list.password    = "";
  login_list.dbname      = "";
  login_list.host        = "";
  login_list.description = "";
  lb_dbname->clear();
  close();
}
/*!
  After reading DB file, All new values show in interface.
*/
void US_Database::update_screen()
{
  for(int i=0; i<count; i++)
  {
    if ( db_name == dblist[i].dbname  &&  host_name == dblist[i].host )
    {
      QString str = dblist[i].description + " (default)";
      lb_dbname->insertItem(str);
      lb_dbname->setSelected(i, true);
    }
    else
    {
      lb_dbname->insertItem(dblist[i].description);
    }
  }
}
