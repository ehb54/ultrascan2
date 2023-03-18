#include "../include/us_data_io.h"
#include "../include/us_limsconv.h"

US_Data_IO::US_Data_IO(struct runinfo *run_inf, bool baseline_flag, QObject *parent, const char *name)
   : QObject (parent, name)
{
   this->baseline_flag = baseline_flag;
   this->run_inf = run_inf;
   USglobal = new US_Config();
   temperature = 20.0;
   VBAR = new US_Vbar(temperature); //CAUTION: Temperature needs to be updated when data is loaded
   connect(VBAR, SIGNAL(newMessage(QString, int)), this, SIGNAL(newMessage(QString, int)));
   BUFFER = new US_Buffer();
   connect(BUFFER, SIGNAL(newMessage(QString, int)), this, SIGNAL(newMessage(QString, int)));
}

US_Data_IO::~US_Data_IO()
{
   delete USglobal;
   delete BUFFER;
   delete VBAR;
}

void US_Data_IO::assign_simparams(struct SimulationParameters *sp, unsigned int cell, unsigned int lambda, unsigned int channel)
{
   (*sp).mesh_radius.resize(0); // the radii from a user-selected mesh file (mesh == 3)
   (*sp).speed_step.resize(1);
   unsigned int secs = (*run_inf).time[cell][lambda][(*run_inf).scans[cell][lambda]-1];
   (*sp).speed_step[0].duration_hours = (int) secs/3600;
   (*sp).speed_step[0].duration_minutes = 1 + (secs - ((*sp).speed_step[0].duration_hours * 3600))/60;
   (*sp).speed_step[0].acceleration = 400; // 400 rpm/sec is the default
   (*sp).speed_step[0].delay_hours = 0;
   (*sp).speed_step[0].delay_minutes = ((*run_inf).rpm[cell][lambda][channel]/(*sp).speed_step[0].acceleration)/60.0;
   (*sp).speed_step[0].scans = (*run_inf).scans[cell][lambda];
   (*sp).speed_step[0].rotorspeed = (*run_inf).rpm[cell][lambda][channel];
   (*sp).speed_step[0].acceleration_flag = true;

   (*sp).simpoints = 100;            // number of radial grid points for simulation (should be overridden by user)
                                     // are assigned in the constraint GUI
   (*sp).mesh = 0;                    // 0 = ASTFEM, 1 = Claverie, 2 = moving hat,
   // 3 = user-selected mesh, 4 = nonuniform constant mesh
   (*sp).moving_grid = 1;               // use adaptive time steps = 1, fixed time steps = 0
   (*sp).radial_resolution = (float) 0.001;   // the radial datapoint increment/resolution
   // of the final data
   (*sp).meniscus = (*run_inf).meniscus[cell];            // meniscus position at first constant speed
   //cout << "Rotor: " << (*run_inf).rotor << ", centerpiece: " << (*run_inf).centerpiece[cell] << endl;
   (*sp).bottom = calc_bottom((*run_inf).rotor, (*run_inf).centerpiece[cell], channel, 0); // bottom position at rest
   (*sp).rnoise = 0.0;               // random noise
   (*sp).tinoise = 0.0;               // time invariant noise
   (*sp).rinoise = 0.0;               // radially invariant noise
   (*sp).rotor = (*run_inf).rotor;   // rotor serial number in database
   vector <struct centerpieceInfo> cp_info;
   readCenterpieceInfo(&cp_info);
   if (cp_info[(*run_inf).centerpiece[cell]].sector == 4)
   {
      (*sp).band_forming = true;
   }
   else
   {
      (*sp).band_forming = false;
   }
   (*sp).band_volume = 0.015;         // should be overridden by user
}

