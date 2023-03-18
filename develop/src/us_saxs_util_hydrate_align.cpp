#include "../include/us_saxs_util.h"
#include "../include/us_tnt_jama.h"

// note: this program uses cout and/or cerr and this should be replaced

// #define DEBUG_ALIGN

bool US_Saxs_Util::atom_align( vector < point > transform_from, 
                               vector < point > transform_to, 
                               vector < point > apply_from, 
                               vector < point > &result )
{
   errormsg = "";
#if defined( DEBUG_ALIGN )
   // for testing:
   vector < point > save_from = transform_from; 
#endif

   if ( transform_from.size() < 3 )
   {
      errormsg = "At least 3 points are needed to align atoms";
      return false;
   }
   if ( transform_from.size() != transform_to.size() )
   {
      errormsg = QString( "Size of transform from (%1) does not equal size of transform to (%2)" )
         .arg( transform_from.size() )
         .arg( transform_to.size() );
      return false;
   }
   
   point center_from = transform_from[ 0 ];
   point center_to = transform_to[ 0 ];

   for ( unsigned int i = 1; i < transform_from.size(); i++ )
   {
      for ( unsigned int j = 0; j < 3; j++ )
      {
         center_from.axis[ j ] += transform_from[ i ].axis[ j ];
         center_to.axis  [ j ] += transform_to  [ i ].axis[ j ];
      }
   }
   for ( unsigned int j = 0; j < 3; j++ )
   {
      center_from.axis[ j ] /= transform_from.size();
      center_to.axis  [ j ] /= transform_to.size();
   }

   for ( unsigned int i = 0; i < transform_from.size(); i++ )
   {
      for ( unsigned int j = 0; j < 3; j++ )
      {
         transform_from[ i ].axis[ j ] -= center_from.axis[ j ];
         transform_to  [ i ].axis[ j ] -= center_to  .axis[ j ];
      }
   }

#if defined( DEBUG_ALIGN )
   cout << "center point from " << center_from << endl;
   cout << "center point to   " << center_to << endl;
#endif

   TNT::Array2D < float > H( 3, 3 );

   for ( int i = 0; i < 3; i++ )
   {
      for ( int j = 0; j < 3; j++ ) 
      {
         H[ i ][ j ] = 0.0f;
      }
   }

   for ( unsigned int i = 0; i < transform_from.size(); i++ ) 
   {
      for ( int j = 0; j < 3; j++ ) 
      {
         for ( int k = 0; k < 3; k++ ) 
         {
            H[ j ][ k ] +=  transform_from[ i ].axis[ j ] * transform_to[ i ].axis[ k ];
         }
      }
   }

   JAMA::SVD < float >    svd( H );
   TNT::Array2D < float > U(3, 3);
   TNT::Array2D < float > V(3, 3);
   svd.getU( U );
   svd.getV( V );

   // the rotation matrix is R = VU^T
   TNT::Array2D < float > UT = transpose( U );
   TNT::Array2D < float > rot(3, 3);
   rot = matmult( V, UT );

   // check for reflection
   if ( det( rot ) < 0) {
      TNT::Array2D < float > VT = transpose( V );
      TNT::Array2D < float > UVT = matmult( U, VT );
      TNT::Array2D < float > S( 3, 3 );
      S[ 0 ][ 0 ] = S[ 1 ][ 1 ] = 1.0;
      S[ 2 ][ 2 ] = det( UVT );
      S[ 0 ][ 1 ] = 
         S[ 0 ][ 2 ] = 
         S[ 1 ][ 0 ] = 
         S[ 1 ][ 2 ] = 
         S[ 2 ][ 0 ] = 
         S[ 2 ][ 1 ] = 0.0;
      rot = matmult( matmult( U, S ), VT );
   }

#if defined( DEBUG_ALIGN )
   cout << "rotation matrix: ";
   for ( int i = 0; i < 3; i++ )
   {
      for ( int j = 0; j < 3; j++ )
      {
         cout << QString(" %1").arg( rot[ i ][ j ] );
      }
      cout << "\n";
   }
   cout << "\n";
#endif

   // compute translation:

   point trans = center_to;

   for ( int i = 0; i < 3; i++ )
   {
      for ( int j = 0; j < 3; j++ )
      {
         trans.axis[ i ] -= rot[ i ][ j ] * center_from.axis[ j ];
      }
   }

#if defined( DEBUG_ALIGN )
   cout << "trans:\n" << trans << endl;

   // test transformation on transform_from:

   cout << "test on transform_from:\n";
   vector < point > test_to( save_from.size() );
   for ( int i = 0; i < save_from.size(); i++ )
   {
      for ( int j = 0; j < 3; j++ )
      {
         test_to[ i ].axis[ j ] = trans.axis[ j ];
         for ( int k = 0; k < 3; k++ )
         {
            test_to[ i ].axis[ j ] += rot[ j ][ k ] * save_from[ i ].axis[ k ];
         }
      }
      cout << test_to[ i ] << endl;
   }
#endif

   // apply translations to results

   result.resize( apply_from.size() );

   for ( unsigned int i = 0; i < apply_from.size(); i++ )
   {
      for ( int j = 0; j < 3; j++ )
      {
         result[ i ].axis[ j ] = trans.axis[ j ];
         for ( int k = 0; k < 3; k++ )
         {
            result[ i ].axis[ j ] += rot[ j ][ k ] * apply_from[ i ].axis[ k ];
         }
      }
   }      

   return true;
}
