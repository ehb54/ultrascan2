#include "../include/us_astfem_rsa.h"
#include "../include/us_util.h"
#include <algorithm>

#if defined(USE_US_TIMER)
#  include "../include/us_timer.h"
   US_Timer us_timers;
#endif

#undef DEBUG

// #define DEBUG_ALLOC
// #define DEBUG_RSS
// #define DEBUG
// #define DEBUG_THREAD

#if defined(DEBUG_ALLOC)
void init_matrices_alloc();                   // initializes matrix alloc array
void list_matrices_alloc();                   // lists matrix alloc array
#endif

#if defined(DEBUG_RSS)
extern long getrss(int pid);

#if !defined(USE_MPI)
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define DEV_ENCODE(M,m) ( \
               ( (M&0xfff) << 8) | ( (m&0xfff00) << 12) | (m&0xff) \
)

#include <asm/param.h> /* HZ */
#include <asm/page.h> /* PAGE_SIZE */
#define NO_TTY_VALUE DEV_ENCODE(0,0)
#ifndef HZ
#warning HZ not defined, assuming it is 100
#define HZ 100
#endif

#ifndef PAGE_SIZE
#warning PAGE_SIZE not defined, assuming it is 4096
#define PAGE_SIZE 4096
#endif

long getrss(int pid)
{
   if (!pid)
   {
      pid = getpid();
   }
   char P_tty_text[16];
   char P_cmd[16];
   char P_state;
   int P_euid;
   int P_pid;
   int P_ppid, P_pgrp, P_session, P_tty_num, P_tpgid;
   unsigned long P_flags, P_min_flt, P_cmin_flt, P_maj_flt, P_cmaj_flt, P_utime, P_stime;
   long P_cutime, P_cstime, P_priority, P_nice, P_timeout, P_alarm;
   unsigned long P_start_time, P_vsize;
   long P_rss;
   unsigned long P_rss_rlim, P_start_code, P_end_code, P_start_stack, P_kstk_esp, P_kstk_eip;
   unsigned P_signal, P_blocked, P_sigignore, P_sigcatch;
   unsigned long P_wchan, P_nswap, P_cnswap;
   char buf[800]; /* about 40 fields, 64-bit decimal is about 20 chars */
   int num;
   int fd;
   char* tmp;
   struct stat sb; /* stat() used to get EUID */
   snprintf(buf, 32, "/proc/%d/stat", pid);
   if ( (fd = open(buf, O_RDONLY, 0) ) == -1 )
      return 0;
   num = read(fd, buf, sizeof buf - 1);
   fstat(fd, &sb);
   P_euid = sb.st_uid;
   close(fd);
   if (num<80)
      return 0;
   buf[num] = '\0';
   tmp = strrchr(buf, ')');   /* split into "PID (cmd" and "<rest>" */
   *tmp = '\0';            /* replace trailing ')' with NUL */
                           /* parse these two strings separately, skipping the leading "(". */
   memset(P_cmd, 0, sizeof P_cmd);      /* clear */
   sscanf(buf, "%d (%15c", &P_pid, P_cmd); /* comm[16] in kernel */
   num = sscanf(tmp + 2,               /* skip space after ')' too */
                "%c "
                "%d %d %d %d %d "
                "%lu %lu %lu %lu %lu %lu %lu "
                "%ld %ld %ld %ld %ld %ld "
                "%lu %lu "
                "%ld "
                "%lu %lu %lu %lu %lu %lu "
                "%u %u %u %u " /* no use for RT signals */
                "%lu %lu %lu",
                &P_state,
                &P_ppid, &P_pgrp, &P_session, &P_tty_num, &P_tpgid,
                &P_flags, &P_min_flt, &P_cmin_flt, &P_maj_flt, &P_cmaj_flt, &P_utime, &P_stime,
                &P_cutime, &P_cstime, &P_priority, &P_nice, &P_timeout, &P_alarm,
                &P_start_time, &P_vsize,
                &P_rss,
                &P_rss_rlim, &P_start_code, &P_end_code, &P_start_stack, &P_kstk_esp, &P_kstk_eip,
                &P_signal, &P_blocked, &P_sigignore, &P_sigcatch,
                &P_wchan, &P_nswap, &P_cnswap
                );
   /*   fprintf(stderr, "stat2proc converted %d fields.\n",num); */
   P_vsize /= 1024;
   P_rss *= (PAGE_SIZE/1024);

   memcpy(P_tty_text, " ? ", 8);
   if (P_tty_num != NO_TTY_VALUE)
   {
      int tty_maj = (P_tty_num>>8)&0xfff;
      int tty_min = (P_tty_num&0xff) | ((P_tty_num>>12)&0xfff00);
      snprintf(P_tty_text, sizeof P_tty_text, "%3d,%-3d", tty_maj, tty_min);
   }

   if (num < 30)
      return 0;
   if (P_pid != pid)
      return 0;
   return P_rss;
}
#endif

static long last_rss = 0;
static long deltasum = 0;

static void dm(char *s) {
   deltasum += getrss(0) - last_rss;
   printf("%s: rss %lu delta %ld sum %ld\n", s, (unsigned long)getrss(0), (getrss(0) - last_rss), deltasum); fflush(stdout);
   last_rss = getrss(0);
}

#endif

US_Astfem_RSA::US_Astfem_RSA(bool guiFlag, QObject *parent, const char *name) : QObject(parent, name)
{
   this->guiFlag = guiFlag;
   stopFlag = false;
   movieFlag = false;
   use_time = false;
   time_correction = true;
#if defined(DEBUG_ALLOC)
   init_matrices_alloc();
#endif
}

US_Astfem_RSA::~US_Astfem_RSA()
{
}