int US_Data_IO::load_run(QString fn, int run_type, bool *has_data,
                         vector <struct centerpieceInfo> *cp_list)
{
   bool gridcontrol_flag = false;
   if (run_type == 201) // us_gridcontrol_t sends a '201' flag to signal veloc data
   {
      gridcontrol_flag = true;
      run_type = 1;
   }
   unsigned int i, j, k;
   if ((*run_inf).temperature != NULL)
   {
      for (i=0; i<8; i++)
      {
         for (j=0; j<(*run_inf).wavelength_count[i]; j++)
         {
            delete [] (*run_inf).temperature[i][j];
         }
         delete [] (*run_inf).temperature[i];
      }
      delete [] (*run_inf).temperature;
   }
   if ((*run_inf).rpm != NULL)
   {
      for (i=0; i<8; i++)
      {
         for (j=0; j<(*run_inf).wavelength_count[i]; j++)
         {
            delete [] (*run_inf).rpm[i][j];
         }
         delete [] (*run_inf).rpm[i];
      }
      delete [] (*run_inf).rpm;
   }
   if ((*run_inf).time != NULL)
   {
      for (i=0; i<8; i++)
      {
         for (j=0; j<(*run_inf).wavelength_count[i]; j++)
         {
            delete [] (*run_inf).time[i][j];
         }
         delete [] (*run_inf).time[i];
      }
      delete [] (*run_inf).time;
   }
   if ((*run_inf).omega_s_t != NULL)
   {
      for (i=0; i<8; i++)
      {
         for (j=0; j<(*run_inf).wavelength_count[i]; j++)
         {
            delete [] (*run_inf).omega_s_t[i][j];
         }
         delete [] (*run_inf).omega_s_t[i];
      }
      delete [] (*run_inf).omega_s_t;
   }
   if ((*run_inf).plateau != NULL)
   {
      for (i=0; i<8; i++)
      {
         for (j=0; j<(*run_inf).wavelength_count[i]; j++)
         {
            delete [] (*run_inf).plateau[i][j];
         }
         delete [] (*run_inf).plateau[i];
      }
      delete [] (*run_inf).plateau;
   }

   // run_type: 1 = velocity, 2 = equilibrium
   QString version, str;
   int et;
   if (run_type == 1 || run_type == 3 || run_type == 5 || run_type == 7 || run_type == 9 || run_type == 11 || run_type == 31 || run_type == 32)
   {
      QFile f(fn);
      //      cerr << "File used: " << fn << endl;
      if (!f.open(IO_ReadOnly))
      {
         return(-1); // fails to open file
      }
      QDataStream ds (&f);
      if (ds.atEnd())
      {
         f.close();
         return(-2); // empty file
      }
      ds >> version;
      QString tmp_v = version.left(3);
      if (tmp_v.toFloat() < 6.0)
      {
         str = tr("These data were edited with an usupported release\n"
                  "of UltraScan (version " + version + "), which is not\n"
                  "binary compatible with the current version (" + US_Version + ").\n\n"
                  "Please re-edit the experimental data before\n"
                  "using the data for data analysis.");
         cerr << str << endl;
         emit newMessage(str, -3);
         f.close();
         return(-3);   // -3 error code = wrong version
      }
      ds >> (*run_inf).data_dir;
      //cerr << "data_dir: " << (*run_inf).data_dir.latin1() << endl;
      ds >> (*run_inf).run_id;
      //cerr << "Run ID: " << (*run_inf).run_id.latin1() << endl;
      ds >> (*run_inf).avg_temperature;
      temperature = (*run_inf).avg_temperature;
      VBAR->temperature = temperature; // update VBAR object's temperature as well'
      //cerr << "Avg. temp: " << (*run_inf).avg_temperature << endl;
      ds >> (*run_inf).temperature_check;
      //cerr << "temp check: " << (*run_inf).temperature_check << endl;
      ds >> (*run_inf).time_correction;
      //cerr << "Time correction: " << (*run_inf).time_correction << endl;
      ds >> (*run_inf).duration;
      //cerr << "duration: " << (*run_inf).duration << endl;
      ds >> (*run_inf).total_scans;
      //cerr << "Total Scans: " << (*run_inf).total_scans << endl;
      ds >> (*run_inf).delta_r;
      //cerr << "Delta-r: " << (*run_inf).delta_r << endl;
      ds >> (*run_inf).expdata_id;
      //cerr << "data id: " << (*run_inf).expdata_id << endl;
      ds >> (*run_inf).investigator;
      //cerr << "investigator: " << (*run_inf).investigator << endl;
      ds >> (*run_inf).date;
      //cerr << "date: " << (*run_inf).date << endl;
      ds >> (*run_inf).description;
      //cerr << "description: " << (*run_inf).description << endl;
      ds >> (*run_inf).dbname;
      //cerr << "dbname: " << (*run_inf).dbname << endl;
      ds >> (*run_inf).dbhost;
      //cerr << "dbhost: " << (*run_inf).dbhost << endl;
      ds >> (*run_inf).dbdriver;
      //cerr << "dbdriver: " << (*run_inf).dbdriver << endl;
      ds >> et;
      (*run_inf).exp_type.velocity = (bool) et;
      //cerr << "velocity: " << (*run_inf).exp_type.velocity << endl;
      ds >> et;
      (*run_inf).exp_type.equilibrium = (bool) et;
      //cerr << "equilibrium: " << (*run_inf).exp_type.equilibrium << endl;
      ds >> et;
      (*run_inf).exp_type.diffusion = (bool) et;
      //cerr << "diffusion: " << (*run_inf).exp_type.diffusion << endl;
      ds >> et;
      (*run_inf).exp_type.simulation = (bool) et;
      //cerr << "simulation: " << (*run_inf).exp_type.simulation << endl;
      ds >> et;
      (*run_inf).exp_type.interference = (bool) et;
      //cerr << "interference: " << (*run_inf).exp_type.interference << endl;
      ds >> et;
      (*run_inf).exp_type.absorbance = (bool) et;
      //cerr << "absorbance: " << (*run_inf).exp_type.absorbance << endl;
      ds >> et;
      (*run_inf).exp_type.fluorescence = (bool) et;
      //cerr << "fluorescence: " << (*run_inf).exp_type.fluorescence << endl;
      ds >> et;
      (*run_inf).exp_type.intensity = (bool) et;
      //cerr << "intensity: " << (*run_inf).exp_type.intensity << endl;
      ds >> et;
      (*run_inf).exp_type.wavelength = (bool) et;
      //cerr << "wavelength: " << (*run_inf).exp_type.wavelength << endl;
      for (i=0; i<8; i++)
      {
         ds >> (*run_inf).centerpiece[i];
         //cerr <<  "Centerpiece[" << i << "]: " << (*run_inf).centerpiece[i] << endl;
         ds >> (*run_inf).meniscus[i];
         //cerr <<  "Meniscus[" << i << "]: " << (*run_inf).meniscus[i] << endl;
         ds >> (*run_inf).cell_id[i];
         //cerr <<  "cell_id[" << i << "]: " << (*run_inf).cell_id[i] << endl;
         ds >> (*run_inf).wavelength_count[i];
         //cerr <<  "wavelenth_count[" << i << "]: " << (*run_inf).wavelength_count[i] << endl;
      }
      for (i=0; i<8; i++)
      {
         if (version.toFloat() < 7.0)
         {
            for (j=0; j<4; j++)   // one for each channel
            {
               ds >> (*run_inf).DNA_serialnumber[i][j][0];
               ds >> (*run_inf).buffer_serialnumber[i][j];
               ds >> (*run_inf).peptide_serialnumber[i][j][0];
               //cerr << "Cell " << i+1 <<", Channel " << j+1 << " DNA, buffer, peptide: " << (*run_inf).DNA_serialnumber[i][j][0] << ", " <<  (*run_inf).buffer_serialnumber[i][j] << ", " << (*run_inf).peptide_serialnumber[i][j][0] << "\n";
            }
         }
         else
         {
            for (j=0; j<4; j++)
            {
               ds >> (*run_inf).buffer_serialnumber[i][j];
               for(int k=0; k<3; k++)
               {
                  ds >> (*run_inf).peptide_serialnumber[i][j][k];
                  ds >> (*run_inf).DNA_serialnumber[i][j][k];
                  //cerr << "Cell " << i+1 <<", Channel " << j+1 << " DNA, buffer, peptide: " << (*run_inf).DNA_serialnumber[i][j][k] << ", " <<  (*run_inf).buffer_serialnumber[i][j] << ", " << (*run_inf).peptide_serialnumber[i][j][k] << "\n";
               }
            }
         }
         for (j=0; j<3; j++)
         {
            ds >> (*run_inf).wavelength[i][j];
            ds >> (*run_inf).scans[i][j];
            ds >> (*run_inf).baseline[i][j];
            if ((*run_inf).centerpiece[i] >= 0)
            {
               for (k=0; k<(*cp_list)[(*run_inf).centerpiece[i]].channels; k++)
               {
                  ds >> (*run_inf).range_left[i][j][k];
                  ds >> (*run_inf).range_right[i][j][k];
                  ds >> (*run_inf).points[i][j][k];
                  ds >> (*run_inf).point_density[i][j][k];
               }
            }
         }
      }
      (*run_inf).temperature = new float** [8];
      (*run_inf).rpm = new unsigned int** [8];
      (*run_inf).time = new unsigned int** [8];
      (*run_inf).omega_s_t   = new float** [8];
      (*run_inf).plateau    = new float** [8];
      for (i=0; i<8; i++)
      {
         (*run_inf).temperature[i] = new float* [(*run_inf).wavelength_count[i]];
         (*run_inf).rpm[i] = new unsigned int* [(*run_inf).wavelength_count[i]];
         (*run_inf).time[i]      = new unsigned int* [(*run_inf).wavelength_count[i]];
         (*run_inf).omega_s_t[i]   = new float* [(*run_inf).wavelength_count[i]];
         (*run_inf).plateau[i]    = new float* [(*run_inf).wavelength_count[i]];
         for (j=0; j<(*run_inf).wavelength_count[i]; j++)
         {
            (*run_inf).temperature[i][j] = new float [(*run_inf).scans[i][j]];
            (*run_inf).rpm[i][j] = new unsigned int [(*run_inf).scans[i][j]];
            (*run_inf).time[i][j]      = new unsigned int [(*run_inf).scans[i][j]];
            (*run_inf).omega_s_t[i][j]   = new float [(*run_inf).scans[i][j]];
            (*run_inf).plateau[i][j]    = new float [(*run_inf).scans[i][j]];
         }
      }
      for (i=0; i<8; i++)
      {
         for (j=0; j<(*run_inf).wavelength_count[i]; j++)
         {
            for (k=0; k<(*run_inf).scans[i][j]; k++)
            {
               ds >> (*run_inf).rpm[i][j][k];
               ds >> (*run_inf).temperature[i][j][k];
               ds >> (*run_inf).time[i][j][k];
               (*run_inf).time[i][j][k] -= (unsigned int) ((*run_inf).time_correction + 0.5);
               ds >> (*run_inf).omega_s_t[i][j][k];
               ds >> (*run_inf).plateau[i][j][k];
               //cerr << "Cell " << i+1 <<", wavelength " << j+1 << ", scan " << k+1 << " (rpm, temp, time, omega, plateau: " << (*run_inf).rpm[i][j][k] << ", " << (*run_inf).temperature[i][j][k] << ", " << (*run_inf).time[i][j][k] << ", " << (*run_inf).omega_s_t[i][j][k] << ", " << (*run_inf).plateau[i][j][k] << "\n";
               if (run_type == 1 && baseline_flag)
               {
                  //cout << "subtracting baseline in data_io...\n";
                  (*run_inf).plateau[i][j][k] -= (*run_inf).baseline[i][j];
               }
            }
         }
      }
      for (i=0; i<8; i++)
      {
         has_data[i]=false;
         if ((*run_inf).scans[i][0] != 0)
         {
            has_data[i] = true;
         }
      }
      ds >> (*run_inf).rotor;
      f.close();
   }
   else if (run_type == 2)
   {
      QFile f(fn);
      if (!f.open(IO_ReadOnly))
      {
         return(-1); // fails to open file
      }
      QDataStream ds (&f);
      if (ds.atEnd())
      {
         f.close();
         return(-2); // empty file
      }
      ds >> version;
      //cerr << "Version: " << version << endl;
      if (version.toFloat() < 6.0)
      {
         str = tr("These data were edited with a previous release\n"
                  "of UltraScan (version " + version + "), which is not\n"
                  "binary compatible with the current version (" + US_Version + ").\n\n"
                  "Please re-edit the experimental data before\n"
                  "using the data for data analysis.");
         emit newMessage(str, -3);
         cerr << str << endl;
         f.close();
         return (-3); // wrong version
      }
      //cerr << "Data being read in:\n";
      ds >> (*run_inf).data_dir;
      //cerr << "(*run_inf).data_dir: " << (*run_inf).data_dir << endl;
      ds >> (*run_inf).run_id;
      //cerr << "(*run_inf).run_id: " << (*run_inf).run_id << endl;
      ds >> (*run_inf).duration;
      //cerr << "(*run_inf).duration: " << (*run_inf).duration << endl;
      ds >> (*run_inf).total_scans;
      //cerr << "(*run_inf).total_scans: " << (*run_inf).total_scans << endl;
      ds >> (*run_inf).delta_r;
      //cerr << "(*run_inf).delta_r: " << (*run_inf).delta_r << endl;
      ds >> (*run_inf).expdata_id;
      ds >> (*run_inf).investigator;
      ds >> (*run_inf).date;
      ds >> (*run_inf).description;
      ds >> (*run_inf).dbname;
      ds >> (*run_inf).dbhost;
      ds >> (*run_inf).dbdriver;
      ds >> et;
      (*run_inf).exp_type.velocity = (bool) et;
      ds >> et;
      (*run_inf).exp_type.equilibrium = (bool) et;
      ds >> et;
      (*run_inf).exp_type.diffusion = (bool) et;
      ds >> et;
      (*run_inf).exp_type.simulation = (bool) et;
      ds >> et;
      (*run_inf).exp_type.interference = (bool) et;
      ds >> et;
      (*run_inf).exp_type.absorbance = (bool) et;
      ds >> et;
      (*run_inf).exp_type.fluorescence = (bool) et;
      ds >> et;
      (*run_inf).exp_type.intensity = (bool) et;
      ds >> et;
      (*run_inf).exp_type.wavelength = (bool) et;
      for (i=0; i<8; i++)
      {
         ds >> (*run_inf).centerpiece[i];
         //cerr << "(*run_inf).centerpiece[" << i << "]: " << (*run_inf).centerpiece[i] << endl;
         ds >> (*run_inf).cell_id[i];
         //cerr << "(*run_inf).cell_id[" << i << "]: " << (*run_inf).cell_id[i] << endl;
         ds >> (*run_inf).wavelength_count[i];
         //cerr << "(*run_inf).wavelength_count[" << i << "]: " << (*run_inf).wavelength_count[i] << endl;
      }
      for (i=0; i<8; i++)
      {
         if (version.toFloat() < 7.0)
         {
            for (j=0; j<4; j++)   // one for each channel
            {
               ds >> (*run_inf).DNA_serialnumber[i][j][0];
               ds >> (*run_inf).buffer_serialnumber[i][j];
               ds >> (*run_inf).peptide_serialnumber[i][j][0];
            }
         }
         else
         {
            for (j=0; j<4; j++)
            {
               ds >> (*run_inf).buffer_serialnumber[i][j];
               for(int k=0; k<3; k++)
               {
                  ds >> (*run_inf).peptide_serialnumber[i][j][k];
                  ds >> (*run_inf).DNA_serialnumber[i][j][k];
               }
            }
         }
         for (j=0; j<3; j++)
         {
            ds >> (*run_inf).wavelength[i][j];
            //cerr << "(*run_inf).wavelength[" << i << "][" << j << "]: " << (*run_inf).wavelength[i][j] << endl;
            ds >> (*run_inf).scans[i][j];
            //cerr << "(*run_inf).scans[" << i << "][" << j << "]: " << (*run_inf).scans[i][j] << endl;
            if ((*run_inf).centerpiece[i] >= 0 && (*run_inf).scans[i][j] > 0)
            {
               //cerr << "Channels: " << (*cp_list)[(*run_inf).centerpiece[i]].channels << endl;
               for (k=0; k<(*cp_list)[(*run_inf).centerpiece[i]].channels; k++)
               {
                  ds >> (*run_inf).range_left[i][j][k];
                  //cerr << "(*run_inf).range_left[" << i << "][" << j << "][" << k << "]: " << (*run_inf).range_left[i][j][k] << endl;
                  ds >> (*run_inf).range_right[i][j][k];
                  //cerr << "(*run_inf).range_right[" << i << "][" << j << "][" << k << "]: " << (*run_inf).range_right[i][j][k] << endl;
                  ds >> (*run_inf).points[i][j][k];
                  //cerr << "(*run_inf).range_right[" << i << "][" << j << "][" << k << "]: " << (*run_inf).range_right[i][j][k] << endl;
                  ds >> (*run_inf).point_density[i][j][k];
                  //cerr << "(*run_inf).point_density[" << i << "][" << j << "][" << k << "]: " << (*run_inf).point_density[i][j][k] << endl;
               }
            }
         }
      }
      (*run_inf).temperature = new float** [8];
      (*run_inf).rpm = new unsigned int** [8];
      (*run_inf).time      = new unsigned int** [8];
      (*run_inf).omega_s_t   = new float** [8];
      for (i=0; i<8; i++)
      {
         (*run_inf).temperature[i] = new float* [(*run_inf).wavelength_count[i]];
         (*run_inf).rpm[i] = new unsigned int* [(*run_inf).wavelength_count[i]];
         (*run_inf).time[i]      = new unsigned int* [(*run_inf).wavelength_count[i]];
         (*run_inf).omega_s_t[i]   = new float* [(*run_inf).wavelength_count[i]];
         for (j=0; j<(*run_inf).wavelength_count[i]; j++)
         {
            (*run_inf).temperature[i][j] = new float [(*run_inf).scans[i][j]];
            (*run_inf).rpm[i][j] = new unsigned int [(*run_inf).scans[i][j]];
            (*run_inf).time[i][j]      = new unsigned int [(*run_inf).scans[i][j]];
            (*run_inf).omega_s_t[i][j]   = new float [(*run_inf).scans[i][j]];
         }
      }
      for (i=0; i<8; i++)
      {
         //cerr << "wavelength_count[" << i << "]: " << (*run_inf).wavelength_count[i] << endl;
         for (j=0; j<(*run_inf).wavelength_count[i]; j++)
         {
            //cerr << "scans[" << i << "][" << j << "]: " << (*run_inf).scans[i][j] << endl;
            for (k=0; k<(*run_inf).scans[i][j]; k++)
            {
               ds >> (*run_inf).rpm[i][j][k];
               //cerr << "rpm[" << i << "][" << j << "][" << k << "]: " << (*run_inf).rpm[i][j][k] << endl;
               ds >> (*run_inf).temperature[i][j][k];
               //cerr << "temperature[" << i << "][" << j << "][" << k << "]: " << (*run_inf).temperature[i][j][k] << endl;
               ds >> (*run_inf).time[i][j][k];
               //cerr << "time[" << i << "][" << j << "][" << k << "]: " << (*run_inf).time[i][j][k] << endl;
               ds >> (*run_inf).omega_s_t[i][j][k];
               //cerr << "omega[" << i << "][" << j << "][" << k << "]: " << (*run_inf).omega_s_t[i][j][k] << endl;
            }
         }
      }
      for (i=0; i<8; i++)
      {
         has_data[i]=false;
         if ((*run_inf).scans[i][0] != 0)
         {
            has_data[i] = true;
         }
      }
      ds >> (*run_inf).rotor;
      f.close();
   }
   // now check to make sure the databases match and update if the old DB name was used
   // if the database doesn't match our configured database we need to first check if 
   // the database name is old and needs to be updated. If it is old, we also need to 
   // check if the new name matches the default db. If it does, we need to re-write 
   // the file with the new database name updated
   bool db_used = false;
   for(i=0; i<8; i++)
   {
      for(j=0; j<4; j++)
      {
         if((*run_inf).buffer_serialnumber[i][j] != -1)
         {
            db_used = true;
         }
      }
   }
   if (db_used)
   {
      QString default_db;
      // if we load data from us_gridcontrol_t we don't need to do the database check
      // since the database name comes from php in the selected LIMS instance.
      if (gridcontrol_flag)  return(0);
      // check to see if the dbname matches the default database
      if(!check_dbname((*run_inf).dbname, &default_db))  
      {
         // if it doesn't match, check if it is an old database name     
         for (i=0; i<57; i++)
         {
            // if it is an old name, we need to check if the corresponding new name 
            // matches the default database      
            if ((*run_inf).dbname == limsold[i])
            {
cout << "default: " << default_db << ", limsold: " << limsold[i] << ", dbname: " << (*run_inf).dbname << ", limsnew: " << limsnew[i] << endl;
               // if the corresponding new name matches, we update the database name and
               // write the new name to the data file and break out of the loop     
               if(check_dbname(limsnew[i], &default_db))
               {
                  (*run_inf).dbname = limsnew[i];
                  int write_error = write_run(fn, run_type, has_data, cp_list);
                  cout << "write error: " << write_error << endl;
                  if (write_error < 0) return (write_error);
                  break;
               }
               // else we will signal an error
               else
               {
                  return(-4); // database doesn't match     
               }
            }
         }
      }
   }
   return(0);
}

