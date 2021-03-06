 /****************************************************************************
 **
 **  Copyright (C) 2014,2015 Andreas Mussgiller
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
#include <cmath>
#include <algorithm>

#include "TkModLabSpline2D.h"

extern "C" {
  void surfit_(int *iopt, int *m,
               double *x, double *y, double *z, double *w,
               double *xb, double *xe,
               double *yb, double *ye,
               int *kx, int *ky, double *s,
               int *nxest, int *nyest, int *nmax,
               double *eps,
               int *nx, double *tx,
               int *ny, double *ty,
               double *c, double *fp,
               double *wrk1, int *lwrk1,
               double *wrk2, int *lwrk2,
               int *iwrk, int *kwrk,
               int *ier);

  void regrid_(int *iopt,
               int *mx, double *x,
               int *my, double *y,
               double *z,
               double *xb, double *xe,
               double *yb, double *ye,
               int *kx, int *ky, double *s,
               int *nxest, int *nyest,
               int *nx, double *tx,
               int *ny, double *ty,
               double *c, double *fp,
               double *wrk, int *lwrk,
               int *iwrk, int *kwrk,
               int *ier);

  void bispeu_(double *tx, int *nx,
               double *ty, int *ny,
               double *c,
               int *kx, int *ky,
               double *x, double *y, double *z,
               int *m,
               double *wrk, int *lwrk,
               int *ier);
}

ClassImp(TkModLabSpline2D);

TkModLabSpline2D::TkModLabSpline2D()
{

}

int TkModLabSpline2D::Surfit(const std::vector<double>& x,
                       const std::vector<double>& y,
                       const std::vector<double>& z,
                       const std::vector<double>& w,
                       int kx, int ky, double s,
                       double nxy)
{
  if (x.size()!=w.size()) {

  }

  int m = x.size();

  int nxest = std::max(nxy*(kx+1+std::ceil(std::sqrt(m/2))), 2.0*(kx+1));
  int nyest = std::max(nxy*(ky+1+std::ceil(std::sqrt(m/2))), 2.0*(ky+1));
  int nmax = std::max(nxest, nyest);

  double eps = 1.0e-16;

  double xb = x[0];
  double xe = xb;
  double yb = y[0];
  double ye = yb;

  std::vector<double> rx(x.begin(), x.end());
  std::vector<double> ry(y.begin(), y.end());
  std::vector<double> rz(z.begin(), z.end());
  std::vector<double> rw(w.begin(), w.end());

  for (double xv : rx) {
    xb = std::min(xb, xv);
    xe = std::max(xe, xv);
  }
  for (double yv : ry) {
    yb = std::min(yb, yv);
    ye = std::max(ye, yv);
  }

  int nx;
  tx_.resize(nxest);
  int ny;
  ty_.resize(nyest);
  c_.resize((nxest-kx-1) * (nyest-ky-1));
  double fp;
  int ier;

  int lwrk1 = CalcSurfitLwrk1(m, ky, kx, nyest, nxest);
  int lwrk2 = 1;
  int kwrk = m + (nxest - 2*kx - 1) * (nyest - 2*ky - 1);

  std::vector<double> wrk1(lwrk1);
  std::vector<double> wrk2(lwrk2);
  std::vector<int> iwrk(kwrk);

  int iopt = 0;

  surfit_(&iopt, &m,
          ry.data(), rx.data(), rz.data(), rw.data(),
          &yb, &ye,
          &xb, &xe,
          &ky, &kx, &s,
          &nyest, &nxest, &nmax,
          &eps,
          &ny, ty_.data(),
          &nx, tx_.data(),
          c_.data(), &fp,
          wrk1.data(), &lwrk1,
          wrk2.data(), &lwrk2,
          iwrk.data(), &kwrk,
          &ier);

  while (ier>10) {
    lwrk2 = ier;
    wrk2.resize(lwrk2);
    surfit_(&iopt, &m,
            ry.data(), rx.data(), rz.data(), rw.data(),
            &yb, &ye,
            &xb, &xe,
            &ky, &kx, &s,
            &nyest, &nxest, &nmax,
            &eps,
            &ny, ty_.data(),
            &nx, tx_.data(),
            c_.data(), &fp,
            wrk1.data(), &lwrk1,
            wrk2.data(), &lwrk2,
            iwrk.data(), &kwrk,
            &ier);
  }

  if (ier == 0 || ier == -1 || ier == -2) {
    // good
  } else if (ier < -2) {

  } else {
    // error
  }

  tx_.resize(nx);
  ty_.resize(ny);
  c_.resize((nx - kx - 1) * (ny - ky - 1));
  kx_ = kx;
  ky_ = ky;
  fp_ = fp;

  return ier;
}

int TkModLabSpline2D::Surfit(const std::vector<double>& x,
                       const std::vector<double>& y,
                       const std::vector<double>& z,
                       int kx, int ky, double s,
                       double nxy)
{
  std::vector<double> w(x.size(), 1.0);

  return this->Surfit(x, y, z, w, kx, ky, s, nxy);
}

int TkModLabSpline2D::Surfit(int n,
                              const double *x,
                              const double *y,
                              const double *z,
                              int kx, int ky, double s,
                              double nxy)
{
  std::vector<double> vx(x, x + n);
  std::vector<double> vy(y, y + n);
  std::vector<double> vz(z, z + n);
  std::vector<double> w(n, 1.0);

  return this->Surfit(vx, vy, vz, w, kx, ky, s, nxy);
}

void TkModLabSpline2D::Regrid(const std::vector<double>& x,
                       const std::vector<double>& y,
                       const std::vector<double>& z,
                       int kx, int ky, double s)
{
  int mx = x.size();
  int my = y.size();
  int mz = z.size();

  if (mz!=mx*my) {

  }

  std::vector<double> rx(x.begin(), x.end());
  std::vector<double> ry(y.begin(), y.end());
  std::vector<double> rz(z.begin(), z.end());

  double xb = rx.front();
  double xe = rx.back();
  double yb = ry.front();
  double ye = ry.back();

  int nxest = mx + kx + 1;
  int nyest = my + ky + 1;

  int nx;
  tx_.resize(nxest);
  int ny;
  ty_.resize(nyest);
  c_.resize((nxest-kx-1) * (nyest-ky-1));
  double fp;
  int ier;

  int lwrk = (4 + nyest * (mx+2*ky+5) + nxest * (2*kx+5) + my*(ky+1) + mx*(kx+1) + std::max(mx, nyest));
  std::vector<double> wrk(lwrk);
  int kwrk = 3 + mx + my + nxest + nyest;
  std::vector<int> iwrk(kwrk);

  int iopt = 0;

  regrid_(&iopt,
          &my, ry.data(),
          &mx, rx.data(),
          rz.data(),
          &yb, &ye,
          &xb, &xe,
          &ky, &kx, &s,
          &nyest, &nxest,
          &ny, ty_.data(),
          &nx, tx_.data(),
          c_.data(), &fp,
          wrk.data(), &lwrk,
          iwrk.data(), &kwrk,
          &ier);

  if (!(ier == 0 || ier == -1 || ier == -2)) {
    // error
  }

  tx_.resize(nx);
  ty_.resize(ny);
  c_.resize((nx - kx - 1) * (ny - ky - 1));
  kx_ = kx;
  ky_ = ky;
  fp_ = fp;
}

bool TkModLabSpline2D::Evaluate(double x, double y, double& z)
{
  int m = 1;

  int ier;
  int lwrk = kx_ + ky_ + 2;
  std::vector<double> wrk(lwrk);
  double rz;

  int nx = tx_.size();
  int ny = ty_.size();

  bispeu_(ty_.data(), &ny,
          tx_.data(), &nx,
          c_.data(),
          &ky_, &kx_,
          &y, &x, &rz,
          &m,
          wrk.data(), &lwrk, &ier);

  z = rz;

  if (ier==0) {
    return false;
  }

  return true;
}

bool TkModLabSpline2D::Evaluate(const std::vector<double>& x,
                                const std::vector<double>& y,
                                std::vector<double>& z)
{
  int m = x.size();

  int ier;
  int lwrk = kx_ + ky_ + 2;
  std::vector<double> wrk(lwrk);

  std::vector<double> rx(x.begin(), x.end());
  std::vector<double> ry(y.begin(), y.end());
  std::vector<double> rz(m);

  int nx = tx_.size();
  int ny = ty_.size();

  bispeu_(ty_.data(), &ny,
          tx_.data(), &nx,
          c_.data(),
          &ky_, &kx_,
          ry.data(), rx.data(), rz.data(),
          &m,
          wrk.data(), &lwrk, &ier);

  z.assign(rz.begin(), rz.end());

  if (ier==0) {
    return false;
  }

  return true;
}

int TkModLabSpline2D::CalcSurfitLwrk1(int m,
                               int ky, int kx,
                               int nyest, int nxest)
{
  int u = nxest - kx - 1;
  int v = nyest - ky - 1;

  int km = std::max(kx, ky) + 1;
  int ne = std::max(nxest, nyest);

  int bx = kx*v + ky + 1;
  int by = ky*u + kx + 1;

  int b1 = 0;
  int b2 = 0;

  if (bx<=by) {
    b1 = bx;
    b2 = bx + v - ky;
  } else {
    b1 = by;
    b2 = by + u - kx;
  }

  return u*v*(2 + b1 + b2) + 2*(u+v+km*(m+ne)+ne-kx-ky) + b2 + 1;
}
