/****************************************************************************
 *
 * $Id: vpFeatureBuilderEllipse.cpp,v 1.6 2006-06-23 14:45:06 brenier Exp $
 *
 * Copyright (C) 1998-2006 Inria. All rights reserved.
 *
 * This software was developed at:
 * IRISA/INRIA Rennes
 * Projet Lagadic
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * http://www.irisa.fr/lagadic
 *
 * This file is part of the ViSP toolkit.
 *
 * This file may be distributed under the terms of the Q Public License
 * as defined by Trolltech AS of Norway and appearing in the file
 * LICENSE included in the packaging of this file.
 *
 * Licensees holding valid ViSP Professional Edition licenses may
 * use this file in accordance with the ViSP Commercial License
 * Agreement provided with the Software.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Contact visp@irisa.fr if any conditions of this licensing are
 * not clear to you.
 *
 * Description:
 * Conversion between tracker and visual feature ellipse.
 *
 * Authors:
 * Eric Marchand
 *
 *****************************************************************************/


/*!
  \file vpFeatureBuilderEllipse.cpp
  \brief  conversion between tracker
  and visual feature Ellipse
*/

#include <visp/vpFeatureBuilder.h>


#include <visp/vpMath.h>

// create vpFeatureEllipse feature
void vpFeatureBuilder::create(vpFeatureEllipse &s, const vpCircle &t )
{
  try
  {

    // 3D data
    double alpha = t.cP[0] ;
    double beta = t.cP[1] ;
    double gamma = t.cP[2] ;

    double X0 = t.cP[3] ;
    double Y0 = t.cP[4] ;
    double Z0 = t.cP[5] ;

    // equation p 318 prior eq (39)
    double d = alpha*X0 + beta*Y0 + gamma*Z0 ;

    double A = alpha / d ;
    double B = beta / d ;
    double C = gamma / d ;

    s.setABC(A,B,C) ;


    //2D data
    s.buildFrom( t.p[0],  t.p[1],  t.p[2],  t.p[3],  t.p[4] ) ;

  }
  catch(...)
  {
    vpERROR_TRACE(" ") ;
    throw ;
  }

}
void vpFeatureBuilder::create(vpFeatureEllipse &s,  const vpSphere &t)
{
  try
  {

    // 3D data
    double X0 = t.cP[0] ;
    double Y0 = t.cP[1] ;
    double Z0 = t.cP[2] ;
    double R = t.cP[3] ;

    double d = vpMath::sqr(X0) + vpMath::sqr(Y0) + vpMath::sqr(Z0) -
      vpMath::sqr(R) ;


    double A = X0 / d ;
    double B = Y0 / d ;
    double C = Z0 / d ;

    s.setABC(A,B,C) ;

    //2D data
    s.buildFrom( t.p[0],  t.p[1],  t.p[2],  t.p[3],  t.p[4] ) ;


  }
  catch(...)
  {
    vpERROR_TRACE(" ") ;
    throw ;
  }
}

// create vpFeatureEllipse feature
void vpFeatureBuilder::create(vpFeatureEllipse &s,
			      const vpCameraParameters &cam,
			      const vpDot &t )
{
  try
  {

    int order = 3 ;
    vpMatrix mp(order,order) ; mp =0 ;
    vpMatrix m(order,order) ; m = 0 ;

    mp[0][0] = t.m00 ;
    mp[1][0] = t.m10;
    mp[0][1] = t.m01 ;
    mp[2][0] = t.m20 ;
    mp[1][1] = t.m11 ;
    mp[0][2] = t.m02 ;

    vpPixelMeterConversion::convertMoment(cam,order,mp,m) ;

    double  m00 = m[0][0] ;
    double  m01 = m[0][1] ;
    double  m10 = m[1][0] ;
    double  m02 = m[0][2] ;
    double  m11 = m[1][1] ;
    double  m20 = m[2][0] ;

    double xc = m10/m00 ; // sum j /S
    double yc = m01/m00 ; // sum i /S

    double mu20 = 4*(m20 - m00*vpMath::sqr(xc))/(m00) ;
    double mu02 = 4*(m02 - m00*vpMath::sqr(yc))/(m00) ;
    double mu11 = 4*(m11 - m00*xc*yc)/(m00) ;

    s.buildFrom(xc, yc,  mu20, mu11, mu02  ) ;
  }
  catch(...)
  {
    vpERROR_TRACE(" ") ;
    throw ;
  }

}

// create vpFeatureEllipse feature
void vpFeatureBuilder::create(vpFeatureEllipse &s,
			      const vpCameraParameters &cam,
			      const vpDot2 &t )
{
  try
  {

    int order = 3 ;
    vpMatrix mp(order,order) ; mp =0 ;
    vpMatrix m(order,order) ; m = 0 ;

    mp[0][0] = t.m00 ;
    mp[1][0] = t.m10;
    mp[0][1] = t.m01 ;
    mp[2][0] = t.m20 ;
    mp[1][1] = t.m11 ;
    mp[0][2] = t.m02 ;

    vpPixelMeterConversion::convertMoment(cam,order,mp,m) ;

    double  m00 = m[0][0] ;
    double  m01 = m[0][1] ;
    double  m10 = m[1][0] ;
    double  m02 = m[0][2] ;
    double  m11 = m[1][1] ;
    double  m20 = m[2][0] ;

    double xc = m10/m00 ; // sum j /S
    double yc = m01/m00 ; // sum i /S

    double mu20 = 4*(m20 - m00*vpMath::sqr(xc))/(m00) ;
    double mu02 = 4*(m02 - m00*vpMath::sqr(yc))/(m00) ;
    double mu11 = 4*(m11 - m00*xc*yc)/(m00) ;

    s.buildFrom(xc, yc,  mu20, mu11, mu02  ) ;
  }
  catch(...)
  {
    vpERROR_TRACE(" ") ;
    throw ;
  }

}
/*
 * Local variables:
 * c-basic-offset: 2
 * End:
 */