int US_Data_IO::write_run(QString fn, int run_type, bool *has_data,
                         vector <struct centerpieceInfo> *cp_list)
{
   unsigned int i, j, k;     
   cout << "runtype: " << run_type << endl;
   if (fn.isEmpty())
   {
      return (-22);
   }
   if (   run_type ==   1 || 
          run_type ==   3 ||
          run_type ==   5 ||
          run_type ==   7 ||
          run_type ==   9 ||
          run_type ==  11 ||
          run_type ==  13 ||
          run_type ==  17 ||
          run_type ==  19 ||
          run_type ==  21 ||
          run_type ==  31 ||
          run_type == 101 )
   {
      QFile f(fn);
      f.remove();
      if (!f.open(IO_WriteOnly))
      {
         QString str = tr("UltraScan encountered a Problem when trying to open the edited data file.\n\n"
                          "Please make sure that the file: \"")  + fn +
            tr("\"\ncan be written to your results directory.\n\n"
               "Current Results Directory: \"")
            + USglobal->config_list.result_dir + "\"";
         cout << str << endl;
         return(-20);   // -20 error code = file can't be written
      }
      QDataStream ts (&f);
      ts << US_Version;
      ts << (*run_inf).data_dir;
      ts << (*run_inf).run_id;
      ts << (*run_inf).avg_temperature;
      ts << (*run_inf).temperature_check;
      ts << (*run_inf).time_correction;
      ts << (*run_inf).duration;
      ts << (*run_inf).total_scans;
      ts << (*run_inf).delta_r;
      ts << (*run_inf).expdata_id;
      ts << (*run_inf).investigator;
      ts << (*run_inf).date;
      ts << (*run_inf).description;
      ts << (*run_inf).dbname;
      ts << (*run_inf).dbhost;
      ts << (*run_inf).dbdriver;
      ts << (int)(*run_inf).exp_type.velocity;
      ts << (int)(*run_inf).exp_type.equilibrium;
      ts << (int)(*run_inf).exp_type.diffusion;
      ts << (int)(*run_inf).exp_type.simulation;
      ts << (int)(*run_inf).exp_type.interference;
      ts << (int)(*run_inf).exp_type.absorbance;
      ts << (int)(*run_inf).exp_type.fluorescence;
      ts << (int)(*run_inf).exp_type.intensity;
      ts << (int)(*run_inf).exp_type.wavelength;
      for (i=0; i<8; i++)
      {
         ts << (*run_inf).centerpiece[i];
         ts << (*run_inf).meniscus[i];
         ts << (*run_inf).cell_id[i];
         ts << (*run_inf).wavelength_count[i];
      }
      for (i=0; i<8; i++)
      {
         for (j=0; j<4; j++)   // one for each channel
         {
            ts << (*run_inf).buffer_serialnumber[i][j];
            for(int k=0; k<3;k++)
            {
               ts << (*run_inf).peptide_serialnumber[i][j][k];
               ts << (*run_inf).DNA_serialnumber[i][j][k];
            }
         }
         for (j=0; j<3; j++)
         {
            ts << (*run_inf).wavelength[i][j];
            ts << (*run_inf).scans[i][j];
            ts << (*run_inf).baseline[i][j];
            if ((*run_inf).centerpiece[i] >= 0)
            {
               for (k=0; k<(*cp_list)[(*run_inf).centerpiece[i]].channels; k++)
               {
                  ts << (*run_inf).range_left[i][j][k];
                  ts << (*run_inf).range_right[i][j][k];
                  ts << (*run_inf).points[i][j][k];
                  ts << (*run_inf).point_density[i][j][k];
               }
            }
         }
      }
      for (i=0; i<8; i++)
      {
         for (j=0; j<(*run_inf).wavelength_count[i]; j++)
         {
            for (k=0; k<(*run_inf).scans[i][j]; k++)
            {
               ts << (*run_inf).rpm[i][j][k];
               ts << (*run_inf).temperature[i][j][k];
               // add the time correction back on before writing the data back to disk:
               (*run_inf).time[i][j][k] += (unsigned int) ((*run_inf).time_correction + 0.5);
               ts << (*run_inf).time[i][j][k];
               ts << (*run_inf).omega_s_t[i][j][k];
               //cout << "cell: " << i + 1 << ", lambda: " << j+1 << ", channel: " << k +1 << ": " << (*run_inf).plateau[i][j][k] << endl;;
               ts << (*run_inf).plateau[i][j][k];
               //               cout << "writing plateau: [i=" << i << "][j=" << j << "][k=" << k << "]: " << temp_run.plateau[i][j][k] << endl;
            }
         }
      }
      ts << (*run_inf).rotor;
   }
   if (   run_type ==  2 || 
         run_type ==  4 ||
         run_type ==  6 ||
         run_type ==  8 ||
         run_type == 10 ||
         run_type == 12 ||
         run_type == 14 ||
         run_type == 32 )
   {
      QFile f(fn);
      f.open(IO_WriteOnly);
      QDataStream ts (&f);
      ts << US_Version;
      ts << (*run_inf).data_dir;
      ts << (*run_inf).run_id;
      ts << (*run_inf).duration;
      ts << (*run_inf).total_scans;
      ts << (*run_inf).delta_r;
      ts << (*run_inf).expdata_id;
      ts << (*run_inf).investigator;
      ts << (*run_inf).date;
      ts << (*run_inf).description;
      ts << (*run_inf).dbname;
      ts << (*run_inf).dbhost;
      ts << (*run_inf).dbdriver;
      ts << (int)(*run_inf).exp_type.velocity;
      ts << (int)(*run_inf).exp_type.equilibrium;
      ts << (int)(*run_inf).exp_type.diffusion;
      ts << (int)(*run_inf).exp_type.simulation;
      ts << (int)(*run_inf).exp_type.interference;
      ts << (int)(*run_inf).exp_type.absorbance;
      ts << (int)(*run_inf).exp_type.fluorescence;
      ts << (int)(*run_inf).exp_type.intensity;
      ts << (int)(*run_inf).exp_type.wavelength;
      for (i=0; i<8; i++)
      {
         ts << (*run_inf).centerpiece[i];
         ts << (*run_inf).cell_id[i];
         ts << (*run_inf).wavelength_count[i];
      }
      for (i=0; i<8; i++)
      {
         for (j=0; j<4; j++)   // one for each channel
         {
            ts << (*run_inf).buffer_serialnumber[i][j];
            for(int k=0; k<3;k++)
            {
               ts << (*run_inf).peptide_serialnumber[i][j][k];
               ts << (*run_inf).DNA_serialnumber[i][j][k];
            }
         }
         for (j=0; j<3; j++)
         {
            ts << (*run_inf).wavelength[i][j];
            ts << (*run_inf).scans[i][j];
            if ((*run_inf).centerpiece[i] >= 0  && (*run_inf).scans[i][j] != 0)
            {
               for (k=0; k<(*cp_list)[(*run_inf).centerpiece[i]].channels; k++)
               {
                  ts << (*run_inf).range_left[i][j][k];
                  ts << (*run_inf).range_right[i][j][k];
                  ts << (*run_inf).points[i][j][k];
                  ts << (*run_inf).point_density[i][j][k];
               }
            }
         }
      }
      for (i=0; i<8; i++)
      {
         for (j=0; j<(*run_inf).wavelength_count[i]; j++)
         {
            for (k=0; k<(*run_inf).scans[i][j]; k++)
            {
               ts << (*run_inf).rpm[i][j][k];
               ts << (*run_inf).temperature[i][j][k];
               ts << (*run_inf).time[i][j][k];
               ts << (*run_inf).omega_s_t[i][j][k];
            }
         }
      }
      for (i=0; i<8; i++)
      {
         has_data[i]=false;
         if ((*run_inf).scans[i][0] != 0)
         {
            has_data[i] = true;
         }
      }
      ts << (*run_inf).rotor;
      f.close();
   }
   return(0);
}


