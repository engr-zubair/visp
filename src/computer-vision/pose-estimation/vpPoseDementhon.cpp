/****************************************************************************
 *
 * $Id: vpPoseDementhon.cpp,v 1.6 2006-06-23 14:45:05 brenier Exp $
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
 * This file is part of the ViSP toolkit
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
 * Pose computation.
 *
 * Authors:
 * Eric Marchand
 * Francois Chaumette
 *
 *****************************************************************************/



#include <visp/vpPose.h>
#include <visp/vpMath.h>

#define DEBUG_LEVEL1 0
#define DEBUG_LEVEL2 0
#define DEBUG_LEVEL3 0

/* FC
#ifndef DEG
#define DEG (180.0/M_PI)
#endif
*/

/*!
  \brief  Compute the pose using Dementhon approach for non planar objects
          this is a direct implementation of the algorithm proposed by
	  Dementhon and Davis in their 1995 paper.

	  D. Dementhon, L. Davis. --
	  Model-based object pose in 25 lines of codes. --
	  Int. J. of  Computer Vision, 15:123--141, 1995.
*/

void
vpPose::poseDementhonNonPlan(vpHomogeneousMatrix &cMo)
{

  int i ;
  double normI = 0., normJ = 0.;
  double Z0 = 0.;
  double seuil=1.0;
  double f=1.;

  //  CPoint c3d[npt] ;


  if (c3d !=NULL) delete [] c3d ;
  c3d = new vpPoint[npt] ;

  vpPoint p0 ;
  listP.front() ;
  p0 = listP.value() ;


  vpPoint P ;
  listP.front() ;
  i=0 ;
  while (!listP.outside())
  {
    P= listP.value() ;
    c3d[i] = P ;
    c3d[i].set_oX(P.get_oX()-p0.get_oX()) ;
    c3d[i].set_oY(P.get_oY()-p0.get_oY()) ;
    c3d[i].set_oZ(P.get_oZ()-p0.get_oZ()) ;
    i++  ;
    listP.next() ;
  }

  vpMatrix a ;
  try{
    a.resize(npt,3) ;
  }
  catch(...)
  {
    vpERROR_TRACE(" ") ;
    throw ;
  }


  for (i=0 ; i < npt ; i++)
  {
    a[i][0]=c3d[i].get_oX();
    a[i][1]=c3d[i].get_oY();
    a[i][2]=c3d[i].get_oZ();
  }


  //cout << a << endl ;
  // calcul a^T a
  vpMatrix ata ;
  ata = a.t()*a ;

  // calcul (a^T a)^-1 par decomposition LU
  vpMatrix ata1 ;
  ata1 = ata.pseudoInverse(1e-6) ; //InverseByLU() ;


  vpMatrix b ;
  b = (a*ata1).t() ;


  if (DEBUG_LEVEL2)
  {
    cout << "a" << endl <<a<<endl ;
    cout << "ata" << endl <<ata<<endl ;
    cout << "ata1" << endl <<ata1<<endl ;
    cout<< " ata*ata1"  << endl <<  ata*ata1 ;
    cout<< " b"  << endl <<  (a*ata1).t() ;

  }

  // calcul de la premiere solution

  vpColVector eps(npt) ;
  eps =0 ;


  int cpt = 0 ;
  vpColVector I, J, k ;
  try{
    I.resize(3) ;
      }
  catch(...)
  {
    vpERROR_TRACE(" ") ;
    throw ;
  }
  try{
    J.resize(3) ;
  }
  catch(...)
  {
    vpERROR_TRACE(" ") ;
    throw ;
  }


  try {
    k.resize(3) ;
  }
  catch(...)
  {
    vpERROR_TRACE(" ") ;
    throw ;
  }

  while(cpt < 20)
  {
    I = 0 ;
    J = 0 ;

    vpColVector xprim(npt) ;
    vpColVector yprim(npt) ;
    for (i=0;i<npt;i++)
    {
      xprim[i]=(1+ eps[i])*c3d[i].get_x() - c3d[0].get_x();
      yprim[i]=(1+ eps[i])*c3d[i].get_y() - c3d[0].get_y();
    }
    I = b*xprim ;
    J = b*yprim ;
    normI = sqrt(I.sumSquare()) ;
    normJ = sqrt(J.sumSquare()) ;
    I = I/normI ;
    J = J/normJ ;

    if (normI+normJ < 1e-10)
    {
      vpERROR_TRACE(" normI+normJ = 0, division par zero " ) ;
      throw(vpException(vpException::divideByZeroError,
			"division by zero  ")) ;
    }

    k = vpColVector::cross(I,J) ;
    Z0=2*f/(normI+normJ);
    cpt=cpt+1; seuil=0.0;
    for(i=0;i<npt;i++)
    {
      double      epsi_1 = eps[i] ;
      eps[i]=(c3d[i].get_oX()*k[0]+c3d[i].get_oY()*k[1]+c3d[i].get_oZ()*k[2])/Z0;
      seuil+=fabs(eps[i]-epsi_1);
    }
    if (npt==0)
    {
      vpERROR_TRACE( " npt = 0, division par zero ");
      throw(vpException(vpException::divideByZeroError,
			"division by zero  ")) ;
    }
    seuil/=npt;
  }
  k.normalize();
  J = vpColVector::cross(k,I) ;
  /*matrice de passage*/


  cMo[0][0]=I[0];
  cMo[0][1]=I[1];
  cMo[0][2]=I[2];
  cMo[0][3]=c3d[0].get_x()*2/(normI+normJ);

  cMo[1][0]=J[0];
  cMo[1][1]=J[1];
  cMo[1][2]=J[2];
  cMo[1][3]=c3d[0].get_y()*2/(normI+normJ);

  cMo[2][0]=k[0];
  cMo[2][1]=k[1];
  cMo[2][2]=k[2];
  cMo[2][3]=Z0;

  cMo[0][3] -= (p0.get_oX()*cMo[0][0]+p0.get_oY()*cMo[0][1]+p0.get_oZ()*cMo[0][2]);
  cMo[1][3] -= (p0.get_oX()*cMo[1][0]+p0.get_oY()*cMo[1][1]+p0.get_oZ()*cMo[1][2]);
  cMo[2][3] -= (p0.get_oX()*cMo[2][0]+p0.get_oY()*cMo[2][1]+p0.get_oZ()*cMo[2][2]);

  delete [] c3d ; c3d = NULL ;
}