int US_Astfem_RSA::calculate(struct ModelSystem *system, 
                             struct SimulationParameters *simparams,
                             vector < mfem_data > *exp_data, 
                             int *progress_pos,
                             int thread,
                             vector < rotorInfo > *rotor_list)
{
#if defined(USE_US_TIMER)
   us_timers.start_timer( "astfem rsa" );
#endif
   this->thread = thread;
   vector < rotorInfo > alt_rotor_list;
   if ( !rotor_list ) 
   {
      printf("reading rotor list\n");
      readRotorInfo(&alt_rotor_list);
      rotor_list = &alt_rotor_list;
   }
   this->rotor_list = rotor_list;
#if defined(DEBUG)
   US_FemGlobal fg;
   fg.write_experiment(system, simparams, "astfem_rsa-output");
#endif

#if defined(DEBUG_RSS)
   deltasum = 0;
   dm("start us_astfem_rsa::calculate");
   guiFlag = false;
   // return 0;
#endif

   this->simparams = simparams;
   this->system = system;
   unsigned int duration, initial_npts=1000, current_assoc;
   unsigned int i, j, k, m, n, ss;
   float current_time, current_speed;
   double accel_time, dr;
   mfem_data simdata;
   mfem_initial CT0;         // initial total concentration
   mfem_initial *vC0 = NULL;      // initial concentration for multiple components
   vector <bool> reacting;
   reacting.resize( (*system).component_vector.size() );
   af_params.first_speed = (*simparams).speed_step[0].rotorspeed;
   af_params.simpoints = simparams->simpoints;
   update_assocv();
   initialize_rg();
   if (guiFlag)
   {
      print_rg();
   }
   adjust_limits((*simparams).speed_step[0].rotorspeed);
   for (k=0; k<(*system).component_vector.size(); k++)
   {
#if defined(DEBUG_THREAD)
      printf("%d: ASTFEM RSA working on component %d of %d\n", thread,
                   k, (*system).component_vector.size());
#endif
      if ( progress_pos ) 
      {
         *progress_pos = (int) k;
      }
      reacting[k] = false;
      for (j=0; j<(*system).assoc_vector.size(); j++)
      {
         for(n=0; n< (*system).assoc_vector[j].comp.size(); n++)
         {
            if (k == (*system).assoc_vector[j].comp[n])
            {
               reacting[k] = true;
               current_assoc = j;
               break;                  // since a comp appears at most once in an assoc rule
            }
         }
      }
      current_time = 0.0;
      last_time = 0.0;
      current_speed = 0.0;
      w2t_integral = 0.0;
      CT0.radius.clear();
      CT0.concentration.clear();
#if defined(DEBUG)
      printf("k = %u af_params.current_bottom %f\n", k, af_params.current_bottom);
#endif
      dr = (af_params.current_bottom - af_params.current_meniscus)/(initial_npts-1);
      for (j=0; j<initial_npts; j++)
      {
         CT0.radius.push_back(af_params.current_meniscus + j * dr );
         CT0.concentration.push_back(0.0);
      }
      // once time invariant noise has been removed in a band experiment, we can use the
      // first scan of the first speed step of the experiment as the initial concentration
      // of the simulation. The approach will copy the 1st scan concentration vector into
      // each component's c0 vector. NNLS will scale the appropriate concentration for each
      // component. The assumption is made that any potential differentiation of components
      // in the initial scan is minimal compared to any solute flow disturbances at the
      // meniscus. For this approach to work well it is necessary to pick the first data
      // point close to the meniscus and to include the earliest possible scan in the
      // experiment. Also, time invariant noise should be subtracted first.
      if (0 && (*simparams).band_firstScanIsConcentration)
      {
         // DISABLED for now -Emre
         mfem_initial scan1;
         scan1.radius.clear();
         scan1.concentration.clear();
         for (j=0; j<(*exp_data)[0].scan[0].conc.size(); j++)
         {
            scan1.radius.push_back((*exp_data)[0].radius[j]);
            scan1.concentration.push_back((*exp_data)[0].scan[0].conc[j]);
         }
         interpolate_C0(&scan1, &(*system).component_vector[k].c0);
      }
      if (!reacting[k]) // noninteracting
      {
         initialize_conc(k, &CT0, true);
         af_params.s.resize(1);
         af_params.D.resize(1);
         af_params.kext.resize(1);
         af_params.s[0] = (*system).component_vector[k].s;
         af_params.D[0] = (*system).component_vector[k].D;
         af_params.kext[0] = (*system).component_vector[k].extinction;
         for (ss=0; ss<(*simparams).speed_step.size(); ss++)
         {
            adjust_limits((*simparams).speed_step[ss].rotorspeed);
            (*exp_data)[ss].meniscus = af_params.current_meniscus;
            (*exp_data)[ss].bottom = af_params.current_bottom;
#if defined(DEBUG)
            printf("ss %u k = %u af_params.current_bottom %f\n", ss, k, af_params.current_bottom);
#endif
            accel_time = 0.0;
            if((*simparams).speed_step[ss].acceleration_flag) // we need to simulate acceleration
            {// if the speed difference is larger than acceleration rate then we have at least 1 acceleration step
               af_params.time_steps = (unsigned int) fabs((*simparams).speed_step[ss].rotorspeed
                                                          - current_speed)/(*simparams).speed_step[ss].acceleration;
               af_params.dt = 1.0; // each simulation step is 1 second long in the acceleration phase
               af_params.simpoints = 2 * (*simparams).simpoints; // use a fixed grid with refinement at both ends and with twice the number of points
               af_params.start_time = current_time;
#if defined(DEBUG)
               print_af();
#endif

               calculate_ni(current_speed, (*simparams).speed_step[ss].rotorspeed, &CT0, &simdata, true);

               // add the acceleration time:
               accel_time = af_params.dt * af_params.time_steps;
               current_time += accel_time;
               //cout << "Current time: " << current_time << endl;
               if (guiFlag)
               {
                  emit new_time(current_time);
                  qApp->processEvents();
               }
               if (stopFlag)
               {
                  if (guiFlag)
                  {
                     qApp->processEvents();
                  }
#if defined(USE_US_TIMER)
                  us_timers.end_timer( "astfem rsa" );
#endif
                  return(1); // early termination = 1
               }
            }  // end of for acceleration
            duration = (unsigned int) ((*simparams).speed_step[ss].duration_hours * 3600
                                       + (*simparams).speed_step[ss].duration_minutes * 60);

            if(ss == (*simparams).speed_step.size() - 1)
            {
               duration += (unsigned int) (duration * 0.05); // + 5%
            }

            if (accel_time > duration)
            {
               cerr << "Attention: acceleration time exceeds duration - please check initialization\n";
#if defined(USE_US_TIMER)
               us_timers.end_timer( "astfem rsa" );
#endif
               return (-1);
            }
            else
            {
               duration -= (unsigned int) accel_time;
            }
            af_params.omega_s = pow((*simparams).speed_step[ss].rotorspeed * M_PI/30.0, 2.0);
            af_params.dt = log((*exp_data)[ss].bottom/(*exp_data)[ss].meniscus)
               /((af_params.omega_s * fabs((*system).component_vector[k].s)) *((*simparams).simpoints - 1));
            if (af_params.dt > duration)
            {
               af_params.dt = duration;
               af_params.simpoints = 1 + (unsigned int) (log((*exp_data)[ss].bottom/(*exp_data)[ss].meniscus)
                                                         /(af_params.omega_s * fabs((*system).component_vector[k].s) * af_params.dt));
            }
            if (af_params.simpoints > 10000)
            {
               af_params.simpoints = 10000;
            }
            // find out the minimum number of simpoints needed to provide the necessary dt:
            af_params.time_steps = (unsigned int) (1+duration/af_params.dt);
            af_params.start_time = current_time;
#if defined(DEBUG)
            printf("us_astfem_rsa::calc before calculate_no af_params.current_bottom %f (*simparams).bottom %f\n",
                   af_params.current_bottom,
                   simparams->bottom);
#endif
            calculate_ni((*simparams).speed_step[ss].rotorspeed, (*simparams).speed_step[ss].rotorspeed, &CT0, &simdata, false);

            // set the current time to the last scan of this speed step
            current_time = (*simparams).speed_step[ss].duration_hours * 3600
               + (*simparams).speed_step[ss].duration_minutes * 60;
            // interpolate the simulated data onto the experimental time- and radius grid
#if defined(DEBUG)
            printf("us_astfem_rsa::calc before interpolate af_params.current_bottom %f (*simparams).bottom %f\n",
                   af_params.current_bottom,
                   simparams->bottom);
#endif
#if defined(USE_US_TIMER)
            us_timers.start_timer( "interpolate" );
#endif
            interpolate(&(*exp_data)[ss], &simdata, use_time);
#if defined(USE_US_TIMER)
            us_timers.end_timer( "interpolate" );
#endif
            // set the current speed to the constant rotor speed of the current speed step
            current_speed = (*simparams).speed_step[ss].rotorspeed;
            if (guiFlag)
            {
               qApp->processEvents();
            }
            if (stopFlag)
            {
#if defined(USE_US_TIMER)
               us_timers.end_timer( "astfem rsa" );
#endif
               return(1); // early termination = 1
            }
         } // speed step loop
         if (guiFlag)
         {
            emit current_component(k+1);
            qApp->processEvents();
         }
      }
   }

#if defined(DEBUG_RSS)
   //          return 0;
#endif
   // vector <unsigned int> local_index: mapping from global index to local index in each rg.
   af_params.local_index.resize( (*system).component_vector.size() );
   unsigned int num_comp, num_rule, rule;
   double s_max;

   for (unsigned int group=0; group<rg.size(); group++)
   {
      num_comp = rg[group].GroupComponent.size();
      num_rule = rg[group].association.size();
      af_params.rg_index = group;
      af_params.s.resize(num_comp);
      af_params.D.resize(num_comp);
      af_params.kext.resize(num_comp);
      af_params.role.resize(num_comp);
      af_params.association.resize(num_rule);
      for(m=0; m<num_rule; m++)
      {
         af_params.association[m] = (*system).assoc_vector[ rg[group].association[m] ];
      }
      for (j=0; j<num_comp; j++)
      {
         af_params.s[j] = (*system).component_vector[rg[group].GroupComponent[j]].s;
         af_params.D[j] = (*system).component_vector[rg[group].GroupComponent[j]].D;
         af_params.kext[j] = (*system).component_vector[rg[group].GroupComponent[j]].extinction;
         af_params.local_index[ rg[group].GroupComponent[j] ] = j;      // global to local index
         af_params.role[j].comp_index = rg[group].GroupComponent[j];
         af_params.role[j].assoc.clear();
         af_params.role[j].react.clear();
         af_params.role[j].st.clear();
         for(m=0; m<rg[group].association.size(); m++)    // check all assoc rule in this rg
         {
            rule = rg[group].association[m];
            for (n=0; n<(*system).assoc_vector[rule].comp.size(); n++)   // check all comp in rule[m]
            {
               if(af_params.role[j].comp_index == (*system).assoc_vector[rule].comp[n])
               {
                  af_params.role[j].assoc.push_back( m );       // local index for the rule
                  af_params.role[j].react.push_back( (*system).assoc_vector[rule].react[n] );
                  af_params.role[j].st.push_back(    (*system).assoc_vector[rule].stoich[n] );
                  break;
               }
            }
         }
      }
      for(m=0; m<num_rule; m++)
      {
         for (n=0; n<af_params.association[m].comp.size(); n++)
         {
            af_params.association[m].comp[n] = af_params.local_index[  af_params.association[m].comp[n] ];
         }
      }

      /*** print
           for(m=0; m<rg[group].association.size(); m++)    // check all assoc rule in this rg
           {
           rule = rg[group].association[m];
           printf("assoc[%d]:\n", rule);
           for (n=0; n<(*system).assoc_vector[rule].comp.size(); n++)   // check all comp in rule[m]
           {
           printf("comp=%d react=%d st=%d \n", (*system).assoc_vector[rule].comp[n], (*system).assoc_vector[rule].react[n],
           (*system).assoc_vector[rule].stoich[n]);
           }
           }

           for (j=0; j<num_comp; j++)
           {
           printf("comp local_index[%d] = %d\n", af_params.local_index[ rg[group].GroupComponent[j] ], j);
           printf("comp index = [%d] \n", af_params.role[j].comp_index);
           for(m=0; m<af_params.role[j].assoc.size(); m++)
           {
           printf("assoc=[%d] react=[%d] st=[%d]\n", af_params.role[j].assoc[m],
           af_params.role[j].react[m], af_params.role[j].st[m]);
           }
           }
           printf(" end of role\n");
      ***/

      current_time = 0.0;
      current_speed = 0.0;
      w2t_integral = 0.0;
      last_time = 0.0;

#if defined(DEBUG)
      printf("k = %u af_params.current_bottom %f\n", k, af_params.current_bottom);
#endif
      dr = (af_params.current_bottom - af_params.current_meniscus)/(initial_npts-1);
      if (vC0 != NULL) delete [] vC0;
      vC0 = new mfem_initial [ rg[group].GroupComponent.size() ];
      for (j=0; j<rg[group].GroupComponent.size(); j++)
      {
         CT0.radius.clear();
         CT0.concentration.clear();
         for (i=0; i<initial_npts; i++)
         {
            CT0.radius.push_back(af_params.current_meniscus + i * dr );
            CT0.concentration.push_back(0.0);
         }
         initialize_conc(rg[group].GroupComponent[j], &CT0, false);
         vC0[j] = CT0;
      }

      decompose(vC0);

      for (ss=0; ss<(*simparams).speed_step.size(); ss++)
      {
         adjust_limits((*simparams).speed_step[ss].rotorspeed);
         (*exp_data)[ss].meniscus = af_params.current_meniscus;
         (*exp_data)[ss].bottom = af_params.current_bottom;
#if defined(DEBUG)
         printf("ss %u k = %u af_params.current_bottom %f\n", ss, k, af_params.current_bottom);
#endif
         accel_time = 0.0;
         if((*simparams).speed_step[ss].acceleration_flag) // we need to simulate acceleration
         {// if the speed difference is larger than acceleration rate then we have at least 1 acceleration step
            af_params.time_steps = (unsigned int) fabs((*simparams).speed_step[ss].rotorspeed
                                                       - current_speed)/(*simparams).speed_step[ss].acceleration;
            af_params.dt = 1.0; // each simulation step is 1 second long in the acceleration phase
            af_params.simpoints = 2 * (*simparams).simpoints; // use a fixed grid with refinement at both ends and with twice the number of points
            af_params.start_time = current_time;
            calculate_ra2(current_speed, (*simparams).speed_step[ss].rotorspeed, vC0, &simdata, true);

            // add the acceleration time:
            accel_time = af_params.dt * af_params.time_steps;
            current_time += accel_time;
            if (guiFlag)
            {
               emit new_time(current_time);
               qApp->processEvents();
            }
            if (stopFlag)
            {
               if (guiFlag)
               {
                  qApp->processEvents();
               }
#if defined(USE_US_TIMER)
               us_timers.end_timer( "astfem rsa" );
#endif
               return(1); // early termination = 1
            }
         }  // end of for acceleration
         duration = (unsigned int) ((*simparams).speed_step[ss].duration_hours * 3600
                                    + (*simparams).speed_step[ss].duration_minutes * 60);
         if(ss == (*simparams).speed_step.size() - 1)
         {
            duration += (unsigned int) (duration * 0.05); // + 5%
         }
         if (accel_time > duration)
         {
            cerr << "Attention: acceleration time exceeds duration - please check initialization\n";
#if defined(USE_US_TIMER)
            us_timers.end_timer( "astfem rsa" );
#endif
            return (-1);
         }
         else
         {
            duration -= (unsigned int) accel_time;
         }
         s_max = fabs( af_params.s[0] );      // find the largest s
         for (m=1;m<af_params.s.size();m++)
         {
            if( s_max < fabs( af_params.s[m] ) ) s_max = fabs( af_params.s[m] );
         }
         af_params.omega_s = pow((*simparams).speed_step[ss].rotorspeed * M_PI/30.0, 2.0);
         af_params.dt = log((*exp_data)[ss].bottom/(*exp_data)[ss].meniscus)
            /(af_params.omega_s * s_max *((*simparams).simpoints - 1));
         if (af_params.dt > duration)
         {
            af_params.dt = duration;
            af_params.simpoints = 1 + (unsigned int) (log((*exp_data)[ss].bottom/(*exp_data)[ss].meniscus)
                                                      /(af_params.omega_s * s_max * af_params.dt));
         }
         if (af_params.simpoints > 10000)
         {
            af_params.simpoints = 10000;
         }
         // find out the minimum number of simpoints needed to provide the necessary dt:
         af_params.time_steps = (unsigned int) (1+duration/af_params.dt);
         af_params.start_time = current_time;
         calculate_ra2((*simparams).speed_step[ss].rotorspeed, (*simparams).speed_step[ss].rotorspeed, vC0, &simdata, false);
         // set the current time to the last scan of this speed step
         current_time = (*simparams).speed_step[ss].duration_hours * 3600
            + (*simparams).speed_step[ss].duration_minutes * 60;
         // interpolate the simulated data onto the experimental time- and radius grid
#if defined(USE_US_TIMER)
         us_timers.start_timer( "interpolate" );
#endif
         interpolate(&(*exp_data)[ss], &simdata, use_time);
#if defined(USE_US_TIMER)
         us_timers.end_timer( "interpolate" );
#endif
         // set the current speed to the constant rotor speed of the current speed step
         current_speed = (*simparams).speed_step[ss].rotorspeed;
         if (guiFlag)
         {
            qApp->processEvents();
         }
         if (stopFlag)
         {
#if defined(USE_US_TIMER)
            us_timers.end_timer( "astfem rsa" );
#endif
            return(1); // early termination = 1
         }
      } // speed step loop
      if (guiFlag)
      {
         emit current_component(k+1);
         qApp->processEvents();
      }
   }
   if(time_correction)
   {
      for (ss=0; ss<(*simparams).speed_step.size(); ss++) // check each speed step to see if it contains acceleration
      {
         if((*simparams).speed_step[ss].acceleration_flag) // we need to correct time
         {
            float *xtmp, *ytmp, slope, intercept, correlation, sigma, correction;
            xtmp = new float [(*simparams).speed_step[ss].scans];
            ytmp = new float [(*simparams).speed_step[ss].scans];
            for (i=0; i<(*simparams).speed_step[ss].scans; i++) // only fit the scans that belong to this speed step
            {
               xtmp[i] = (*exp_data)[ss].scan[i].time;
               ytmp[i]    = (*exp_data)[ss].scan[i].omega_s_t;
            }
            linefit( &xtmp, &ytmp, &slope, &intercept, &sigma, &correlation, (*simparams).speed_step[ss].scans);
            correction = -intercept/slope;
            for (i=0; i<(*simparams).speed_step[ss].scans; i++)
            {
               (*exp_data)[ss].scan[i].time -= correction;
            }
            delete [] xtmp;
            delete [] ytmp;
         }
      }
   }
   rg.clear();
   af_params.s.clear();
   af_params.D.clear();
   af_params.kext.clear();
   af_params.role.clear();
   af_params.local_index.clear();
   af_params.association.clear();
   if (vC0 != NULL)   delete [] vC0;

#if defined(DEBUG_ALLOC)
   list_matrices_alloc();
#endif
#if defined(DEBUG_RSS)
   dm("end   us_astfem_rsa::calculate");
#endif
#if defined(USE_US_TIMER)
   us_timers.end_timer( "astfem rsa" );
#endif
   return 0;
}

void US_Astfem_RSA::update_assocv()
{
   for (unsigned int i=0; i<(*system).assoc_vector.size(); i++)
   {
      (*system).assoc_vector[i].comp.clear();
      (*system).assoc_vector[i].stoich.clear();
      (*system).assoc_vector[i].react.clear();
      if ((*system).assoc_vector[i].component3 == -1)
      {
         (*system).assoc_vector[i].comp.push_back((*system).assoc_vector[i].component1);
         (*system).assoc_vector[i].comp.push_back((*system).assoc_vector[i].component2);
         (*system).assoc_vector[i].stoich.push_back((*system).assoc_vector[i].stoichiometry2);
         (*system).assoc_vector[i].stoich.push_back((*system).assoc_vector[i].stoichiometry1);
         (*system).assoc_vector[i].react.push_back(1);
         (*system).assoc_vector[i].react.push_back(-1);
      }
      else
      {
         (*system).assoc_vector[i].comp.push_back((*system).assoc_vector[i].component1);
         (*system).assoc_vector[i].comp.push_back((*system).assoc_vector[i].component2);
         (*system).assoc_vector[i].comp.push_back((*system).assoc_vector[i].component3);
         (*system).assoc_vector[i].stoich.push_back((*system).assoc_vector[i].stoichiometry1);
         (*system).assoc_vector[i].stoich.push_back((*system).assoc_vector[i].stoichiometry2);
         (*system).assoc_vector[i].stoich.push_back((*system).assoc_vector[i].stoichiometry3);
         (*system).assoc_vector[i].react.push_back(1);
         (*system).assoc_vector[i].react.push_back(1);
         (*system).assoc_vector[i].react.push_back(-1);
      }
   }
}

/*************************************************************
New version : 03/03/2009
*************************************************************/
void US_Astfem_RSA::initialize_rg() // Setup reaction groups
{
   // if there are no reactions, then it is all noninteracting
   if ((*system).assoc_vector.size() == 0) return;
   
   vector <struct Association> tmp_rule;
   struct ReactionGroup tmp_rg;
   unsigned int i, j, m;
   bool isnew, connected, GCupdate;
   unsigned int NoneExist = 65535;   // index for none exisiting component
   
   // make a copy of all association rules to avoid changing the original one
   tmp_rule = (*system).assoc_vector;

   rg.clear();
   i = 0;
   while ( i <(*system).assoc_vector.size() )
   {
      tmp_rg.association.clear();
      tmp_rg.GroupComponent.clear();
      // find the first rulle with unsed components
      for(m=i; m<(*system).assoc_vector.size(); m++)
      {
         if( tmp_rule[m].component1 != NoneExist ||
             tmp_rule[m].component2 != NoneExist ||
             tmp_rule[m].component3 != -1 )  break;
      }
      if (m==(*system).assoc_vector.size() )
      {
         return;    // no more rule with unused components
      }
      else
      {
         i = m;
      }
      
      // i-th rule has unused comps
      tmp_rg.association.push_back(i);
      tmp_rg.GroupComponent.push_back( tmp_rule[i].component1 );
      tmp_rg.GroupComponent.push_back( tmp_rule[i].component2 );
      if ( tmp_rule[i].component3 != -1 )
         tmp_rg.GroupComponent.push_back( tmp_rule[i].component3 );

      do      // loop to find all rules connected to rule[i]
      {
         GCupdate = false;
         for (m=i+1; m<(*system).assoc_vector.size(); m++)
         {
            connected = false;
            for (j=0; j<tmp_rg.GroupComponent.size(); j++)
            {
               if ( tmp_rule[m].component1 == tmp_rg.GroupComponent[j] ||
                    tmp_rule[m].component2 == tmp_rg.GroupComponent[j] ||
                    tmp_rule[m].component3 == (int)tmp_rg.GroupComponent[j] )
               {
                  connected = true;
                  break;
               }
            } // j
            if (connected)
            {      // rules[m] belongs to tmp_rg
               GCupdate = true;       // tmp_rg.GroupComponent has to be updated
               tmp_rg.association.push_back(m);

               isnew = true;    // check if tmp_rule[m].comp1 is in tmp_rg.GroupComponent
               for (j=0; j<tmp_rg.GroupComponent.size(); j++)
               {
                  if (tmp_rule[m].component1 == tmp_rg.GroupComponent[j]) isnew = false;
               }
               if (isnew) tmp_rg.GroupComponent.push_back( tmp_rule[m].component1 );

               isnew = true;    // check if tmp_rule[m].comp1 is in tmp_rg.GroupComponent
               for (j=0; j<tmp_rg.GroupComponent.size(); j++)
               {
                  if (tmp_rule[m].component2 == tmp_rg.GroupComponent[j]) isnew = false;
               }
               if (isnew) tmp_rg.GroupComponent.push_back( tmp_rule[m].component2 );

               isnew = true;    // check if tmp_rule[m].comp1 is in tmp_rg.GroupComponent
               for (j=0; j<tmp_rg.GroupComponent.size(); j++)
               {
                  if (tmp_rule[m].component3 == (int)tmp_rg.GroupComponent[j] || tmp_rule[m].component3 == -1 ) isnew = false;
               }
               if (isnew) tmp_rg.GroupComponent.push_back( tmp_rule[m].component3 );

               // reset tmp_rule[m] to indicate all its comps are included
               tmp_rule[m].component1 = NoneExist;
               tmp_rule[m].component2 = NoneExist;
               tmp_rule[m].component3 = -1;
            } // if connected
         }  // m
      } while( GCupdate );

      // all rules connected to to rule[i] are examined
      rg.push_back( tmp_rg );
      i ++;

   } // while (i)

}