int US_Data_IO::load_veloc_scan(struct channelData *data,
                                unsigned int selected_cell, unsigned int selected_lambda,
                                unsigned int selected_channel)
{
   QString str, filename;
   float val;
   unsigned int scan, i, points;
   points = (*run_inf).points[selected_cell][selected_lambda][selected_channel];
   str = USglobal->config_list.result_dir + "/" + (*run_inf).run_id;
   filename.sprintf(str + ".veloc.%d%d", selected_cell+1, selected_lambda+1);
   QFile f(filename);
   if (!f.open(IO_ReadOnly))
   {
      str = tr("Sorry, a requested file is missing:\n\n") + filename +
         tr("\n\nPlease restore the file, then try again.");
      cerr << str << endl;
      emit newMessage(str, -1);
      return (-1);
   }
   (*data).absorbance.clear();
   (*data).radius.clear();
   (*data).absorbance.resize((*run_inf).scans[selected_cell][selected_lambda]);
   (*data).radius.resize(points);
   QDataStream ds(&f);
   for(scan=0; scan<(*run_inf).scans[selected_cell][selected_lambda]; scan++)
   {
      for (i=0; i<points; i++)
      {
         ds >> val;
         (*data).absorbance[scan].push_back(val);
      }
      if (baseline_flag)
      {
         for (i=0; i<points; i++)
         {
            (*data).absorbance[scan][i] -= (*run_inf).baseline[selected_cell][selected_lambda];
         }
      }
   }
   for (i=0; i<points; i++)
   {
      (*data).radius[i] = (*run_inf).range_left[selected_cell][selected_lambda][0]
         + i * (*run_inf).delta_r;
   }
   f.close();
   return(0);
}

