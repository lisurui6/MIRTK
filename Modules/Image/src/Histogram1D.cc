/*
 * Medical Image Registration ToolKit (MIRTK)
 *
 * Copyright 2008-2015 Imperial College London
 * Copyright 2008-2015 Daniel Rueckert, Julia Schnabel
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mirtk/Histogram1D.h"

#include "mirtk/Math.h"
#include "mirtk/Memory.h"

#include "mirtk/CommonExport.h"


namespace mirtk {


// Global "debug" flag (cf. mirtk/Options.h)
MIRTK_Common_EXPORT extern int debug;


// =============================================================================
// Histogram1D
// =============================================================================

// -----------------------------------------------------------------------------
template <class HistogramType>
Histogram1D<HistogramType>::Histogram1D(const Histogram1D &h)
:
  Object(h), _bins(NULL)
{
  _min   = h._min;
  _max   = h._max;
  _width = h._width;
  _nbins = h._nbins;
  _nsamp = h._nsamp;
  if (_nbins > 0) {
    Allocate(_bins, _nbins);
    memcpy(RawPointer(), h.RawPointer(), _nbins * sizeof(HistogramType));
  }
}

// -----------------------------------------------------------------------------
template <class HistogramType>
Histogram1D<HistogramType>::Histogram1D(int nbins)
{
  _min   = 0;
  _max   = nbins;
  _width = 1;
  _nbins = nbins;
  _nsamp = 0;
  CAllocate(_bins, _nbins);
}

// -----------------------------------------------------------------------------
template <class HistogramType>
Histogram1D<HistogramType>::Histogram1D(double min, double max, double width)
{
  _min   = min;
  _max   = max;
  _nbins = static_cast<int>((_max - _min) / width);
  _width = static_cast<double>(_max - _min) / _nbins;
  _nsamp = 0;
  if (_nbins < 1) {
    cerr << "Histogram1D::Histogram1D: Should have at least one bin" << endl;
    exit(1);
  }
  CAllocate(_bins, _nbins);
}

// -----------------------------------------------------------------------------
template <class HistogramType>
Histogram1D<HistogramType>::Histogram1D(const char *filename)
{
  ifstream from(filename);
  if (!from) {
    cerr << "Histogram1D::Read: Can't open file " << filename << endl;
    exit(1);
  }
  char buffer[255];
  from >> buffer;
  if (strcmp(buffer, "Histogram1D") != 0) {
    cerr << "Histogram1D::Read: Invalid format" << endl;
    exit(1);
  }
  from >> _nbins >> _nsamp >> _min >> _max >> _width;
  Allocate(_bins, _nbins);
  for (int i = 0; i < _nbins; ++i) from >> _bins[i];
}

// -----------------------------------------------------------------------------
template <class HistogramType>
Histogram1D<HistogramType>::~Histogram1D()
{
  Deallocate(_bins);
  _nbins = 0;
  _nsamp = 0;
  _min   = 0;
  _max   = 0;
  _width = 0;
}

// -----------------------------------------------------------------------------
template <class HistogramType>
void Histogram1D<HistogramType>::Reset()
{
  memset(_bins, 0, _nbins * sizeof(HistogramType));
  _nsamp = 0;
}

// -----------------------------------------------------------------------------
template <class HistogramType>
void Histogram1D<HistogramType>::PutMin(double min)
{
  Min(min);
  this->Reset();
}

// -----------------------------------------------------------------------------
template <class HistogramType>
void Histogram1D<HistogramType>::PutMax(double max)
{
  Max(max);
  this->Reset();
}

// -----------------------------------------------------------------------------
template <class HistogramType>
void Histogram1D<HistogramType>::PutWidth(double width)
{
  const int nbins = iround((_max - _min) / width);
  if (nbins < 1) {
    cerr << "Histogram1D::PutWidth: Should have at least one bin" << endl;
    exit(1);
  }
  if (_nbins != nbins) {
    Deallocate(_bins);
    Allocate(_bins, nbins);
    _nbins = nbins;
  }
  _width = (_max - _min) / _nbins;
  this->Reset();
}

// -----------------------------------------------------------------------------
template <class HistogramType>
void Histogram1D<HistogramType>::PutNumberOfBins(int nbins)
{
  if (nbins < 1) {
    cerr << "Histogram1D::PutNumberOfBins: Should have at least one bin" << endl;
    exit(1);
  }
  NumberOfBins(nbins);
  this->Reset();
}

// -----------------------------------------------------------------------------
template <class HistogramType>
void Histogram1D<HistogramType>::Log()
{
  const HistogramType zero(0);
  if (_nsamp == zero) {
    if (debug) {
      cerr << "Histogram1D::Log: No samples in Histogram" << endl;
    }
    return;
  }
  for (int i = 0; i < _nbins; ++i) {
    if (_bins[i] > zero) {
      _bins[i] = static_cast<HistogramType>(log(static_cast<double>(_bins[i]) / static_cast<double>(_nsamp)));
    } else {
      _bins[i] = zero;
    }
  }
}

// -----------------------------------------------------------------------------
template <class HistogramType>
void Histogram1D<HistogramType>::Smooth()
{
  if (_nsamp == 0) {
    if (debug) {
      cerr << "Histogram1D::Smooth: No samples in Histogram" << endl;
    }
    return;
  }
  if (_nbins == 0) return;

  // Smoothing kernel
  const double kernel[3] = { 1.0 / 6.0, 2.0 / 3.0, 1.0 / 6.0 };

  // Allocate new histogram
  HistogramType *bins = Allocate<HistogramType>(_nbins);

  // Smooth histogram
  if (_nbins > 1) {
    bins[0] = static_cast<HistogramType>(kernel[1] * static_cast<double>(_bins[0])
                                       + kernel[0] * static_cast<double>(_bins[1]));
  }
  else {
    bins[0] = static_cast<HistogramType>(kernel[1] * static_cast<double>(_bins[0]));
  }
  for (int i = 1; i < _nbins - 1; ++i) {
    bins[i] = static_cast<HistogramType>(kernel[2] * static_cast<double>(_bins[i - 1])
                                       + kernel[1] * static_cast<double>(_bins[i])
                                       + kernel[0] * static_cast<double>(_bins[i + 1]));
  }
  if (_nbins > 1) {
    bins[_nbins - 1] = static_cast<HistogramType>(kernel[1] * static_cast<double>(_bins[_nbins - 1])
                                                + kernel[2] * static_cast<double>(_bins[_nbins - 2]));
  }

  // Replace histogram by smoothed version
  Deallocate(_bins);
  _bins = bins;
}

// -----------------------------------------------------------------------------
template <class HistogramType>
double Histogram1D<HistogramType>::Mean() const
{
  if (_nsamp == 0) {
    if (debug) {
      cerr << "Histogram1D::Mean: No samples in Histogram" << endl;
    }
    return 0;
  }
  double val = 0;
  for (int i = 0; i < _nbins; ++i) {
    val += _bins[i] * this->BinToVal(i);
  }
  return val / _nsamp;
}

// -----------------------------------------------------------------------------
template <class HistogramType>
double Histogram1D<HistogramType>::Variance() const
{
  if (_nsamp == 0) {
    if (debug) {
      cerr << "Histogram1D::Variance: No samples in Histogram" << endl;
    }
    return 0;
  }
  double val = 0, mean = this->Mean();
  for (int i = 0; i < _nbins; ++i) {
    val += _bins[i] * (this->BinToVal(i) - mean) * (this->BinToVal(i) - mean);
  }
  return val / _nsamp;
}

// -----------------------------------------------------------------------------
template <class HistogramType>
double Histogram1D<HistogramType>::StandardDeviation() const
{
  return sqrt(this->Variance());
}

// -----------------------------------------------------------------------------
template <class HistogramType>
double Histogram1D<HistogramType>::Entropy() const
{
  if (_nsamp == 0) {
    if (debug) {
      cerr << "Histogram1D::Entropy: No samples in Histogram" << endl;
    }
    return 0;
  }
  // Attention: Parallel summation yielded slightly different results each
  //            time this function was executed. This might be caused by a
  //            different summation of values, which causes different numerical
  //            cancelations. Qhen used for NMI gradient computation, the
  //            registration result could differ from run to run!
  double p, sum = .0;
  const HistogramType *bin = _bins;
  for (int i = 0; i != _nbins; ++i, ++bin) {
    p = static_cast<double>(*bin);
    if (p > .0) sum += p * log(p);
  }
  // H = - sum (p/n) log(p/n) = log(n) - sum(p log p) / n
  return log(_nsamp) - sum / _nsamp;
}

// -----------------------------------------------------------------------------
template <class HistogramType>
void Histogram1D<HistogramType>::Read(const char *filename)
{
  ifstream from(filename);
  if (!from) {
    cerr << "Histogram1D::Read: Can't open file " << filename << endl;
    exit(1);
  }

  char buffer[255];
  from >> buffer;
  if (strcmp(buffer, "Histogram1D") != 0) {
    cerr << "Histogram1D::Read: Invalid format" << endl;
    exit(1);
  }

  int nbins;
  from >> nbins >> _nsamp >> _min >> _max >> _width;
  if (_nbins != nbins) {
    Deallocate(_bins);
    Allocate(_bins, nbins);
    _nbins = nbins;
  }
  for (int i = 0; i < _nbins; ++i) from >> _bins[i];
}

// -----------------------------------------------------------------------------
template <class HistogramType>
void Histogram1D<HistogramType>::Write(const char *filename) const
{
  ofstream to(filename);
  if (!to) {
    cerr << "Histogram1D::Write: Can't open file " << filename << endl;
    exit(1);
  }
  to << "Histogram1D\n";
  to << _nbins << " " << _nsamp << " " << _min << " " << _max << " " << _width << endl;
  for (int i = 0; i < _nbins; ++i) to << _bins[i] << endl;
}

// -----------------------------------------------------------------------------
template <class HistogramType>
void Histogram1D<HistogramType>::Print() const
{
  cout << _nbins << " " << _nsamp << " " << _min << " " << _max << " " << _width << endl;
  for (int i = 0; i < _nbins; ++i) cout << _bins[i] << endl;
}

// =============================================================================
// Explicit instantiations
// =============================================================================

template class Histogram1D<int>;
template class Histogram1D<double>;
template class Histogram1D<unsigned char>;
template class Histogram1D<short>;
template class Histogram1D<unsigned short>;
template class Histogram1D<float>;


} // namespace mirtk