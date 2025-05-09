#ifndef TFitParticleSpher_hh
#define TFitParticleSpher_hh


#include "TAbsFitParticle.h"
#include "TLorentzVector.h"
#include "TMatrixD.h"

class TFitParticleSpher: public TAbsFitParticle {

public :

  TFitParticleSpher();
  TFitParticleSpher( const TFitParticleSpher& fitParticle );
  TFitParticleSpher(TLorentzVector* pini, const TMatrixD* theCovMatrix);
  TFitParticleSpher(const TString &name, const TString &title, 
	       TLorentzVector* pini,
	       const TMatrixD* theCovMatrix);
  virtual ~TFitParticleSpher();
  virtual TAbsFitParticle* clone( const TString& newname = TString("") ) const;

  // returns derivative dP/dy with P=(p,E) and y=(r, theta, phi, ...) 
  // the free parameters of the fit. The columns of the matrix contain 
  // (dP/dr, dP/dtheta, ...).
  virtual TMatrixD* getDerivative();
  virtual TMatrixD* transform(const TLorentzVector& vec);
  virtual void setIni4Vec(const TLorentzVector* pini);
  virtual TLorentzVector* calc4Vec( const TMatrixD* params );
  ClassDef(TFitParticleSpher, 1);

protected :

  void init(TLorentzVector* pini, const TMatrixD* theCovMatrix);


private:
  

};

#endif