int US_Data_IO::load_hydrodynamics(struct hydrodynamicData *hydro_inf)
{
   bool db_used = false;
   struct US_DatabaseLogin db_login;
   struct BufferData Buffer;
   QString str;
   unsigned int i, j, k;
   for(i=0; i<8; i++)
   {
      for(j=0; j<4; j++)
      {
         if((*run_inf).buffer_serialnumber[i][j] != -1)
         {
            db_used = true;
         }
      }
   }
   if (db_used)
   {
      QString default_db;
      if(!check_dbname((*run_inf).dbname, &default_db))
      {
         str = tr("The current dataset is obtained from the " + (*run_inf).dbname + " database.\n"
                  "Your default database is set to the " + default_db + " database.\n"
                  "Please change your default database to the " + (*run_inf).dbname + " database\n"
                  "before analyzing this dataset!\n\n(File:Configuration:Database Preferences:Change)\n\n");
         emit newMessage(str, -1);
         cerr << str << endl;
         cerr << "The data loading function exited with -1" << endl;
         return(-1);
      }
      if (BUFFER->db_connect() < 0)
      {
         str = tr("Could not connect to the currently defined " + (*run_inf).dbname + " database.\n"
                  "Please check your login information for the " + (*run_inf).dbname + " database\n"
                  "before analyzing this dataset! (File:Configuration:Database Preferences:Change)");
         emit newMessage(str, -2);
         cerr << str << endl;
         cerr << "The data loading function exited with -2" << endl;
         return (-2);
      }
   }
   for(i=0; i<8; i++)
   {
      for(j=0; j<4; j++)
      {
         if((*run_inf).buffer_serialnumber[i][j] > 0)
         {
            Buffer = BUFFER->export_buffer((*run_inf).buffer_serialnumber[i][j]);
            (*hydro_inf).Density[i][j] = Buffer.density;
            (*hydro_inf).Viscosity[i][j] = Buffer.viscosity;
         }
         else
         {
            (*hydro_inf).Density[i][j] = (float) DENS_20W;
            (*hydro_inf).Viscosity[i][j] = (float) (100.0 * VISC_20W);
         }
         for(k=0; k<3; k++)
         {
            //            cout << "Cell " << i << ", channel " << j<< ", item " << k << ", Peptide: " << (*run_inf).peptide_serialnumber[i][j][k] << ", DNA: " << (*run_inf).DNA_serialnumber[i][j][k] << endl;
            if((*run_inf).peptide_serialnumber[i][j][k] > 0)
            {
               Vbar_info = VBAR->export_vbar((*run_inf).peptide_serialnumber[i][j][k]);
               (*hydro_inf).Vbar[i][j][k] = Vbar_info.vbar;
               (*hydro_inf).Vbar20[i][j][k] = Vbar_info.vbar20;
            }
            else if((*run_inf).DNA_serialnumber[i][j][k] > 0)
            {
               Vbar_info = VBAR->export_DNA_vbar((*run_inf).DNA_serialnumber[i][j][k]);
               if (Vbar_info.vbar == 0.0 || Vbar_info.vbar20 == 0.0)
               {
                  //                  cout << "its dna\n";
                  Vbar_info.vbar   = (float) 0.55;
                  Vbar_info.vbar20 = (float) 0.55;
               }
               (*hydro_inf).Vbar[i][j][k] = Vbar_info.vbar;
               (*hydro_inf).Vbar20[i][j][k] = Vbar_info.vbar20;
               //cout << "in us_data_io: vbar:" << (*hydro_inf).Vbar[i][j][k] << ", vbar20: " << Vbar_info.vbar20 << endl;
            }
            else
            {
               (*hydro_inf).Vbar20[i][j][k] = (float) 0.72;
               (*hydro_inf).Vbar[i][j][k] = (*hydro_inf).Vbar20[i][j][k] + (4.25e-4 * ((*run_inf).avg_temperature - 20));
            }
            //            cout << "Cell " << i << ", channel " << j<< ", item " << k << ", Peptide: " << (*run_inf).peptide_serialnumber[i][j][k] << ", DNA: " << (*run_inf).DNA_serialnumber[i][j][k] << ", in us_data_io: vbar:" << (*hydro_inf).Vbar[i][j][k] << ", vbar20: " << (*hydro_inf).Vbar20[i][j][k] << endl;
         }
      }
   }
   BUFFER->close_db();
   return(0);
}

