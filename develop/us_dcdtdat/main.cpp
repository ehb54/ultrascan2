#include "../include/us_dcdtdat.h"
#include "../include/us_register.h"

int main (int argc, char **argv)
{
   QApplication a(argc, argv);
   US_Register *us_register;
   us_register = new US_Register();

   if(us_register->read())      //this is a correct file
   {
      delete us_register;
      dcdt_dat_W *dcdt_w;
      dcdt_w = new dcdt_dat_W();   
      dcdt_w->setCaption("Time Derivative - dC/dt Analysis:");
      dcdt_w->show();
      a.setMainWidget(dcdt_w);
      a.setDesktopSettingsAware(false);
   }
   else     // problem with license (either missing, incorrect or expired)
   {
      a.setMainWidget(us_register);
      a.setDesktopSettingsAware(false);
   }
   return a.exec();
}
