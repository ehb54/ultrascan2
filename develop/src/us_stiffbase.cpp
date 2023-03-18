#include "../include/us_stiffbase.h"
//
// numerical integration over a quadrilateral
//
void StiffBase::CompLocalStif(int NK,  double xd[4][2], double D, double sw2, double **Stif)
{
   int i, j, k, n_gauss;
   double xg[2], jcbv[5], wt;
   double phi[4], phix[4], phiy[4], tmp;

   n_gauss = (NK==3)? n_gaussT:n_gaussQ;

   for(i=0; i<4; i++) {
      for(j=0; j<4; j++) {
         Stif[i][j] = 0.;
      }
   }

   for(k=0;k<n_gauss; k++) {
      
      AffineMapping(NK, xd, k, xg);
      Jacobian(NK, xd, k, jcbv) ;
      wt = (NK==3)? xgT[k].w : xgQ[k].w;

      if(NK==3) {
         for(i=0;i<NK;i++) {
            phi[i] = phiT[k][i];
            phix[i] = phiT1[k][i] * jcbv[1] + phiT2[k][i] * jcbv[3];
            phiy[i] = phiT1[k][i] * jcbv[2] + phiT2[k][i] * jcbv[4];
         }
      } else {
         for(i=0;i<NK;i++) {
            phi[i] = phiQ[k][i];
            phix[i] = phiQ1[k][i] * jcbv[1] + phiQ2[k][i] * jcbv[3];
            phiy[i] = phiQ1[k][i] * jcbv[2] + phiQ2[k][i] * jcbv[4];
         }
      }

      for(i=0; i<NK; i++) {
         for(j=0; j<NK; j++) {
            tmp = xg[0]*phiy[j]*phi[i] + ( D*xg[0]*phix[j] - sw2 * xg[0]*xg[0]*phi[j] ) * phix[i];
            Stif[j][i] += tmp * jcbv[0] * wt;
         }
      }

   } // k

}

// END



StiffBase::StiffBase()
{  
   n_gaussT = 28;
   n_basisT = 3; 
   xgT = new Gauss2D [n_gaussT];
   phiT  = new double * [n_gaussT];
   phiT1 = new double * [n_gaussT];
   phiT2 = new double * [n_gaussT];
   for(int i=0;i<n_gaussT; i++) {
      phiT[i]  = new double [n_basisT];
      phiT1[i] = new double [n_basisT];
      phiT2[i] = new double [n_basisT];
   }

   n_gaussQ = 25;
   n_basisQ = 4; 
   xgQ = new Gauss2D [n_gaussQ];
   phiQ  = new double * [n_gaussQ];
   phiQ1 = new double * [n_gaussQ];
   phiQ2 = new double * [n_gaussQ];
   for(int i=0;i<n_gaussQ; i++) {
      phiQ[i]  = new double [n_basisQ];
      phiQ1[i] = new double [n_basisQ];
      phiQ2[i] = new double [n_basisQ];
   }

   SetGauss();
   LinearBasis();

}

StiffBase::~StiffBase()
{  
   delete [] xgT;
   for(int i=0;i<n_gaussT; i++) {
      delete [] phiT[i];
      delete [] phiT1[i];
      delete [] phiT2[i];
   }
   delete [] phiT ;
   delete [] phiT1;
   delete [] phiT2;


   delete [] xgQ;
   for(int i=0;i<n_gaussQ; i++) {
      delete [] phiQ[i];
      delete [] phiQ1[i];
      delete [] phiQ2[i];
   }
   delete [] phiQ ;
   delete [] phiQ1;
   delete [] phiQ2;
}



void StiffBase::LambdaG(unsigned int kk, double lam1, double lam2, double w, Gauss2D *Lm)
{
   double lam3;
   lam3 = 1.0 - lam1 - lam2;
   switch (kk)
   {
   case 1:  // one point
      {
         Lm[0].x = lam1;
         Lm[0].y = lam1;
         Lm[0].w = w;
         break;
      }
   case 3:  // 3 points
      {
         Lm[0].x = lam1;
         Lm[0].y = lam2;
         Lm[0].w = w;

         Lm[1].x = lam3;
         Lm[1].y = lam1;
         Lm[1].w = w;

         Lm[2].x = lam2;
         Lm[2].y = lam3;
         Lm[2].w = w;
         break;
      }
   case 6:  // 6 points
      {
         Lm[0].x = lam1;
         Lm[0].y = lam2;
         Lm[0].w = w;

         Lm[1].x = lam1;
         Lm[1].y = lam3;
         Lm[1].w = w;

         Lm[2].x = lam2;
         Lm[2].y = lam1;
         Lm[2].w = w;

         Lm[3].x = lam2;
         Lm[3].y = lam3;
         Lm[3].w = w;

         Lm[4].x = lam3;
         Lm[4].y = lam1;
         Lm[4].w = w;

         Lm[5].x = lam3;
         Lm[5].y = lam2;
         Lm[5].w = w;
         break;
      }
   default:
      {
         return;
      }
   }
}