void US_Astfem_RSA::initialize_conc(unsigned int k, struct mfem_initial *CT0, bool noninteracting) // initializes total concentration vector
{
   unsigned int j;
   if ((*system).component_vector[k].c0.concentration.size() == 0) // we don't have an existing CT0 concentration vector
   { // build up the initial concentration vector with constant concentration
      if ((*simparams).band_forming)
      {
         // calculate the width of the lamella
         double lamella_width = pow(af_params.current_meniscus * af_params.current_meniscus
                                    + (*simparams).band_volume * 360.0/(2.5 * 1.2 * M_PI), 0.5) - af_params.current_meniscus;
         // calculate the spread of the lamella:
         for (j=0; j<(*CT0).concentration.size(); j++)
         {
            (*CT0).concentration[j] += (*system).component_vector[k].concentration * exp(-pow( ((*CT0).radius[j] - af_params.current_meniscus) / lamella_width, 4.0));
         }
      }
      else
      {
         for (j=0; j<(*CT0).concentration.size(); j++)
         {
            (*CT0).concentration[j] += (*system).component_vector[k].concentration;
         }
      }
   }
   else
   {
      if (noninteracting)
      {
         // take the existing initial concentration vector and copy it to the temporary CT0 vector:
         // needs rubber band to make sure meniscus and bottom equal current_meniscus and current_bottom
         (*CT0).radius.clear();
         (*CT0).concentration.clear();
         (*CT0) = (*system).component_vector[k].c0;
      }
      else // interpolation
      {
         mfem_initial C;
         C.radius.clear();
         C.concentration.clear();
#if defined(DEBUG)
         printf("init conc af_params.current_bottom %f\n", af_params.current_bottom);
#endif
         double dr = (af_params.current_bottom - af_params.current_meniscus)/((*CT0).concentration.size()-1);
         for (j=0; j<(*CT0).concentration.size(); j++)
         {
            C.radius.push_back(af_params.current_meniscus + j * dr );
            C.concentration.push_back(0.0);
         }
         interpolate_C0(&(*system).component_vector[k].c0, &C);
         for (j=0; j<(*CT0).concentration.size(); j++)
         {
            (*CT0).concentration[j] += C.concentration[j];
         }
      }
   }
}


int US_Astfem_RSA::calculate_ni(double rpm_start, double rpm_stop, mfem_initial *C_init, mfem_data *simdata, bool accel) // non-interacting solute, constant speed
{
   unsigned int i, j;
   double sw2, rpm_current;
   double **CA;            // stiffness matrix on left hand side
   // CA[0...Ms-1][0...N-1][4]
   double **CB;            // stiffness matrix on right hand side
   // CB[0...Ms-1][0...N-1][4]
   double *C0, *C1;         // C[m][j]: current/next concentration of m-th component at x_j
   // C[0...Ms-1][0....N-1]:
   double **CA1, **CA2, **CB1, **CB2;      // for matrices used in acceleration

   /*
     FILE *outf;
     if (guiFlag)
     {
     outf = fopen("tmp.out", "a");
     }
   */
   //cout << "rpm_start: " << rpm_start << ", rpm_stop: " << rpm_stop << endl;
   CA = NULL;
   CB = NULL;
   C0 = NULL;
   C1 = NULL;
   (*simdata).radius.clear();
   (*simdata).scan.clear();
   mfem_scan simscan;
   //
   // generate the adaptive mesh
   //
   sw2 = af_params.s[0] * pow(rpm_stop * M_PI/30.0, 2.0);
   vector <double> nu;
   nu.clear();
   nu.push_back(sw2/af_params.D[0]);
   mesh_gen(nu, (*simparams).mesh);
   if (accel)        // refine left hand side (when s>0) or
   {                              // right hand side (when s<0) for acceleration
      double xc ;
      if (af_params.s[0] > 0)
      { // radial distance from meniscus how far the boundary will move during this acceleration step (without diffusion)
         xc = af_params.current_meniscus + sw2 * (af_params.time_steps * af_params.dt) /3.;
         for (j=0; j<N-3; j++)
         {
            if (x[j] > xc)
            {
               break;
            }
         }
      }
      else
      {
         xc = af_params.current_bottom + sw2 * (af_params.time_steps * af_params.dt) /3.;
         for (j=0; j<N-3; j++)
         {
            if (x[N-j-1] < xc)
            {
               break;
            }
         }
      }
      mesh_gen_RefL(j+1, 4*j);
   }
#if defined(DEBUG)
   printf("calc_ni af_params.current_bottom %f\n", af_params.current_bottom);
#endif

   for (i=0; i<N; i++)
   {
      (*simdata).radius.push_back(x[i]);
   }

   //
   // initialize the coefficient matrices
   //
   initialize_2d(3, N, &CA);
   initialize_2d(3, N, &CB);

   if(!accel) // no acceleration
   {
      sw2 = af_params.s[0] * pow(rpm_stop * M_PI/30.0, 2.0);
      if (!(*simparams).moving_grid)
      {
         ComputeCoefMatrixFixedMesh(af_params.D[0], sw2, CA, CB);
      }
      else
      {
         if (af_params.s[0] > 0)
         {
            ComputeCoefMatrixMovingMeshR(af_params.D[0], sw2, CA, CB);
         }
         else
         {
            ComputeCoefMatrixMovingMeshL(af_params.D[0], sw2, CA, CB);
         }
      }
   }
   else            // for acceleration
   {
      initialize_2d(3, N, &CA1);
      initialize_2d(3, N, &CB1);
      initialize_2d(3, N, &CA2);
      initialize_2d(3, N, &CB2);

      sw2 = 0.;
      ComputeCoefMatrixFixedMesh(af_params.D[0], sw2, CA1, CB1);
      sw2 = af_params.s[0] * pow(rpm_stop * M_PI/30.0, 2.0);
      ComputeCoefMatrixFixedMesh(af_params.D[0], sw2, CA2, CB2);
   }

   //
   // Initial condition
   //
   C0 = new double [N];
   C1 = new double [N];
   interpolate_C0(C_init, C0, &x); //interpolate the given C_init vector on the new C0 grid

   //
   // time evolution
   //

   double *right_hand_side;
   right_hand_side = new double [N];
   for (i=0; i<af_params.time_steps+1; i++) // calculate all time steps f
   {
      rpm_current = rpm_start + (rpm_stop - rpm_start) * (i+0.5)/af_params.time_steps;
      if (guiFlag)
      {
         emit current_speed((unsigned int) rpm_current);
         qApp->processEvents();
      }

      // printf("rpm=%12.5e time_steps i=%d C_ttl=%20.10e \n", rpm_current, i,
      //           IntConcentration(x, C0));

      if(accel) // then we have acceleration
      {
         for(unsigned int j1=0; j1<3; j1++)
         {
            for(unsigned int j2=0; j2<N; j2++)
            {
               CA[j1][j2] = CA1[j1][j2] + pow(rpm_current/rpm_stop, 2.0) * (CA2[j1][j2] - CA1[j1][j2]);
               CB[j1][j2] = CB1[j1][j2] + pow(rpm_current/rpm_stop, 2.0) * (CB2[j1][j2] - CB1[j1][j2]);
            }
         }
      }
      simscan.rpm = (unsigned int) rpm_current;
      simscan.time = af_params.start_time + i * af_params.dt;
      w2t_integral += (simscan.time - last_time) * pow(rpm_current * M_PI/30.0, 2.0);
      last_time = simscan.time;
      simscan.omega_s_t = w2t_integral;
      //      cout << "rpm: " << simscan.rpm << ", t: " << simscan.time << ", w2t: " << simscan.omega_s_t << ", dt: " << af_params.dt << endl;
      if (guiFlag)
      {
         if(movieFlag)
         {
            emit new_scan(&x, C0);
            emit new_time((float) simscan.time);
            if (guiFlag)
            {
               qApp->processEvents();
            }
         }
      }
      simscan.conc.clear();
      for (j=0; j<N; j++)
      {
         simscan.conc.push_back(C0[j]);
      }
      (*simdata).scan.push_back(simscan);
      /*
        if(guiFlag)
        {
        if(i%1 == 0 || i<5)
        {
        for (j=0; j<N; j++)
        {
        fprintf(outf, "%12.5e %15.8e %15.8e\n", simscan.time, x[j], C0[j]);
        }
        fprintf(outf, "\n\n");
        }
        }
      */
      //
      // sedimentation part:
      // Calculate thr right hand side vector //
      //
      if (accel || !(*simparams).moving_grid)
      {
         right_hand_side[0] = -CB[1][0] * C0[0] - CB[2][0] * C0[1];
         for(j=1; j<N-1; j++)
         {
            right_hand_side[j] = -CB[0][j] * C0[j-1] - CB[1][j] * C0[j] - CB[2][j] * C0[j+1];
         }
         j = N-1;
         right_hand_side[j] = -CB[0][j] * C0[j-1] - CB[1][j] * C0[j];
      }
      else
      {
         if (af_params.s[0] > 0)
         {
            right_hand_side[0] = -CB[2][0] * C0[0];
            right_hand_side[1] = -CB[1][1] * C0[0] - CB[2][1] * C0[1];
            for(j=2; j<N; j++)
            {
               right_hand_side[j] = -CB[0][j] * C0[j-2] - CB[1][j] * C0[j-1] - CB[2][j] * C0[j];
            }
         }
         else
         {
            for(j=0; j<N-2; j++)
            {
               right_hand_side[j] = -CB[0][j] * C0[j] - CB[1][j] * C0[j+1] - CB[2][j] * C0[j+2];
            }
            j = N-2;
            right_hand_side[j] = -CB[0][j] * C0[j] - CB[1][j] * C0[j+1];
            j = N-1;
            right_hand_side[j] = -CB[0][j] * C0[j];
         }
      }
      tridiag(CA[0], CA[1], CA[2], right_hand_side, C1, N);
      for (j=0; j<N; j++)
      {
         C0[j] = C1[j];
      }
   } // time loop
   /*
     if (guiFlag)
     {
     fclose(outf);
     }
   */
   (*C_init).radius.clear();
   (*C_init).concentration.clear();
   for (j=0; j<N; j++)
   {
      (*C_init).radius.push_back( x[j] );
      (*C_init).concentration.push_back( C1[j] );
   }
   delete [] right_hand_side;
   delete [] C0;
   delete [] C1;
   clear_2d(3, CA);
   clear_2d(3, CB);
   if(accel) // then we have acceleration
   {
      clear_2d(3, CA1);
      clear_2d(3, CB1);
      clear_2d(3, CA2);
      clear_2d(3, CB2);
   }
   return (0);
}

// ***
// *** this is the SNI version of operator scheme
// ***

