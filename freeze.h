#ifndef FREEZE_H
#define FREEZE_H

#define NUMDECAY 2000
#define MAXINTV         2000000 /* size of arry for Montecarlo numbers note that I modified pdg05.dat by replacing 90...... 9......*/
#define MHALF           (MAXINTV/2)
#define NY		200 /* size of arry for storage of the y-spectrum */
#define NPT		50 /* size of arry for storage of the pt-spectrum */
#define NPHI		50 /* size of arry for storage of the phi-spectrum */
#define NPHI1		NPHI + 1 
#define	PTS3	12		/* normalization of 3-body decay */
#define	PTS4	12		/* inv. mass integral 3-body    */

#define	PTN1	8
#define	PTN2	8		/* 2-body between the poles     */

#define PTCHANGE    1.


#include <string.h>
#include <iostream>
#include <mpi.h>
#include "data.h"
#include "util.h"
#include "int.h"
#include "eos.h"
#include <iterator>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>
#include <gsl/gsl_interp.h>

class Freeze{

 private:
  typedef struct particle
  {
    int number;
    char *name;
    double mass;
    double width;
    int degeneracy;
    int baryon;
    int strange;
    int charm;
    int bottom;
    int isospin;
    double charge;
    int decays;
    int stable;
    int     ny;
    int     npt;
    int     nphi;
    double phimin;
    double phimax;
    double ymax;
    double deltaY;
    double  resCont[NY][NPT][NPHI];
    double  dNdydptdphi[NY][NPT][NPHI+1];
    double  dNdydptdphi2[NY][NPT][NPHI+1];
    double  mt[NPT];	/* mt values for spectrum */
    double  pt[NPT];	/* pt values for spectrum */
    double  y[NY];	/* y values for spectrum */
    double  slope;	/* assymtotic slope of pt-spectrum */
    double muAtFreezeOut; // the chemical potential at freeze-out for the partial chemical equilibrium calculation
  } Particle ;
  
  struct de 
  {
    int	 reso;       /* Montecarlo number of decaying resonance */
    int	 numpart;    /* number of daughter particles after decay */
    double branch;     /* branching ratio */
    int    part[5];    /* array of daughter particles Montecarlo number */
  }  decay[NUMDECAY];
  
  typedef struct pblockN
  {
    double pt, mt, y, e, pl;	/* pt, mt, y of decay product 1 */
    double phi;
    double m1, m2, m3;		/* masses of decay products     */
    double mr;			/* mass of resonance            */
    double costh, sinth;
    double e0, p0;
    int res_num;			/* Montecarlo number of the Res. */
  } pblockN;
  
  
  typedef struct nblock
  {
    double a, b, c, d;
  } nblock;			/* for normalisation integral of 3-body decays */
  
  
  typedef struct surfaceElement
  {
    double x[4]; // position in (tau, x, y, eta)
    double s[4]; // hypersurface vector in (tau, x, y, eta)
    double u[4]; // flow velocity in (tau, x, y, eta)
    double W[4][4]; // W^{\mu\nu}
    
    double epsilon_f;
    double T_f;
    double mu_B; 
    double sFO; // entropy density
  } SurfaceElement;
  
  SurfaceElement *surface;
  Particle *particleList;
  int NCells;
  int decayMax, particleMax;

  double ***sumYPtPhi;
  int *partid;
  // array for converting Montecarlo numbers in internal numbering of the resonances 
  double *phiArray;
  Int *integral;
  Util *util;
  int pseudofreeze;

 public:
  Freeze();//constructor
  ~Freeze();//destructor