void StiffBase::SetGauss() 
{
   // set Gass Quadrature points on Quad elem [0,1]x[0,1]
   switch(n_gaussQ) {
   case 25:
      int ind;
      double Gs1D[5], Gs1w[5];   // Gauss pts and weights in 1D
      Gs1D[0] = -sqrt(5.+2.*sqrt(10./7.))/3.; 
      Gs1w[0] = 0.3*(0.7+5.*sqrt(0.7))/(2.+5.*sqrt(0.7));
      Gs1D[1] = -sqrt(5.-2.*sqrt(10./7.))/3.; 
      Gs1w[1] = 0.3*(0.7-5.*sqrt(0.7))/(2.-5.*sqrt(0.7));
      Gs1D[2] =  0.0;
      Gs1w[2] = 128./225.;
      Gs1D[3] = -Gs1D[1];                
      Gs1w[3] =  Gs1w[1];
      Gs1D[4] = -Gs1D[0];                
      Gs1w[4] =  Gs1w[0];
      for(int i=0; i<5; i++) {
         for(int j=0; j<5; j++) {
            ind = j + 5*i;
            xgQ[ind].x = (Gs1D[i] +1.0)/2.;
            xgQ[ind].y = (Gs1D[j] +1.0)/2.;
            xgQ[ind].w = Gs1w[i] * Gs1w[j] / 4.;
         }
      }
      break;
       
   default:
      printf("Gauss Quadrature on Quad elem with %d points not implemented\n", n_gaussQ );
      return;
   }

   // set Gass Quadrature points on Triangular elem [0,1]x[0,1]
   switch(n_gaussT) {
   case 28:
      double lam1, lam2, w;
      lam1=0.3333333333;
      lam2=0.3333333333;
      w= 0.2178563571;
      LambdaG(1, lam1, lam2, w/4, xgT);

      lam1=0.1063354684;
      lam2=0.1063354684;
      w= 0.1104193374;
      LambdaG(3, lam1, lam2, w/4, xgT+1);

      lam1=0.0000000000;
      lam2=0.5000000002;
      w= 0.0358939762;
      LambdaG(3, lam1, lam2, w/4, xgT+4);

      lam1=0.0000000000;
      lam2=0.0000000000;
      w= 0.0004021278;
      LambdaG(3, lam1, lam2, w/4, xgT+7);

      lam1=0.1171809171;
      lam2=0.3162697959;
      w= 0.1771348660;
      LambdaG(6, lam1, lam2, w/4, xgT+10);

      lam1=0.0000000000;
      lam2=0.2655651402;
      w= 0.0272344079;
      LambdaG(6, lam1, lam2, w/4, xgT+16);

      lam1=0.0000000000;
      lam2=0.0848854223;
      w= 0.0192969460;
      LambdaG(6, lam1, lam2, w/4, xgT+22);

      break;
       
   default:
      printf("Gauss Quadrature on Triangular elem with %d points not implemented\n", n_gaussT );
      return;
   }
}


void StiffBase::AffineMapping(int NK, double xd[4][2], int gauss_ind, double x[2]) 
{
   x[0] = 0.;
   x[1] = 0.;
   if(NK==3) {
      for(int i=0; i<3; i++) {   //  triangular element
         x[0] += xd[i][0] * phiT[gauss_ind][i];
         x[1] += xd[i][1] * phiT[gauss_ind][i];
      }
   } else {   // quadrilateral element
      for(int i=0; i<4; i++) {
         x[0] += xd[i][0] * phiQ[gauss_ind][i];
         x[1] += xd[i][1] * phiQ[gauss_ind][i];
      }
   }
}

 

void StiffBase::Jacobian(int NK, double xd[4][2], int gauss_ind, double jcbv[5]) 
{
   double J11, J12, J21, J22;    // jcb = d_x/d_xi

   J11 = 0.; J12 = 0.; J21 = 0.; J22 = 0.;
    
   if(NK==3) {
      for(int i=0; i<3; i++) {   //  triangular element
         J11 += xd[i][0] * phiT1[gauss_ind][i];
         J12 += xd[i][0] * phiT2[gauss_ind][i];
         J21 += xd[i][1] * phiT1[gauss_ind][i];
         J22 += xd[i][1] * phiT2[gauss_ind][i];
      }
   } else {   // quadrilateral element
      for(int i=0; i<4; i++) {
         J11 += xd[i][0] * phiQ1[gauss_ind][i];
         J12 += xd[i][0] * phiQ2[gauss_ind][i];
         J21 += xd[i][1] * phiQ1[gauss_ind][i];
         J22 += xd[i][1] * phiQ2[gauss_ind][i];
      }
   }
   jcbv[0] =  J11*J22 - J12*J21;    // = det(jcb)
   jcbv[1] =  J22 / jcbv[0];      // = d_xi / d_x
   jcbv[2] = -J12 / jcbv[0];      // = d_xi / d_y
   jcbv[3] = -J21 / jcbv[0];      // = d_et / d_x
   jcbv[4] =  J11 / jcbv[0];      // = d_et / d_y
}



void StiffBase::LinearBasis()
{
   double x, y;

   //
   // Linear basis for triangular element: (0,0), (1,0), (0,1)
   //
   for(int i=0; i<n_gaussT; i++) {
      x = xgT[i].x;
      y = xgT[i].y;

      phiT [i][0] = 1.0 - x - y;
      phiT1[i][0] = -1.0;
      phiT2[i][0]= -1.0;
   
      phiT [i][1] = x;
      phiT1[i][1] = 1.0;
      phiT2[i][1] = 0.0;
   
      phiT [i][2] = y;
      phiT1[i][2] = 0.0;
      phiT2[i][2] = 1.0;

   }

   //
   // Linear basis for Quadrilateral element: (0,0), (1,0), (1,1), (0,1)
   //
   for(int i=0; i<n_gaussQ; i++) {
      x = xgQ[i].x;
      y = xgQ[i].y;

      phiQ [i][0] = (1.-x) * (1.-y);
      phiQ1[i][0] = -(1.-y);
      phiQ2[i][0] = -(1.-x);
      
      phiQ [i][1] = x * (1.-y);
      phiQ1[i][1] = 1.-y;
      phiQ2[i][1] = -x;

      phiQ [i][2] = x * y;
      phiQ1[i][2] = y;
      phiQ2[i][2] = x;

      phiQ [i][3] = (1.- x) * y;
      phiQ1[i][3] = -y;
      phiQ2[i][3] = 1.-x;
   }
}