int US_Data_IO::load_hydrodynamics(struct US_DatabaseLogin db_login, struct hydrodynamicData *hydro_inf)
{
   bool db_used = false;
   unsigned int i, j, k;
   struct BufferData Buffer;
   QString str;
   for(i=0; i<8; i++)
   {
      for(j=0; j<4; j++)
      {
         if((*run_inf).buffer_serialnumber[i][j] != -1)
         {
            db_used = true;
         }
      }
   }
   if (db_used)
   {
      cerr << "loading hydrodynamics from the database using db_login information...\n";
      if (db_login.user.isEmpty() ||
          db_login.driver.isEmpty() ||
          db_login.database.isEmpty() ||
          db_login.password.isEmpty() ||
          db_login.host.isEmpty())
      {
         if (BUFFER->db_connect() < 0)
         {
            str = tr("Could not connect to the currently defined " + (*run_inf).dbname + " database.\n"
                     "Please check your login information for the " + (*run_inf).dbname + " database\n"
                     "before analyzing this dataset! (File:Configuration:Database Preferences:Change)");
            emit newMessage(str, -2);
            cerr << str << endl;
            cerr << "The data loading function exited with -2" << endl;
            return (-2);
         }
      }
      if (BUFFER->db_connect(db_login) < 0)
      {
         str = tr("Could not connect to the currently defined " + (*run_inf).dbname + " database.\n"
                  "Please check your login information for the " + (*run_inf).dbname + " database\n"
                  "before analyzing this dataset! (File:Configuration:Database Preferences:Change)");
         emit newMessage(str, -3);
         cerr << str << endl;
         cerr << "The data loading function exited with -3" << endl;
         return (-3);
      }
   }
   for(i=0; i<8; i++)
   {
      for(j=0; j<4; j++)
      {
         if((*run_inf).buffer_serialnumber[i][j] > 0)
         {
            Buffer = BUFFER->export_buffer((*run_inf).buffer_serialnumber[i][j]);
            (*hydro_inf).Density[i][j] = Buffer.density;
            (*hydro_inf).Viscosity[i][j] = Buffer.viscosity;
         }
         else
         {
            (*hydro_inf).Density  [i][j] = (float) DENS_20W;
            (*hydro_inf).Viscosity[i][j] = (float) (100.0 * VISC_20W);
         }
         for(k=0; k<3; k++)
         {
            if((*run_inf).peptide_serialnumber[i][j][k] > 0)
            {
               Vbar_info = VBAR->export_vbar((*run_inf).peptide_serialnumber[i][j][k]);
               (*hydro_inf).Vbar[i][j][k] = Vbar_info.vbar;
               (*hydro_inf).Vbar20[i][j][k] = Vbar_info.vbar20;
            }
            else if((*run_inf).DNA_serialnumber[i][j][k] > 0)
            {
               Vbar_info = VBAR->export_DNA_vbar((*run_inf).DNA_serialnumber[i][j][k]);
               if (Vbar_info.vbar == 0.0 || Vbar_info.vbar20 == 0.0)
               {
                  Vbar_info.vbar   = (float) 0.55;
                  Vbar_info.vbar20 = (float) 0.55;
               }
               (*hydro_inf).Vbar[i][j][k] = Vbar_info.vbar;
               (*hydro_inf).Vbar20[i][j][k] = Vbar_info.vbar20;
            }
            else
            {
               (*hydro_inf).Vbar20[i][j][k] = (float) 0.72;
               (*hydro_inf).Vbar[i][j][k] = (*hydro_inf).Vbar20[i][j][k] + (4.25e-4 * ((*run_inf).avg_temperature - 20));
            }
         }
      }
   }
   BUFFER->close_db();
   return(0);
}