int US_Astfem_RSA::calculate_ra2(double rpm_start, double rpm_stop, mfem_initial *C_init, mfem_data *simdata, bool accel)
{
   unsigned int Mcomp, i, j, kkk;
   double sw2, rpm_current, dval;
   double alpha, s_max, s_min;
   double ***CA, ***CA1, ***CA2;   // stiffness matrix on left hand side
   // CA[0...Ms-1][4][0...N-1]
   double ***CB, ***CB1, ***CB2;   // stiffness matrix on right hand side
   // CB[0...Ms-1][4][0...N-1]
   double **C0, **C1;            // C[m][j]: current/next concentration of m-th component at x_j
   // C[0...Ms-1][0....N-1]:
   double *CT0, *CT1;            // total concentration at current and next time step
   vector <double> xb;            // grid for moving adaptive FEM for faster sedimentation

#if defined(DEBUG)
   FILE *outf = (FILE *) 0;
   if (guiFlag)
   {
      outf = fopen("tmp.out", "w");
   }
#endif
   Mcomp = af_params.s.size();
   s_max = maxval( af_params.s );     // used for mesh and dt
   s_min = minval( af_params.s );     // used for mesh and dt

#if defined(DEBUG)
   if (guiFlag)
   {
      print_af(outf);
   }
#endif

// wmc test
      print_af();
// end test

   (*simdata).radius.clear();
   (*simdata).scan.clear();
   mfem_scan simscan;

   // generate the adaptive mesh
   vector <double> nu;
   nu.clear();
   for (i=0; i<Mcomp; i++)
   {
      sw2 = af_params.s[i] * pow( rpm_stop * M_PI/30., 2.0);
      nu.push_back( sw2 / af_params.D[i]);
      //printf("s[%d]=%20.12e  D=%20.12e, sw2=%20.12e\n", i, af_params.s[i], af_params.D[i], sw2);
   }
   mesh_gen(nu, (*simparams).mesh);

   // refine left hand side (when s_max>0) or  right hand side (when s<0) for acceleration
   if (accel)
   {
      double xc ;
      if ( s_min > 0 )              // all sediment towards bottom
      {
         sw2 = s_max * pow( rpm_stop * M_PI/30., 2.0);
         xc = af_params.current_meniscus + sw2 * (af_params.time_steps * af_params.dt) /3.;
         for (j=0; j<N-3; j++)
         {
            if (x[j] > xc )
            {
               break;
            }
         }
         mesh_gen_RefL(j+1, 4*j);
      }
      else if ( s_max < 0 )      // all float towards meniscus
      {
         sw2 = s_min * pow( rpm_stop * M_PI/30., 2.0);   // s_min corresponds to fastest component
         xc = af_params.current_bottom + sw2 * (af_params.time_steps * af_params.dt) /3.;
         for (j=0; j<N-3; j++)
         {
            if (x[N-j-1] < xc )
            {
               break;
            }
         }
         mesh_gen_RefL(j+1, 4*j);
      }
      else
      {
         cerr << "multicomponent system with sedimentation and floating mixed, use uniform mesh" << endl;
      }
   }
   for (i=0; i<N; i++)
   {
      (*simdata).radius.push_back(x[i]);
   }

   // initialize the coefficient matrices
   initialize_3d(Mcomp, 4, N, &CA);
   initialize_3d(Mcomp, 4, N, &CB);

   if(accel) //  acceleration, so use fixed grid
   {
      initialize_3d(Mcomp, 3, N, &CA1);
      initialize_3d(Mcomp, 3, N, &CA2);
      initialize_3d(Mcomp, 3, N, &CB1);
      initialize_3d(Mcomp, 3, N, &CB2);
      for( i=0; i<Mcomp; i++)
      {
         sw2 = 0.;
         ComputeCoefMatrixFixedMesh(af_params.D[i], sw2, CA1[i], CB1[i]);
         sw2 = af_params.s[i] * pow(rpm_stop * M_PI/30.0, 2.0);
         ComputeCoefMatrixFixedMesh(af_params.D[i], sw2, CA2[i], CB2[i]);
      }
   }
   else      // constant sedimentation speed
   {
      if (!(*simparams).moving_grid )
      {
         for( i=0; i<Mcomp; i++)
         {
            sw2 = af_params.s[i] * pow(rpm_stop * M_PI/30.0, 2.0);
            ComputeCoefMatrixFixedMesh(af_params.D[i], sw2, CA[i], CB[i]);
         }
      }
      else   // moving grid
      {
         if ( s_min > 0)      // all components sedimenting
         {
            for (i=0; i<Mcomp; i++)
            {
               sw2 = af_params.s[i] * pow(rpm_stop * M_PI/30.0, 2.0);
               xb.clear();
               xb.push_back(af_params.current_meniscus);
               for (j=0; j<N-1; j++)
               {
                  dval = 0.1*exp( sw2/af_params.D[i]*( pow(0.5*(x[j]+x[j+1]), 2.0) - pow(af_params.current_bottom, 2.0) )/2. );
                  alpha = af_params.s[i]/s_max * (1.-dval) + dval;
                  // alpha = af_params.s[i]/s_max ;
                  xb.push_back( pow(x[j], alpha) * pow(x[j+1], (1.0 - alpha) ) );
               }
               GlobalStiff(&xb, CA[i], CB[i], af_params.D[i], sw2 );
               // GlobalStiff_ellam(&xb, CA[i], CB[i], af_params.D[i], sw2 );
            }
         }
         else if (s_max <0)       // all components floating
         {
            cerr << "all components floating, not implemented yet" << endl;
            return(-1);
         }
         else       // sedmientation and floating mixed
         {
            cerr << "sedimentation and floating mixed, suppose use fixed grid!" << endl;
            return(-1);
         }
      } // moving mesh
   } // acceleration

   // Initial condition
   initialize_2d(Mcomp, N, &C0);
   initialize_2d(Mcomp, N, &C1);
   CT0 = new double [N];
   CT1 = new double [N];

   // here we need the interpolatie the initial partial concentration onto new grid x[j]
   for( i=0; i<Mcomp; i++)
   {
      interpolate_C0(&(C_init[i]), C0[i], &x); //interpolate the given C_init vector on the new C0 grid
   }
   for (j=0; j<N; j++)
   {
      CT0[j] = 0.;
      for (i=0; i<Mcomp; i++)
      {
         CT0[j] += C0[i][j];
      }
      CT1[j] = CT0[j];
   }

   // time evolution
   double *right_hand_side;
   right_hand_side = new double [N];
   for (kkk=0; kkk<af_params.time_steps+2; kkk +=2)      // two steps in together
   {
      rpm_current = rpm_start + (rpm_stop - rpm_start) * (kkk+0.5)/af_params.time_steps;
      if (guiFlag) 
      {
         emit current_speed((unsigned int) rpm_current);
      }
      simscan.time = af_params.start_time + kkk * af_params.dt;
      simscan.rpm = (unsigned int) rpm_current;
      w2t_integral += (simscan.time - last_time) * pow(rpm_current * M_PI/30.0, 2.0);
      last_time = simscan.time;
      simscan.omega_s_t = w2t_integral;
      //      cout << "rpm: " << simscan.rpm << ", t: " << simscan.time << ", w2t: " << simscan.omega_s_t << ", dt: " << af_params.dt << endl;
      if (guiFlag)
      {
         if(movieFlag)
         {
            emit new_scan(&x, CT0);
            emit new_time((float) simscan.time);
            if (guiFlag)
            {
               qApp->processEvents();
            }
         }
      }
      simscan.conc.clear();
      for (j=0; j<N; j++)
      {
         simscan.conc.push_back(CT0[j]);
      }
      (*simdata).scan.push_back(simscan);
      /*
        if (guiFlag && (kkk%10 == 0 || kkk<5))
        {
        for(j=0; j<N; j++)
        {
        fprintf(outf, "%12.5e %15.8e %15.8e ", simscan.time, x[j], CT0[j]);
        for(i=0; i<Mcomp; i++) fprintf(outf, "%15.8e ", C0[i][j]);
        fprintf(outf, "\n");
        }
        fprintf(outf, "\n\n");
        // printf("t=%12.5e C_ttl=%15.8e \n", simscan.time, IntConcentration(x, CT0));
        }
      */
      //
      // first half step of sedimentation:
      //
      if( accel ) // need to reconstruct CA and CB by linear interpolation
      {
         dval =  pow(rpm_current/rpm_stop, 2.0) ;
         for(i=0; i<Mcomp; i++)
         {
            for(unsigned int j1=0; j1<3; j1++)
            {
               for(unsigned int j2=0; j2<N; j2++)
               {
                  CA[i][j1][j2] = CA1[i][j1][j2] + dval * (CA2[i][j1][j2] - CA1[i][j1][j2]) ;
                  CB[i][j1][j2] = CB1[i][j1][j2] + dval * (CB2[i][j1][j2] - CB1[i][j1][j2]) ;
               }
            }
         }
      }
      if (accel || !(*simparams).moving_grid)   // for fixed grid
      {
         for (i=0; i<Mcomp; i++)
         {
            right_hand_side[0] = -CB[i][1][0] * C0[i][0] - CB[i][2][0] * C0[i][1];
            for(j=1; j<N-1; j++)
            {
               right_hand_side[j] = -CB[i][0][j] * C0[i][j-1] - CB[i][1][j] * C0[i][j] - CB[i][2][j] * C0[i][j+1];
            }
            j = N-1;
            right_hand_side[j] = -CB[i][0][j] * C0[i][j-1] - CB[i][1][j] * C0[i][j];
            tridiag(CA[i][0], CA[i][1], CA[i][2], right_hand_side, C1[i], N);
         }
      }
      else // moving grid
      {
         for (i=0; i<Mcomp; i++)
         {
            // Calculate the right hand side vector //
            right_hand_side[0] = -CB[i][2][0] * C0[i][0] - CB[i][3][0] * C0[i][1];
            right_hand_side[1] = -CB[i][1][1] * C0[i][0] - CB[i][2][1] * C0[i][1] - CB[i][3][1] * C0[i][2];
            for (j=2; j<N-1; j++)
            {
               right_hand_side[j] = - CB[i][0][j] * C0[i][j-2]
                  - CB[i][1][j] * C0[i][j-1]
                  - CB[i][2][j] * C0[i][j]
                  - CB[i][3][j] * C0[i][j+1];
            }
            j = N-1;
            right_hand_side[j] = - CB[i][0][j] * C0[i][j-2]
               - CB[i][1][j] * C0[i][j-1]
               - CB[i][2][j] * C0[i][j];

            QuadSolver(CA[i][0], CA[i][1], CA[i][2], CA[i][3], right_hand_side, C1[i], N);
            // QuadSolver_ellam(CA[i][0], CA[i][1], CA[i][2], CA[i][3], right_hand_side, C1[i], N);
         }
      }
      //
      // reaction part: instantanuous reaction at each node
      //
      // instantanuous reaction at each node
      // [C1]=ReactionOneStep_inst(C1);
      //
      // finite reaction rate: linear interpolation of instantaneous reaction
      ReactionOneStep_Euler_imp(N, C1, 2*af_params.dt);
      //
      //
      // for next half time-step in SNI operator splitting scheme
      //
      for (j=0; j<N; j++)
      {
         CT1[j] = 0.;
         for (i=0; i<Mcomp; i++)
         {
            CT1[j] += C1[i][j];
            C0[i][j] = C1[i][j];
         }
         CT0[j] = CT1[j];
      }

      //
      // 2nd half step of sedimentation:
      //
      rpm_current = rpm_start + (rpm_stop - rpm_start) * (kkk+1.5)/af_params.time_steps;
      if( accel ) // need to reconstruct CA and CB by linear interpolation
      {
         dval =  pow(rpm_current/rpm_stop, 2.0) ;
         for(i=0; i<Mcomp; i++)
         {
            for(unsigned int j1=0; j1<3; j1++)
            {
               for(unsigned int j2=0; j2<N; j2++)
               {
                  CA[i][j1][j2] = CA1[i][j1][j2] + dval * (CA2[i][j1][j2] - CA1[i][j1][j2]) ;
                  CB[i][j1][j2] = CB1[i][j1][j2] + dval * (CB2[i][j1][j2] - CB1[i][j1][j2]) ;
               }
            }
         }
      }
      if (accel || !(*simparams).moving_grid)   // for fixed grid
      {
         for (i=0; i<Mcomp; i++)
         {
            right_hand_side[0] = -CB[i][1][0] * C0[i][0] - CB[i][2][0] * C0[i][1];
            for(j=1; j<N-1; j++)
            {
               right_hand_side[j] = -CB[i][0][j] * C0[i][j-1] - CB[i][1][j] * C0[i][j] - CB[i][2][j] * C0[i][j+1];
            }
            j = N-1;
            right_hand_side[j] = -CB[i][0][j] * C0[i][j-1] - CB[i][1][j] * C0[i][j];
            tridiag(CA[i][0], CA[i][1], CA[i][2], right_hand_side, C1[i], N);
         }
      }
      else // moving grid
      {
         for (i=0; i<Mcomp; i++)
         {
            // Calculate the right hand side vector //
            right_hand_side[0] = -CB[i][2][0] * C0[i][0] - CB[i][3][0] * C0[i][1];
            right_hand_side[1] = -CB[i][1][1] * C0[i][0] - CB[i][2][1] * C0[i][1] - CB[i][3][1] * C0[i][2];
            for (j=2; j<N-1; j++)
            {
               right_hand_side[j] = - CB[i][0][j] * C0[i][j-2]
                  - CB[i][1][j] * C0[i][j-1]
                  - CB[i][2][j] * C0[i][j]
                  - CB[i][3][j] * C0[i][j+1];
            }
            j = N-1;
            right_hand_side[j] = - CB[i][0][j] * C0[i][j-2]
               - CB[i][1][j] * C0[i][j-1]
               - CB[i][2][j] * C0[i][j];
            QuadSolver(CA[i][0], CA[i][1], CA[i][2], CA[i][3], right_hand_side, C1[i], N);
            // QuadSolver_ellam(CA[i][0], CA[i][1], CA[i][2], CA[i][3], right_hand_side, C1[i], N);
         }
      }
      // end of 2nd half step of sendimentation

      //
      // for next 2 time steps
      //
      for (j=0; j<N; j++)
      {
         CT1[j] = 0.;
         for (i=0; i<Mcomp; i++)
         {
            CT1[j] += C1[i][j];
            C0[i][j] = C1[i][j];
         }
         CT0[j] = CT1[j];
      }


   } // time loop
   if ( guiFlag )
   {
      emit new_scan(&x, CT0);
   }
#if defined(DEBUG)
   if (guiFlag)
   {
      fclose(outf);
   }
#endif
   for(i=0;i<Mcomp;i++)
   {
      C_init[i].radius.clear();
      C_init[i].concentration.clear();
      for (j=0; j<N; j++)
      {
         C_init[i].radius.push_back( x[j] );
         C_init[i].concentration.push_back( C1[i][j] );
      }
   }
   delete [] CT1;
   delete [] CT0;
   delete [] right_hand_side;
   clear_2d(Mcomp, C0);
   clear_2d(Mcomp, C1);
   clear_3d(Mcomp, 4, CA);
   clear_3d(Mcomp, 4, CB);
   if( accel ) // then we have acceleration
   {
      clear_3d(Mcomp, 3, CA1);
      clear_3d(Mcomp, 3, CB1);
      clear_3d(Mcomp, 3, CA2);
      clear_3d(Mcomp, 3, CB2);
   }
   return(0);
}


//************ new version *************

