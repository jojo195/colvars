// -*- c++ -*-

// This file is part of the Collective Variables module (Colvars).
// The original version of Colvars and its updates are located at:
// https://github.com/Colvars/colvars
// Please update all Colvars source files before making any changes.
// If you wish to distribute your changes, please submit them to the
// Colvars repository at GitHub.

#ifndef COLVARBIAS_ABF_H
#define COLVARBIAS_ABF_H

#include <vector>
#include <list>
#include <sstream>
#include <iomanip>
#include <memory>

#include "colvarproxy.h"
#include "colvarbias.h"
#include "colvargrid.h"
#include "colvar_UIestimator.h"

typedef cvm::real *gradient_t;


/// ABF bias
class colvarbias_abf : public colvarbias {

public:

  /// Constructor for ABF bias
  colvarbias_abf(char const *key);
  /// Initializer for ABF bias
  virtual int init(std::string const &conf);
  /// Default destructor for ABF bias
  virtual ~colvarbias_abf();
  /// Per-timestep update of ABF bias
  virtual int update();

private:

  /// Base filename(s) for reading previous gradient data (replaces data from restart file)
  std::vector<std::string> input_prefix;

  /// Adapt the bias at each time step (as opposed to keeping it constant)?
  bool    update_bias;
  /// Use normalized definition of PMF for distance functions? (flat at long distances)
  /// by including the Jacobian term separately of the recorded PMF
  bool    hide_Jacobian;
  /// Integrate gradients into a PMF on output
  bool    b_integrate;

  /// Number of samples per bin before applying the full biasing force
  size_t  full_samples;
  /// Number of samples per bin before applying a scaled-down biasing force
  size_t  min_samples;
  /// Latest absolute time step at which history files were written
  cvm::step_number history_last_step;
  /// Write CZAR output file for stratified eABF (.zgrad)
  bool    b_czar_window_file;
  /// Number of timesteps between recording data in history files (if non-zero)
  size_t  history_freq;
  /// Umbrella Integration estimator of free energy from eABF
  UIestimator::UIestimator eabf_UI;
  /// Run UI estimator?
  bool  b_UI_estimator;
  /// Run CZAR estimator?
  bool  b_CZAR_estimator;

  /// Frequency for updating pABF PMF (if zero, pABF is not used)
  int   pabf_freq;
  /// Max number of CG iterations for integrating PMF at startup and for file output
  int       integrate_iterations;
  /// Tolerance for integrating PMF at startup and for file output
  cvm::real integrate_tol;
  /// Max number of CG iterations for integrating PMF at on-the-fly pABF updates
  int       pabf_integrate_iterations;
  /// Tolerance for integrating PMF at on-the-fly pABF updates
  cvm::real pabf_integrate_tol;

  /// Cap the biasing force to be applied? (option maxForce)
  bool                    cap_force;
  /// Maximum force to be applied
  std::vector<cvm::real>  max_force;

  // Internal data and methods

  /// Current bin in sample grid
  std::vector<int>  bin;
  /// Current bin in force grid
  std::vector<int> force_bin;
  /// Cuurent bin in "actual" coordinate, when running extended Lagrangian dynamics
  std::vector<int> z_bin;

  /// Measured instantaneous system force
  gradient_t system_force;

  /// n-dim grid of free energy gradients
  std::unique_ptr<colvar_grid_gradient> gradients;
  /// n-dim grid of number of samples
  std::unique_ptr<colvar_grid_count>    samples;
  /// n-dim grid of pmf (dimension 1 to 3)
  std::unique_ptr<integrate_potential>  pmf;
  /// n-dim grid: average force on "real" coordinate for eABF z-based estimator
  std::unique_ptr<colvar_grid_gradient> z_gradients;
  /// n-dim grid of number of samples on "real" coordinate for eABF z-based estimator
  std::unique_ptr<colvar_grid_count>    z_samples;
  /// n-dim grid containing CZAR estimatr of "real" free energy gradients
  std::unique_ptr<colvar_grid_gradient> czar_gradients;
  /// n-dim grid of CZAR pmf (dimension 1 to 3)
  std::unique_ptr<integrate_potential>  czar_pmf;