#define DMIN		0.01    /* distance min entre la cible et la camera */
#define EPS		0.0000001
#define EPS_DEM		0.001

static void
calculRTheta(double s, double c, double &r, double &theta)
{
  if ((fabs(c) > EPS_DEM) || (fabs(s) > EPS_DEM))
  {
    r = sqrt(sqrt(s*s+c*c));
    theta = atan2(s,c)/2.0;
  }
  else
  {
    if (fabs(c) > fabs(s))
    {
      r = fabs(c);
      if (c >= 0.0)
	theta = M_PI/2;
      else
	theta = -M_PI/2;
    }
    else
    {
      r = fabs(s);
      if (s >= 0.0)
	theta = M_PI/4.0;
      else
	theta = -M_PI/4.0;
    }
  }
}

static
void calculSolutionDementhon(double xi0, double yi0,
			     vpColVector &I, vpColVector &J,
			     vpHomogeneousMatrix &cMo )
{

  if (DEBUG_LEVEL1)
    cout << "begin (Dementhon.cc)CalculSolutionDementhon() " << endl;

  double normI, normJ, normk, Z0;
  vpColVector  k(3);

  // normalisation de I et J
  normI = sqrt(I.sumSquare()) ;
  normJ = sqrt(J.sumSquare()) ;

  I/=normI;
  J/=normJ;


  k = vpColVector::cross(I,J) ; // k = I^I

  Z0=2.0/(normI+normJ);

  normk = sqrt(k.sumSquare()) ;
  k /= normk ;

  J = vpColVector::cross(k,I) ;

  //calcul de la matrice de passage
  cMo[0][0]=I[0];
  cMo[0][1]=I[1];
  cMo[0][2]=I[2];
  cMo[0][3]=xi0*Z0;

  cMo[1][0]=J[0];
  cMo[1][1]=J[1];
  cMo[1][2]=J[2];
  cMo[1][3]=yi0*Z0;

  cMo[2][0]=k[0];
  cMo[2][1]=k[1];
  cMo[2][2]=k[2];
  cMo[2][3]=Z0;


  if (DEBUG_LEVEL1)
    cout << "end (Dementhon.cc)CalculSolutionDementhon() " << endl;

}