void US_Data_IO::calc_correction(struct correctionTerms *corr_inf)
{
   float t = (*corr_inf).temperature;
   (*corr_inf).density_wt    = 1.000028e-3 * ((999.83952 + 16.945176 * t) / (1 + 16.879850e-3 * t))
      - 1.000028e-3 * ((7.9870401e-3 * pow((double) t, (double) 2.0) + 46.170461e-6 * pow((double) t, (double) 3.0))
                       / (1.0 + 16.87985e-3 * t))
      + 1.000028e-3 * ((105.56302e-9 * pow((double) t, (double) 4.0) - 280.54253e-12 * pow((double) t, (double) 5.0))
                       / (1.0 + 16.87985e-3 * t));
   (*corr_inf).density_wt_20    = (float) DENS_20W;

   if (t < 20.0)
   {
      (*corr_inf).viscosity_wt = 100.0 * pow((double) 10.0, (double) (1301 / (998.333 + 8.1855 * (t - 20.0)
                                                                              + 5.85e-3 * pow((double) (t-20.0), (double) 2.0)) - 3.302));
   }
   else
   {
      (*corr_inf).viscosity_wt = (100.0 * VISC_20W) * pow((double) 10.0, (double) ((1.3272 * (20.0 - t) - 1.053e-3 * pow((double) (t - 20.0), (double) 2.0))
                                                                                   / (t + 105.0)));
   }

   (*corr_inf).density_tb   = (*corr_inf).density * (*corr_inf).density_wt / (*corr_inf).density_wt_20;
   (*corr_inf).viscosity_tb = (*corr_inf).viscosity * (*corr_inf).viscosity_wt / (100.0 * VISC_20W);
   (*corr_inf).buoyancyb    = 1.0 - (*corr_inf).vbar * (*corr_inf).density_tb;
   (*corr_inf).buoyancyw    = 1.0 - (*corr_inf).vbar20 * (*corr_inf).density_wt_20;
   (*corr_inf).correction   = ((*corr_inf).buoyancyw / (*corr_inf).buoyancyb) * ((*corr_inf).viscosity_tb / (100.0 * VISC_20W));

   /*
     cerr << "in data_IO: densitytb:" <<   (*corr_inf).density_tb
     << ", temperature: " << t
     << ", viscosity tb: " << (*corr_inf).viscosity_tb
     << ", viscosity: " << (*corr_inf).viscosity
     << ", viscosity_wt: " << (*corr_inf).viscosity_wt
     << ", buoyancyb: " << (*corr_inf).buoyancyb
     << ", buoyancyw: " << (*corr_inf).buoyancyw
     << ", correction: " << (*corr_inf).correction << endl;
   */

}

bool check_dbname(QString dbname, QString *defaultdb)
{
   QString temp_dbname;
   bool flag = false;

   QString dbfile = US_Config::get_home_dir() + USDB;

   QFile f( dbfile );

   if ( f.exists() )
   {
      f.open( IO_ReadOnly );
      QDataStream ds( &f );

      double num[64];
      int code[64];
      int i,length;

      ds >> length;

      for(i=0; i<length; i++)
      {
         ds >> num[i];
         code[i] = (int)(num[i]+0.5);
         temp_dbname[i] = QChar(code[i]);
      }

      if(temp_dbname == dbname)
      {
         flag = true;
      }

      *defaultdb = temp_dbname;
   }
   return (flag);
}