//////////////////////////////////////////////////////////////%
//
// Generate exponential mesh and refine cell bottom (for s>0)
//
//////////////////////////////////////////////////////////////%
void US_Astfem_RSA::mesh_gen_s_pos(vector <double> nu)
{
   double uth = 1.0/af_params.simpoints;      // threshold of u for steep region
   double tmp_xc, tmp_Hstar, xa;
   unsigned int IndLayer=0;      // number of layers for grids in steep region
   unsigned int i, j, tmp_Nf;
   vector <double> xc, Hstar, y;
   vector <unsigned int> Nf;
   xc.clear();
   Hstar.clear();
   Nf.clear();

   for (i=0; i<af_params.s.size(); i++)    // markers for steep regions
   {
      tmp_xc = af_params.current_bottom - (1.0/(nu[i] * af_params.current_bottom)) * log(nu[i]
                                                                                         * (pow((double) af_params.current_bottom, (double) 2.0)
                                                                                            - pow((double) af_params.current_meniscus, (double) 2.0))/(2.0 * uth));
      tmp_Nf = (int) (M_PI/2.0 * (af_params.current_bottom - tmp_xc)
                      * nu[i] * af_params.current_bottom/2.0 + 0.5) + 1; // # of pts for i-th layer
      tmp_Hstar = (af_params.current_bottom - tmp_xc)/tmp_Nf * M_PI/2.0;         // step required by Pac(i)<1
      if ((tmp_xc > af_params.current_meniscus) &&
          (af_params.current_bottom - af_params.current_meniscus *
           pow((double) af_params.current_bottom/af_params.current_meniscus,
               (double)((af_params.simpoints-4.0/2.0)/(af_params.simpoints-1.0))) > tmp_Hstar))
         //         (double)((af_params.simpoints-5.0/2.0)/(af_params.simpoints-1.0))) > tmp_Hstar))
      {
         xc.push_back(tmp_xc);
         Nf.push_back(tmp_Nf);
         Hstar.push_back(tmp_Hstar);
         IndLayer ++;
      }
   }
   xc.push_back(af_params.current_bottom);
#if defined(DEBUG)
   printf("mesh_gen_s_pos af_params.current_bottom %f\n", af_params.current_bottom);
#endif

   // print_vector(&xc);

   if (IndLayer == 0)   // use Schuck's grid only
   {
      x.push_back(af_params.current_meniscus);
      //      for(i=1; i<af_params.simpoints ; i++)   // add one more point to Schuck's grids
      for(i=1; i<af_params.simpoints -1 ; i++)   // add one more point to Schuck's grids
      {
         x.push_back(af_params.current_meniscus * pow((double) (af_params.current_bottom/af_params.current_meniscus),
                                                      (((double) i - 0.0)/((double)(af_params.simpoints - 1)))));   // Schuck's mesh
         //         (((double) i - 0.5)/((double)(af_params.simpoints - 1)))));   // Schuck's mesh
      }
      x.push_back(af_params.current_bottom);
   }
   else            // need a composite grid
   {
      // steep region
      unsigned int indp = 0, Mp=0;    // index for a grid point
      double HL, HR, Hf, alpha, beta, xi;
      for (i=0; i<IndLayer; i++)  // consider i-th steep region
      {
         if (i < IndLayer-1)   // use linear distribution for step size distrib
         {
            HL = Hstar[i];
            HR = Hstar[i+1];
            Mp = int ((xc[i+1] - xc[i]) * 2.0/(HL + HR));
            if ( Mp>1 ) {
               // alpha = Mp * HL - ((HR - HL)/2.0) * Mp/(Mp - 1.0);
               // beta = ((HR-HL)/2.0) * pow(Mp, 2.0)/(Mp - 1.0);
               beta = ((HR-HL)/2.0) * Mp ;
               alpha = (xc[i+1]-xc[i])-beta;
               for (j=0; j<=Mp-1; j++)
               {
                  xi = (double) j/ (double) Mp;
                  y.push_back(xc[i] + alpha * xi + beta * pow(xi, 2.0));
                  indp++;
               }
            }
         }
         else      // last layer, use sine distribution for grids
         {
            for (j=0; j<=Nf[i]-1; j++)
            {
               indp++;
               y.push_back(xc[i] + (af_params.current_bottom - xc[i]) * sin(j/(Nf[i] - 1.0) * M_PI/2.0));
               if (y[indp-1] > xc[i+1])
               {
                  break;
               }
            }
         }
      }
      int NfTotal = indp;
      vector <double> ytmp;
      ytmp.clear();
      // reverse the order of y
      j = NfTotal;
      do
      {
         j--;
         ytmp.push_back(y[j]);
      } while (j != 0);
      y.clear();
      y = ytmp;
      // transition region
      // smallest step size in transit region
      Hf = y[NfTotal - 2] - y[NfTotal-1];
      unsigned int Nm = (unsigned int) (floor(log(af_params.current_bottom/((af_params.simpoints - 1) * Hf)
                                                  * log(af_params.current_bottom/af_params.current_meniscus))/log(2.0))+1) ; // number of pts in trans region
      xa = y[NfTotal-1] - Hf * (pow(2.0, (double)Nm) -1.);
      unsigned int Js = (unsigned int) (floor(0.0 + (af_params.simpoints - 1)
                                              * log(xa/af_params.current_meniscus)/log(af_params.current_bottom/af_params.current_meniscus)));
      // xa is  modified so that y[NfTotal-Nm] matches xa exactly
      xa = af_params.current_meniscus*pow((double)(af_params.current_bottom/af_params.current_meniscus),
                                          (((double) Js - 0.0)/((double)af_params.simpoints - 1.0)));
      tmp_xc = y[NfTotal-1];
      HL = xa * (1.-af_params.current_meniscus/af_params.current_bottom);
      HR = y[NfTotal - 2]- y[NfTotal-1];
      Mp = (unsigned int) (((tmp_xc - xa) * 2.0/(HL + HR))+1);
      if ( Mp>1 ) {
         beta = ((HR-HL)/2.0) * Mp ;
         alpha = (tmp_xc - xa) -beta;
         for (j=Mp-1; j>0; j--)
         {
            xi = (double) j/ (double) Mp;
            y.push_back(xa + alpha * xi + beta * pow(xi, 2.0));
         }
      }
      Nm = Mp;

      // printf("xa=%15.8e Hf=%12.5e Nm=%d, Js=%d, NfTotal=%d\n", xa, HR, Nm, Js, NfTotal);

      // regular region
      x.push_back(af_params.current_meniscus);
      for (j=1; j<=Js; j++)
      {
         x.push_back(af_params.current_meniscus*
                     pow((double) (af_params.current_bottom/af_params.current_meniscus),
                         (((double) j - 0.0)/((double)af_params.simpoints - 1.0))));
      }

      for (j=0; j<NfTotal+Nm-1; j++)
      {
         x.push_back(y[NfTotal+Nm-j-2]);
      }

   }
}

//////////////////////////////////////////////////////////////%
//
// Generate exponential mesh and refine cell meniscus (for  s<0)
//
//////////////////////////////////////////////////////////////%
void US_Astfem_RSA::mesh_gen_s_neg(vector <double> nu)
{
   unsigned int j, Js, Nf, Nm;
   double uth = 1.0/af_params.simpoints;      // threshold of u for steep region
   double xc, xa, Hstar;
   vector <double> yr, ys, yt;

   x.clear();
   yr.clear();
   ys.clear();
   yt.clear();

   double nu0 = nu[0];
   xc = af_params.current_meniscus + 1./(fabs(nu0) * af_params.current_meniscus) *
      log((pow(af_params.current_bottom, 2.0) - pow(af_params.current_meniscus, 2.0)) * fabs(nu0)/(2.0*uth));
   Nf = (unsigned int) (1 + (floor)( (xc - af_params.current_meniscus) * fabs(nu0) * af_params.current_meniscus * M_PI/4.0));
   Hstar = (xc - af_params.current_meniscus)/Nf * M_PI/2.0;
   Nm = (unsigned int) (1 + (floor) (log(af_params.current_meniscus/((af_params.simpoints - 1.) * Hstar)
                                         * log(af_params.current_bottom/af_params.current_meniscus))/log(2.0)));
   xa = xc + (pow(2.0, (double) Nm) - 1.0) * Hstar;
   Js = (unsigned int) ((floor)((af_params.simpoints - 1) * log(af_params.current_bottom/xa)/log(af_params.current_bottom/af_params.current_meniscus) + 0.5 ));
   // printf("Nf=%d Nm=%d Js=%d \n", Nf, Nm, Js);
   // printf("xc=%12.5e xa=%12.5e \n", xc, xa);

   // all grid points at exponentials
   yr.push_back(af_params.current_bottom);
   //   for(j=1; j<(int) af_params.simpoints-1; j++)      // standard Schuck's grids
   for(j=1; j<af_params.simpoints; j++)      // add one more point to Schuck's grids
   {
      yr.push_back(af_params.current_bottom * pow((*simparams).meniscus/af_params.current_bottom, (j - 0.5)/(af_params.simpoints - 1.0)));
   }
   yr.push_back(af_params.current_meniscus);
   if(af_params.current_bottom * (pow(af_params.current_meniscus/af_params.current_bottom, (af_params.simpoints - 3.5)/(af_params.simpoints - 1.0))
                                  - pow(af_params.current_meniscus/af_params.current_bottom, (af_params.simpoints - 2.5)/(af_params.simpoints - 1.0))) < Hstar || Nf <= 2 )
   {
      // no need for steep region
      for(j=0; j<af_params.simpoints; j++)
      {
         x.push_back(yr[af_params.simpoints - 1 - j]);
      }
      if (guiFlag)
      {
         cerr << "use exponential grid only!\n" << endl;
      }
   }
   else
   {// Nf>2
      for(j=1;j<Nf;j++)
      {
         ys.push_back(xc - (xc - af_params.current_meniscus) * sin((j - 1.0)/(Nf - 1.0) * M_PI/2.0));
      }
      ys.push_back(af_params.current_meniscus);
      for (j=0; j<Nm; j++)
      {
         yt.push_back(xc + (pow(2.0, (double) j) - 1.0) * Hstar);
      }
      // set x:
      for(j=0; j<Nf; j++)
      {
         x.push_back(ys[Nf - 1 - j]);
      }
      for(j=1; j<Nm; j++)
      {
         x.push_back(yt[j]);
      }
      for(j=Js+1; j>0; j--)
      {
         x.push_back(yr[j-1]);
      }
      // smooth out
      x[Nf + Nm] = (x[Nf + Nm - 1] + x[Nf + Nm + 1])/2.0;
      x[Nf + Nm + 1] = (x[Nf + Nm] + x[Nf + Nm + 2])/2.0;
   } // if
}


///////////////////////////////////////////////////////////
//
//   mesh_gen_RefL:   refine mesh near meniscus (for s>0) or near bottom (for s<0)
//                  to be used for the acceleration stage
//  parameters: N0 = number of elements near meniscus (or bottom) to be refined
//            M0 = number of elements to be used for the refined region
//
///////////////////////////////////////////////////////////
void US_Astfem_RSA::mesh_gen_RefL(int N0, int M0)
{
   int j;
   vector <double> zz;   // temperary array for adaptive grids
   zz.clear();
   if( minval(af_params.s) > 0 )            // all species with s>0
   {
      // refine around the meniscus for acceleration
      for(j=0; j<M0; j++)
      {
         double tmp = (double)(j)/(double)(M0);
         tmp = 1. - cos(tmp*M_PI/2.);
         zz.push_back( x[0]*(1.-tmp) + x[N0]*tmp );
      }
      for(j=N0; j<(int) x.size(); j++)
      {
         zz.push_back( x[j] );
      }
      x.clear();
      for(j=0;j<(int) zz.size(); j++)
      {
         x.push_back( zz[j] );
      }
   }
   else if ( maxval(af_params.s)<0 )    //  all species with s<0
   {
      for(j=0; j<(int) x.size()-N0; j++)
      {
         zz.push_back( x[j] );
      }
      // refine around the bottom for acceleration
      for(j=1; j<=M0; j++)
      {
         double tmp = (double)(j)/(double)(M0);
         tmp = sin(tmp*M_PI/2.);
         zz.push_back( x[ x.size() -N0-1 ]*(1.-tmp) + x[ x.size() -1 ]*tmp );
      }
      x.clear();
      for (j=0; j<(int) zz.size();j++)
      {
         x.push_back(zz[j]);
      }
   }
   else                        // sedimentation and floating mixed up
   {
      cerr << "no refinement at ends since sedimentation and floating mixed ...\n" ;
   }
   N = x.size();
}


void US_Astfem_RSA::mesh_gen(vector <double> nu, unsigned int MeshOpt)
{
   //////////////////////////////////////////////////////////////%
   //
   // Generate adaptive grids for multi-component Lamm equations
   //
   //
   // Here: N: Number of points in the ASTFEM
   //      m, b: meniscus, bottom
   //      nuMax, nuMin = max and min of nu=sw^2/D
   //      MeshType: = 0 ASTFEM grid based on all nu (composite in sharp region)
   //               = 1 Claverie (uniform), etc,
   //               = 2 Exponential mesh (Schuck's formular, no refinement at bottom)
   //               = 3 input from data file: "mesh_data.dat"
   //               = 10, acceleration mesh (left and right refinement)
   //////////////////////////////////////////////////////////////%

   ////////////////////%
   // generate the mesh
   ////////////////////%

   US_Config* USglobal;
   USglobal = NULL;
   x.clear();
   sort(nu.begin(), nu.end());   // put nu in ascending order
   switch ( MeshOpt )
   {
      //////////////////////%
      // Mesh Type 0 (default): adaptive mesh based on all nu
      //////////////////////%
   case 0: //Astfem without left hand refinement
      {
         if( nu[0]>0 )
         {
            //            cout << "exponential mesh plus refinement at bottom, for s>0 ...\n";
            mesh_gen_s_pos(nu);
         }
         else if ( nu[ nu.size()-1 ] < 0 )
         {
            //            cout << "exponential mesh plus refinement at meniscus, for s<0 ...\n";
            mesh_gen_s_neg(nu);
         }
         else                     // some species with s<0 and some with s>0
         {
            //            cout << "multicomponent system with sedimentation and floating mixed, use uniform mesh...\n";
            for ( unsigned int i=0; i<af_params.simpoints; i++)
            {
               x.push_back(af_params.current_meniscus + (af_params.current_bottom -
                                                         af_params.current_meniscus) * i/(af_params.simpoints-1));
            }
         }
         break;
      }
   case 1: //Claverie mesh without left hand refinement
      {
         //         cout << "using uniform mesh ...\n";
         for ( unsigned int i=0; i<af_params.simpoints; i++)
         {
            x.push_back(af_params.current_meniscus + (af_params.current_bottom -
                                                      af_params.current_meniscus) * i/(af_params.simpoints-1));
         }
         break;
      }
   case 2: //Moving Hat (Peter Schuck's Mesh) without left hand side refinement
      {
         //cout << "using moving hat mesh...\n";
         x.push_back(af_params.current_meniscus);
         for(unsigned int i=1; i<af_params.simpoints-1; i++)  // standard Schuck's grids
         {
            x.push_back(af_params.current_meniscus * pow((double) (af_params.current_bottom/af_params.current_meniscus),
                                                         (((double) i - 0.5)/((double)(af_params.simpoints-1)))));   // Schuck's mesh
         }
         x.push_back(af_params.current_bottom);
         break;
      }
   case 3: // user defined mesh generated from data file
      {
         //cout << "using mesh from file $ULTRASCAN/mesh.dat...\n";
         // QString str = getenv("ULTRASCAN");

#if defined(USE_MPI)
         cout << "using mesh from file $ULTRASCAN/mesh.dat...\n";
         QString str = getenv("ULTRASCAN");
#else
         USglobal = new US_Config();
         QString    str      = USglobal->config_list.system_dir;
         if (USglobal != NULL) delete USglobal;
#endif

         QFile f(str + "/mesh.dat");
         if (f.open(IO_ReadOnly))
         {
            QTextStream ts(&f);
            while(!ts.atEnd())
            {
               str = ts.readLine();
               x.push_back(str.toDouble());
            }
            f.close();
            if (fabs(x[0] - af_params.current_meniscus) > 1.0e7)
            {
               cerr << "The meniscus from the mesh file does not match the set meniscus - using Claverie Mesh instead\n";
            }
            if (fabs(x[x.size()-1] - af_params.current_bottom) > 1.0e7)
            {
               cerr << "The cell bottom from the mesh file does not match the set cell bottom - using Claverie Mesh instead\n";
            }
         }
         else
         {
            cerr << tr("Could not read the mesh file - using Claverie Mesh instead\n");
            for (unsigned int i=0; i<af_params.simpoints; i++)
            {
               x.push_back(af_params.current_meniscus + (af_params.current_bottom -
                                                         af_params.current_meniscus) * i/(af_params.simpoints-1));
            }
         }
         break;
      }
   default:
      {
         cerr << "undefined mesh option\n";
      }
   }
   N = x.size();
   //cout << "total number of points = " << N << "\n";
}