int
vpPose::calculArbreDementhon(vpMatrix &b, vpColVector &U,
			     vpHomogeneousMatrix &cMo)
{

  if (DEBUG_LEVEL1)
    cout << "begin vpPose::CalculArbreDementhon() " << endl;

  int i, k;
  int erreur = 0;
  int cpt;
  double s,c,si,co;
  double smin,smin_old, s1,s2;
  double r, theta;
  vpHomogeneousMatrix  cMo1,cMo2,cMo_old;

  int iter_max = 20;
  vpMatrix eps(iter_max+1,npt) ;


  // on test si tous les points sont devant la camera
  for(i = 0; i < npt; i++)
  {
    double z ;
    z = cMo[2][0]*c3d[i].get_oX()+cMo[2][1]*c3d[i].get_oY()+cMo[2][2]*c3d[i].get_oZ() + cMo[2][3];
    if (z <= 0.0) erreur = -1;
  }

  smin = sqrt(computeResidualDementhon(cMo)/npt)  ;

  vpColVector xi(npt) ;
  vpColVector yi(npt) ;

  if (erreur==0)
  {
    k=0;
    for(i = 0; i < npt; i++)
    {
      xi[k] = c3d[i].get_x();
      yi[k] = c3d[i].get_y();

      if (k != 0)
      { // On ne prend pas le 1er point
	eps[0][k] = (cMo[2][0]*c3d[i].get_oX() +
		     cMo[2][1]*c3d[i].get_oY() +
		     cMo[2][2]*c3d[i].get_oZ())/cMo[2][3];
      }
      k++;
    }


    vpColVector I0(3) ;
    vpColVector J0(3) ;
    vpColVector I(3) ;
    vpColVector J(3) ;

    vpHomogeneousMatrix cMo_old ;
    smin_old = 2*smin ;

    cpt = 0;
    while ((cpt<20) && (smin_old > 0.01) && (smin <= smin_old))
    {
      if (DEBUG_LEVEL2)
      {
	cout << "cpt " << cpt << endl ;
	cout << "smin_old " << smin_old << endl ;
	cout << "smin " << smin << endl ;
      }

      smin_old = smin;
      cMo_old = cMo;

      I0 = 0 ;
      J0 = 0 ;

      for (i=1;i<npt;i++)
      {
	s = (1.0+eps[cpt][i])*xi[i] - xi[0];
	I0[0] += b[0][i-1] * s;
	I0[1] += b[1][i-1] * s;
	I0[2] += b[2][i-1] * s;
	s = (1.0+eps[cpt][i])*yi[i] - yi[0];
	J0[0] += b[0][i-1] * s;
	J0[1] += b[1][i-1] * s;
	J0[2] += b[2][i-1] * s;
      }

      s = -2.0*(vpColVector::dotProd(I0,J0));
      c = J0.sumSquare() - I0.sumSquare() ;

      calculRTheta(s,c,r,theta);
      co = cos(theta);
      si = sin(theta);

      /* 1ere branche	*/
      I = I0 + U*r*co ;
      J = J0 + U*r*si ;

      if (DEBUG_LEVEL3)
      {
	cout << "I " << I.t() ;
	cout << "J " << J.t() ;
      }

      calculSolutionDementhon(xi[0],yi[0],I,J,cMo1);
      s1 =  sqrt(computeResidualDementhon(cMo1)/npt)  ;
      if (DEBUG_LEVEL3)
	cout << "cMo1 "<< endl << cMo1 << endl ;

      /* 2eme branche	*/
      I = I0 - U*r*co ;
      J = J0 - U*r*si ;
      if (DEBUG_LEVEL3)
      {
	cout << "I " << I.t() ;
	cout << "J " << J.t() ;
      }

      calculSolutionDementhon(xi[0],yi[0],I,J,cMo2);
      s2 =  sqrt(computeResidualDementhon(cMo2)/npt)  ;
      if (DEBUG_LEVEL3)
	cout << "cMo2 "<< endl << cMo2 << endl ;

      cpt ++;
      if (s1 <= s2)
      {
	smin = s1;
	k = 0;
	for(i = 0; i < npt; i++)
	{
	  if (k != 0) { // On ne prend pas le 1er point
	    eps[cpt][k] = (cMo1[2][0]*c3d[i].get_oX() + cMo1[2][1]*c3d[i].get_oY()
			   + cMo1[2][2]*c3d[i].get_oZ())/cMo1[2][3];
	  }
	  k++;
	}
	cMo = cMo1 ;
      }
      else
      {
	smin = s2;
	k = 0;
	for(i = 0; i < npt; i++)
	{
	  if (k != 0) { // On ne prend pas le 1er point
	    eps[cpt][k] = (cMo2[2][0]*c3d[i].get_oX() + cMo2[2][1]*c3d[i].get_oY()
			   + cMo2[2][2]*c3d[i].get_oZ())/cMo2[2][3];
	  }
	  k++;
	}
	cMo = cMo2 ;
      }

      if (smin > smin_old)
      {
	if (DEBUG_LEVEL2) cout << "Divergence "  <<  endl ;
	cMo = cMo_old ;
      }
      if (DEBUG_LEVEL2)
      {
	cout << "s1 = " << s1 << endl ;
	cout << "s2 = " << s2 << endl ;
	cout << "smin = " << smin << endl ;
	cout << "smin_old = " << smin_old << endl ;
      }
    }
  }
  if (DEBUG_LEVEL1)
    cout << "end vpPose::CalculArbreDementhon() return "<< erreur  << endl;

  return erreur ;
}