  /// Calculate system force for all colvars
  int update_system_force();

  /// Calulate the biasing force for the current bin
  int calc_biasing_force(std::vector<cvm::real> &force);

  /// Calulate the smoothing factor to apply to biasing forces for given local count
  cvm::real smoothing_factor(cvm::real weight);

  // shared ABF
  bool    shared_on;
  size_t  shared_freq;
  cvm::step_number shared_last_step;

  // Share between replicas -- may be called independently of update
  virtual int replica_share();

  // Share data needed for CZAR between replicas - called before output only
  int replica_share_CZAR();

  /// Report the frequency at which this bias needs to communicate with replicas
  virtual size_t replica_share_freq() const;

  // Data just after the last share (start of cycle) in shared ABF
  std::unique_ptr<colvar_grid_gradient> last_gradients;
  std::unique_ptr<colvar_grid_count>    last_samples;
  // eABF/CZAR local data last shared
  std::unique_ptr<colvar_grid_gradient> last_z_gradients;
  std::unique_ptr<colvar_grid_count>    last_z_samples;
  // ABF data from local replica only in shared ABF
  std::unique_ptr<colvar_grid_gradient> local_gradients;
  std::unique_ptr<colvar_grid_count>    local_samples;
  std::unique_ptr<integrate_potential>  local_pmf;
  // eABF/CZAR data collected from all replicas in shared eABF on replica 0
  // if non-shared, aliases of regular CZAR grids, for output purposes
  colvar_grid_gradient *global_z_gradients;
  colvar_grid_count    *global_z_samples;
  colvar_grid_gradient *global_czar_gradients;
  integrate_potential  *global_czar_pmf;


  // For Tcl implementation of selection rules.
  /// Give the total number of bins for a given bias.
  virtual int bin_num();
  /// Calculate the bin index for a given bias.
  virtual int current_bin();
  //// Give the count at a given bin index.
  virtual int bin_count(int bin_index);
  /// Return the average number of samples in a given "radius" around current bin
  virtual int local_sample_count(int radius);

  /// Write human-readable FE gradients and sample count, and DX file in dim > 2
  /// \param local write grids contining replica-local data in shared ABF
  void write_gradients_samples(const std::string &prefix, bool close = true, bool local = false);

  /// Read human-readable FE gradients and sample count (if not using restart)
  int read_gradients_samples();

  /// Shorthand template used in write_gradient_samples()
  template <class T> int write_grid_to_file(T const *grid,
                                            std::string const &name,
                                            bool close);

private:

  /// Generic stream writing function (formatted and not)
  template <typename OST> OST &write_state_data_template_(OST &os);

  /// Generic stream readingx function (formatted and not)
  template <typename IST> IST &read_state_data_template_(IST &is);

public:

  virtual std::ostream &write_state_data(std::ostream &os);

  virtual cvm::memory_stream &write_state_data(cvm::memory_stream &os);

  virtual std::istream &read_state_data(std::istream &is);

  virtual cvm::memory_stream &read_state_data(cvm::memory_stream &is);

  virtual int write_output_files();

  /// Calculate the bias energy for 1D ABF
  virtual int calc_energy(std::vector<colvarvalue> const *values);

  /// Initialize specific dependencies of ABF derived class
  /// Adding them to those of base class colvarbias
  /// Alternately we could overload the init_dependencies() function
  virtual int init_dependencies();

  enum features_bias_abf {
    /// Start after generic cvb features
    /// There is at least one ext-Lagrangian colvar -> run eABF
    f_cvb_abf_extended = f_cvb_ntot,
    /// Total number of features for an ABF bias
    f_cvb_abf_ntot
  };

  /// \brief Implementation of the feature list for colvarbias_abf
  static std::vector<feature *> cvb_abf_features;

  /// \brief Implementation of the feature list accessor for colvarbias
  virtual const std::vector<feature *> &features() const
  {
    return cvb_abf_features;
  }
  virtual std::vector<feature *> &modify_features()
  {
    return cvb_abf_features;
  }
  static void delete_features() {
    for (size_t i=0; i < cvb_abf_features.size(); i++) {
      delete cvb_abf_features[i];
    }
    cvb_abf_features.clear();
  }

};
#endif