//
//   Compute the coefficient matrices based on fixed mesh
//
void US_Astfem_RSA::ComputeCoefMatrixFixedMesh(double D, double sw2, double **CA, double **CB)
{
   unsigned int k;
   // compute local stiffness matrices
   StiffBase stfb0 ;
   double ***Stif;    // Local stiffness matrix at each element
   initialize_3d(N-1, (unsigned int) 4, (unsigned int) 4, &Stif);
   double xd[4][2];      // coord for verices of quad elem
   for(k=0; k<N-1; k++)
   {    // loop for all elem
      xd[0][0] = x[k  ];
      xd[0][1] = 0.;
      xd[1][0] = x[k+1];
      xd[1][1] = 0.;
      xd[2][0] = x[k+1];
      xd[2][1] = af_params.dt;
      xd[3][0] = x[k  ];
      xd[3][1] = af_params.dt;
      stfb0.CompLocalStif(4, xd, D, sw2, Stif[k]);
   }
   // assembly coefficient matrices
   // elem[0]; i=0
   k = 0;
   CA[1][k] = Stif[k][3][0] + Stif[k][3][3];    // j=3;
   CA[2][k] = Stif[k][2][0] + Stif[k][2][3];   // j=2;
   CB[1][k] = Stif[k][0][0] + Stif[k][0][3];    // j=0;
   CB[2][k] = Stif[k][1][0] + Stif[k][1][3];   // j=1;

   for(k=1; k<N-1;k++)
   {    // loop for all elem
      // elem k-1: i=1,2
      CA[0][k]  = Stif[k-1][3][1] + Stif[k-1][3][2];   // j=3;
      CA[1][k]  = Stif[k-1][2][1] + Stif[k-1][2][2];   // j=2;
      CB[0][k]  = Stif[k-1][0][1] + Stif[k-1][0][2];   // j=0;
      CB[1][k]  = Stif[k-1][1][1] + Stif[k-1][1][2];   // j=1;

      // elem k: i=0,3
      CA[1][k] += Stif[k  ][3][0] + Stif[k  ][3][3];   // j=3;
      CA[2][k]  = Stif[k  ][2][0] + Stif[k  ][2][3];   // j=2;
      CB[1][k] += Stif[k  ][0][0] + Stif[k  ][0][3];   // j=0;
      CB[2][k]  = Stif[k  ][1][0] + Stif[k  ][1][3];   // j=1;
   }

   // elem[N-2]; i=1,2
   k = N-1;
   CA[0][k]  = Stif[k-1][3][1] + Stif[k-1][3][2];   // j=3;
   CA[1][k]  = Stif[k-1][2][1] + Stif[k-1][2][2];   // j=2;
   CB[0][k]  = Stif[k-1][0][1] + Stif[k-1][0][2];   // j=0;
   CB[1][k]  = Stif[k-1][1][1] + Stif[k-1][1][2];   // j=1;
   clear_3d(N-1, (unsigned int) 4, Stif);
}

void US_Astfem_RSA::ComputeCoefMatrixMovingMeshR(double D, double sw2, double **CA, double **CB)
{
   unsigned int k;
   // compute local stiffness matrices
   StiffBase stfb0 ;
   double ***Stif;    // Local stiffness matrix at each element
   initialize_3d(N, (unsigned int) 4, (unsigned int) 4, &Stif);
   double xd[4][2];      // coord for verices of quad elem

   // elem[0]: triangle
   xd[0][0] = x[0];   xd[0][1] = 0.;
   xd[1][0] = x[1];   xd[1][1] = af_params.dt;
   xd[2][0] = x[0];   xd[2][1] = af_params.dt;
   stfb0.CompLocalStif(3, xd, D, sw2, Stif[0]);

   // elem[k]: k=1..(N-2), quadrilateral
   for(k=1; k<N-1; k++)
   {    // loop for all elem
      xd[0][0] = x[k-1];   xd[0][1] = 0.;
      xd[1][0] = x[k  ];   xd[1][1] = 0.;
      xd[2][0] = x[k+1];   xd[2][1] = af_params.dt;
      xd[3][0] = x[k  ];   xd[3][1] = af_params.dt;
      stfb0.CompLocalStif(4, xd, D, sw2, Stif[k]);
   }

   // elem[N-1]: triangle
   xd[0][0] = x[N-2];   xd[0][1] = 0.;
   xd[1][0] = x[N-1];   xd[1][1] = 0.;
   xd[2][0] = x[N-1];   xd[2][1] = af_params.dt;
   stfb0.CompLocalStif(3, xd, D, sw2, Stif[N-1]);

   // assembly coefficient matrices

   k = 0;
   CA[1][k] = Stif[k][2][2] ;
   CA[2][k] = Stif[k][1][2] ;
   CB[2][k] = Stif[k][0][2] ;

   k = 1;
   CA[0][k] = Stif[k-1][2][0] + Stif[k-1][2][1];
   CA[1][k] = Stif[k-1][1][0] + Stif[k-1][1][1];
   CA[1][k]+= Stif[k  ][3][0] + Stif[k  ][3][3];
   CA[2][k] = Stif[k  ][2][0] + Stif[k  ][2][3] ;

   CB[1][k] = Stif[k-1][0][0] + Stif[k-1][0][1];    // j=0;
   CB[1][k]+= Stif[k  ][0][0] + Stif[k  ][0][3];    // j=0;
   CB[2][k] = Stif[k  ][1][0] + Stif[k  ][1][3];   // j=1;

   for(k=2; k<N-1;k++)
   {    // loop for all elem
      // elem k-1: i=1,2
      CA[0][k]  = Stif[k-1][3][1] + Stif[k-1][3][2];   // j=3;
      CA[1][k]  = Stif[k-1][2][1] + Stif[k-1][2][2];   // j=2;
      CB[0][k]  = Stif[k-1][0][1] + Stif[k-1][0][2];   // j=0;
      CB[1][k]  = Stif[k-1][1][1] + Stif[k-1][1][2];   // j=1;

      // elem k: i=0,3
      CA[1][k] += Stif[k  ][3][0] + Stif[k  ][3][3];   // j=3;
      CA[2][k]  = Stif[k  ][2][0] + Stif[k  ][2][3];   // j=2;
      CB[1][k] += Stif[k  ][0][0] + Stif[k  ][0][3];   // j=0;
      CB[2][k]  = Stif[k  ][1][0] + Stif[k  ][1][3];   // j=1;
   }

   k = N-1;
   // elem[k-1]: quadrilateral
   CA[0][k]  = Stif[k-1][3][1] + Stif[k-1][3][2];   // j=3;
   CA[1][k]  = Stif[k-1][2][1] + Stif[k-1][2][2];   // j=2;
   CB[0][k]  = Stif[k-1][0][1] + Stif[k-1][0][2];   // j=0;
   CB[1][k]  = Stif[k-1][1][1] + Stif[k-1][1][2];   // j=1;

   // elem[k]: triangle
   CA[1][k] += Stif[k][2][0] + Stif[k][2][1] + Stif[k][2][2];
   CB[1][k] += Stif[k][0][0] + Stif[k][0][1] + Stif[k][0][2];
   CB[2][k]  = Stif[k][1][0] + Stif[k][1][1] + Stif[k][1][2];
   clear_3d(N, (unsigned int) 4, Stif);
}

void US_Astfem_RSA::ComputeCoefMatrixMovingMeshL(double D, double sw2, double **CA, double **CB)
{
   unsigned int k;
   // compute local stiffness matrices
   StiffBase stfb0 ;
   double ***Stif;    // Local stiffness matrix at each element
   initialize_3d(N, (unsigned int) 4, (unsigned int) 4, &Stif);
   double xd[4][2];      // coord for verices of quad elem
   // elem[0]: triangle
   xd[0][0] = x[0];
   xd[0][1] = 0.;
   xd[1][0] = x[1];   xd[1][1] = 0.;
   xd[2][0] = x[0];   xd[2][1] = af_params.dt;
   stfb0.CompLocalStif(3, xd, D, sw2, Stif[0]);

   // elem[k]: k=1..(N-2), quadrilateral
   for(k=1; k<N-1;k++)
   {    // loop for all elem
      xd[0][0] = x[k  ];   xd[0][1] = 0.;
      xd[1][0] = x[k+1];   xd[1][1] = 0.;
      xd[2][0] = x[k  ];   xd[2][1] = af_params.dt;
      xd[3][0] = x[k-1];   xd[3][1] = af_params.dt;
      stfb0.CompLocalStif(4, xd, D, sw2, Stif[k]);
   }

   // elem[N-1]: triangle
   xd[0][0] = x[N-1];   xd[0][1] = 0.;
   xd[1][0] = x[N-1];   xd[1][1] = af_params.dt;
   xd[2][0] = x[N-2];   xd[2][1] = af_params.dt;
   stfb0.CompLocalStif(3, xd, D, sw2, Stif[N-1]);


   // assembly coefficient matrices

   k = 0;
   CA[1][0] = Stif[0][2][0] + Stif[0][2][1] + Stif[0][2][2];
   CB[0][0] = Stif[0][0][0] + Stif[0][0][1] + Stif[0][0][2] ;
   CB[1][0] = Stif[0][1][0] + Stif[0][1][1] + Stif[0][1][2] ;

   CA[1][0]+= Stif[1][3][0] + Stif[1][3][3] ;
   CA[2][0] = Stif[1][2][0] + Stif[1][2][3] ;
   CB[1][0]+= Stif[1][0][0] + Stif[1][0][3] ;
   CB[2][0] = Stif[1][1][0] + Stif[1][1][3] ;

   for(k=1; k<N-2; k++)
   {    // loop for all elem
      // elem k:
      CA[0][k]  = Stif[k  ][3][1] + Stif[k  ][3][2];   // j=3;
      CA[1][k]  = Stif[k  ][2][1] + Stif[k  ][2][2];   // j=2;
      CB[0][k]  = Stif[k  ][0][1] + Stif[k  ][0][2];   // j=0;
      CB[1][k]  = Stif[k  ][1][1] + Stif[k  ][1][2];   // j=1;

      // elem k+1:
      CA[1][k] += Stif[k+1][3][0] + Stif[k+1][3][3];   // j=3;
      CA[2][k]  = Stif[k+1][2][0] + Stif[k+1][2][3];   // j=2;
      CB[1][k] += Stif[k+1][0][0] + Stif[k+1][0][3];   // j=0;
      CB[2][k]  = Stif[k+1][1][0] + Stif[k+1][1][3];   // j=1;
   }

   k = N-2;
   // elem k:
   CA[0][k]  = Stif[k  ][3][1] + Stif[k  ][3][2];   // j=3;
   CA[1][k]  = Stif[k  ][2][1] + Stif[k  ][2][2];   // j=2;
   CB[0][k]  = Stif[k  ][0][1] + Stif[k  ][0][2];   // j=0;
   CB[1][k]  = Stif[k  ][1][1] + Stif[k  ][1][2];   // j=1;

   // elem k+1: (triangle)
   CA[1][k] += Stif[k+1][2][0] + Stif[k+1][2][2];   // j=3;
   CA[2][k]  = Stif[k+1][1][0] + Stif[k+1][1][2];   // j=2;
   CB[1][k] += Stif[k+1][0][0] + Stif[k+1][0][2];   // j=0;


   k = N-1;
   // elem[k]: triangle
   CA[0][k]  = Stif[k  ][2][1] ;
   CA[1][k]  = Stif[k  ][1][1] ;
   CB[0][k]  = Stif[k  ][0][1] ;
   clear_3d(N, (unsigned int) 4, Stif);
}