/*!
  \brief  Compute the pose using Dementhon approach for planar objects
          this is a direct implementation of the algorithm proposed by
	  Dementhon in his PhD

  \author Francois Chaumette (simplified by Eric Marchand)
*/

void
vpPose::poseDementhonPlan(vpHomogeneousMatrix &cMo)
{  if (DEBUG_LEVEL1)
    cout << "begin CCalculPose::PoseDementhonPlan()" << endl ;

  int i,j,k ;

  if (c3d !=NULL) delete []c3d ;
  c3d = new vpPoint[npt] ;


  vpPoint p0 ;
  listP.front() ;
  p0 = listP.value() ;


  vpPoint P ;
  listP.front() ;
  i=0 ;
  while (!listP.outside())
  {
    P= listP.value() ;
    c3d[i] = P ;
    c3d[i].set_oX(P.get_oX()-p0.get_oX()) ;
    c3d[i].set_oY(P.get_oY()-p0.get_oY()) ;
    c3d[i].set_oZ(P.get_oZ()-p0.get_oZ()) ;
    i++  ;
    listP.next() ;
  }

  vpMatrix a ;
  try
  {
    a.resize(npt-1,3) ;
  }
  catch(...)
  {
    vpERROR_TRACE(" ") ;
    throw ;
  }


  for (i=1 ; i < npt ; i++)
  {
    a[i-1][0]=c3d[i].get_oX();
    a[i-1][1]=c3d[i].get_oY();
    a[i-1][2]=c3d[i].get_oZ();
  }

  // calcul a^T a
  vpMatrix ata ;
  ata = a.t()*a ;

  /* essai FC pour debug SVD */
  /*
  vpMatrix ata_old ;
  ata_old = a.t()*a ;

  vpMatrix ata((ata_old.getRows()-1),(ata_old.getCols()-1)) ;
  for (i=0;i<ata.getRows();i++)
    for (j=0;j<ata.getCols();j++) ata[i][j] = ata_old[i][j];
  */
  vpMatrix ata_sav;
  ata_sav = ata;

  if (DEBUG_LEVEL2)
  {
    cout << "a" << endl <<a<<endl ;
    cout << "ata" << endl <<ata<<endl ;
  }

  // calcul (a^T a)^-1
  vpMatrix ata1(ata.getRows(),ata.getCols()) ;
  vpMatrix v(ata.getRows(),ata.getCols());
  vpColVector sv(ata.getRows());
  //  ata1 = ata.i() ;
  int imin = 0;
  double s = 0.0;

  //calcul de ata^-1
  ata.svd(sv,v) ;

  int nc = sv.getRows() ;
  for (i=0; i < nc ; i++)
    if (sv[i] > s) s = sv[i];

  s *= 0.0002;
  int  irank = 0;
  for (i=0;i<nc;i++)
    if (sv[i] > s ) irank++;

  double svm = 100.0;
  for (i = 0; i < nc; i++)
    if (sv[i] < svm) { imin = i; svm = sv[i]; }

  if (DEBUG_LEVEL2)
  {
    cout << "rang: " << irank << endl ;;
    cout <<"imin = " << imin << endl ;
    cout << "sv " << sv.t() << endl ;
  }


  for (i=0 ; i < ata.getRows() ; i++)
    for (j=0 ; j < ata.getCols() ; j++)
    {
      ata1[i][j] = 0.0;
      for (k=0 ; k < nc ; k++)
	if (sv[k] > s)
	  ata1[i][j] += ((v[i][k]*ata[j][k])/sv[k]);
    }



  vpMatrix b ;   // b=(at a)^-1*at
  b = ata1*a.t() ;

  //calcul de U
  vpColVector U(3) ;
  U = ata.column(imin+1) ;

  if (DEBUG_LEVEL2)
  {
    cout << "a" << endl <<a<<endl ;
    cout << "ata" << endl <<ata_sav<<endl ;
    cout << "ata1" << endl <<ata1<<endl ;
    cout << "ata1*ata"  << endl <<  ata1*ata_sav ;
    cout << "b"  << endl <<  b ;
    cout << "U " << U.t()  << endl ;

  }

  vpColVector xi(npt) ;
  vpColVector yi(npt) ;
  //calcul de la premiere solution
  for (i = 0; i < npt; i++)
  {
    xi[i] = c3d[i].get_x() ;
    yi[i] = c3d[i].get_y() ;

  }

  vpColVector I0(3) ; I0 = 0 ;
  vpColVector J0(3) ; J0 = 0 ;
  vpColVector I(3) ;
  vpColVector J(3) ;

  for (i=1;i<npt;i++)
  {
    I0[0] += b[0][i-1] * (xi[i]-xi[0]);
    I0[1] += b[1][i-1] * (xi[i]-xi[0]);
    I0[2] += b[2][i-1] * (xi[i]-xi[0]);

    J0[0] += b[0][i-1] * (yi[i]-yi[0]);
    J0[1] += b[1][i-1] * (yi[i]-yi[0]);
    J0[2] += b[2][i-1] * (yi[i]-yi[0]);
  }


  if (DEBUG_LEVEL2)
  {
    cout << "I0 "<<I0.t() ;
    cout << "J0 "<<J0.t() ;
  }

  s = -2.0*vpColVector::dotProd(I0,J0);
  double c = J0.sumSquare() - I0.sumSquare() ;

  double r,theta,si,co ;
  calculRTheta(s, c, r, theta);
  co = cos(theta);
  si = sin(theta);

  // calcul de la premiere solution
  I = I0 + U*r*co ;
  J = J0 + U*r*si ;

  vpHomogeneousMatrix cMo1f ;
  calculSolutionDementhon(xi[0], yi[0], I, J, cMo1f);


  int erreur1 = calculArbreDementhon(b, U, cMo1f);

  // calcul de la deuxieme solution
  I = I0 - U*r*co ;
  J = J0 - U*r*si ;

  vpHomogeneousMatrix cMo2f;
  calculSolutionDementhon(xi[0], yi[0], I, J, cMo2f);

  int erreur2 = calculArbreDementhon(b, U, cMo2f);

  if ((erreur1 == 0) && (erreur2 == -1))   cMo = cMo1f ;
  if ((erreur1 == -1) && (erreur2 == 0))   cMo = cMo2f ;
  if ((erreur1 == 0) && (erreur2 == 0))
  {
    double s1 =  sqrt(computeResidualDementhon(cMo1f)/npt)  ;
    double s2 =  sqrt(computeResidualDementhon(cMo2f)/npt)  ;

    if (s1<=s2) cMo = cMo1f ; else cMo = cMo2f ;
  }

  cMo[0][3] -= p0.get_oX()*cMo[0][0]+p0.get_oY()*cMo[0][1]+p0.get_oZ()*cMo[0][2];
  cMo[1][3] -= p0.get_oX()*cMo[1][0]+p0.get_oY()*cMo[1][1]+p0.get_oZ()*cMo[1][2];
  cMo[2][3] -= p0.get_oX()*cMo[2][0]+p0.get_oY()*cMo[2][1]+p0.get_oZ()*cMo[2][2];

  delete [] c3d ; c3d = NULL ;
  if (DEBUG_LEVEL1)
    cout << "end CCalculPose::PoseDementhonPlan()" << endl ;
}