  double gauss(int n, double (Freeze::*f)(double, void *), double xlo, double xhi, void *optvec );
  void ReadParticleData(InitData *DATA, EOS *eos);
  void ReadFreezeOutSurface(InitData *DATA);
  void ReadSpectra(InitData *DATA);
  void Read3Spectra(InitData *DATA);
  void ReadSingleSpectrum(InitData* DATA);
  void ReadFullSpectra(InitData *DATA);
  void ReadFullSpectra2(InitData *DATA);
  void ComputeAveragePT(int number, double ptmax);
  void ComputeChargedHadrons(InitData* DATA,double ptmax);
  void Compute3ChargedHadrons(InitData *DATA, double ptmax);
  void ComputeCorrelations(InitData* DATA, double ptmax);
  double summation(double px, double py, double y, double m, int deg, int baryon, double mu, InitData *DATA);
  void ComputeParticleSpectrum(InitData *DATA, int number, double ptmax, int anti, int iptmax, int iphimax, int size, int rank);
  void OutputFullParticleSpectrum(InitData *DATA, int number, double ptmax, int anti, int full);
  
  // --------------------------------------------------------------------------------------
  // the following routines are adapted from the public version of
  /* ... the resonance decay calculation using the output  */
  /* generated by the hydrodynamical code azhydro0p2.  The majority of the code was  */
  /* developed by Josef Sollfrank and Peter Kolb.  Additional work and comments  */
  /* were added by Evan Frodermann, September 2005. */
  /* Please refer to the papers  */
  /* J. Sollfrank, P. Koch, and U. Heinz, Phys. Lett B 252 (1990) and  */
  /* J. Sollfrank, P. Koch, and U. Heinz, Z. Phys. C 52 (1991)  */
  /* for a description of the formalism utilized in this program. */
  
  double norm3int (double x, void *paranorm); // this computes "Q(m_R,m_1,m_2,m_3)"
  double	Edndp3(double yr, double ptr, double phirin, int res_num);
  double dnpir2N (double phi, void *para1);    
  double dnpir1N (double costh, void* para1);
  double dn2ptN (double w2, void* para1);
  double dn3ptN (double x, void* para1);
  double Edndp3_2bodyN (double y, double pt, double phi, double m1, double m2, double mr, int res_num);
  double Edndp3_3bodyN (double y, double pt, double phi, double m1, double m2,
			double m3, double mr, double norm3, int res_num);
  void add_reso (int pn, int pnR, int k, int j);
  void cal_reso_decays (int maxpart, int maxdecay, int bound, int mode);
  // --------------------------------------------------------------------------------------
  
  int countLines (std::istream& in);
  void checkForReadError(FILE *file, char* name);
  void CooperFrye(int particleSpectrumNumber, int mode, InitData *DATA, EOS *eos, int size, int rank);

  //When the pseudorapidity mode is used
  void ReadFSpectra_pseudo(InitData *DATA);
  void ReadSpectra_pseudo(InitData* DATA, int full);
  void ComputeParticleSpectrum_pseudo(InitData *DATA, int number, int anti, int size, int rank);
  void OutputFullParticleSpectrum_pseudo(InitData *DATA, int number, int anti, int full);
  void CooperFrye_pseudo(int particleSpectrumNumber, int mode, InitData *DATA, EOS *eos, int size, int rank);
  double	Edndp3_pseudo(double yr, double ptr, double phirin, int res_num);
  double dnpir2N_pseudo (double phi, void *para1);    
  double dnpir1N_pseudo (double costh, void* para1);
  double dn2ptN_pseudo (double w2, void* para1);
  double dn3ptN_pseudo (double x, void* para1);
  double Edndp3_2bodyN_pseudo (double y, double pt, double phi, double m1, double m2, double mr, int res_num);
  double Edndp3_3bodyN_pseudo (double y, double pt, double phi, double m1, double m2,
			double m3, double mr, double norm3, int res_num);
  void add_reso_pseudo (int pn, int pnR, int k, int j, int pseudofreeze);
  void cal_reso_decays_pseudo (int maxpart, int maxdecay, int bound, int mode, int pseudofreeze);
  double Rap(double eta, double pt, double m);
  double PseudoRap(double y, double pt, double m);
  void OutputDifferentialFlowAtMidrapidity(InitData *DATA, int number, int full);
  void OutputIntegratedFlowForCMS(InitData *DATA, int number, int full);
  double OutputYieldForCMS(InitData *DATA, int number, int full);

};
#endif
  