void US_Astfem_RSA::GlobalStiff(vector <double> *xb, double **ca, double **cb,
                                double D, double sw2)
{

   //
   //  4: global stifness matrix
   //

   //function [CA, CB]=4(x, xb, dt, D, sw2)

   unsigned int i;
   double ***Stif=NULL;
   vector <double> vx;
   initialize_3d(N, 6, 2, &Stif);

   // 1st elem
   vx.clear();
   vx.push_back(x[0]);
   vx.push_back(x[1]);
   vx.push_back(x[0]);
   vx.push_back(x[1]);
   vx.push_back(x[2]);
   vx.push_back((*xb)[1]);
   IntQT1(vx, D, sw2, Stif[0], af_params.dt);

   // elems in middle
   for (i=1; i<(N-2); i++)
   {
      vx.clear();
      vx.push_back(x[i-1]);
      vx.push_back(x[i]);
      vx.push_back(x[i+1]);
      vx.push_back(x[i]);
      vx.push_back(x[i+1]);
      vx.push_back(x[i+2]);
      vx.push_back((*xb)[i]);
      vx.push_back((*xb)[i+1]);
      IntQTm(vx, D, sw2, Stif[i], af_params.dt);
   }

   // 2nd last elems
   vx.clear();
   vx.push_back(x[N-3]);
   vx.push_back(x[N-2]);
   vx.push_back(x[N-1]);
   vx.push_back(x[N-2]);
   vx.push_back(x[N-1]);
   vx.push_back((*xb)[N-2]);
   vx.push_back((*xb)[N-1]);

   IntQTn2(vx, D, sw2, Stif[N-2], af_params.dt);

   // last elems
   vx.clear();
   vx.push_back(x[N-2]);
   vx.push_back(x[N-1]);
   vx.push_back(x[N-1]);
   vx.push_back((*xb)[N-1]);
   IntQTn1(vx, D, sw2, Stif[N-1], af_params.dt);

   //
   // assembly into global stiffness matrix
   //

   ca[0][0] = 0.0;
   ca[1][0] = Stif[0][2][0];
   ca[2][0] = Stif[0][3][0];
   ca[3][0] = Stif[0][4][0];

   cb[0][0] = 0.0;
   cb[1][0] = 0.0;
   cb[2][0] = Stif[0][0][0];
   cb[3][0] = Stif[0][1][0];

   // i=2
   ca[0][1] = Stif[0][2][1];
   ca[1][1] = Stif[0][3][1] + Stif[1][3][0];
   ca[2][1] = Stif[0][4][1] + Stif[1][4][0];
   ca[3][1] =                 Stif[1][5][0];

   cb[0][1] = 0.0;
   cb[1][1] = Stif[0][0][1] + Stif[1][0][0];
   cb[2][1] = Stif[0][1][1] + Stif[1][1][0];
   cb[3][1] =                 Stif[1][2][0];

   // i: middle
   for (i=2; i<N-2; i++)
   {
      ca[0][i] = Stif[i-1][3][1];
      ca[1][i] = Stif[i-1][4][1] + Stif[i][3][0];
      ca[2][i] = Stif[i-1][5][1] + Stif[i][4][0];
      ca[3][i] =                   Stif[i][5][0];

      cb[0][i] = Stif[i-1][0][1];
      cb[1][i] = Stif[i-1][1][1] + Stif[i][0][0];
      cb[2][i] = Stif[i-1][2][1] + Stif[i][1][0];
      cb[3][i] =                   Stif[i][2][0];
   }

   // i=n
   i = N-2;
   ca[0][i] = Stif[i-1][3][1];
   ca[1][i] = Stif[i-1][4][1] + Stif[i][3][0];
   ca[2][i] = Stif[i-1][5][1] + Stif[i][4][0];
   ca[3][i] = 0.0;

   cb[0][i] = Stif[i-1][0][1];
   cb[1][i] = Stif[i-1][1][1] + Stif[i][0][0];
   cb[2][i] = Stif[i-1][2][1] + Stif[i][1][0];
   cb[3][i] =                   Stif[i][2][0];

   // i=n+1
   i = N-1;
   ca[0][i] = Stif[i-1][3][1];
   ca[1][i] = Stif[i-1][4][1] + Stif[i][2][0];
   ca[2][i] = 0.0;
   ca[3][i] = 0.0;

   cb[0][i] = Stif[i-1][0][1];
   cb[1][i] = Stif[i-1][1][1] + Stif[i][0][0];
   cb[2][i] = Stif[i-1][2][1] + Stif[i][1][0];
   cb[3][i] = 0.0;

   clear_3d(N, 6, Stif);
}


//
// ************* ELLAM ***********
//

void US_Astfem_RSA::GlobalStiff_ellam(vector <double> *xb, double **ca, double **cb,
                                      double D, double sw2)
{
   //
   //  4: global stifness matrix
   //

   //function [CA, CB]=4(x, xb, dt, D, sw2)

   unsigned int i;
   double ***Stif=NULL;
   vector <double> vx;
   initialize_3d(N, 6, 2, &Stif);

   // 1st elem
   vx.clear();
   vx.push_back(x[0]);
   vx.push_back(x[1]);
   vx.push_back(x[0]);
   vx.push_back(x[1]);
   vx.push_back((*xb)[1]);
   IntQT1_ellam(vx, D, sw2, Stif[0], af_params.dt);

   // elems in middle
   for (i=1; i<=(N-2); i++)
   {
      vx.clear();
      vx.push_back(x[i-1]);
      vx.push_back(x[i]);
      vx.push_back(x[i+1]);
      vx.push_back(x[i-1]);
      vx.push_back(x[i]);
      vx.push_back(x[i+1]);
      vx.push_back((*xb)[i]);
      vx.push_back((*xb)[i+1]);
      IntQTm_ellam(vx, D, sw2, Stif[i], af_params.dt);
   }


   // last elems
   vx.clear();
   vx.push_back(x[N-2]);
   vx.push_back(x[N-1]);
   vx.push_back(x[N-2]);
   vx.push_back(x[N-1]);
   vx.push_back((*xb)[N-1]);
   IntQTn1_ellam(vx, D, sw2, Stif[N-1], af_params.dt);

   //
   // assembly into global stiffness matrix
   //
   ca[0][0] = 0.0;
   ca[1][0] = 0.0;
   ca[2][0] = Stif[0][2][0];
   ca[3][0] = Stif[0][3][0];

   cb[0][0] = 0.0;
   cb[1][0] = 0.0;
   cb[2][0] = Stif[0][0][0];
   cb[3][0] = Stif[0][1][0];

   // i=1
   ca[0][1] = 0.0;
   ca[1][1] = Stif[0][2][1] + Stif[1][3][0];
   ca[2][1] = Stif[0][3][1] + Stif[1][4][0];
   ca[3][1] =                 Stif[1][5][0];

   cb[0][1] = 0.0;
   cb[1][1] = Stif[0][0][1] + Stif[1][0][0];
   cb[2][1] = Stif[0][1][1] + Stif[1][1][0];
   cb[3][1] =                 Stif[1][2][0];

   // i: middle
   for (i=2; i<=N-2; i++)
   {
      ca[0][i] = Stif[i-1][3][1];
      ca[1][i] = Stif[i-1][4][1] + Stif[i][3][0];
      ca[2][i] = Stif[i-1][5][1] + Stif[i][4][0];
      ca[3][i] =                   Stif[i][5][0];

      cb[0][i] = Stif[i-1][0][1];
      cb[1][i] = Stif[i-1][1][1] + Stif[i][0][0];
      cb[2][i] = Stif[i-1][2][1] + Stif[i][1][0];
      cb[3][i] =                   Stif[i][2][0];
   }

   // i=n-1
   i = N-1;
   ca[0][i] = Stif[i-1][3][1];
   ca[1][i] = Stif[i-1][4][1] + Stif[i][2][0];
   ca[2][i] = Stif[i-1][5][1] + Stif[i][3][0];
   ca[3][i] = 0.0;

   cb[0][i] = Stif[i-1][0][1];
   cb[1][i] = Stif[i-1][1][1] + Stif[i][0][0];
   cb[2][i] = Stif[i-1][2][1] + Stif[i][1][0];
   cb[3][i] = 0.0;

   clear_3d(N, 6, Stif);
}


//////////////////////////////%
//
// ReactionOneStep_Euler_imp:  implicit Mid-point Euler
//
//////////////////////////////%
void US_Astfem_RSA::ReactionOneStep_Euler_imp(unsigned int Npts, double **C1, double TimeStep)
{
   unsigned int num_comp;
   unsigned int i, j, k;
   double ct;

   num_comp = af_params.role.size();

   // special case:  self-association  n A <--> An
   if( num_comp==2 )              // only  2 components and one association rule
   {
      unsigned int rule, st0, st1;
      double keq, koff;
      double dva, dvb, dvc, uhat;
      rule = rg[af_params.rg_index].association[0];   // current rule used
      st0 = (*system).assoc_vector[rule].stoich[0];
      st1 = (*system).assoc_vector[rule].stoich[1];
      keq = (double)(*system).assoc_vector[rule].keq;
      koff = (double)(*system).assoc_vector[rule].k_off;
cout << "assoc: " << keq << ", koff: " << koff << endl;
      for (j=0; j<Npts; j++)
      {
         ct = C1[0][j] + C1[1][j] ;
         dva = TimeStep * koff * keq;
         dvb = TimeStep * koff + 2.;
         dvc = TimeStep * koff * ct + 2. * C1[0][j];
//cout << "RSA:Eul:   j ct" << j <<", " << ct << ", dva dvb dvc " << dva << ", " << dvb << ", " << dvc << endl;


         if (st0 ==2 && st1 == 1 )                // mono <--> dimer
         {
            uhat = 2*dvc / ( dvb+sqrt(dvb*dvb+4.*dva*dvc) );
         }
         else if (st0 ==3 && st1 == 1 )             // mono <--> trimer
         {
            uhat = cube_root(-dvc/dva, dvb/dva, 0.0);
         }
         else if (st0 > 3 && st1 == 1 )             // mono <--> n-mer
         {
            uhat = find_C1_mono_Nmer( st0, dva/dvb, dvc/dvb);
         }
         else
         {
            cerr << "warning: invalid stoichiometry in decompose()" << endl;
            return;
         }

         if (af_params.role[0].react[0] == 1)          // c1=reactant
         {
            C1[0][j] = 2.*uhat - C1[0][j];
            C1[1][j] = ct - C1[0][j];
         }
         else
         {
            C1[1][j] = 2.*uhat - C1[1][j];
            C1[0][j] = ct - C1[1][j];
         }
      }
      return;
   }

   // general cases
   unsigned int iter, iter_max = 20;       // maximum number of Newton iteration allowed
   double **A, *y0, *delta_n, *b, diff;
   double *y0_ref, *y1_ref, diff_ref;

   y0 = new double [num_comp];
   delta_n = new double [num_comp];
   b = new double [num_comp];
   initialize_2d(num_comp, num_comp, &A);

   y0_ref = new double [num_comp];      // reference y0
   y1_ref = new double [num_comp];      // reference y1 (no need recalculate y1, if y0 same)
   for(i=0;i<num_comp;i++) 
   {
      y0_ref[i] = 0.;
      y1_ref[i] = 0.;
   }
   
   //   for(i=0;i<num_comp;i++) A[i] = new double [num_comp];
   //   cout << "Npts: " << Npts << "num_comp: " << num_comp << endl;
   for (j=0; j<Npts; j++)
   {
      ct = 0.;
      diff_ref = 0.;
      for(i=0;i<num_comp;i++)
      {
         y0[i]=C1[i][j];
         delta_n[i]=0.;
         ct += fabs(y0[i]);
         diff_ref += fabs(y0_ref[i]-y0[i]);
      }

      if(diff_ref<ct*1.e-7 || diff_ref<1.e-9)
      {
         for(i=0;i<num_comp;i++) C1[i][j] = y1_ref[i];
      } 
      else 
      {

         for(iter=0; iter<iter_max;iter++)       // Newton iteration
         {
            for(i=0;i<num_comp;i++) y0[i]=C1[i][j]+delta_n[i];

            Reaction_dydt(y0, b);                  // b=dy/dt

            Reaction_dfdy(y0, A);                  // A=df/dy

            for(i=0;i<num_comp;i++)
            {
               for(k=0;k<num_comp;k++) A[i][k] *= (-TimeStep);
               A[i][i] += 2.;
               b[i] = -(2.*delta_n[i] - TimeStep * b[i]);
            }

            if( GaussElim(num_comp, A, b) ==-1 )
            {
               printf("Matrix singular in Reaction_Euler_imp: model 12\n");
               break;
            }
            else
            {
               diff = 0.;
               for(i=0;i<num_comp;i++)
               {
                  delta_n[i] += b[i];
                  diff += fabs(delta_n[i]);
               }
            }
            if( diff < 1.e-7*ct ) break;
         } // end of Newton iteration;
         for(i=0;i<num_comp;i++) 
         {
            y0_ref[i] = C1[i][j];
            C1[i][j] += delta_n[i];
            y1_ref[i] = C1[i][j];
         }

      } // if diff_ref

   } // end of j (pts)

   clear_2d(num_comp, A);
   delete [] y0_ref;
   delete [] y1_ref;
   delete [] b;
   delete [] delta_n;
   delete [] y0;
}


//
// given total concentration of a group of components involved,
// find the concentration of each component by equlibrium condition
//
void US_Astfem_RSA::decompose(struct mfem_initial *C0)
{
   unsigned int num_comp, Npts;
   unsigned int i, j;

   num_comp = af_params.role.size();
   Npts = C0[0].radius.size();   // note: all components must be defined on the same radial grids

   // special case:  self-association  n A <--> An
   if( num_comp==2 )              // only  2 components and one association rule
   {
      double c1, c2, ct, keq;
      unsigned int st0, st1;
      st0 = af_params.association[0].stoich[0];
      st1 = af_params.association[0].stoich[1];
      keq = af_params.association[0].keq;

      for (j=0; j<Npts; j++)
      {
         ct = C0[0].concentration[j] + C0[1].concentration[j] ;
         if (st0 ==2 && st1 == 1 )                // mono <--> dimer
         {
            c1 = (sqrt(1.0 + 4.0 * keq * ct) - 1.0)/(2.0 * keq) ;
         }
         else if (st0 ==3 && st1 == 1 )             // mono <--> trimer
         {
            c1 = cube_root(-ct/keq, 1.0/keq, 0.0);
         }
         else if (st0 > 3 && st1 == 1 )             // mono <--> n-mer
         {
            c1 = find_C1_mono_Nmer( st0, keq, ct );
         }
         else
         {
            cerr << "warning: invalid stoichiometry in decompose()" << endl;
            return;
         }
         c2 = keq * pow(c1, (double) st0 );

         if (af_params.role[0].react[0] == 1)          // c1=reactant
         {
            C0[0].concentration[j] = c1 ;
            C0[1].concentration[j] = c2 ;
         }
         else
         {
            C0[0].concentration[j] = c2 ;   // c1=product
            C0[1].concentration[j] = c1 ;
         }
      }
      return;
   }

   // general cases
   double **C1, **C2;                  // array for all component at all radius position
   double diff, ct ;

   initialize_2d(num_comp, Npts, &C1);
   initialize_2d(num_comp, Npts, &C2);

   for(i=0; i<num_comp; i++)
   {
      for(j=0; j<Npts; j++)
      {
         C1[i][j] = C0[i].concentration[j];
         C2[i][j] = C1[i][j];
      }
   }

   // estimate max time to reach equilibrium and suitable step size:
   // using e^{-k_min * N * dt ) < 1.e-7
   unsigned int time_max = 100 ;          // maximum number of time steps for reacing equlibrium
   double TimeStepSize ;                  // time step size
   double k_min = 1.e12;
   for(i=0; i<af_params.association.size(); i++)
   {
      if (k_min > af_params.association[i].k_off) k_min = af_params.association[i].k_off;
   }
   if(k_min<1.e-12) k_min=1.e-12;
   TimeStepSize = -log(1.e-7) / ( k_min * (double)time_max );

   // time loop
   for(unsigned int ti=0; ti<time_max; ti++)
   {
      ReactionOneStep_Euler_imp(Npts, C1, TimeStepSize);

      diff = 0.;
      ct = 0.;
      for(i=0; i<num_comp; i++)
      {
         for(j=0; j<Npts; j++)
         {
            diff += fabs( C2[i][j] - C1[i][j] );
            ct += fabs( C1[i][j] );
            C2[i][j] = C1[i][j];
         }
      }

      if(diff < 1.e-5*ct ) break;
   } // end time steps

   for(i=0; i<num_comp; i++)
   {
      for(j=0; j<Npts; j++)
      {
         C0[i].concentration[j] = C1[i][j];
      }
   }

   clear_2d(num_comp, C1);
   clear_2d(num_comp, C2);
}