#undef DMIN
#undef EPS
#undef EPS_DEM


/*!
  \brief Compute and return the residual expressed in meter for
         the pose matrix 'pose'
  \param input CMatrix &pose : the matrix that defines the pose to be tested
  \return the value of he residual in meter
*/
double vpPose::computeResidualDementhon(vpHomogeneousMatrix &cMo)
{
  int i ;
  double residual = 0 ;


  residual  =0 ;
  for (i =0 ; i < npt ; i++)
  {

    double X = c3d[i].get_oX()*cMo[0][0]+c3d[i].get_oY()*cMo[0][1]+c3d[i].get_oZ()*cMo[0][2] + cMo[0][3];
    double Y = c3d[i].get_oX()*cMo[1][0]+c3d[i].get_oY()*cMo[1][1]+c3d[i].get_oZ()*cMo[1][2] + cMo[1][3];
    double Z = c3d[i].get_oX()*cMo[2][0]+c3d[i].get_oY()*cMo[2][1]+c3d[i].get_oZ()*cMo[2][2] + cMo[2][3];

    double x = X/Z ;
    double y = Y/Z ;



    residual += vpMath::sqr(x-c3d[i].get_x()) +  vpMath::sqr(y-c3d[i].get_y())  ;
  }
  return residual ;
}


#undef DEBUG_LEVEL1
#undef DEBUG_LEVEL2
#undef DEBUG_LEVEL3


/*
 * Local variables:
 * c-basic-offset: 2
 * End:
 */
