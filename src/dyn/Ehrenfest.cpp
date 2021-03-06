/*********************************************************************************
* Copyright (C) 2017 Alexey V. Akimov
*
* This file is distributed under the terms of the GNU General Public License
* as published by the Free Software Foundation, either version 2 of
* the License, or (at your option) any later version.
* See the file LICENSE in the root directory of this distribution
* or <http://www.gnu.org/licenses/>.
*
*********************************************************************************/
/**
  \file Ehrenfest.cpp
  \brief The file implements all about Ehrenfest calculations
    
*/

#include "electronic/libelectronic.h"
#include "Dynamics.h"
#include "Surface_Hopping.h"

/// liblibra namespace
namespace liblibra{

/// libdyn namespace 
namespace libdyn{

using namespace libelectronic;


void Ehrenfest0(double dt, MATRIX& q, MATRIX& p, MATRIX& invM, CMATRIX& C, nHamiltonian& ham, bp::object py_funct, bp::object params, int rep){
/**
  \brief One step of the Ehrenfest algorithm for electron-nuclear DOFs for one trajectory

  \param[in] Integration time step
  \param[in,out] q [Ndof x Ntraj] nuclear coordinates. Change during the integration.
  \param[in,out] p [Ndof x Ntraj] nuclear momenta. Change during the integration.
  \param[in] invM [Ndof  x 1] inverse nuclear DOF masses. 
  \param[in,out] C nadi x nadi or ndia x ndia matrix containing the electronic coordinates
  \param[in] ham Is the Hamiltonian object that works as a functor (takes care of all calculations of given type) - its internal variables
  (well, actually the variables it points to) are changed during the compuations
  \param[in] py_funct Python function object that is called when this algorithm is executed. The called Python function does the necessary 
  computations to update the diabatic Hamiltonian matrix (and derivatives), stored externally.
  \param[in] params The Python object containing any necessary parameters passed to the "py_funct" function when it is executed.
  \param[in] rep The representation to run the calculations: 0 - diabatic, 1 - adiabatic

*/
  int ndof = q.n_rows;
  int dof;
 
  //============== Electronic propagation ===================
  if(rep==0){  
    ham.compute_nac_dia(p, invM);
    ham.compute_hvib_dia();
  }
  else if(rep==1){  
    ham.compute_nac_adi(p, invM); 
    ham.compute_hvib_adi();
  }

  propagate_electronic(0.5*dt, C, ham, rep);   

  //============== Nuclear propagation ===================
    
       if(rep==0){  p = p + ham.Ehrenfest_forces_dia(C, 0).real() * 0.5*dt;  }
  else if(rep==1){  p = p + ham.Ehrenfest_forces_adi(C, 0).real() * 0.5*dt;  }


  for(dof=0; dof<ndof; dof++){  
    q.add(dof, 0,  invM.get(dof,0) * p.get(dof,0) * dt ); 
  }


  ham.compute_diabatic(py_funct, bp::object(q), params);
  ham.compute_adiabatic(1);


       if(rep==0){  p = p + ham.Ehrenfest_forces_dia(C, 0).real() * 0.5*dt;  }
  else if(rep==1){  p = p + ham.Ehrenfest_forces_adi(C, 0).real() * 0.5*dt;  }

  //============== Electronic propagation ===================
  if(rep==0){  
    ham.compute_nac_dia(p, invM);
    ham.compute_hvib_dia();
  }
  else if(rep==1){  
    ham.compute_nac_adi(p, invM); 
    ham.compute_hvib_adi();
  }

  propagate_electronic(0.5*dt, C, ham, rep);   


}



void Ehrenfest1(double dt, MATRIX& q, MATRIX& p, MATRIX& invM, CMATRIX& C,
                nHamiltonian& ham, bp::object py_funct, bp::object params, int rep){
/**
  \brief One step of the Ehrenfest algorithm for electron-nuclear DOFs for an ensemble of trajectories

  \param[in] Integration time step
  \param[in,out] q [Ndof x Ntraj] nuclear coordinates. Change during the integration.
  \param[in,out] p [Ndof x Ntraj] nuclear momenta. Change during the integration.
  \param[in] invM [Ndof  x 1] inverse nuclear DOF masses. 
  \param[in,out] C [nadi x ntraj] or [ndia x ntraj] matrix containing the electronic coordinates
  \param[in] ham Is the Hamiltonian object that works as a functor (takes care of all calculations of given type) - its internal variables
  (well, actually the variables it points to) are changed during the compuations
  \param[in] py_funct Python function object that is called when this algorithm is executed. The called Python function does the necessary 
  computations to update the diabatic Hamiltonian matrix (and derivatives), stored externally.
  \param[in] params The Python object containing any necessary parameters passed to the "py_funct" function when it is executed.
  \param[in] rep The representation to run the calculations: 0 - diabatic, 1 - adiabatic

*/

  int ndof = q.n_rows;
  int ntraj = q.n_cols;
  int traj, dof;

 
  //============== Electronic propagation ===================
  // Update NACs and Hvib for all trajectories
  if(rep==0){  
    ham.compute_nac_dia(p, invM, 0, 1);
    ham.compute_hvib_dia(1);
  }
  else if(rep==1){  
    ham.compute_nac_adi(p, invM, 0, 1); 
    ham.compute_hvib_adi(1);
  }

  // Evolve electronic DOFs for all trajectories
  propagate_electronic(0.5*dt, C, ham.children, rep);   

  //============== Nuclear propagation ===================

  // Update the Ehrenfest forces for all trajectories
  if(rep==0){  p = p + ham.Ehrenfest_forces_dia(C, 1).real() * 0.5*dt;  }
  else if(rep==1){  p = p + ham.Ehrenfest_forces_adi(C, 1).real() * 0.5*dt;  }


  // Update coordinates of nuclei for all trajectories
  for(traj=0; traj<ntraj; traj++){
    for(dof=0; dof<ndof; dof++){  
      q.add(dof, traj,  invM.get(dof,0) * p.get(dof,traj) * dt ); 
    }
  }

  ham.compute_diabatic(py_funct, bp::object(q), params, 1);
  ham.compute_adiabatic(1, 1);


  // Update the Ehrenfest forces for all trajectories
  if(rep==0){  p = p + ham.Ehrenfest_forces_dia(C, 1).real() * 0.5*dt;  }
  else if(rep==1){  p = p + ham.Ehrenfest_forces_adi(C, 1).real() * 0.5*dt;  }


  //============== Electronic propagation ===================
  // Update NACs and Hvib for all trajectories
  if(rep==0){  
    ham.compute_nac_dia(p, invM, 0, 1);
    ham.compute_hvib_dia(1);
  }
  else if(rep==1){  
    ham.compute_nac_adi(p, invM, 0, 1); 
    ham.compute_hvib_adi(1);
  }

  // Evolve electronic DOFs for all trajectories
  propagate_electronic(0.5*dt, C, ham.children, rep);   

}



void Ehrenfest2(double dt, MATRIX& q, MATRIX& p, MATRIX& invM, CMATRIX& C, 
                nHamiltonian& ham, bp::object py_funct, bp::object params, int rep, int do_reordering, int do_phase_correction){

/**
  \brief One step of the TSH algorithm for electron-nuclear DOFs for one trajectory

  \param[in] Integration time step
  \param[in,out] q [Ndof x Ntraj] nuclear coordinates. Change during the integration.
  \param[in,out] p [Ndof x Ntraj] nuclear momenta. Change during the integration.
  \param[in] invM [Ndof  x 1] inverse nuclear DOF masses. 
  \param[in,out] C [nadi x ntraj]  or [ndia x ntraj] matrix containing the electronic coordinates
  \param[in] ham Is the Hamiltonian object that works as a functor (takes care of all calculations of given type) 
  - its internal variables (well, actually the variables it points to) are changed during the compuations
  \param[in] py_funct Python function object that is called when this algorithm is executed. The called Python function does the necessary 
  computations to update the diabatic Hamiltonian matrix (and derivatives), stored externally.
  \param[in] params The Python object containing any necessary parameters passed to the "py_funct" function when it is executed.

  Return: propagates C, q, p and updates state variables

*/


  int ndof = q.n_rows;
  int ntraj = q.n_cols;
  int nst = C.n_rows;    
  int traj, dof, i;

  CMATRIX** Uprev; 
  CMATRIX* X;
  vector<int> perm_t; 
  vector<int> el_stenc_x(nst, 0); for(i=0;i<nst;i++){  el_stenc_x[i] = i; }
  vector<int> el_stenc_y(1, 0); 

 
  //============== Electronic propagation ===================
  // Update NACs and Hvib for all trajectories
  if(rep==0){  
    ham.compute_nac_dia(p, invM, 0, 1);
    ham.compute_hvib_dia(1);
  }
  else if(rep==1){  
    ham.compute_nac_adi(p, invM, 0, 1); 
    ham.compute_hvib_adi(1);
  }

  // Evolve electronic DOFs for all trajectories
  propagate_electronic(0.5*dt, C, ham.children, rep);   

  //============== Nuclear propagation ===================

  // Update the Ehrenfest forces for all trajectories
  if(rep==0){  p = p + ham.Ehrenfest_forces_dia(C, 1).real() * 0.5*dt;  }
  else if(rep==1){  p = p + ham.Ehrenfest_forces_adi(C, 1).real() * 0.5*dt;  }


  // Update coordinates of nuclei for all trajectories
  for(traj=0; traj<ntraj; traj++){
    for(dof=0; dof<ndof; dof++){  
      q.add(dof, traj,  invM.get(dof,0) * p.get(dof,traj) * dt ); 
    }
  }

  // Allocate transformation matrices, in case we are going to compute phase correction
  if(rep==1){      
    if(do_phase_correction || do_reordering){
      Uprev = new CMATRIX*[ntraj];
      for(traj=0; traj<ntraj; traj++){
        Uprev[traj] = new CMATRIX(ham.nadi, ham.nadi);
        *Uprev[traj] = ham.children[traj]->get_basis_transform();  
      }
    }// do_reordering
  }// rep == 1

  ham.compute_diabatic(py_funct, bp::object(q), params, 1);
  ham.compute_adiabatic(1, 1);

                                                  
  if(rep==1){  // Only for adiabatic representation

    // Reordering, if needed
    if(do_reordering){
      X = new CMATRIX(ham.nadi, ham.nadi);

      for(traj=0; traj<ntraj; traj++){
        *X = (*Uprev[traj]).H() * ham.children[traj]->get_basis_transform();
        perm_t = get_reordering(*X);

        ham.children[traj]->update_ordering(perm_t, 1);

        el_stenc_y[0] = traj;
        CMATRIX x(ham.nadi, 1); 
        x = C.col(traj);
        x.permute_rows(perm_t);
        push_submatrix(C, x, el_stenc_x, el_stenc_y);
      }// for trajectories

      delete X;
    }// do_reordering

    // Phase correction, if needed
    if(do_phase_correction){
      CMATRIX* phases; phases = new CMATRIX(ham.nadi, 1); 

      for(traj=0; traj<ntraj; traj++){
        // Phase correction in U, NAC, and Hvib
        *phases = ham.children[traj]->update_phases(*Uprev[traj], 1);

        // Phase correction in Cadi
        el_stenc_y[0] = traj;
        CMATRIX x(ham.nadi, 1); x = C.col(traj);
        phase_correct_ampl(&x, phases);
        push_submatrix(C, x, el_stenc_x, el_stenc_y);

      }// for traj

      delete phases;
    }// phase correction


    if(do_phase_correction || do_reordering){
      for(traj=0; traj<ntraj; traj++){  delete Uprev[traj]; }
      delete Uprev;
    }

  }// rep == 1



  // Update the Ehrenfest forces for all trajectories
  if(rep==0){  p = p + ham.Ehrenfest_forces_dia(C, 1).real() * 0.5*dt;  }
  else if(rep==1){  p = p + ham.Ehrenfest_forces_adi(C, 1).real() * 0.5*dt;  }


  //============== Electronic propagation ===================
  // Update NACs and Hvib for all trajectories
  if(rep==0){  
    ham.compute_nac_dia(p, invM, 0, 1);
    ham.compute_hvib_dia(1);
  }
  else if(rep==1){  
    ham.compute_nac_adi(p, invM, 0, 1); 
    ham.compute_hvib_adi(1);
  }

  // Evolve electronic DOFs for all trajectories
  propagate_electronic(0.5*dt, C, ham.children, rep);   


}


void Ehrenfest2(double dt, MATRIX& q, MATRIX& p, MATRIX& invM, CMATRIX& C, 
                nHamiltonian& ham, bp::object py_funct, bp::object params, int rep){

  int do_reordering = 1; 
  int do_phase_correction = 1;

  Ehrenfest2(dt, q, p, invM, C, ham, py_funct, params, rep, do_reordering, do_phase_correction);

}



}// namespace libdyn
}// liblibra