void US_Astfem_RSA::Reaction_dydt(double *y, double *yt)
{
   unsigned int num_comp, num_rule;
   unsigned int i, m, n, ind_cn;
   double k1, k_1, stn, extn;
   double *Q, Q_reactant, Q_product;

   num_comp = rg[af_params.rg_index].GroupComponent.size();
   num_rule = rg[af_params.rg_index].association.size();
   Q = new double [num_rule];
   for(m=0; m<num_rule; m++)
   {
      k_1  = (double)af_params.association[m].k_off;
      k1   = (double)af_params.association[m].keq * k_1;
      Q_reactant = 1 ;
      Q_product  = 1;
      for(n=0;n<af_params.association[m].comp.size();n++)
      {
         // local index of the n-th component in assoc[rule]
         ind_cn = af_params.association[m].comp[n] ;
         // stoich of n-th comp in the rule
         stn = (double)af_params.association[m].stoich[n] ;
         // extinction coeffieint of n-th comp 
         extn = af_params.kext[ rg[af_params.rg_index].GroupComponent[ind_cn] ];		
         if( af_params.association[m].react[n] == 1 )       // comp[n] here is reactant
         {
            Q_reactant *= pow( y[ind_cn]/extn, stn );			
         }
         else                                              // comp[n] here is reactant
         {
            Q_product  *= pow( y[ind_cn]/extn, stn );		
         }
      }
      Q[m] = k1 * Q_reactant - k_1 * Q_product;
   }

   for(i=0; i<num_comp; i++)
   {
      yt[i] = 0.;
      for(m=0; m<af_params.role[i].assoc.size(); m++)
      {
         yt[i] += -af_params.role[i].react[m] * (double)af_params.role[i].st[m]
            * Q[ af_params.role[i].assoc[m] ];
      }
      yt[i] *= af_params.kext[ rg[af_params.rg_index].GroupComponent[i] ];		// convert into signal conc.

   }

   /*  print
       FILE *outf;
       if (guiFlag)
       {
       outf = fopen("tmp1.out","a");
       fprintf(outf, "y[i]\n");
       for(i=0; i<num_comp; i++) fprintf(outf, " %12.5e ", y[i]);
       fprintf(outf, "\n");
       fprintf(outf, "yt[i]\n");
       for(i=0; i<num_comp; i++) fprintf(outf, " %12.5e ", yt[i]);
       fprintf(outf, "\n");
       fclose(outf);
       }
   */

   delete [] Q;
}

void US_Astfem_RSA::Reaction_dfdy(double *y, double **dfdy)
{
   unsigned int num_comp, num_rule;
   unsigned int i, j, m, n, ind_cn;
   double k1, k_1, stn, extn;
   double **QC, Q_reactant, Q_product, deriv_r, deriv_p;

   num_comp = rg[af_params.rg_index].GroupComponent.size();
   num_rule = rg[af_params.rg_index].association.size();

   initialize_2d(num_rule, num_comp, &QC);

   for(m=0; m<num_rule; m++)
   {
      k_1  = (double)af_params.association[m].k_off;
      k1   = (double)af_params.association[m].keq * k_1;

      for(j=0; j<num_comp; j++)
      {
         Q_reactant = 1 ;
         Q_product  = 1;
         deriv_r = 0.;
         deriv_p = 0.;
         for(n=0;n<af_params.association[m].comp.size();n++)
         {
            // local index of the n-th component in assoc[rule]
            ind_cn = af_params.association[m].comp[n] ;
            // stoich of n-th comp in the rule
            stn = (double)af_params.association[m].stoich[n];
            // extinction coeffieint of n-th comp 
            extn = af_params.kext[ rg[af_params.rg_index].GroupComponent[ind_cn] ];		

            if( af_params.association[m].comp[n] == j )          // comp[j] is in the rule
            {
               if( af_params.association[m].react[n] == 1 )    // comp[n] is reactant
               {
                  deriv_r = stn / extn * pow( y[ind_cn]/extn, stn - 1. );		
               }
               else                                              // comp[n] in this rule is reactant
               {
                  deriv_p = stn / extn * pow( y[ind_cn]/extn, stn - 1. );	
               }
            }
            else                                                // comp[j] is not in the rule
            {
               if( af_params.association[m].react[n] == 1 )    // comp[n] is reactant
               {
                  Q_reactant *= pow( y[ind_cn]/extn, stn ); 	
               }
               else                                              // comp[n] in this rule is reactant
               {
                   Q_product  *= pow( y[ind_cn]/extn, stn ); 
               }
            }
         }
         QC[m][j] = k1 * Q_reactant * deriv_r - k_1 * Q_product * deriv_p;
      }  // C_j
   }    // m-rule


   for(i=0; i<num_comp; i++)
   {
      for(j=0; j<num_comp; j++)
      {
         dfdy[i][j] = 0.;
         for(m=0; m<af_params.role[i].assoc.size(); m++)
         {
            dfdy[i][j] += -af_params.role[i].react[m] * (double)af_params.role[i].st[m] * QC[ af_params.role[i].assoc[m] ][j];
         }
         dfdy[i][j] *= af_params.kext[ rg[af_params.rg_index].GroupComponent[i] ];		// convert into signal conc.
      }
   }

   /* print
      FILE *outf = fopen("tmp1.out","a");
      for(i=0; i<num_comp; i++)
      {
      for(j=0; j<num_comp; j++)
      {
      for(m=0; m<af_params.role[i].assoc.size(); m++)
      {
      fprintf(outf, "dfdy[%d][%d]: react=%d st=%d ru=%d\n", i, j,
      af_params.role[i].react[m], af_params.role[i].st[m],  af_params.role[i].assoc[m]);
      }
      }
      }
      fprintf(outf, "y[i]\n");
      for(i=0; i<num_comp; i++) fprintf(outf, " %12.5e ", y[i]);
      fprintf(outf, "\n");
      fprintf(outf, "matrix QC[m][j]\n");
      for(m=0; m<num_rule; m++) {
      for(j=0; j<num_comp; j++) fprintf(outf, " %12.5e ", QC[m][j]);
      fprintf(outf, "\n");
      }
      fprintf(outf, "matrix dfdy[i][j]\n");
      for(i=0; i<num_comp; i++) {
      for(j=0; j<num_comp; j++) fprintf(outf, " %12.5e ", dfdy[i][j]);
      fprintf(outf, "\n");
      }
      fprintf(outf, "\n");
      fclose(outf);
   */

   clear_2d(num_rule, QC);
}

void US_Astfem_RSA::adjust_limits(unsigned int speed)
{
   // first correct meniscus to theoretical position at rest:
   double stretch_val = stretch_with_rotor_list((*simparams).rotor, af_params.first_speed, rotor_list);
   //cout << "rotor: " << (*simparams).rotor << ", stretch: " << stretch_val << ", speed: " << speed  ;
   // this is the meniscus at rest
   af_params.current_meniscus = (*simparams).meniscus - stretch_val;
   //cout << ", 1st speed meniscus: " << (*simparams).meniscus << ", rest meniscus: " << af_params.current_meniscus;
   // calculate rotor stretch at current speed
   stretch_val = stretch_with_rotor_list((*simparams).rotor, speed, rotor_list);
   // add current stretch to meniscus at rest
   af_params.current_meniscus +=  stretch_val;
   // add current stretch to bottom at rest
   af_params.current_bottom = (*simparams).bottom + stretch_val;
#if defined(DEBUG)
   printf("adjust_limits af_params.current_bottom %f\n", af_params.current_bottom);
#endif
   //cout << ", corrected meniscus: " << af_params.current_meniscus << ", current_bottom: " << af_params.current_bottom << endl;
}

void US_Astfem_RSA::adjust_grid(unsigned int old_speed, unsigned int new_speed, vector <double> *radius)
{
   double stretch_val1 = stretch_with_rotor_list((*simparams).rotor, old_speed, rotor_list);
   double stretch_val2 = stretch_with_rotor_list((*simparams).rotor, new_speed, rotor_list);
   for (unsigned int i=0; i<(*radius).size(); i++)
   {
      (*radius)[i] += stretch_val2 - stretch_val1;
   }
}

void US_Astfem_RSA::setTimeCorrection(bool flag)
{
   time_correction = flag; // correct time for rotor acceleration
}

void US_Astfem_RSA::setTimeInterpolation(bool flag)
{
   use_time = flag; // use time interpolation if true, omega-square-t integral interpolation otherwise
}

void US_Astfem_RSA::setMovie(bool flag)
{
   movieFlag = flag; // plot movie
}

void US_Astfem_RSA::setStopFlag(bool flag)
{
   stopFlag = flag; // stop calculation
}

void US_Astfem_RSA::print_vector(double *dval, unsigned int ival)
{
   unsigned int i;
   for (i=0; i<ival; i++)
   {
      printf("x[%d]=%20.10e \n", i, dval[i]);
   }
}

void US_Astfem_RSA::print_vector(vector <double> *dval)
{
   unsigned int i;
   for (i=0; i<(*dval).size(); i++)
   {
      printf("x[%d]=%20.10e \n", i, (*dval)[i]);
   }
}

void US_Astfem_RSA::print_simparams()
{
}

void US_Astfem_RSA::print_rg()
{
   unsigned int i, k ;

   cout << "Reaction Group Info: " << endl;
   for (k=0; k< rg.size(); k++)
   {
      cout << "Group ["<< k << "] "<< endl;

      cout << "Assoc Rule: " ;
      for (i=0; i< rg[k].association.size(); i++)
      {
         cout << "[" << rg[k].association[i] << "] ";
      }
      cout << endl;

      cout << "Group Components: ";
      for (i=0; i< rg[k].GroupComponent.size(); i++)
      {
         cout << "[" << rg[k].GroupComponent[i] << "] ";
      }
      cout << endl;
   }
   cout << "End of Reaction Group Info" << endl;
}

void US_Astfem_RSA::print_af()
{
   unsigned int i, j;
   QString str;
   cout.precision(10);
   cout << "Simparams and system variables:\n";
   cout << "Model Number:\t" << (*system).model << endl;
   cout << "Rotor:\t" << (*simparams).rotor << endl;
   cout << "mesh:\t\t" << (*simparams).mesh << endl;
   cout << "moving grid\t\t" << (*simparams).moving_grid << endl;
   cout << "\n\naf_params variables:\n";
   cout << "First speed:\t" << af_params.first_speed << endl;
   cout << "Simpoints:\t" << af_params.simpoints << endl;
   cout << "\nHydrodynamic Parameters:\n";
   for (i=0; i< af_params.s.size(); i++)
   {
      cout << "s[" << i << "]:\t\t" << af_params.s[i] << endl;
      cout << "D[" << i << "]:\t\t" << af_params.D[i] << endl;
      cout << "ext[" << i << "]:\t\t" << af_params.kext[i] << endl;
   }
   cout << "\ndt:\t\t" << af_params.dt << endl;
   cout << "pathlength:\t" << af_params.pathlength << endl;
   cout << "time_steps:\t" << af_params.time_steps << endl;
   cout << "omega_s:\t" << af_params.omega_s << endl;
   cout << "start_time:\t" << af_params.start_time << endl;
   cout << "current meniscus:\t" << af_params.current_meniscus << endl;
   cout << "current bottom:\t\t" << af_params.current_bottom << endl;
   cout << "reaction group index:\t\t" << af_params.rg_index << endl;
   for (i=0; i< af_params.local_index.size(); i++)
   {
      cout << "local index[" << i << "]:\t\t" << af_params.local_index[i] << endl;
   }
   for (i=0; i< af_params.association.size(); i++)
   {
      cout << "\n\nAssociation " << i << ":\t\t\n";
      cout << "keq[" << i << "]:\t\t" << af_params.association[i].keq << endl;
      cout << "koff[" << i << "]:\t\t" << af_params.association[i].k_off << endl;
      cout << "units[" << i << "]:\t\t" << af_params.association[i].units << endl;
      cout << "component1[" << i << "]:\t\t" << af_params.association[i].component1 << endl;
      cout << "component2[" << i << "]:\t\t" << af_params.association[i].component2 << endl;
      cout << "component3[" << i << "]:\t\t" << af_params.association[i].component3 << endl;
      cout << "stoichiometry1[" << i << "]:\t\t" << af_params.association[i].stoichiometry1 << endl;
      cout << "stoichiometry2[" << i << "]:\t\t" << af_params.association[i].stoichiometry2 << endl;
      cout << "stoichiometry3[" << i << "]:\t\t" << af_params.association[i].stoichiometry3 << endl;
      for (j=0; j<af_params.association[i].comp.size(); j++)
      {
         cout << "component involved in this reaction[" << j << "]:\t\t" << af_params.association[i].comp[j] << endl;
      }
      for (j=0; j<af_params.association[i].stoich.size(); j++)
      {
         cout << "stoichiometry of component[" << j << "]:\t\t" << af_params.association[i].stoich[j] << endl;
      }
      for (j=0; j<af_params.association[i].react.size(); j++)
      {
         cout << "reaction position[" << j << "]:\t\t" << af_params.association[i].react[j] << endl;
      }
   }

}

void US_Astfem_RSA::print_af(FILE *outf)
{
   unsigned int i;

   fprintf(outf, "#####################################################\n");
   fprintf(outf, "#  \n");
   fprintf(outf, "#  Model Number: %d \n", (*system).model);
   fprintf(outf, "#  Number of species = %d\n", (int)af_params.s.size() );
   for (i=0; i<af_params.s.size(); i++)
   {
      fprintf(outf, "#  s[%d]=%20.12e D[%d]=%20.12e \n", i, af_params.s[i], i, af_params.D[i]);
   }
   fprintf(outf, "#  \n");
   fprintf(outf, "#  parameters for simulation:\n");
   fprintf(outf, "#  current meniscus =%12.5e \n",  af_params.current_meniscus);
   fprintf(outf, "#  current bottom =%12.5e \n",  af_params.current_bottom);
   fprintf(outf, "#  start time =%12.5e \n",  af_params.start_time);
   fprintf(outf, "#  mesh opt =%d \n",  (*simparams).mesh);
   if ((*simparams).moving_grid) fprintf(outf, "#  grids = moving \n");
   else fprintf(outf, "#  grids = fixed \n");
   fprintf(outf, "#  simpoints =%d \n",  af_params.simpoints);
   fprintf(outf, "#  dt =%12.5e \n",  af_params.dt);
   fprintf(outf, "#  Total Number of Steps =%d \n",  af_params.time_steps);

   fprintf(outf, "#  \n");
   fprintf(outf, "#####################################################\n");

   return;
}

